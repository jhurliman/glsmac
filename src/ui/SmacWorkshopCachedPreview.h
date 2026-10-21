#pragma once
#include "loader/cvr/CvrActorMaterial.h"
#include <functional>
#include <string>

namespace ui {
class SmacWorkshopCachedPreview {
public:
    using Reader=std::function<std::string(const std::string&)>;
    struct Image {std::vector<uint8_t> indexed,rgba;};
    static SmacWorkshopCachedPreview Load(const Reader& read);
    Image Render(int icon,int variant) const;
private:
    std::array<std::vector<loader::cvr::CvrActorMaterial::WorkshopSprite>,10> m_sprites;
    std::array<loader::cvr::CvrPalette::Color,256> m_colors;
    std::array<uint8_t,256> m_shadow;
    std::vector<uint8_t> m_backdrop;
};
}
