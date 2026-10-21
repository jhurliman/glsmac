#include "SmacWorkshopCachedPreview.h"
#include "game/frontend/SmacDevelopedOverlay.h"
#include "types/texture/SmacIndexedSprite.h"

namespace ui {
SmacWorkshopCachedPreview SmacWorkshopCachedPreview::Load(const Reader& read) {
    SmacWorkshopCachedPreview result;
    result.m_sprites=loader::cvr::CvrActorMaterial::ReadWorkshopUnitSprites(read("units.pcx"));
    result.m_colors=loader::cvr::CvrActorMaterial::ReadWorkshop(read("palette.pcx"),read("vbp.cvr"),read("system.txt")).colors;
    result.m_shadow=game::frontend::smac_developed_overlay::ShadowLookup(result.m_colors);
    result.m_backdrop=loader::cvr::CvrActorMaterial::ReadWorkshopBackdrop(read("deswin.pcx"));
    return result;
}
SmacWorkshopCachedPreview::Image SmacWorkshopCachedPreview::Render(const int icon,const int variant) const {
    if(icon<0 || icon>9 || variant<0 || size_t(variant)>=m_sprites.at(size_t(icon)).size()) {throw std::invalid_argument("Invalid cached Workshop sprite identity");}
    const auto& sprite=m_sprites.at(size_t(icon)).at(size_t(variant));auto destination=m_backdrop;
    namespace indexed=types::texture::smac_indexed_sprite;
    if(!sprite.pixels.empty()) {
        const indexed::Source source{sprite.pixels,sprite.width,sprite.height,sprite.width,sprite.offset,9};
        const auto plan=indexed::Prepare(source,{202,94},{30,16},{140,9,451,320});
        indexed::Paint(source,plan,[&](int x,int y){return destination.at(size_t(y)*623+size_t(x));},
            [&](int x,int y,uint8_t color){destination.at(size_t(y)*623+size_t(x))=color;},&m_shadow);
    }
    Image result;result.indexed.reserve(311*311);result.rgba.reserve(311*311*4);
    for(size_t y=9;y<320;++y) {for(size_t x=140;x<451;++x) {
        const auto color=destination.at(y*623+x);result.indexed.push_back(color);
        for(const auto channel:m_colors.at(color)) {result.rgba.push_back(channel);}result.rgba.push_back(255);
    }}return result;
}
}
