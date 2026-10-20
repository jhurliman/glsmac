#include "game/frontend/NativeActorMarker.h"
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
#include "loader/texture/TextureLoader.h"
#include "loader/texture/SmacUnitBadge.h"
#include "loader/texture/SmacGraphPalette.h"
#include "game/SmacHealthbar.h"
#include <cstring>
#include "game/backend/map/Consts.h"

namespace game {
namespace frontend {
namespace unit {

SlotBadges::SlotBadges(
	BadgeDefs* badge_defs,
	sprite::InstancedSpriteManager* ism,
	const size_t slot_index,
	const faction::Faction* faction,
    uint8_t native_palette_slot
)
	: m_badge_defs( badge_defs )
	, m_ism( ism )
	, m_badges_key( faction ? "UnitBadges_" + std::to_string( slot_index ) : "NativeBadges" )
	, m_faction( faction ), m_native_palette_slot(native_palette_slot) {

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

std::shared_ptr<types::texture::Texture> SlotBadges::MakeCardOverlay(int level,int variant,float health,int32_t maximum,char glyph,uint8_t color) const {
    auto image=std::make_shared<types::texture::Texture>("SMAC console Unit overlay",23,30,types::texture::TF_SMAC_PALETTE);
    if(!m_faction) {
        // Owner0 has a separately proved palette and always uses variant0.
        // Reuse that retained bank; never infer a human loaded palette slot.
        const auto source=m_badge_defs->GetNativeBadgeSprite(level)->instanced_sprite->texture_owner;
        if(!source || source->GetWidth()!=23 || source->GetHeight()!=30) {return {};}
        std::memcpy(image->GetBitmap(),source->GetBitmap(),source->GetBitmapSize());
    } else {
        if(m_native_palette_slot<1 || m_native_palette_slot>7) {return {};}
        const auto source=g_engine->GetTextureLoader()->GetSmacUnitBadge(m_native_palette_slot,level,variant);
        if(!source) {return {};}
        std::memcpy(image->GetBitmap(),source->rgba.data(),source->rgba.size());
    }
    const auto bar=CalculateSmacHealthbar(health,maximum);
    image->Fill(6,6,7,26,types::Color::FromRGB(0,0,0));
    if(bar.filled) {
        const auto rgb=bar.color==SmacHealthbar::Color::Green ? types::Color::FromRGB(0,255,0)
            : bar.color==SmacHealthbar::Color::Yellow ? types::Color::FromRGB(255,255,0) : types::Color::FromRGB(255,0,0);
        image->Fill(6,6,7,5+bar.filled,rgb);
    }
    if(glyph) {
        const auto palette=g_engine->GetTextureLoader()->GetSmacGraphPalette();
        if(!palette) {return {};}
        const auto& rgb=palette->colors[color];
        auto* font=g_engine->GetFontLoader()->LoadCustomFont("arialbd.ttf",12,loader::font::RasterMode::MONOCHROME);
        if(!font) {throw std::runtime_error("Console order font unavailable");}
        PaintSmacOrderGlyph(*image,*font,glyph,types::Color::FromRGB(rgb[0],rgb[1],rgb[2]));
    }
    image->FullUpdate();return image;
}

sprite::Sprite* SlotBadges::GetNativeMapBadge(backend::unit::morale_t level,bool spent,char glyph,uint8_t color) {
    const uint32_t key=(uint32_t(static_cast<unsigned char>(glyph))<<24)|(uint32_t(color)<<16)|(uint32_t(level)<<1)|(spent?1u:0u);
    auto found=m_native_map_badges.find(key);
    if(found==m_native_map_badges.end()) {
        auto pixels=MakeCardOverlay(level,spent?1:0,1,10,glyph,color);
        if(!pixels) {return nullptr;}
        // The separate map health actor owns this exact two-column opening.
        pixels->Fill(6,6,7,26,types::Color::FromRGBA(0,0,0,0));pixels->FullUpdate();
        auto* sprite=m_ism->GetInstancedSprite(m_badges_key+"_NATIVE_MAP_"+std::to_string(key),pixels.get(),
            {0,0},{23,30},{23,0},{23.0f/96,30.0f/48},ZL_UNITS,0.008f);
        sprite->texture_owner=std::move(pixels);
        sprite->actor->GetSpriteActor()->SetRenderFlags(scene::actor::Actor::RF_IGNORE_LIGHTING);
        try {found=m_native_map_badges.emplace(key,sprite::Sprite{sprite,1}).first;}
        catch(...) {m_ism->RemoveInstancedSpriteByKey(sprite->key);throw;}
    }
    return &found->second;
}

sprite::Sprite* SlotBadges::GetLowZoomMarker(int zoom) {
    if(m_faction && (m_native_palette_slot<1 || m_native_palette_slot>7)) {return nullptr;}
    auto found=m_low_zoom_markers.find(zoom);
    if(found==m_low_zoom_markers.end()) {
        const auto palette=g_engine->GetTextureLoader()->GetSmacGraphPalette();if(!palette) {return nullptr;}
        const auto fill=palette->colors[palette->normal[m_faction?m_native_palette_slot:0]];
        const auto border=palette->colors[m_faction?native_actor::MarkerBorderIndex(m_faction->m_id):0];
        auto* marker=m_ism->GetNativeActorMarker(m_badges_key,false,zoom,
            types::Color::FromRGB(fill[0],fill[1],fill[2]).GetRGBA(),types::Color::FromRGB(border[0],border[1],border[2]).GetRGBA());
        found=m_low_zoom_markers.emplace(zoom,sprite::Sprite{marker,1}).first;
    }return &found->second;
}

sprite::Sprite* SlotBadges::GetStackMarker(backend::unit::morale_t level,bool spent) {
    if(m_faction && (m_native_palette_slot<1 || m_native_palette_slot>7)) {return nullptr;}
    const uint32_t key=uint32_t(level)*2+(m_faction && spent ? 1u:0u);
    auto found=m_stack_markers.find(key);
    if(found==m_stack_markers.end()) {
        const auto palette=g_engine->GetTextureLoader()->GetSmacGraphPalette();
        if(!palette) {return nullptr;}
        const auto color=palette->colors[palette->alternate[m_faction ? m_native_palette_slot:0]];
        auto pixels=std::make_shared<types::texture::Texture>("SMAC stack silhouette",23,30,types::texture::TF_SMAC_PALETTE);
        // Use the raw flag mask, before health or order text is composed.
        std::shared_ptr<const loader::texture::SmacUnitBadge> source;
        std::shared_ptr<types::texture::Texture> native;
        if(m_faction) {source=g_engine->GetTextureLoader()->GetSmacUnitBadge(m_native_palette_slot,level,spent?1:0);if(!source) {return nullptr;}}
        else {native=m_badge_defs->GetNativeBadgeSprite(level)->instanced_sprite->texture_owner;if(!native || native->GetWidth()!=23 || native->GetHeight()!=30) {return nullptr;}}
        const auto* rgba=source ? source->rgba.data():native->GetBitmap();
        for(size_t p=0;p<23*30;++p) {if(rgba[p*4+3]) {pixels->SetPixel(p%23,p/23,types::Color::FromRGB(color[0],color[1],color[2]));}}
        pixels->FullUpdate();
        auto* sprite=m_ism->GetInstancedSprite(m_badges_key+"_STACK_"+std::to_string(key),pixels.get(),
            {0,0},{23,30},{23,0},{23.0f/96,30.0f/48},ZL_UNITS,0.007f);
        sprite->texture_owner=std::move(pixels);
        sprite->actor->GetSpriteActor()->SetRenderFlags(scene::actor::Actor::RF_IGNORE_LIGHTING);
        try {found=m_stack_markers.emplace(key,sprite::Sprite{sprite,1}).first;}
        catch(...) {m_ism->RemoveInstancedSpriteByKey(sprite->key);throw;}
    }
    return &found->second;
}

SlotBadges::~SlotBadges() {
    for(const auto& row:m_low_zoom_markers) {m_ism->RemoveInstancedSpriteByKey(row.second.instanced_sprite->key);}
    for(const auto& item:m_stack_markers) {m_ism->RemoveInstancedSpriteByKey(item.second.instanced_sprite->key);}
    for(const auto& item:m_native_map_badges) {m_ism->RemoveInstancedSpriteByKey(item.second.instanced_sprite->key);}
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
