#pragma once
#include "types/texture/Texture.h"
#include <array>
#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>

namespace game::frontend::widget {
struct SmacBaseCard {
    std::shared_ptr<types::texture::Texture> texture;
    std::array<int,2> offset;
};
// The original scales retained offset and dimensions separately, not a fitted
// transparent100x75 work surface. Native destination endpoints truncate.
inline std::array<int,4> SmacBaseCardRectangle(std::array<int,2> retained_offset,
    std::array<int,2> retained_size,bool roster) {
    const int numerator=roster?8:12;
    if(retained_offset[0]<0 || retained_offset[1]<0 || retained_size[0]<=0 || retained_size[1]<=0
        || retained_offset[0]+retained_size[0]>100 || retained_offset[1]+retained_size[1]>75) {
        throw std::invalid_argument("Invalid retained native Base frame");
    }
    const int x=retained_offset[0]*numerator/16,y=retained_offset[1]*numerator/16;
    return {{x,y,x+retained_size[0]*numerator/16,y+retained_size[1]*numerator/16}};
}
inline std::optional<SmacBaseCard> MakeSmacBaseCard(const types::texture::Texture& atlas,
    size_t frame_x,size_t frame_y,bool roster) {
    if(frame_x>atlas.GetWidth() || frame_y>atlas.GetHeight() || atlas.GetWidth()-frame_x<100 || atlas.GetHeight()-frame_y<75) {return {};}
    int left=100,top=75,right=0,bottom=0;
    for(int y=0;y<75;++y) {for(int x=0;x<100;++x) {
        if(atlas.GetPixel(frame_x+x,frame_y+y)>>24) {
            left=std::min(left,x);top=std::min(top,y);right=std::max(right,x+1);bottom=std::max(bottom,y+1);
        }
    }}
    if(right<=left || bottom<=top) {return {};}
    const auto rect=SmacBaseCardRectangle({{left,top}},{{right-left,bottom-top}},roster);
    const auto width=rect[2]-rect[0],height=rect[3]-rect[1];
    if(!width || !height) {return {};}
    auto texture=std::make_shared<types::texture::Texture>("SMAC Base card",width,height);
    for(int y=0;y<height;++y) {for(int x=0;x<width;++x) {
        // Explicit nearest-neighbor presentation until the native scaled-copy
        // pixel stepping receipt closes this separate resampling boundary.
        const int sx=left+(2*x+1)*(right-left)/(2*width);
        const int sy=top+(2*y+1)*(bottom-top)/(2*height);
        texture->SetPixel(x,y,atlas.GetPixel(frame_x+sx,frame_y+sy));
    }}
    return SmacBaseCard{std::move(texture),{{rect[0],rect[1]}}};
}
}
