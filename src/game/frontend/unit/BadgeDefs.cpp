#include "BadgeDefs.h"
#include "game/SmacHealthbar.h"
#include <memory>

#include "engine/Engine.h"
#include "loader/texture/TextureLoader.h"
#include "types/texture/Texture.h"
#include "game/frontend/sprite/InstancedSpriteManager.h"
#include "game/backend/map/Consts.h"
#include "game/frontend/sprite/InstancedSprite.h"

namespace game {
namespace frontend {
namespace unit {

const BadgeDefs::consts_t BadgeDefs::s_consts = {};

BadgeDefs::BadgeDefs( sprite::InstancedSpriteManager* ism )
	: m_ism( ism ) {
	//
}

BadgeDefs::~BadgeDefs() {
    for(const auto& badge:m_native_badges) {if(badge.instanced_sprite) {m_ism->RemoveInstancedSpriteByKey(badge.instanced_sprite->key);}}
	for ( const auto& it : m_unitbadge_sprites ) {
		for ( const auto& it2 : it.second ) {
			m_ism->RemoveInstancedSpriteByKey( it2.second->key );
		}
	}
	if ( m_fake_badge ) {
		m_ism->RemoveInstancedSpriteByKey( m_fake_badge->key );
	}
	for ( const auto& it : m_healthbar_sprites ) {
		m_ism->RemoveInstancedSpriteByKey( it.second.instanced_sprite->key );
	}
	for ( const auto& it : m_healthbar_textures ) {
		delete it.second;
	}
}

const types::Vec3 BadgeDefs::GetBadgeCoords( const types::Vec3& unit_coords ) {
	return {
		unit_coords.x + backend::map::s_consts.tile.scale.x * s_consts.offset.x,
		unit_coords.y - backend::map::s_consts.tile.scale.y * s_consts.offset.y * backend::map::s_consts.sprite.y_scale,
		unit_coords.z
	};
}

const types::Vec3 BadgeDefs::GetBadgeHealthbarCoords( const types::Vec3& unit_coords, bool canonical ) {
    if(canonical) {
        // Badge pivot(11,15), bar pivot(1,10), backing starts at(6,6).
        const auto x=s_consts.offset.x-4*s_consts.scale.x/23;
        const auto y=s_consts.offset.y+s_consts.scale.y/30;
        return {unit_coords.x+backend::map::s_consts.tile.scale.x*x,
            unit_coords.y-backend::map::s_consts.tile.scale.y*y*backend::map::s_consts.sprite.y_scale,unit_coords.z};
    }
	return {
		unit_coords.x + backend::map::s_consts.tile.scale.x * s_consts.healthbars.offset.x,
		unit_coords.y - backend::map::s_consts.tile.scale.y * s_consts.healthbars.offset.y * backend::map::s_consts.sprite.y_scale,
		unit_coords.z
	};
}

sprite::InstancedSprite* BadgeDefs::GetBadgeSprite( const badge_type_t badge_type, const backend::unit::morale_t morale ) {
	auto it1 = m_unitbadge_sprites.find( badge_type );
	if ( it1 == m_unitbadge_sprites.end() ) {
		it1 = m_unitbadge_sprites.insert(
			{
				badge_type,
				{}
			}
		).first;
	}
	auto it2 = it1->second.find( morale );
	if ( it2 == it1->second.end() ) {
		auto* texture = GetBadgesTexture();
		const auto w = s_consts.badges.width;
		const auto h = s_consts.badges.height;
		const auto m = s_consts.badges.margin;
		const uint32_t x = ( morale - backend::unit::MORALE_MIN ) * ( w + m ) + m;
		const uint32_t y = badge_type * ( h + m ) + m;

		// cut hole for healthbar
		texture->Fill(
			x + 6, y + 6, x + 7, y + 26, types::Color{
				0.0f,
				0.0f,
				0.0f,
				0.0f
			}
		);

		it2 = it1->second.insert(
			{
				morale,
				m_ism->GetInstancedSprite(
					"UnitBadge_" + std::to_string( badge_type ) + "_" + std::to_string( badge_type ),
					texture,
					{
						x,
						y,
					},
					{
						w,
						h,
					},
					{
						x + w / 2,
						y + h / 2,
					},
					{
						backend::map::s_consts.tile.scale.x * s_consts.scale.x,
						backend::map::s_consts.tile.scale.y * s_consts.scale.y * backend::map::s_consts.sprite.y_scale
					},
					ZL_UNITS,
					0.008f
				)
			}
		).first;
	}
	return it2->second;
}

sprite::Sprite* BadgeDefs::GetNativeBadgeSprite(backend::unit::morale_t level) {
    if(level>6) {THROW("Native badge level exceeds6");}
    auto& badge=m_native_badges.at(level);
    if(!badge.instanced_sprite) {
        // Native owner0 is not the current human slot and keeps variant0
        // regardless of spent movement in ordinary local rendering.
        std::shared_ptr<types::texture::Texture> texture(g_engine->GetTextureLoader()->CreateSmacNativeBadgeTexture(level,0));
        auto* sprite=m_ism->GetInstancedSprite("NativeBadge_"+std::to_string(level),texture.get(),{0,0},{23,30},{11,15},
            {backend::map::s_consts.tile.scale.x*s_consts.scale.x,backend::map::s_consts.tile.scale.y*s_consts.scale.y*backend::map::s_consts.sprite.y_scale},ZL_UNITS,0.008f);
        sprite->texture_owner=std::move(texture);badge.instanced_sprite=sprite;
    }
    return &badge;
}

sprite::InstancedSprite* BadgeDefs::GetFakeBadgeSprite() {
	if ( !m_fake_badge ) {
		m_fake_badge = m_ism->GetInstancedSprite(
			"UnitBadge_FAKE",
			GetBadgesTexture(),
			{
				21,
				5,
			},
			{
				3,
				17,
			},
			{
				22,
				12,
			},
			{
				backend::map::s_consts.tile.scale.x * s_consts.fake_badges.scale.x,
				backend::map::s_consts.tile.scale.y * s_consts.fake_badges.scale.y * backend::map::s_consts.sprite.y_scale
			},
			ZL_UNITS,
			0.002f
		);
	}
	return m_fake_badge;
}

sprite::Sprite* BadgeDefs::GetBadgeHealthbarSprite( const float health, int32_t maximum_hit_points ) {
    if(maximum_hit_points>0) {
        const auto bar=CalculateSmacHealthbar(health,maximum_hit_points);
        const auto key=static_cast<uint8_t>(128+22*static_cast<uint8_t>(bar.color)+bar.filled);
        auto found=m_healthbar_sprites.find(key);
        if(found==m_healthbar_sprites.end()) {
            auto pixels=m_healthbar_textures.find(key);
            if(pixels==m_healthbar_textures.end()) {
                auto owned=std::make_unique<types::texture::Texture>("SmacHealthbar_"+std::to_string(key),2,21);
                owned->Fill(0,0,1,20,types::Color::FromRGB(0,0,0));
                if(bar.filled) {
                    const auto color=bar.color==SmacHealthbar::Color::Green ? types::Color::FromRGB(0,255,0)
                        :bar.color==SmacHealthbar::Color::Yellow ? types::Color::FromRGB(255,255,0):types::Color::FromRGB(255,0,0);
                    owned->Fill(0,0,1,bar.filled-1,color);
                }
                pixels=m_healthbar_textures.emplace(key,owned.get()).first;owned.release();
            }
            auto* texture=pixels->second;
            found=m_healthbar_sprites.emplace(key,sprite::Sprite{
                m_ism->GetInstancedSprite("UnitBadge_SmacHealthbar_"+std::to_string(key),texture,{0,0},{2,21},{1,10},
                    {backend::map::s_consts.tile.scale.x*s_consts.scale.x*2/23,
                        backend::map::s_consts.tile.scale.y*s_consts.scale.y*21/30*backend::map::s_consts.sprite.y_scale},ZL_UNITS,0.005f),1}).first;
        }
        return &found->second;
    }
	const uint8_t step = round( health * ( s_consts.healthbars.resolution - 1 ) );
	auto it = m_healthbar_sprites.find( step );
	if ( it == m_healthbar_sprites.end() ) {

		const auto res = s_consts.healthbars.resolution;

		ASSERT( m_healthbar_textures.find( step ) == m_healthbar_textures.end(), "healthbar texture already exists" );
		auto* texture = m_healthbar_textures.insert(
			{
				step,
				new types::texture::Texture( "HealthBar_" + std::to_string( step ), 1, res, types::texture::TF_MIPMAPS )
			}
		).first->second;

		texture->Fill(
			0, 0, 0, step, {
				1.0f - health,
				health,
				0.0f,
				1.0f
			}
		);
		if ( step < res - 1 ) {
			texture->Fill(
				0, step + 1, 0, res - 1, types::Color{
					0.0f,
					0.0f,
					0.0f,
					1.0f
				}
			);
		}
		it = m_healthbar_sprites.insert(
			{
				step,
				{
					m_ism->GetInstancedSprite(
						"UnitBadge_Healthbar_" + std::to_string( step ),
						texture,
						{
							0,
							0,
						},
						{
							1,
							res,
						},
						{
							0,
							(unsigned int)res / 2,
						},
						{
							backend::map::s_consts.tile.scale.x * s_consts.healthbars.scale.x,
							backend::map::s_consts.tile.scale.y * s_consts.healthbars.scale.y * backend::map::s_consts.sprite.y_scale
						},
						ZL_UNITS,
						0.005f
					),
					1
				}
			}
		).first;
	}
	return &it->second;
}


const types::Vec3 BadgeDefs::GetFakeBadgeCoords( const types::Vec3& coords, const uint8_t offset ) const {
	return {
		coords.x + backend::map::s_consts.tile.scale.x * s_consts.fake_badges.offset.x + s_consts.fake_badges.step_x * offset,
		coords.y - backend::map::s_consts.tile.scale.y * s_consts.fake_badges.offset.y * backend::map::s_consts.sprite.y_scale,
		coords.z
	};
}

types::texture::Texture* BadgeDefs::GetBadgesTexture() {
	if ( !m_badges_texture ) {
		m_badges_texture = g_engine->GetTextureLoader()->LoadTexture( resource::PCX_FLAGS, types::texture::TF_MIPMAPS );
	}
	return m_badges_texture;
}

}
}
}
