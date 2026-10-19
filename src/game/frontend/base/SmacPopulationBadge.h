#pragma once

#include "types/Font.h"
#include "types/texture/Texture.h"
#include <memory>
#include <stdexcept>
#include <array>
#include <cmath>

namespace game::frontend::base {

inline bool MatchesSmacPopulationMetadata(const types::texture::Texture& faction,
    const std::array<types::Color,256>& palette, uint8_t primary, uint8_t secondary, uint8_t text,
    const std::array<types::Color,3>& configured) {
    return faction.GetWidth()>161 && faction.GetHeight()>763
        && faction.GetPixel(4,739)==palette[primary].GetRGBA()
        && faction.GetPixel(4,747)==palette[secondary].GetRGBA()
        && faction.GetPixel(4,755)==palette[text].GetRGBA()
        && faction.GetPixel(4,755)==configured[0].GetRGBA()
        && faction.GetPixel(4,763)==configured[1].GetRGBA()
        && faction.GetPixel(161,749)==configured[2].GetRGBA();
}

inline types::Color SmacPopulationDisplayColor(const std::array<types::Color,256>& palette,uint8_t index) {
    const auto rgba=palette[index].GetRGBA();
    if(index<10 || index>245) {return palette[index];}
    const auto clamp=[](uint8_t channel){return channel>235 ? uint8_t(235):channel;};
    // Software gamma100 caps common colors; system-reserved entries stay exact.
    const auto color=types::Color::FromRGBA(rgba).value;
    return types::Color::FromRGB(clamp(static_cast<uint8_t>(std::lround(color.red*255))),
        clamp(static_cast<uint8_t>(std::lround(color.green*255))),clamp(static_cast<uint8_t>(std::lround(color.blue*255))));
}

// RE-SMAC-MAP-OVERLAYS-001, zoom0. The caller supplies the actual palette
// colors and occupancy style; font layout never adds shadows or glyph padding.
inline std::unique_ptr<types::texture::Texture> MakeSmacPopulationBadge(
    size_t population, const types::Font& font, const types::Color& background,
    const types::Color& foreground, const types::Color& border
) {
    if (population > 127) { throw std::invalid_argument("SMAC population exceeds signed native byte"); }
    const size_t advance = font.GetTextWidth(population < 10 ? "8" : "88");
    if (!advance || advance > 256) { throw std::invalid_argument("Invalid population font advance"); }
    auto texture = std::make_unique<types::texture::Texture>("SMAC population", advance + 5, 16);
    const int width = static_cast<int>(texture->GetWidth());
    texture->Fill(0, 0, width - 1, 15, background);
    texture->Fill(0, 0, width - 1, 0, border);
    texture->Fill(0, 15, width - 1, 15, border);
    texture->Fill(0, 0, 0, 15, border);
    texture->Fill(width - 1, 0, width - 1, 15, border);
    int pen = 3;
    for (const char digit : std::to_string(population)) {
        const auto& glyph = font.m_symbols[static_cast<unsigned char>(digit)];
        if (glyph.width > 256 || glyph.height > 256 || glyph.ax < 0 || glyph.ax > 256
            || (glyph.width && glyph.height && !glyph.data)) {
            throw std::invalid_argument("Invalid population glyph");
        }
        for (size_t y = 0; y < glyph.height; ++y) {
            for (size_t x = 0; x < glyph.width; ++x) {
                const int64_t px = int64_t(pen) + glyph.left + static_cast<int>(x);
                const int64_t py = int64_t(14) - glyph.top + static_cast<int>(y);
                if (px >= 0 && px < width && py >= 0 && py < 16 && glyph.data[y * glyph.width + x]) {
                    texture->SetPixel(px, py, foreground);
                }
            }
        }
        pen += glyph.ax;
    }
    return texture;
}
}
