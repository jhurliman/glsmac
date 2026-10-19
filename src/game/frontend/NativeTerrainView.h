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
};
using ViewRegions=std::array<std::array<int,4>,2>;
inline ViewLayout Layout(int x,int y,int zoom,bool wrap) {
    if(x<0 || x>=80 || y<0 || y>=80 || zoom< -1 || zoom>8) {
        throw std::invalid_argument("Unsupported native main-view configuration");
    }
    const int unit=std::max(1,(50*(zoom+16)/16+1)/4);
    const auto ceil_div=[](int n,int d) {return (n+d-1)/d;};
    const int sx=ceil_div(1024,8*unit),sy=ceil_div(511+2*unit,4*unit);
    const int ox=ceil_div(1024+4*unit,8*unit);
    int fx=x-sx;const int fy=std::clamp(y-sy,-2,80+2*(2-sy));
    if(wrap) {fx=(fx%80+80)%80;}else {fx=std::clamp(fx,-4,80+2*(2-sx));}
    fx-=((fx%2)+2)%2;fx+=(fy%2+2)%2;
    return {{{fx,fy}},{{sx,sy,ox,sy}},unit,wrap};
}
inline std::vector<Placement> SelectView(const ViewLayout& view,
    const std::optional<ViewRegions>& regions=std::nullopt,bool objects=false) {
    const auto& s=view.span;std::vector<Placement> result;
    for(int row=0;row<=s[1]+s[3];++row) {
        const int y=view.first[1]+row;if(y<0 || y>=80) {continue;}
        for(int col=0;col<(row&1?s[0]:s[2]);++col) {
            const int visible_x=view.first[0]+(row&1)+2*col;
            const int x=view.horizontal_wrap?(visible_x%80+80)%80:visible_x;
            if(x<0 || x>=80) {continue;}
            if(regions) {
                bool selected=false;
                for(const auto& rect:*regions) {
                    const int expand=objects?2:0;
                    const int l=rect[0]-expand,t=rect[1]-expand,r=rect[2]+expand,b=rect[3]+expand;
                    if(t<=y && y<=b && (l<=r?(l<=x && x<r):(x>=l || x<r))) {selected=true;break;}
                }
                if(!selected) {continue;}
            }
            result.push_back({{{x,y}},{{(visible_x-view.first[0])*4*view.unit,row*2*view.unit}},
                static_cast<uint32_t>(44*(40*y+x/2))});
        }
    }
    return result;
}
// Measured small interior horizontal pans only. Other incremental damage shapes
// require their own native controls; absence must not be called a faithful pan.
inline std::optional<ViewRegions> HorizontalPan(const ViewLayout& old,const ViewLayout& next) {
    const int a=old.first[0],b=next.first[0];const auto& s=next.span;
    if(old.first[1]!=next.first[1] || old.unit!=next.unit || old.span!=s ||
        old.horizontal_wrap!=next.horizontal_wrap || std::abs(b-a)!=2 ||
        std::min(a,b)<=2 || std::max(a,b)+s[0]+s[2]+1>=80) {return {};}
    const int y=next.first[1];
    return ViewRegions{{b>a?std::array<int,4>{{a+s[0]+s[2]-3,y,b+s[0]+s[2]+1,y+s[1]+s[3]}}:
        std::array<int,4>{{b-1,y,a+2,y+s[1]+s[3]}},{{0,0,0,0}}}};
}
}
