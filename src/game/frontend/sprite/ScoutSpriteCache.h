#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "loader/cvr/CvrScoutRenderer.h"
#include "loader/cvr/CvrSpartanRoverRenderer.h"

namespace resource { class ResourceManager; }
namespace game::backend::unit { class CvrScoutRender; }

namespace game::frontend::sprite {

// Scene-lifetime immutable views. No eviction can invalidate an actor/preview.
class ScoutSpriteCache {
public:
	static constexpr size_t MAX_VIEWS = 64;
	std::shared_ptr< types::texture::Texture > Get( resource::ResourceManager& resources, const backend::unit::CvrScoutRender& descriptor );
    std::array<int32_t,2> GetTargetAnchor(resource::ResourceManager& resources, const backend::unit::CvrScoutRender& descriptor);
private:
	std::optional< loader::cvr::CvrScoutRenderer > m_renderer;
	std::optional< loader::cvr::CvrScoutRenderer > m_initialized;
	std::optional< loader::cvr::CvrSpartanRoverRenderer > m_rover;
    struct Cached {std::shared_ptr<types::texture::Texture> texture;std::array<int32_t,2> target_anchor;};
	std::unordered_map<std::string,Cached> m_textures;
};

}
