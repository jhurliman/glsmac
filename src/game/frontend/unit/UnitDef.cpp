#include "UnitDef.h"

#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/SpriteRender.h"
#include "engine/Engine.h"
#include "loader/texture/TextureLoader.h"
#include "util/String.h"
#include "game/frontend/sprite/InstancedSpriteManager.h"
#include "game/backend/map/Consts.h"

namespace game {
namespace frontend {
namespace unit {
namespace {
// Preserve the original raw byte in state; only proven renderer inputs are
// admitted. Unknown bytes and old records retain their presentation descriptor.
int32_t Facing(std::optional<uint8_t> value,int32_t fallback) {
    return !value ? fallback : *value<8 ? *value : *value==255 ? -1 : fallback;
}
}

UnitDef::UnitDef( sprite::InstancedSpriteManager* ism, const backend::unit::Def* unitdef, bool native_projection )
	: m_ism( ism )
    , m_native_projection(native_projection)
	, m_id( unitdef->m_id )
	, m_name( unitdef->m_name )
	, m_type( unitdef->m_type ) {

	switch ( unitdef->m_type ) {
		case backend::unit::DT_STATIC: {
			const auto* def = (backend::unit::StaticDef*)unitdef;

			switch ( def->m_render->m_type ) {
				case backend::unit::Render::RT_PORT_COLONY_MARKER:
					m_colony_marker = true;
					static_.movement_type = def->m_movement_type;
					static_.movement_per_turn = def->m_movement_per_turn;
					static_.render.is_sprite = true;
					break;
				case backend::unit::Render::RT_PORT_FORMER_MARKER:
					m_former_marker = true;
					static_.movement_type = def->m_movement_type;
					static_.movement_per_turn = def->m_movement_per_turn;
					static_.render.is_sprite = true;
					break;
				case backend::unit::Render::RT_CVR_SCOUT:
				case backend::unit::Render::RT_CVR_SPARTAN_ROVER:
				case backend::unit::Render::RT_CVR_INITIALIZED_SCOUT: {
					m_scout_render.emplace( *static_cast< const backend::unit::CvrScoutRender* >( def->m_render ) );
					static_.movement_type = def->m_movement_type;
					static_.movement_per_turn = def->m_movement_per_turn;
					static_.render.is_sprite = true;
					break;
				}

				case backend::unit::Render::RT_SPRITE: {
					m_render = ( (backend::unit::SpriteRender*)def->m_render )->m_render;

					static_.movement_type = def->m_movement_type;
					static_.movement_per_turn = def->m_movement_per_turn;
					static_.render.is_sprite = true;

					break;
				}
				default:
					THROW( "unknown unit render type: " + std::to_string( def->m_render->m_type ) );
			}
			break;
		}
		default:
			THROW( "unknown unit def type: " + std::to_string( unitdef->m_type ) );
	}
}

UnitDef::~UnitDef() {
	if ( m_type == backend::unit::DT_STATIC ) {
		if ( m_render.morale_based_xshift ) {
			if ( static_.render.morale_based_sprites ) {
				DELETE( static_.render.morale_based_sprites );
			}
		}
	}
}

const bool UnitDef::IsArtillery() const {
	return m_id != "SporeLauncher";
}

bool UnitDef::UsesNativeMapPlacement() const {
    if(!m_native_projection || !m_scout_render) {return false;}
    const auto& render=*m_scout_render;const auto& view=render.view;
    // Native sprite placement derives its bounds from the returned CVR anchor;
    // normal startup retains legacy world sizes in its descriptor.
    return view.scale_numerator<=0 || view.scale_numerator==view.scale_denominator;
}

sprite::Sprite* UnitDef::GetDesignSprite(const ConventionalUnitVisual& visual,std::optional<uint8_t> facing,int zoom) {
    const auto direction=Facing(facing,2);const auto key=visual.Key()+":"+std::to_string(direction)+":"+std::to_string(zoom);
    auto found=m_design_sprites.find(key);
    if(found==m_design_sprites.end()) {
        sprite::Sprite prepared;prepared.instanced_sprite=m_ism->GetConventionalSprite(m_id,visual,{direction,zoom,0,16,{0,0}},m_native_projection);
        found=m_design_sprites.emplace(key,prepared).first;
    }return &found->second;
}
std::shared_ptr<types::texture::Texture> UnitDef::GetDesignCardImage(const ConventionalUnitVisual& visual,std::optional<uint8_t> facing,bool roster,const std::array<int,2>& origin) {
    return m_ism->GetConventionalCard(visual,Facing(facing,2),roster,origin);
}
sprite::InstancedSprite* UnitDef::GetDesignCard(const ConventionalUnitVisual& visual,std::optional<uint8_t> facing,bool roster) {
    return m_ism->GetConventionalSprite(m_id,visual,{Facing(facing,2),roster ? -8:-4,0,1,{0,0}},false);
}

sprite::Sprite* UnitDef::GetSprite( const backend::unit::morale_t morale, std::optional<uint8_t> facing, std::optional<int> map_zoom ) {
	ASSERT( m_type == backend::unit::DT_STATIC, "only static units are supported for now" );
	ASSERT( static_.render.is_sprite, "only sprite unitdefs are supported for now" );
	if ( m_colony_marker ) {
		if ( !static_.render.sprite.instanced_sprite ) { static_.render.sprite.instanced_sprite = m_ism->GetColonyMarkerSprite( "Unit_" + m_id ); }
		return &static_.render.sprite;
	}
	if ( m_former_marker ) {
		if ( !static_.render.sprite.instanced_sprite ) { static_.render.sprite.instanced_sprite = m_ism->GetFormerMarkerSprite( "Unit_" + m_id ); }
		return &static_.render.sprite;
	}
	if ( m_scout_render ) {
        const auto direction=Facing(facing,m_scout_render->view.facing);
        if(UsesNativeMapPlacement() && map_zoom && *map_zoom!=m_scout_render->view.zoom) {
            const auto& original=*m_scout_render;
            const int zoom=*map_zoom;
            if(zoom< -14 || zoom>16) {throw std::invalid_argument("Unsupported native unit body zoom");}
            const int key=4096+(zoom+14)*9+(direction<0?8:direction);
            auto found=m_facing_sprites.find(key);
            if(found==m_facing_sprites.end()) {
                // The live map supplies its view. Legacy identity-scale wire
                // descriptors do not freeze ordinary model size across zoom.
                const backend::unit::CvrScoutRender descriptor(direction,zoom,
                    0,16,
                    original.size,original.center,original.composition);
                sprite::Sprite body;body.instanced_sprite=m_ism->GetScoutSprite("Unit_"+m_id+"_Map_"+std::to_string(key),descriptor,true);
                found=m_facing_sprites.emplace(key,body).first;
            }
            return &found->second;
        }
        if(direction!=m_scout_render->view.facing) {
            auto found=m_facing_sprites.find(direction);
            if(found==m_facing_sprites.end()) {
                const auto& original=*m_scout_render;const auto& view=original.view;
                const backend::unit::CvrScoutRender descriptor(direction,view.zoom,view.scale_numerator,view.scale_denominator,
                    original.size,original.center,original.composition);
                sprite::Sprite body;
                body.instanced_sprite=m_ism->GetScoutSprite("Unit_"+m_id+"_Facing_"+std::to_string(direction),descriptor,UsesNativeMapPlacement());
                found=m_facing_sprites.emplace(direction,body).first;
            }
            return &found->second;
        }
		if ( !static_.render.sprite.instanced_sprite ) {
            static_.render.sprite.instanced_sprite = m_ism->GetScoutSprite( "Unit_" + m_id, *m_scout_render, UsesNativeMapPlacement() );
		}
		return &static_.render.sprite;
	}

	if ( m_render.morale_based_xshift ) {
		if ( !static_.render.morale_based_sprites ) {
			NEW( static_.render.morale_based_sprites, morale_based_sprites_t );
		}
		auto it = static_.render.morale_based_sprites->find( morale );
		if ( it == static_.render.morale_based_sprites->end() ) {
			const uint32_t xshift = m_render.morale_based_xshift * ( morale - backend::unit::MORALE_MIN );
			it = static_.render.morale_based_sprites->insert(
				{
					morale,
					{
						m_ism->GetInstancedSprite(
							"Unit_" + m_id + "_" + std::to_string( morale ), GetSpriteTexture(), {
								m_render.x + xshift,
								m_render.y,
							},
							{
								m_render.w,
								m_render.h,
							},
							{
								m_render.cx + xshift,
								m_render.cy,
							},
							{
								backend::map::s_consts.tile.scale.x,
								backend::map::s_consts.tile.scale.y * backend::map::s_consts.sprite.y_scale
							},
							ZL_UNITS
						),
					}
				}
			).first;
		}
		return &it->second;
	}
	else {
		if ( !static_.render.sprite.instanced_sprite ) {
			static_.render.sprite = {
				m_ism->GetInstancedSprite(
					"Unit_" + m_id, GetSpriteTexture(), {
						m_render.x,
						m_render.y,
					},
					{
						m_render.w,
						m_render.h,
					},
					{
						m_render.cx,
						m_render.cy,
					},
					{
						backend::map::s_consts.tile.scale.x,
						backend::map::s_consts.tile.scale.y * backend::map::s_consts.sprite.y_scale
					},
					ZL_UNITS
				),
				1
			};
		}
		return &static_.render.sprite;
	}
}

sprite::InstancedSprite* UnitDef::GetCardSprite( bool roster, std::optional<uint8_t> retained ) const {
	if ( !m_scout_render ) { return nullptr; }
	// Old records retain the initialized descriptor or legacy fixture view.
    const auto fallback=m_scout_render->composition==backend::unit::CvrScoutRender::Composition::InitializedScout
        ? m_scout_render->view.facing : 0;
    const auto facing=Facing(retained,fallback);
	const backend::unit::CvrScoutRender descriptor( facing, roster ? -8 : -4, 0, 1,
		{ 1, 1 }, { 128, 128 }, m_scout_render->composition );
	return m_ism->GetScoutSprite( "UnitCard_" + m_id, descriptor );
}

const bool UnitDef::IsImmovable() const {
	return static_.movement_type == backend::unit::MT_IMMOVABLE;
}

const std::string UnitDef::GetNameString() const {
	return m_name;
}

const std::string UnitDef::GetStatsString() const {
	std::string str = "";
	if ( m_id == "SporeLauncher" ) {
		str += "(?)";
	}
	else {
		str += "?";
	}
	return str + " - ? - " + util::String::ApproximateFloat( static_.movement_per_turn );

}

types::texture::Texture* UnitDef::GetSpriteTexture() {
	if ( !static_.render.texture ) {
		static_.render.texture = g_engine->GetTextureLoader()->LoadCustomTexture( m_render.file, types::texture::TF_MIPMAPS );
	}
	return static_.render.texture;
}

}
}
}
