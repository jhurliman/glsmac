#pragma once
#include "SmacTerrainRaster.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <vector>

namespace game::frontend::native_terrain {
using Point=smac_terrain_raster::Point;
struct Placement {Point tile,flat_center;uint32_t record_offset;};
struct ViewLayout {
    Point first;
    std::array<int,4> span;
    int unit;
    bool horizontal_wrap;
    Point world{{80,80}},viewport{{1024,511}},offset{{0,0}};
};
using ViewRegions=std::array<std::array<int,4>,2>;
// Logical viewport and world dimensions are retained with the layout so that
// geometry, selection, and incremental damage use the same coordinate system.
inline ViewLayout Layout(int x,int y,int zoom,bool wrap,
    Point world={{80,80}},Point viewport={{1024,511}}) {
    const int w=world[0],h=world[1],vw=viewport[0],vh=viewport[1];
    if(w<=0 || h<=0 || (w&1) || (h&1) || w>65534 || h>65534 ||
        uint64_t(w/2)*uint64_t(h)>UINT32_MAX/44u ||
        vw<=0 || vh<=0 || vw>32767 || vh>32767 ||
        x<0 || x>=w || y<0 || y>=h || zoom< -14 || zoom>16) {
        throw std::invalid_argument("Unsupported native main-view configuration");
    }
    const int unit=std::max(1,(50*(zoom+16)/16+1)/4);
    const auto ceil_div=[](int n,int d) {return (n+d-1)/d;};
    int sx=ceil_div(vw,8*unit),sy=ceil_div(vh+2*unit,4*unit);
    int ox=ceil_div(vw+4*unit,8*unit),oy=sy,px=0,py=0;
    if(sx>w/2) {sx=w/2;ox=sx+1;px=(vw-sx*8*unit)/2;}
    if(sy>h/2) {sy=h/2;oy=sy+1;py=(vh-sy*4*unit)/2;}
    sx=std::min(sx,w/2);ox=std::min(ox,w/2);
    int fx=x-sx;const int fy=std::clamp(y-sy,-2,h+2*(2-sy));
    if(wrap) {if(fx<0) {fx+=w;}else if(fx>=w) {fx-=w;}}
    else {fx=std::clamp(fx,-4,w+2*(2-sx));}
    fx-=((fx%2)+2)%2;fx+=(fy%2+2)%2;
    return {{{fx,fy}},{{sx,sy,ox,oy}},unit,wrap,world,viewport,{{px,py}}};
}
inline std::vector<Placement> SelectView(const ViewLayout& view,
    const std::optional<ViewRegions>& regions=std::nullopt,bool objects=false) {
    const auto& s=view.span;std::vector<Placement> result;
    for(int row=0;row<=s[1]+s[3];++row) {
        const int y=view.first[1]+row;if(y<0 || y>=view.world[1]) {continue;}
        for(int col=0;col<(row&1?s[0]:s[2]);++col) {
            const int visible_x=view.first[0]+(row&1)+2*col;
            const int x=view.horizontal_wrap?(visible_x%view.world[0]+view.world[0])%view.world[0]:visible_x;
            if(x<0 || x>=view.world[0]) {continue;}
            if(regions) {
                bool selected=false;
                for(const auto& rect:*regions) {
                    const int expand=objects?2:0;
                    const int l=rect[0]-expand,t=rect[1]-expand,r=rect[2]+expand,b=rect[3]+expand;
                    if(t<=y && y<=b && (l<=r?(l<=x && x<r):(x>=l || x<r))) {selected=true;break;}
                }
                if(!selected) {continue;}
            }
            result.push_back({{{x,y}},{{view.offset[0]+(visible_x-view.first[0])*4*view.unit,view.offset[1]+row*2*view.unit}},
                44u*static_cast<uint32_t>((view.world[0]/2)*y+x/2)});
        }
    }
    return result;
}
// Measured small interior horizontal pans only. Other incremental damage shapes
// require their own native controls; absence must not be called a faithful pan.
inline std::optional<ViewRegions> HorizontalPan(const ViewLayout& old,const ViewLayout& next) {
    const int a=old.first[0],b=next.first[0];const auto& s=next.span;
    if(old.first[1]!=next.first[1] || old.unit!=next.unit || old.span!=s ||
        old.horizontal_wrap!=next.horizontal_wrap || old.world!=next.world ||
        old.viewport!=next.viewport || old.offset!=next.offset || std::abs(b-a)!=2 ||
        std::min(a,b)<=2 || std::max(a,b)+s[0]+s[2]+1>=next.world[0]) {return {};}
    const int y=next.first[1];
    return ViewRegions{{b>a?std::array<int,4>{{a+s[0]+s[2]-3,y,b+s[0]+s[2]+1,y+s[1]+s[3]}}:
        std::array<int,4>{{b-1,y,a+2,y+s[1]+s[3]}},{{0,0,0,0}}}};
}
}
