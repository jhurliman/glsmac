#include "SlotBadges.h"

#include "BaseManager.h"
#include "game/frontend/sprite/InstancedSpriteManager.h"
#include "game/frontend/sprite/InstancedSprite.h"
#include "game/frontend/text/InstancedFont.h"
#include "game/frontend/faction/Faction.h"
#include "game/backend/map/Consts.h"
#include "scene/actor/Instanced.h"
#include "types/texture/Texture.h"
#include "SmacPopulationBadge.h"
#include "engine/Engine.h"
#include "loader/texture/TextureLoader.h"

namespace game {
namespace frontend {
namespace base {

const SlotBadges::consts_t SlotBadges::s_consts = {};

const types::Vec3 SlotBadges::GetBadgeCoords( const types::Vec3& unit_coords ) {
	return {
		unit_coords.x + backend::map::s_consts.tile.scale.x * s_consts.offset.x,
		unit_coords.y - backend::map::s_consts.tile.scale.y * s_consts.offset.y * backend::map::s_consts.sprite.y_scale,
		unit_coords.z
	};
}

types::Vec3 SlotBadges::GetRenderedBadgeCoords(const types::Vec3& base_coords) const {
    if(m_smac_population && m_bm->UsesNativeMapProjection()) {
        // Native body canvas and population box share the dispatcher anchor.
        return {base_coords.x-0.5f,base_coords.y+1.0f,base_coords.z};
    }
    return GetBadgeCoords(base_coords);
}

SlotBadges::SlotBadges(
	BaseManager* bm,
	sprite::InstancedSpriteManager* ism,
	const size_t slot_index,
	const faction::Faction* faction
)
	: m_bm( bm )
	, m_ism( ism )
	, m_slot_index( slot_index )
	, m_faction( faction ) {
	// Stock faction identities, not transport slot numbers (which may be reordered).
	struct Colors { const char* id; uint8_t primary,secondary,text; };
	static constexpr Colors colors[]={{"GAIANS",241,32,242},{"HIVE",252,187,218},
		{"UNIVERSITY",255,209,155},{"MORGANITES",251,85,251},{"SPARTANS",65,79,75},
		{"BELIEVERS",102,109,98},{"PEACEKEEPERS",235,186,235}};
	if(m_bm->m_smac_population) {
		for(const auto& row:colors) {if(m_faction->m_id==row.id) {
			const auto* asset=g_engine->GetTextureLoader()->TryLoadCustomTexture(m_faction->GetBaseFilename());
			if(!asset || !MatchesSmacPopulationMetadata(*asset,m_bm->GetSmacPopulationPalette(),row.primary,row.secondary,row.text,
				{m_faction->m_colors.text,m_faction->m_colors.text_shadow,m_faction->m_colors.border})) {break;}
			m_smac_population=true;m_primary=row.primary;m_secondary=row.secondary;m_population_text=row.text;break;
		}}
	}
}

SlotBadges::~SlotBadges() {
	for ( const auto& it : m_per_population_sprites ) {
		if ( it.second.unguarded ) {
			DELETE( it.second.unguarded );
		}
		if ( it.second.guarded ) {
			DELETE( it.second.guarded );
		}
	}
	for ( const auto& spritemaps : m_badge_sprites ) {
		for ( const auto& it : spritemaps.second ) {
			m_ism->RemoveInstancedSpriteByKey( it.second.sprite->key );
			DELETE( it.second.texture );
		}
	}
}

sprite::Sprite* SlotBadges::GetBaseBadgeSprite( const size_t population, const bool is_guarded ) {
	auto it = m_per_population_sprites.find( population );
	if ( it == m_per_population_sprites.end() ) {
		it = m_per_population_sprites.insert(
			{
				population,
				{
					nullptr,
					nullptr
				}
			}
		).first;
	}
	sprite::Sprite** sprite_ptr = is_guarded
		? &it->second.guarded
		: &it->second.unguarded;
	if ( !*sprite_ptr ) {
		NEW( *sprite_ptr, sprite::Sprite, {
			GetBadgeSprite(
				population,
				is_guarded
			),
			1
		} );
	}
	return *sprite_ptr;
}

sprite::InstancedSprite* SlotBadges::GetBadgeSprite( const size_t population, const bool is_guarded ) {

	auto it1 = m_badge_sprites.find( is_guarded );
	if ( it1 == m_badge_sprites.end() ) {
		it1 = m_badge_sprites.insert(
			{
				is_guarded,
				{}
			}
		).first;
	}
	auto it2 = it1->second.find( population );
	if ( it2 == it1->second.end() ) {
		if(m_smac_population && population<=127) {
			const auto& palette=m_bm->GetSmacPopulationPalette();
			const bool spartan=m_faction->m_id=="SPARTANS";
			auto texture=MakeSmacPopulationBadge(population,*m_bm->GetSmacPopulationFont(),
				is_guarded ? SmacPopulationDisplayColor(palette,m_primary):types::Color::FromRGBA(0,0,0,0),
				SmacPopulationDisplayColor(palette,is_guarded ? (spartan ? 44:0):m_population_text),
				SmacPopulationDisplayColor(palette,is_guarded && spartan ? m_secondary:0));
			const auto width=static_cast<uint32_t>(texture->GetWidth());
			const auto height=static_cast<uint32_t>(texture->GetHeight());
            const bool native=m_bm->UsesNativeMapProjection();
            // Entity inverts mesh Y. A top pivot uses zero source Y so the
            // first source row stays at the declared dispatcher anchor.
			auto* sprite=m_ism->GetInstancedSprite("SmacPopulation_"+std::to_string(m_slot_index)+"_"+
				std::to_string(population)+(is_guarded ? "_filled":"_outline")+(native?"_native":""),texture.get(),{0,0},
				{width,height},{width,native?0u:height},{native?width/96.0f:backend::map::s_consts.tile.scale.x*0.25f*width/23,
				native?height/48.0f:backend::map::s_consts.tile.scale.y*backend::map::s_consts.sprite.y_scale*0.5f*height/30},
				ZL_TERRAIN_TEXT,0.008f);
			try {it2=it1->second.emplace(population,badge_sprite_t{texture.get(),sprite}).first;}
			catch(...) {m_ism->RemoveInstancedSpriteByKey(sprite->key);throw;}
			texture.release();
			return it2->second.sprite;
		}

		const auto& c = m_faction->m_colors;
		const auto transparent = types::Color::FromRGBA( 0, 0, 0, 0 );
		const auto black = types::Color::FromRGB( 0, 0, 0 );

		types::Color background, border, text, text_shadow;
		if ( is_guarded ) {
			background = c.text;
			border = black;
			text = black;
			text_shadow = transparent;
		}
		else {
			background = transparent;
			border = black;
			text = c.text;
			text_shadow = black;
		}

		auto* texture = m_bm->GetBadgeFont()->GetTextTexture(
			std::to_string( population ),
			background,
			text,
			text_shadow,
			3
		);

		// add border
		const auto w = texture->GetWidth() - 1;
		const auto h = texture->GetHeight() - 1;
		const auto b = 2;
		texture->Fill( 0, 0, w, b, border );
		texture->Fill( 0, h - b, w, h, border );
		texture->Fill( 0, 0, b, h, border );
		texture->Fill( w - b, 0, w, h, border );

		const uint32_t tw = texture->GetWidth();
		const uint32_t th = texture->GetHeight();

		it2 = it1->second.insert(
			{
				population,
				{
					texture,
					m_ism->GetInstancedSprite(
						"BaseBadge_" + std::to_string( m_slot_index ) + "_" + std::to_string( population ) + ( is_guarded
							? "_guarded"
							: "_unguarded"
						),
						texture,
						{
							0,
							0,
						},
						{
							tw,
							th,
						},
						{
							(uint32_t)std::round( tw / 2 ),
							(uint32_t)std::round( th / 2 ),
						},
						{
							backend::map::s_consts.tile.scale.x * s_consts.scale.x * tw,
							backend::map::s_consts.tile.scale.y * s_consts.scale.y * backend::map::s_consts.sprite.y_scale * th
						},
						ZL_TERRAIN_TEXT,
						0.008f
					)
				}
			}
		).first;
	}
	return it2->second.sprite;
}

}
}
}
