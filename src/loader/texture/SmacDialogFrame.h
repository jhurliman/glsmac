#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace loader::texture {
struct SmacDialogSource {
    static constexpr int Width=750,Height=900;
    std::vector<uint8_t> indexed;
    std::array<std::array<uint8_t,3>,256> palette{};
};
namespace smac_dialog_frame {
using Rect=std::array<int,4>;
using Call=std::array<int,5>; // style,L,T,R,B
inline void Paint(std::vector<uint8_t>& dst,int width,int height,const SmacDialogSource& source,
    const Call& call,const Rect& clip,std::array<int,2> phase={0,0}) {
    if(width<=0 || height<=0 || width>2048 || height>2048 || dst.size()!=size_t(width)*height
        || source.indexed.size()!=size_t(SmacDialogSource::Width)*SmacDialogSource::Height
        || call[0]<0 || call[0]>=51 || phase[0]<0 || phase[1]<0 || phase[0]>4096 || phase[1]>4096) {
        throw std::invalid_argument("Unsupported small dialog frame bitmap, style or phase");
    }
    for(size_t i=1;i<5;++i) {if(call[i]<-4096 || call[i]>4096) {throw std::invalid_argument("Dialog frame coordinate exceeds its supported range");}}
    const auto at=[&](int x,int y){return source.indexed[size_t(y)*SmacDialogSource::Width+size_t(x)];};
    const int sx=1+39*(call[0]%3),sy=229+39*(call[0]/3);
    const bool center=at(sx+7,sy+7)!=9;
    const auto pattern=[&](int x,int y){return at(sx+7+(x+phase[0])%24,sy+7+(y+phase[1])%24);};
    const auto each=[&](const Rect& rect,const auto& operation) {
        const int l=std::max({0,clip[0],rect[0]}),t=std::max({0,clip[1],rect[1]});
        const int r=std::min({width,clip[2],rect[2]}),b=std::min({height,clip[3],rect[3]});
        for(int y=t;y<b;++y) {for(int x=l;x<r;++x) {operation(x,y,dst[size_t(y)*width+size_t(x)]);}}
    };
    const auto piece=[&](int x,int y,int tw,int th,const Rect& rect) {
        if(rect[2]<=rect[0] || rect[3]<=rect[1]) {return;}
        each(rect,[&](int dx,int dy,uint8_t& value) {
            const auto pixel=at(x+(dx-rect[0])%tw,y+(dy-rect[1])%th);
            if(pixel!=9) {value=pixel;}
        });
        if(center) {each(rect,[&](int x,int y,uint8_t& value){if(value==7) {value=pattern(x,y);}});}
    };
    const int l=call[1],t=call[2],r=call[3],b=call[4];
    // Original order is observable for overlapping corners and destination7.
    piece(sx,sy,6,6,{l,t,l+6,t+6});piece(sx+32,sy,6,6,{r-6,t,r,t+6});
    piece(sx,sy+32,6,6,{l,b-6,l+6,b});piece(sx+32,sy+32,6,6,{r-6,b-6,r,b});
    piece(sx+7,sy,24,6,{l+6,t,r-6,t+6});piece(sx+7,sy+32,24,6,{l+6,b-6,r-6,b});
    piece(sx,sy+7,6,24,{l,t+6,l+6,b-6});piece(sx+32,sy+7,6,24,{r-6,t+6,r,b-6});
    if(center) {each({l+6,t+6,r-6,b-6},[&](int x,int y,uint8_t& value){value=pattern(x,y);});}
}
inline std::vector<uint8_t> Rgba(const std::vector<uint8_t>& indexed,const SmacDialogSource& source) {
    std::vector<uint8_t> rgba(indexed.size()*4);
    for(size_t i=0;i<indexed.size();++i) {const auto index=indexed[i];
        for(size_t c=0;c<3;++c) {rgba[4*i+c]=source.palette[index][c];}rgba[4*i+3]=index==9?0:255;
    }
    return rgba;
}
}
}
