#pragma once

#include "types/Font.h"
#include "types/texture/Texture.h"
#include <memory>
#include <stdexcept>

namespace game::frontend::base {

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
