#pragma once
#include "game/ConventionalUnitVisual.h"
#include "loader/cvr/CvrScoutRenderer.h"
#include <unordered_map>
namespace resource {class ResourceManager;}
namespace game::frontend::sprite {
// Cache by actual graphics inputs, not UnitID, saved display name or Def role.
// Textures retain scene lifetime, as do existing instanced sprites and previews.
class ConventionalSpriteCache {
public:
    struct Image {std::shared_ptr<types::texture::Texture> texture;std::array<int32_t,2> anchor;};
    Image Get(resource::ResourceManager& resources,const ConventionalUnitVisual& design,const loader::cvr::CvrScoutViewInput& view);
    static std::string ViewKey(const ConventionalUnitVisual& design,const loader::cvr::CvrScoutViewInput& view);
private:
    std::unordered_map<std::string,loader::cvr::CvrScoutRenderer> m_models;
    std::unordered_map<std::string,Image> m_images;
};
}
