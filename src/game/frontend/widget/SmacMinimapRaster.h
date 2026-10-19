#pragma once

#include "SmacMinimapGeometry.h"
#include <cstring>
#include <string>

namespace game::frontend::widget::smac_minimap {

struct Raster {
    int32_t width, height;
    std::string rgba;
};

// Pinned original 80x80/zoom-14 base-pass profile. Other sizes deliberately do
// not inherit these placement claims merely from matching coordinate ratios.
inline std::optional<Point> PlanetAnchor(Point tile) {
    const auto x=tile[0],y=tile[1];
    if(x<0 || x>=80 || y<1 || y>=80 || (x&1)!=(y&1)) {return {};}
    auto dx=x-1;
    if(y<79) {if(dx<0) {dx+=80;}}
    else {if(dx>40) {dx-=80;}if(dx< -40) {dx+=80;}}
    return Point{{4*(dx-1),2*(y-2)}};
}

template<typename Paint>
inline void PlanetPixels(Point tile,const Paint& paint) {
    const auto anchor=PlanetAnchor(tile);if(!anchor) {return;}
    // Original polygon raster ink:4/8/4 pixels on rows1/2/3. Its right and
    // bottom clipping excludes the last bitmap column/row, including row77's
    // otherwise geometrically intersecting tip. Full indexed native hash pins
    // this behavior; generic polygon intersection gives a different result.
    for(int32_t row=1;row<=3;++row) {
        const auto y=(*anchor)[1]+row;
        if(y<0 || y>=151) {continue;}
        const int32_t left=row==2?0:2,right=row==2?8:6;
        for(int32_t dx=left;dx<right;++dx) {
            const auto x=(*anchor)[0]+dx;
            if(x>=0 && x<319) {paint(x,y);}
        }
    }
}

inline Raster PlanetSource(const std::string& tiles) {
    if(tiles.size()!=80*80*2) {throw std::invalid_argument("Invalid Planet minimap tile buffer");}
    Raster result{320,152,std::string(320*152*4,'\0')};
    for(int32_t y=1;y<80;++y) {for(int32_t x=y&1;x<80;x+=2) {
        const auto offset=static_cast<size_t>(y*40+x/2)*4;
        if(!tiles[offset+3]) {continue;}
        PlanetPixels({{x,y}},[&](int32_t px,int32_t py) {
            std::memcpy(&result.rgba[static_cast<size_t>(py*320+px)*4],tiles.data()+offset,4);
        });
    }}
    return result;
}

inline Raster OverviewTarget(Point world,Point window,const std::string& tiles,bool horizontal_wrap=true) {
    if(world[0]<2 || world[0]>512 || world[1]<=4 || world[1]>256 || tiles.size()!=static_cast<size_t>(world[0]*world[1]*2)
        || window[0]>8192 || window[1]>8192) {throw std::invalid_argument("Invalid minimap raster extent");}
    const auto layout=OverviewLayout(world,window);
    Raster target{window[0],window[1],std::string(static_cast<size_t>(window[0])*window[1]*4,'\0')};
    const bool native=horizontal_wrap && world==Point{{80,80}};
    const auto source=native?PlanetSource(tiles):Raster{0,0,{}};
    for(int32_t y=0;y<layout.image[1];++y) {for(int32_t x=0;x<layout.image[0];++x) {
        const Point pixel{{x+layout.offset[0],y+layout.offset[1]}};
        const char* color=nullptr;
        if(native) {
            // Explicit nearest-neighbor presentation policy. The native source
            // raster and stretch request are proved; target resampler pixel
            // parity is not asserted by that receipt.
            const auto sx=static_cast<int64_t>(x)*source.width/layout.image[0];
            const auto sy=static_cast<int64_t>(y)*source.height/layout.image[1];
            color=source.rgba.data()+static_cast<size_t>(sy*source.width+sx)*4;
        } else {
            // Functional observer-aware port fallback for other world sizes;
            // not an extrapolation of the native 80x80 diamond profile. Use the
            // same normalized mapping for pixels and clicks, preserving alpha.
            const auto tile=PixelToTile(pixel,layout);if(!tile) {continue;}
            color=tiles.data()+static_cast<size_t>((*tile)[1]*(world[0]/2)+(*tile)[0]/2)*4;
        }
        std::memcpy(&target.rgba[static_cast<size_t>(pixel[1]*target.width+pixel[0])*4],color,4);
    }}
    return target;
}

}
