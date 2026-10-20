#pragma once
#include "NativeTerrainView.h"
#include <functional>
#include <limits>

namespace game::frontend::native_terrain {
struct PickResult {
    int code=1;
    std::optional<Point> written; // Early rejection leaves the caller's output unchanged.
};
struct PickStep {int32_t lhs,rhs;int next_x,row;};
using PickHeight=std::function<int(int,int,unsigned)>;
using PickObserver=std::function<void(const PickStep&)>;
inline int32_t PickProduct(int a,int b) {
    const uint32_t bits=uint32_t(int64_t(a)*int64_t(b));
    return bits<=INT32_MAX?int32_t(bits):int32_t(int64_t(bits)-INT64_C(4294967296));
}
// Native46fa70 walks alternating W/N and N/E edges using the shared corner
// height producer. This is neither inverse mean-height projection nor a
// rendered-triangle test. Equality advances rather than selecting the edge.
inline PickResult PickTerrain(const ViewLayout& view,Point pixel,const PickHeight& height,
    const PickObserver& observer={}) {
    const int w=view.world[0],h=view.world[1],q=view.unit;
    if(w<=0 || h<=0 || w>65534 || h>65534 || (w&1) || (h&1) || q<1 || q>25 ||
        view.viewport[0]<=0 || view.viewport[1]<=0 || view.viewport[0]>32767 || view.viewport[1]>32767 ||
        view.first[0]<-4 || view.first[0]>w+2 || view.first[1]<-2 || view.first[1]>h+2 ||
        view.offset[0]<0 || view.offset[1]<0 || view.offset[0]>view.viewport[0] || view.offset[1]>view.viewport[1]) {
        throw std::invalid_argument("Unsupported native picking view");
    }
    if(pixel[0]>=view.viewport[0] || pixel[1]>=view.viewport[1]) {return {};}
    const int64_t px64=int64_t(pixel[0])-view.offset[0],py64=int64_t(pixel[1])-view.offset[1];
    if(px64<0 || py64<0) {return {};}
    const int px=int(px64),py=int(py64),hw=4*q,hh=2*q,dw=8*q,dh=4*q;
    const int fx=view.first[0],fy=view.first[1];
    const auto wrap=[&](int x) {if(view.horizontal_wrap) {if(x<0) {x+=w;}else if(x>=w) {x-=w;}}return x;};
    const int start=wrap(fx+2*(px/dw)+1),alternate=wrap(fx+2*((px-hw)/dw)+1);
    int row=fy+2*(py/dh),x=0,base=0;bool toggle=alternate==start;
    if(toggle) {x=start+1;base=x;}else {base=start;x=start-1;}
    const int relative=base-fx,x0=hw*wrap(relative-1),x1=hw*wrap(relative);
    const int along=px-x0,span=x1-x0;
    // The frozen reference uses the same explicit bound for unsupported walks;
    // exceeding it is unavailable geometry, never a fabricated tile/miss.
    for(unsigned iteration=0;iteration<10000;++iteration) {
        int y0=0,y1=0,next_x=0;
        if(toggle) {
            const int a=height(x,row,1),b=height(x,row,2);
            y0=(row-fy)*hh-a;y1=(row-fy-1)*hh-b;next_x=x-1;toggle=false;
        }else {
            const int a=height(x,row,2),b=height(x,row,3);
            y0=(row-fy-1)*hh-a;y1=(row-fy)*hh-b;next_x=x+1;toggle=true;
        }
        const int32_t lhs=PickProduct(y1-y0,along),rhs=PickProduct(py-y0,span);
        ++row;x=next_x;if(observer) {observer({lhs,rhs,x,row});}
        if(lhs>rhs) {
            const Point result{{wrap(x),row-2}};
            return {result[0]>=0 && result[0]<w && result[1]>=0 && result[1]<h?0:1,result};
        }
    }throw std::invalid_argument("Native terrain edge walk exceeds validated bound");
}
}
