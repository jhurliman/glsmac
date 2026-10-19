#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace types::texture {

// RE-SMAC-INDEXED-LINE: ordered opaque byte stores. Clipping skips the original
// prefix without restarting its strict-positive midpoint error phase.
template<class Store>
void SmacIndexedLine(std::array<int32_t,4> line,std::array<int32_t,4> clip,int32_t color,Store store,bool backed=true) {
    const auto [left,top,right,bottom]=clip;
    if(left>=right || top>=bottom) {throw std::invalid_argument("Indexed line requires a nonempty clip");}
    if(!backed) {return;}
    int64_t x0=line[0],y0=line[1],x1=line[2],y1=line[3];
    if(x0==x1 && y0==y1) {return;}
    const auto ink=static_cast<uint8_t>(color);
    if(x0==x1) {
        if(x0<left || x0>=right) {return;}
        for(int64_t y=std::max<int64_t>(std::min(y0,y1),top);y<=std::min<int64_t>(std::max(y0,y1),int64_t(bottom)-1);++y) {store(int32_t(x0),int32_t(y),ink);}
        return;
    }
    if(y0==y1) {
        if(y0<top || y0>=bottom) {return;}
        for(int64_t x=std::max<int64_t>(std::min(x0,x1),left);x<=std::min<int64_t>(std::max(x0,x1),int64_t(right)-1);++x) {store(int32_t(x),int32_t(y0),ink);}
        return;
    }
    if(x0>x1) {std::swap(x0,x1);std::swap(y0,y1);}
    const int64_t dx=x1-x0,dy=y1>y0?y1-y0:y0-y1,sy=y1>y0?1:-1;
    // Wider coordinates/overflowing native error arithmetic are unmeasured.
    if(dx>INT32_MAX/2 || dy>INT32_MAX/2) {throw std::invalid_argument("Indexed line exceeds admitted signed32 stepping");}
    const bool x_major=dx>=dy;const int64_t major=x_major?dx:dy,minor=x_major?dy:dx;
    const int64_t first=x_major?std::max<int64_t>(0,int64_t(left)-x0)
        :std::max<int64_t>(0,sy>0?int64_t(top)-y0:y0-(int64_t(bottom)-1));
    const int64_t last=x_major?std::min<int64_t>(major,int64_t(right)-1-x0)
        :std::min<int64_t>(major,sy>0?int64_t(bottom)-1-y0:y0-top);
    if(first>last) {return;}
    // Exact strict-positive minor increments in the skipped prefix. Iterate
    // only the visible major span, even for a tall historical peak.
    const int64_t increments=(2*minor*first+major-1)/(2*major);
    int64_t x=x0+(x_major?first:increments),y=y0+sy*(x_major?increments:first);
    int64_t error=2*minor-major+2*minor*first-2*major*increments;
    for(int64_t step=first;step<=last;++step) {
        if(x>=left && x<right && y>=top && y<bottom) {store(int32_t(x),int32_t(y),ink);}
        if(error>0) {if(x_major) {y+=sy;}else {++x;}error+=2*(minor-major);}
        else {error+=2*minor;}
        if(x_major) {++x;}else {y+=sy;}
    }
}

}
