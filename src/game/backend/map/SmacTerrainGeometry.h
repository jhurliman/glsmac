#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "tile/Types.h"

namespace game::backend::map::smac_terrain {

// Stock ordinary terrain only. Contour controls depth/climate, not land height.
// C,W,N,E,S; see docs/behavior/ordinary-terrain-projection.md.
using Heights = std::array<int, 5>;
inline constexpr int CORNERS[4][4][2] = {
    {{-2,0},{-1,-1},{0,0},{-1,1}},
    {{-1,-1},{0,-2},{1,-1},{0,0}},
    {{0,0},{1,-1},{2,0},{1,1}},
    {{-1,1},{0,0},{1,1},{0,2}}
};

// The accessor returns an altitude band, or absence at a clipped boundary.
// Topology belongs to the accessor; this algorithm never wraps polar rows.
template<class BandAt>
Heights OrdinaryHeights(int x, int y, const BandAt& band_at, bool low_relief=false) {
    Heights result{};
    const auto own=band_at(x,y);
    if(!own || *own<3) {return result;}
    const int step=low_relief?8:25;
    result[0]=step*(*own-3);
    for(size_t corner=0;corner<4;++corner) {
        int sum=0;
        bool land=true;
        for(const auto& offset:CORNERS[corner]) {
            const auto band=band_at(x+offset[0],y+offset[1]);
            if(!band || *band<3) {land=false;break;}
            sum+=step*(*band-3);
        }
        result[corner+1]=land?sum/4:0;
    }
    return result;
}

// The current isometric camera projects world Y and Z with equal/opposite
// screen-Y coefficients. One full world-Y tile corresponds to 48 native
// zoom0 pixels. Keep native pixel heights relative to that tile, rather than
// feeding them through the gameplay elevation-to-Z clamp.
inline tile::tile_vertices_t LandCoordinates(const Heights& height,
    const types::Vec2<float>& center, const types::Vec2<float>& radius) {
    const float pixel_z=2*radius.y/48;
    return {
        {center.x,center.y,height[0]*pixel_z},
        {center.x-radius.x,center.y,height[1]*pixel_z},
        {center.x,center.y-radius.y,height[2]*pixel_z},
        {center.x+radius.x,center.y,height[3]*pixel_z},
        {center.x,center.y+radius.y,height[4]*pixel_z}
    };
}

}
