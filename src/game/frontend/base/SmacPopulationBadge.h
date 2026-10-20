#pragma once

#include "types/Font.h"
#include "types/texture/Texture.h"
#include <memory>
#include <stdexcept>
#include <array>
#include <algorithm>
#include <optional>

namespace game::frontend::base {

inline bool MatchesSmacPopulationMetadata(const types::texture::Texture& faction,
    const std::array<types::Color,256>& palette, uint8_t primary, uint8_t secondary, uint8_t text,
    const std::array<types::Color,3>& configured) {
    const auto imported=[](uint32_t pixel,const types::Color& configured) {
        // Match FactionManager::import_colors without packing its /256 values
        // back through GetRGBA's /255 representation.
        return !(types::Color::FromRGBA(pixel)!=configured);
    };
    return faction.GetWidth()>161 && faction.GetHeight()>763
        && faction.GetPixel(4,739)==palette[primary].GetRGBA()
        && faction.GetPixel(4,747)==palette[secondary].GetRGBA()
        && faction.GetPixel(4,755)==palette[text].GetRGBA()
        && imported(faction.GetPixel(4,755),configured[0])
        && imported(faction.GetPixel(4,763),configured[1])
        && imported(faction.GetPixel(161,749),configured[2]);
}

inline types::Color SmacPopulationDisplayColor(const std::array<types::Color,256>& palette,uint8_t index) {
    const auto rgba=palette[index].GetRGBA();
    if(index<10 || index>245) {return palette[index];}
    const auto clamp=[](uint8_t channel){return channel>235 ? uint8_t(235):channel;};
    // Software gamma100 caps common colors; system-reserved entries stay exact.
    return types::Color::FromRGB(clamp(static_cast<uint8_t>(rgba)),
        clamp(static_cast<uint8_t>(rgba>>8)),clamp(static_cast<uint8_t>(rgba>>16)));
}

// RE-SMAC-MAP-OVERLAYS-001, zoom0. The caller supplies the actual palette
// colors and occupancy style; font layout never adds shadows or glyph padding.
inline std::unique_ptr<types::texture::Texture> MakeSmacPopulationBadge(
    size_t population, const types::Font& font, const types::Color& background,
    const types::Color& foreground, const types::Color& border, int logical_height=12, std::optional<size_t> template_width={}
) {
    if (population > 127) { throw std::invalid_argument("SMAC population exceeds signed native byte"); }
    if(logical_height<1 || logical_height>128) {throw std::invalid_argument("Invalid population font layout height");}
    const size_t advance = template_width.value_or(font.GetTextWidth(population < 10 ? "8" : "88"));
    if (!advance || advance > 256) { throw std::invalid_argument("Invalid population font advance"); }
    const int frame_width=static_cast<int>(advance+5);
    const auto digits=std::to_string(population);
    int64_t paint_width=frame_width;
    int pen=3;
    for(const char digit:digits) {
        const auto& glyph=font.m_symbols[static_cast<unsigned char>(digit)];
        if (glyph.width > 256 || glyph.height > 256 || glyph.ax < 0 || glyph.ax > 256
            || (glyph.width && glyph.height && !glyph.data)) {
            throw std::invalid_argument("Invalid population glyph");
        }
        paint_width=std::max(paint_width,int64_t(pen)+glyph.left+glyph.width);
        pen+=glyph.ax;
    }
    if(paint_width>1024) {throw std::invalid_argument("Invalid population glyph extent");}
    // Native >=10 frames still measure88; TextOutA can paint a third digit
    // outside that frame. Retain a transparent canvas for the complete ink.
    auto texture = std::make_unique<types::texture::Texture>("SMAC population",static_cast<size_t>(paint_width),logical_height+4);
    const int width=static_cast<int>(paint_width);
    texture->Fill(0, 0, frame_width - 1, logical_height+3, background);
    texture->Fill(0, 0, frame_width - 1, 0, border);
    texture->Fill(0, logical_height+3, frame_width - 1, logical_height+3, border);
    texture->Fill(0, 0, 0, logical_height+3, border);
    texture->Fill(frame_width - 1, 0, frame_width - 1, logical_height+3, border);
    pen = 3;
    for (const char digit : digits) {
        const auto& glyph = font.m_symbols[static_cast<unsigned char>(digit)];
        for (size_t y = 0; y < glyph.height; ++y) {
            for (size_t x = 0; x < glyph.width; ++x) {
                const int64_t px = int64_t(pen) + glyph.left + static_cast<int>(x);
                const int64_t py = int64_t(logical_height+2) - glyph.top + static_cast<int>(y);
                if (px >= 0 && px < width && py >= 0 && py < logical_height+4 && glyph.data[y * glyph.width + x]) {
                    texture->SetPixel(px, py, foreground);
                }
            }
        }
        pen += glyph.ax;
    }
    return texture;
}
}
