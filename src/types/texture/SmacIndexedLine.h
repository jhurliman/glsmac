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
    const bool ascending=sy>0;
    if(x0>right || x1<left || (ascending?(y0>bottom || y1<top):(y0<top || y1>bottom))) {return;}
    const bool x_major=dx>=dy;const int64_t major=x_major?dx:dy,minor=x_major?dy:dx;
    const auto point=[&](int64_t step) {
        const int64_t increments=(2*minor*step+major-1)/(2*major);
        return std::array<int64_t,2>{{x0+(x_major?step:increments),y0+sy*(x_major?increments:step)}};
    };
    // Native5fa3f0 advances to the entry clip without first checking the
    // original segment length. Preserve the unconditional first store even
    // when a descending segment just touches the exclusive bottom edge.
    // Bounding rejection above limits this search to at most two lengths.
    int64_t first=0,last=2*major+1;
    while(first<last) {
        const auto middle=first+(last-first)/2;const auto p=point(middle);
        if(p[0]<left || (ascending?p[1]<top:p[1]>=bottom)) {first=middle+1;}else {last=middle;}
    }
    const auto p=point(first);int64_t x=p[0],y=p[1];
    if(x<left || x>=right || y<top || y>=bottom) {return;}
    store(int32_t(x),int32_t(y),ink);
    const int64_t increments=(2*minor*first+major-1)/(2*major);
    int64_t error=2*minor-major+2*minor*first-2*major*increments;
    for(int64_t step=first;step<major;++step) {
        if(error>0) {if(x_major) {y+=sy;}else {++x;}error+=2*(minor-major);}
        else {error+=2*minor;}
        if(x_major) {++x;}else {y+=sy;}
        if(x<left || x>=right || y<top || y>=bottom) {break;}
        store(int32_t(x),int32_t(y),ink);
    }
}

}
