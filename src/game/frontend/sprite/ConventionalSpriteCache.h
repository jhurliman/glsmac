#pragma once
#include "game/ConventionalUnitVisual.h"
#include "loader/cvr/CvrScoutRenderer.h"
#include <unordered_map>
#include "loader/texture/SmacDialogFrame.h"
namespace resource {class ResourceManager;}
namespace game::frontend::sprite {
// Cache by actual graphics inputs, not UnitID, saved display name or Def role.
// Textures retain scene lifetime, as do existing instanced sprites and previews.
class ConventionalSpriteCache {
public:
    struct Image {std::shared_ptr<types::texture::Texture> texture;std::array<int32_t,2> anchor;};
    Image Get(resource::ResourceManager& resources,const ConventionalUnitVisual& design,const loader::cvr::CvrScoutViewInput& view);
    std::shared_ptr<types::texture::Texture> GetCard(resource::ResourceManager& resources,const ConventionalUnitVisual& design,
        int facing,bool roster,const std::array<int,2>& console_origin,const loader::texture::SmacDialogSource& source);
    static std::string ViewKey(const ConventionalUnitVisual& design,const loader::cvr::CvrScoutViewInput& view);
private:
    loader::cvr::CvrScoutRenderer& Model(resource::ResourceManager& resources,const ConventionalUnitVisual& design);
    std::vector<uint8_t> m_console;
    std::array<uint8_t,256> m_shadow_lookup{};
    std::unordered_map<std::string,std::shared_ptr<types::texture::Texture>> m_cards;
    std::unordered_map<std::string,loader::cvr::CvrScoutRenderer> m_models;
    std::unordered_map<std::string,Image> m_images;
};
}
