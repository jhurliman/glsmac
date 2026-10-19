#include "SlotBadges.h"

#include "BadgeDefs.h"
#include "game/frontend/sprite/InstancedSpriteManager.h"
#include "game/frontend/sprite/InstancedSprite.h"
#include "game/frontend/faction/Faction.h"
#include "scene/actor/Instanced.h"
#include "scene/actor/Sprite.h"
#include "SmacOrderGlyph.h"
#include "engine/Engine.h"
#include "loader/font/FontLoader.h"
#include "game/backend/map/Consts.h"

namespace game {
namespace frontend {
namespace unit {

SlotBadges::SlotBadges(
	BadgeDefs* badge_defs,
	sprite::InstancedSpriteManager* ism,
	const size_t slot_index,
	const faction::Faction* faction
)
	: m_badge_defs( badge_defs )
	, m_ism( ism )
	, m_badges_key( faction ? "UnitBadges_" + std::to_string( slot_index ) : "NativeBadges" )
	, m_faction( faction ) {

	const auto c = faction ? faction->m_colors.border.value : types::Color::color_t(1.0f,0.0f,0.0f);
	m_repaint_rules = {
		{ // active
			types::Color::RGB( 252, 0, 0 ),
			types::Color( c.red, c.green, c.blue ).GetRGBA()
		},
		{ // greyed out
			types::Color::RGB( 125, 0, 0 ),
			types::Color( ( 0.5f + c.red / 2 ) / 2, ( 0.5f + c.green / 2 ) / 2, ( 0.5f + c.blue / 2 ) / 2 ).GetRGBA()
		}
	};

	// fake badges should be slightly greyer
	m_fake_badge_repaint_rules = m_repaint_rules;
	for ( auto& rule : m_fake_badge_repaint_rules ) {
		rule.second = types::Color( ( 0.6f + c.red / 2.5 ) / 2, ( 0.6f + c.green / 2.5 ) / 2, ( 0.6f + c.blue / 2.5 ) / 2 ).GetRGBA();
	}

}

SlotBadges::~SlotBadges() {
    for(const auto& item:m_order_sprites) {m_ism->RemoveInstancedSpriteByKey(item.second.instanced_sprite->key);}
	for ( const auto& it : m_per_morale_sprites ) {
		if ( it.second.normal ) {
			m_ism->RemoveRepaintedInstancedSpriteByKey( it.second.normal->instanced_sprite->key );
			DELETE( it.second.normal );
		}
		if ( it.second.greyedout ) {
			m_ism->RemoveRepaintedInstancedSpriteByKey( it.second.greyedout->instanced_sprite->key );
			DELETE( it.second.greyedout );
		}
	}
	if ( m_fake_badge ) {
		m_ism->RemoveRepaintedInstancedSpriteByKey( m_fake_badge->instanced_sprite->key );
		DELETE( m_fake_badge );
	}
}

sprite::Sprite* SlotBadges::GetUnitBadgeSprite( const backend::unit::morale_t morale, const bool is_active, char order_glyph, uint8_t color ) {
    if(m_faction && order_glyph) {return GetOrderBadgeSprite(morale,is_active,order_glyph,color);}
    if(!m_faction) {return m_badge_defs->GetNativeBadgeSprite(morale);}
	auto it = m_per_morale_sprites.find( morale );
	if ( it == m_per_morale_sprites.end() ) {
		it = m_per_morale_sprites.insert(
			{
				morale,
				{
					nullptr,
					nullptr
				}
			}
		).first;
	}
	sprite::Sprite** sprite_ptr = is_active
		? &it->second.normal
		: &it->second.greyedout;
	if ( !*sprite_ptr ) {
		NEW( *sprite_ptr, sprite::Sprite, {
			m_ism->GetRepaintedInstancedSprite(
				m_badges_key + "_" + std::to_string( morale ) + ( is_active
					? "_NORMAL"
					: "_GREYEDOUT"
				),
				m_badge_defs->GetBadgeSprite(
					( m_faction->m_is_progenitor
						? BadgeDefs::BT_PROGENITOR
						: BadgeDefs::BT_DEFAULT
					) | ( is_active
						? BadgeDefs::BT_NORMAL
						: BadgeDefs::BT_GREYEDOUT
					), morale
				),
				m_repaint_rules
			),
			1
		} );
	}
	return *sprite_ptr;
}

sprite::Sprite* SlotBadges::GetOrderBadgeSprite(backend::unit::morale_t morale,bool active,char glyph,uint8_t color) {
    const uint32_t key=(static_cast<uint32_t>(static_cast<unsigned char>(glyph))<<24) | (uint32_t(color)<<16)
        | (static_cast<uint32_t>(morale)<<1) | (active?1u:0u);
    auto found=m_order_sprites.find(key);
    if(found==m_order_sprites.end()) {
        const auto* base=GetUnitBadgeSprite(morale,active,0)->instanced_sprite;
        auto* font=g_engine->GetFontLoader()->LoadCustomFont("arialbd.ttf",12,loader::font::RasterMode::MONOCHROME);
        if(!font) {throw std::runtime_error("Order badge font is unavailable");}
        auto pixels=std::make_shared<types::texture::Texture>("SMAC order badge",23,30);
        const auto* source=base->actor->GetSpriteActor();
        for(size_t y=0;y<30;++y) {for(size_t x=0;x<23;++x) {pixels->SetPixel(x,y,source->GetTexture()->GetPixel(base->xy.x+x,base->xy.y+y));}}
        PaintSmacOrderGlyph(*pixels,*font,glyph,m_badge_defs->GetOrderColor(color));
        auto* sprite=m_ism->GetInstancedSprite(m_badges_key+"_ORDER_"+std::to_string(key),pixels.get(),{0,0},{23,30},{11,15},
            source->GetDimensions(),ZL_UNITS,0.008f);
        sprite->texture_owner=std::move(pixels);sprite->actor->GetSpriteActor()->SetRenderFlags(source->GetRenderFlags());
        found=m_order_sprites.emplace(key,sprite::Sprite{sprite,1}).first;
    }
    return &found->second;
}

const size_t SlotBadges::ShowFakeBadge( const types::Vec3& coords, const uint8_t offset ) {
	auto* fake_badge = GetFakeBadge();
	const size_t instance_id = fake_badge->next_instance_id++;
	fake_badge->instanced_sprite->actor->SetInstance( instance_id, m_badge_defs->GetFakeBadgeCoords( coords, offset ) );
	return instance_id;
}

void SlotBadges::HideFakeBadge( const size_t instance_id ) {
	GetFakeBadge()->instanced_sprite->actor->RemoveInstance( instance_id );
}

sprite::Sprite* SlotBadges::GetFakeBadge() {
	if ( !m_fake_badge ) {
		NEW( m_fake_badge, sprite::Sprite, {
			m_ism->GetRepaintedInstancedSprite(
				m_badges_key + "_FAKE",
				m_badge_defs->GetFakeBadgeSprite(),
				m_fake_badge_repaint_rules
			),
			1
		} );
	}
	return m_fake_badge;
}

}
}
}
