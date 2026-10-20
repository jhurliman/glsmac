#pragma once

#include <cstddef>
#include <unordered_map>
#include <map>
#include <optional>
#include "game/frontend/sprite/Sprite.h"

#include "types/texture/Types.h"

#include "types/Vec3.h"
#include "types/Color.h"

namespace types::texture {
class Texture;
}

namespace gse::tests { class BackendFixture; }

namespace game {
namespace frontend {

namespace sprite {
class Sprite;
class InstancedSprite;
class InstancedSpriteManager;
}

namespace faction {
class Faction;
}

namespace base {

class BaseManager;

class SlotBadges {
public:

	static const types::Vec3 GetBadgeCoords( const types::Vec3& unit_coords );
    types::Vec3 GetRenderedBadgeCoords(const types::Vec3& base_coords) const;

	SlotBadges(
		BaseManager* bm,
		sprite::InstancedSpriteManager* ism,
		const size_t slot_index,
		const faction::Faction* faction
	);
	~SlotBadges();

    sprite::Sprite* GetLowZoomMarker(int zoom);
	sprite::Sprite* GetBaseBadgeSprite( const size_t population, const bool is_guarded, std::optional<int> zoom={} );

private:
    friend class gse::tests::BackendFixture;

	BaseManager* m_bm = nullptr;
	sprite::InstancedSpriteManager* const m_ism;
	const size_t m_slot_index = 0;
	const faction::Faction* m_faction = nullptr;
    std::unordered_map<int,sprite::Sprite> m_low_zoom_markers;
	bool m_smac_population = false;
	uint8_t m_primary = 0, m_secondary = 0, m_population_text = 0;

	class slot_sprites_t {
	public:
		sprite::Sprite* unguarded;
		sprite::Sprite* guarded;
	};
    using PopulationKey=std::pair<size_t,int>;
	std::map< PopulationKey, slot_sprites_t > m_per_population_sprites = {};

	static const struct consts_t {
		const types::Vec2< float > scale = {
			0.005f,
			0.0075f
		};
		const types::Vec2< float > offset = {
			-0.5f,
			-0.6f
		};
	} s_consts;

	struct badge_sprite_t {
		types::texture::Texture* texture;
		sprite::InstancedSprite* sprite;
	};
	typedef std::map< PopulationKey, badge_sprite_t > badge_spritemap_t; // population, sprite
	typedef std::unordered_map< bool, badge_spritemap_t > badge_spritemaps_t; // is_guarded, spritemap
	badge_spritemaps_t m_badge_sprites = {};
	sprite::InstancedSprite* GetBadgeSprite( const size_t population, const bool is_guarded, int zoom );
    PopulationKey Key(size_t population,std::optional<int> zoom) const;

};

}
}
}
