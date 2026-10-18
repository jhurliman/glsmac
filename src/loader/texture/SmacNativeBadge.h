#pragma once
#include <SDL.h>
#include <array>
#include <cstdint>
#include <utility>
#include <stdexcept>

namespace loader::texture {
// RE-SMAC-NATIVE-BADGE-RASTER-001, gamma100 software palette. These are
// indexed substitutions before palette lookup, never local-PCX RGB matching.
struct SmacNativeBadge {
    static constexpr size_t Width=23, Height=30;
    std::array<uint8_t,Width*Height> indexed{};
    std::array<uint8_t,Width*Height*4> rgba{};
};
inline SmacNativeBadge DecodeSmacNativeBadge(const SDL_Surface* units,const SDL_Palette* common,int level,int variant) {
    if(!units || !units->format || units->format->BitsPerPixel!=8 || !units->pixels || units->pitch<units->w
        || !common || common->ncolors!=256 || !common->colors || level<0 || level>6 || variant<0 || variant>1) {
        throw std::invalid_argument("Native badge requires indexed UNITS and256-entry common palette, level0..6 and variant0..1");
    }
    const int x0=1+24*level,y0=700+31*variant;
    if(units->w<x0+int(SmacNativeBadge::Width) || units->h<y0+int(SmacNativeBadge::Height)) {
        throw std::invalid_argument("Native badge template exceeds UNITS bounds");
    }
    using RGB=std::array<uint8_t,3>;
    std::array<RGB,256> palette{};
    for(size_t i=0;i<236;++i) {const auto& c=common->colors[i];palette[i+10]={c.r,c.g,c.b};}
    const std::pair<size_t,RGB> reserved[]{
        {0, {0,0,0}},
        {1, {128,0,0}},
        {2, {0,128,0}},
        {3, {128,128,0}},
        {4, {0,0,128}},
        {5, {128,0,128}},
        {6, {0,128,128}},
        {7, {192,192,192}},
        {8, {8,0,0}},
        {9, {9,0,0}},
        {246, {245,0,0}},
        {247, {246,0,0}},
        {248, {128,128,128}},
        {249, {255,0,0}},
        {250, {0,255,0}},
        {251, {255,255,0}},
        {252, {0,0,255}},
        {253, {255,0,255}},
        {254, {0,255,255}},
        {255, {255,255,255}}
    };
    for(const auto& row:reserved) {palette[row.first]=row.second;}
    SmacNativeBadge result;
    for(size_t y=0;y<SmacNativeBadge::Height;++y) {
        const auto* row=static_cast<const uint8_t*>(units->pixels)+size_t(y0+int(y))*size_t(units->pitch)+x0;
        for(size_t x=0;x<SmacNativeBadge::Width;++x) {
            auto g=static_cast<uint8_t>(row[x]+10);
            if(g==6 || g==7) {g=9;}
            if(g==4) {g=249;} else if(g==3) {g=1;}
            const auto index=y*SmacNativeBadge::Width+x;result.indexed[index]=g;
            if(g!=9) {const auto& c=palette[g];result.rgba[index*4]=c[0];result.rgba[index*4+1]=c[1];result.rgba[index*4+2]=c[2];result.rgba[index*4+3]=255;}
        }
    }
    return result;
}
}
