#pragma once

#include <cstddef>
#include <memory>
#include "game/frontend/sprite/Sprite.h"

#include "game/backend/unit/Types.h"
#include "types/texture/Types.h"

#include "types/Vec3.h"
#include "types/Color.h"
namespace types::texture {class Texture;}

namespace game {
namespace frontend {

namespace sprite {
class Sprite;
class InstancedSpriteManager;
}

namespace faction {
class Faction;
}

namespace unit {

class BadgeDefs;

class SlotBadges {
public:

	SlotBadges(
		BadgeDefs* badge_defs,
		sprite::InstancedSpriteManager* ism,
		const size_t slot_index,
		const faction::Faction* faction,
        uint8_t native_palette_slot=255
	);
	~SlotBadges();

	sprite::Sprite* GetUnitBadgeSprite( const backend::unit::morale_t morale, const bool is_active, char order_glyph = 0, uint8_t color_index = 0 );
    sprite::Sprite* GetNativeMapBadge(backend::unit::morale_t level,bool spent,char glyph,uint8_t color);
    std::shared_ptr<types::texture::Texture> MakeCardOverlay(int level,int variant,float health,int32_t maximum,char glyph,uint8_t color) const;

	const size_t ShowFakeBadge( const types::Vec3& coords, const uint8_t offset );
	void HideFakeBadge( const size_t instance_id );

private:

	sprite::Sprite* GetOrderBadgeSprite(backend::unit::morale_t morale,bool active,char glyph,uint8_t color);
    std::unordered_map<uint32_t,sprite::Sprite> m_order_sprites;
    std::unordered_map<uint32_t,sprite::Sprite> m_native_map_badges;
	BadgeDefs* const m_badge_defs;
	sprite::InstancedSpriteManager* const m_ism;
	const std::string m_badges_key;
	const faction::Faction* m_faction = nullptr;
    const uint8_t m_native_palette_slot;

	class slot_sprites_t {
	public:
		sprite::Sprite* normal;
		sprite::Sprite* greyedout;
	};
	std::unordered_map< backend::unit::morale_t, slot_sprites_t > m_per_morale_sprites = {};

	types::texture::repaint_rules_t m_repaint_rules = {};
	types::texture::repaint_rules_t m_fake_badge_repaint_rules = {};

	sprite::Sprite* m_fake_badge = nullptr;
	sprite::Sprite* GetFakeBadge();
};

}
}
}
