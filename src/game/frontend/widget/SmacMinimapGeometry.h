#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace game::frontend::widget::smac_minimap {

using Point=std::array<int32_t,2>;
struct Layout {
    Point world, window, source, image, offset;
};

// Original overview bitmap request and normalized navigation, without the
// unmeasured bitmap resampler. Positive global-scale overrides are not admitted.
inline Layout OverviewLayout(Point world,Point window) {
    if(world[0]<=0 || world[1]<=4 || (world[0]&1) || (world[1]&1)
        || world[0]>8192 || world[1]>8192 || window[0]<=0 || window[1]<=0) {
        throw std::invalid_argument("Unsupported minimap geometry");
    }
    const Point source{{world[0]*4,(world[1]/2-2)*4}};
    const bool fits=window[0]>=source[0] && window[1]>=source[1];
    return {world,window,source,fits?source:window,
        fits?Point{{(window[0]-source[0])/2,(window[1]-source[1])/2}}:Point{{0,0}}};
}

inline std::optional<Point> PixelToTile(Point pixel,const Layout& layout) {
    const auto& offset=layout.offset;
    if(offset[0] || offset[1]) {
        for(size_t axis=0;axis<2;++axis) {
            if(pixel[axis]<offset[axis] || static_cast<int64_t>(pixel[axis])>=static_cast<int64_t>(offset[axis])+layout.image[axis]) {return {};}
            pixel[axis]-=offset[axis];
        }
    }
    const auto x=static_cast<int64_t>(layout.world[0])*pixel[0]/layout.image[0];
    auto y=static_cast<int64_t>(layout.world[1])*pixel[1]/layout.image[1];
    y=(y&~int64_t{1})|(x&1);
    if(x<0 || x>=layout.world[0] || y<0 || y>=layout.world[1]) {return {};}
    return Point{{static_cast<int32_t>(x),static_cast<int32_t>(y)}};
}

inline std::optional<Point> TileToPixel(Point tile,const Layout& layout,bool flat=false) {
    if(!flat) {
        if(tile[0]<0) {tile[0]+=layout.world[0];}
        else if(tile[0]>=layout.world[0]) {tile[0]-=layout.world[0];}
    }
    if(tile[0]<0 || tile[0]>=layout.world[0]) {return {};}
    tile[1]=std::clamp(tile[1],0,layout.world[1]-1);
    return Point{{layout.offset[0]+static_cast<int32_t>(static_cast<int64_t>(layout.image[0])*tile[0]/layout.world[0]),
        layout.offset[1]+static_cast<int32_t>(static_cast<int64_t>(layout.image[1])*tile[1]/layout.world[1])}};
}

}
