#pragma once
#include "types/texture/Texture.h"
#include <array>
#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <functional>
#include <vector>

namespace loader::texture {
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
// Original604070 downscale table: sample before advancing, with strict>0.
inline std::array<int,800> SmacBaseCardSteps(bool roster) {
    const int low=roster?8:12,high=16,q=high/low,r=high%low;
    int error=r-2*low+(q%2?high:0);
    std::array<int,800> result{};
    for(auto& step:result) {error+=2*r;step=q;if(error>0) {++step;error-=2*low;}}
    return result;
}
inline std::optional<SmacBaseCard> MakeSmacBaseCardPixels(size_t atlas_width,size_t atlas_height,
    size_t frame_x,size_t frame_y,bool roster,const std::function<types::Color::rgba_t(size_t,size_t)>& pixel) {
    if(frame_x>atlas_width || frame_y>atlas_height || atlas_width-frame_x<100 || atlas_height-frame_y<75) {return {};}
    int left=100,top=75,right=0,bottom=0;
    for(int y=0;y<75;++y) {for(int x=0;x<100;++x) {
        if(pixel(frame_x+x,frame_y+y)>>24) {
            left=std::min(left,x);top=std::min(top,y);right=std::max(right,x+1);bottom=std::max(bottom,y+1);
        }
    }}
    if(right<=left || bottom<=top) {return {};}
    const auto rect=SmacBaseCardRectangle({{left,top}},{{right-left,bottom-top}},roster);
    const auto width=rect[2]-rect[0],height=rect[3]-rect[1];
    if(!width || !height) {return {};}
    auto texture=std::make_shared<types::texture::Texture>("SMAC Base card",width,height);
    const auto steps=SmacBaseCardSteps(roster);
    int sy=top;
    for(int y=0;y<height;++y) {
        int sx=left;
        for(int x=0;x<width;++x) {texture->SetPixel(x,y,pixel(frame_x+sx,frame_y+sy));sx+=steps[x];}
        sy+=steps[y];
    }
    return SmacBaseCard{std::move(texture),{{rect[0],rect[1]}}};
}
inline std::optional<SmacBaseCard> MakeSmacBaseCard(const types::texture::Texture& atlas,
    size_t frame_x,size_t frame_y,bool roster) {
    return MakeSmacBaseCardPixels(atlas.GetWidth(),atlas.GetHeight(),frame_x,frame_y,roster,
        [&](size_t x,size_t y) {return atlas.GetPixel(x,y);});
}
// Source bytes have the original+10 modulo256 rebasing. Key9 preserves the
// background; source0's measured destination remap is uniformly black0.
inline std::optional<SmacBaseCard> MakeSmacIndexedBaseCard(const std::vector<uint8_t>& pixels,
    size_t width,size_t height,size_t frame_x,size_t frame_y,bool roster,
    const std::array<types::Color::rgba_t,256>& palette) {
    if(!width || height>pixels.size()/width || width*height!=pixels.size()) {return {};}
    return MakeSmacBaseCardPixels(width,height,frame_x,frame_y,roster,[&](size_t x,size_t y) {
        const auto index=pixels[y*width+x];
        return index==9?types::Color::rgba_t(0):index==0?types::Color::RGB(0,0,0):palette[index];
    });
}
}
