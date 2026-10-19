#pragma once
#include "game/backend/faction/SmacDominanceGraph.h"
#include "loader/texture/SmacGraphPalette.h"
#include "types/texture/SmacIndexedLine.h"

namespace ui {
inline std::vector<uint8_t> RasterSmacDominance(const game::backend::faction::SmacDominanceGraph& graph,
    const loader::texture::SmacGraphPalette& palette) {
    const auto& rect=graph.clip;
    if((rect!=std::array<int32_t,4>{501,76,721,162} && rect!=std::array<int32_t,4>{272,76,492,162})
        || graph.lines.size()>7000) {throw std::invalid_argument("Unsupported dominance raster geometry");}
    constexpr int width=220,height=86;
    std::vector<uint8_t> rgba(width*height*4,0);
    for(const auto& line:graph.lines) {
        types::texture::SmacIndexedLine({line.x1,line.y1,line.x2,line.y2},rect,line.color,[&](int32_t x,int32_t y,uint8_t color) {
            const auto pixel=static_cast<size_t>((y-rect[1])*width+x-rect[0])*4;
            for(size_t channel=0;channel<3;++channel) {rgba[pixel+channel]=palette.colors[color][channel];}
            rgba[pixel+3]=255;
        });
    }
    return rgba;
}
}
