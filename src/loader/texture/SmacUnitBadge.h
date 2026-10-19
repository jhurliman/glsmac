#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>
#include <SDL.h>

namespace loader::texture {
struct SmacUnitBadge {
    static constexpr size_t Width=23,Height=30;
    using Indexed=std::array<uint8_t,Width*Height>;
    Indexed indexed{};
    std::array<uint8_t,Width*Height*4> rgba{};
};
inline SmacUnitBadge::Indexed ExtractSmacUnitBadge(const SDL_Surface* units,int level,int variant) {
    if(!units || !units->format || units->format->BitsPerPixel!=8 || !units->pixels || units->pitch<units->w
        || level<0 || level>6 || variant<0 || variant>1) {throw std::invalid_argument("Invalid indexed Unit badge source");}
    const int left=1+24*level,top=700+31*variant;
    if(units->w<left+23 || units->h<top+30) {throw std::invalid_argument("Unit badge exceeds UNITS atlas");}
    SmacUnitBadge::Indexed result{};
    for(size_t y=0;y<30;++y) {for(size_t x=0;x<23;++x) {
        auto index=static_cast<uint8_t>(static_cast<const uint8_t*>(units->pixels)[size_t(top+y)*units->pitch+left+x]+10);
        if(index==6 || index==7) {index=9;}
        result[y*23+x]=index;
    }}
    return result;
}
inline SmacUnitBadge ColorSmacUnitBadge(const SmacUnitBadge::Indexed& source,uint8_t primary,uint8_t secondary,
    const std::array<std::array<uint8_t,3>,256>& palette) {
    SmacUnitBadge result;
    for(size_t p=0;p<source.size();++p) {
        auto index=source[p];if(index==4) {index=primary;} else if(index==3) {index=secondary;}
        result.indexed[p]=index;
        if(index!=9) {for(size_t c=0;c<3;++c) {result.rgba[4*p+c]=palette[index][c];}result.rgba[4*p+3]=255;}
    }
    return result;
}
}
