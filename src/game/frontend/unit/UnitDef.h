#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include "game/backend/unit/CvrScoutRender.h"

#include "game/backend/unit/Types.h"

#include "types/Vec3.h"
// TODO: remove?
#include "game/frontend/sprite/Sprite.h"

namespace types {
namespace texture {
class Texture;
}
}

namespace game::backend::unit {
class Def;
}

namespace game {
namespace frontend {

namespace sprite {
class InstancedSpriteManager;
class InstancedSprite;
}

namespace unit {

class UnitDef {
public:
	UnitDef( sprite::InstancedSpriteManager* ism, const backend::unit::Def* unitdef, bool native_projection = false );
	~UnitDef();

	const bool IsArtillery() const;

	sprite::Sprite* GetSprite( const backend::unit::morale_t morale, std::optional<uint8_t> facing = {}, std::optional<int> map_zoom = {} );
    bool UsesNativeMapPlacement() const;
	// Native console zoom is independent of the map; initialized Infantry keeps
	// its retained presentation facing. Legacy profiles retain their fixture view.
	sprite::InstancedSprite* GetCardSprite( bool roster, std::optional<uint8_t> facing = {} ) const;

	const bool IsImmovable() const;

	const std::string GetNameString() const;
	const std::string GetStatsString() const;

private:

	sprite::InstancedSpriteManager* const m_ism;

	backend::unit::sprite_render_info_t m_render = {};
	std::optional< backend::unit::CvrScoutRender > m_scout_render;
    const bool m_native_projection;
    std::unordered_map<int32_t,sprite::Sprite> m_facing_sprites;
	bool m_former_marker = false;
	bool m_colony_marker = false;

	std::string m_id;
	std::string m_name;
	backend::unit::def_type_t m_type;

	typedef std::unordered_map< backend::unit::morale_t, sprite::Sprite > morale_based_sprites_t;

	struct {
		backend::unit::movement_type_t movement_type;
		backend::unit::movement_t movement_per_turn;
		struct {
			bool is_sprite = false;
			types::texture::Texture* texture = nullptr;
			sprite::Sprite sprite = {};
			morale_based_sprites_t* morale_based_sprites = nullptr;
		} render = {};
	} static_ = {};

	types::texture::Texture* GetSpriteTexture();
};

}
}
}
