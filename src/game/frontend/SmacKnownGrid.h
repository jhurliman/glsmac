#pragma once
#include "SmacTerrainRaster.h"
#include "game/backend/map/SmacTerrainLighting.h"
#include <array>
#include <cstdint>

namespace game::frontend::smac_known_grid {
using Point=smac_terrain_raster::Point;
struct Preferences {
    uint32_t primary=0,more=0; // Explicit zero for controlled rendering, not fresh user defaults.
    bool operator==(const Preferences& other) const {return primary==other.primary && more==other.more;}
};
struct Line {Point from,to;uint8_t color;};
struct Lines {std::array<Line,2> values{};size_t count=0;};
// The terrain caller supplies already-displaced ordinary vertices. Unknown
// admission/outline is separate and must run before this continuation.
inline Lines Select(Point tile,const std::array<Point,5>& points,
    const backend::map::smac_terrain::BandAt& bands,Point world,bool wrap,
    Preferences preferences,uint32_t flags) {
    Lines result;const bool special=(flags&0x80000000u)!=0;
    if(!((preferences.primary&0x100u) && !special) && !(flags&0x04000000u)) {return result;}
    const auto current=bands(tile[0],tile[1]);if(!current) {return result;}
    const bool sea=special || (preferences.more&0x800000u);
    if(*current<3 && !sea) {return result;}
    for(int edge=0;edge<2;++edge) {
        int x=tile[0]+(edge?1:-1);const int y=tile[1]-1;
        if(wrap) {if(x<0) {x+=world[0];}else if(x>=world[0]) {x-=world[0];}}
        if(x<0 || x>=world[0] || y<0 || y>=world[1]) {continue;}
        const auto neighbor=bands(x,y);if(!neighbor || (*neighbor<3 && !sea)) {continue;}
        result.values[result.count++]={points[edge+1],points[edge+2],uint8_t(*current>=3 && *neighbor>=3?37:190)};
    }return result;
}
}
