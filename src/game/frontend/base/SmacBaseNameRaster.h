#pragma once

#include "types/Font.h"
#include "types/texture/Texture.h"
#include "types/Vec2.h"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace game::frontend::base {

struct SmacBaseNameRaster {
    std::shared_ptr<types::texture::Texture> texture;
    types::Vec2<float> pixel_center;
    int advance = 0;
};

// Original zoom0 plain-name prefix: whole-string advance, floor(width/2),
// baseline centerY+27, shadow (+1,+1), then foreground. The caller supplies
// regular Arial12 monochrome and the faction's actual text/shadow colors.
inline SmacBaseNameRaster MakeSmacBaseNameRaster(const std::string& text,
    const types::Font& font, const types::Color& foreground, const types::Color& shadow) {
    if(text.size()>1024) {throw std::invalid_argument("Base label exceeds raster extent");}
    int pen=0,left=0,right=1,top=0,bottom=1;
    for(const unsigned char code:text) {
        if(code<32 || code>=128) {throw std::invalid_argument("Unsupported Base label character");}
        const auto& glyph=font.m_symbols[code];
        if(glyph.ax<0 || glyph.ax>128 || glyph.width>128 || glyph.height>128 ||
            glyph.left < -128 || glyph.left>128 || glyph.top < -128 || glyph.top>128 ||
            (glyph.width && glyph.height && !glyph.data)) {
            throw std::invalid_argument("Invalid Base label glyph metrics");
        }
        if(glyph.width && glyph.height) {
            left=std::min(left,pen+glyph.left);
            right=std::max(right,pen+glyph.left+static_cast<int>(glyph.width)+1);
            top=std::min(top,-glyph.top);
            bottom=std::max(bottom,-glyph.top+static_cast<int>(glyph.height)+1);
        }
        pen+=glyph.ax;
    }
    SmacBaseNameRaster result;
    result.advance=pen;
    result.texture=std::make_shared<types::texture::Texture>("SMAC Base name",right-left,bottom-top);
    result.pixel_center={left-pen/2+(right-left)/2.0f,27+top+(bottom-top)/2.0f};
    // Paint each complete string in order; a later character's shadow must
    // never cover foreground already painted by an earlier character.
    for(int pass=1;pass>=0;--pass) {
        pen=0;
        for(const unsigned char code:text) {
            const auto& glyph=font.m_symbols[code];
            for(size_t y=0;y<glyph.height;++y) {for(size_t x=0;x<glyph.width;++x) {
                if(glyph.data[y*glyph.width+x]) {
                    result.texture->SetPixel(pen+glyph.left+static_cast<int>(x)-left+pass,
                        -glyph.top+static_cast<int>(y)-top+pass,pass ? shadow:foreground);
                }
            }}
            pen+=glyph.ax;
        }
    }
    result.texture->FullUpdate();
    return result;
}
}
