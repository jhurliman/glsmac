#pragma once
#include "types/Font.h"
#include "types/texture/Texture.h"
#include <stdexcept>
#include <cstdint>

namespace game::frontend::unit {
// Original zoom0 text box(10,5)-(21,19), centered horizontal advance,
// transparent paint, baseline16. Canvas geometry is unchanged by the glyph.
inline void PaintSmacOrderGlyph(types::texture::Texture& canvas,const types::Font& font,char letter,const types::Color& color) {
    const auto code=static_cast<unsigned char>(letter);
    if(canvas.GetWidth()!=23 || canvas.GetHeight()!=30 || code<32 || code>=128) {throw std::invalid_argument("Invalid SMAC order glyph canvas/code");}
    const auto& glyph=font.m_symbols[code];
    if(glyph.ax<=0 || glyph.ax>64 || glyph.width>128 || glyph.height>128 || (glyph.width && glyph.height && !glyph.data)) {throw std::invalid_argument("Invalid SMAC order glyph metrics");}
    const int64_t left=10+(11-glyph.ax)/2;
    for(size_t y=0;y<glyph.height;++y) {for(size_t x=0;x<glyph.width;++x) {
        const int64_t px=left+glyph.left+static_cast<int64_t>(x),py=16-int64_t(glyph.top)+static_cast<int64_t>(y);
        if(px>=0 && px<23 && py>=0 && py<30 && glyph.data[y*glyph.width+x]) {canvas.SetPixel(static_cast<size_t>(px),static_cast<size_t>(py),color);}
    }}
}
}
