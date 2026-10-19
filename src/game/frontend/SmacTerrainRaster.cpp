#include "SmacTerrainRaster.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <tuple>

namespace game::frontend::smac_terrain_raster {
namespace {
[[noreturn]] void Invalid() { throw std::invalid_argument("SMAC ordinary indexed terrain: unsupported input or arithmetic"); }
int32_t Signed32(int64_t value) {
    if(value < std::numeric_limits<int32_t>::min() || value > std::numeric_limits<int32_t>::max()) {Invalid();}
    return static_cast<int32_t>(value);
}
unsigned Width(uint64_t value) {unsigned n=0;while(value) {++n;value>>=1;}return n;}
int64_t Floor(int64_t value,int64_t divisor) {
    return value>=0 ? value/divisor : -((-value+divisor-1)/divisor);
}

// 1/(width*65536), chopped to a 64-bit significand, followed by the
// independently chopped product with delta. Multiplication by a power of two
// is exact. This avoids long double (binary64 on MSVC) and global FP modes.
int32_t HorizontalStep(int64_t delta,int32_t width,unsigned scale_power) {
    Signed32(delta);
    if(!delta) {return 0;}
    const auto k=Width(static_cast<uint32_t>(width-1));
    uint64_t reciprocal=0,remainder=0;
    for(int bit=static_cast<int>(63+k);bit>=0;--bit) {
        remainder=remainder*2+(bit==static_cast<int>(63+k)?1:0);
        reciprocal<<=1;
        if(remainder>=static_cast<uint32_t>(width)) {remainder-=static_cast<uint32_t>(width);reciprocal|=1;}
    }
    const uint64_t magnitude=delta<0?static_cast<uint64_t>(-delta):static_cast<uint64_t>(delta);
    const uint64_t low=uint64_t(uint32_t(reciprocal))*magnitude;
    const uint64_t middle=(reciprocal>>32)*magnitude+(low>>32);
    const uint64_t high=middle>>32,product_low=(middle<<32)|uint32_t(low);
    const auto chop=Width(high);
    const uint64_t product=chop ? (high<<(64-chop))|(product_low>>chop) : product_low;
    const unsigned shift=63+16+k-chop-scale_power;
    const auto integer=shift>=64?0:static_cast<int64_t>(product>>shift);
    return Signed32(delta<0?-integer:integer);
}

struct Edge {
    size_t end;
    int direction,remaining,dy;
    int32_t x,q,remainder,error,sign,u,v,shade,du,dv,dshade;
};

void ValidateBitmap(const Bitmap& bitmap) {
    if(bitmap.width<=0 || bitmap.height<=0 || bitmap.width>=32767 || bitmap.height>=32767 ||
        static_cast<uint64_t>(bitmap.width)*static_cast<uint32_t>(bitmap.height)!=bitmap.pixels.size()) {Invalid();}
}
}

bool Span::operator==(const Span& b) const {
    return std::tie(y,left,right,u,v,du,dv,shade,dshade)==std::tie(b.y,b.left,b.right,b.u,b.v,b.du,b.dv,b.shade,b.dshade);
}

std::vector<Span> GenerateSpans(const Triangle& triangle,const Clip& clip) {
    std::array<int32_t,3> light{};
    size_t low=0,high=0;
    for(size_t i=0;i<3;++i) {
        const auto& v=triangle[i];
        for(auto coordinate:v.screen) {if(coordinate<=-32765 || coordinate>=32767) {Invalid();}}
        for(auto coordinate:v.uv) {if(coordinate< -8192 || coordinate>=8192) {Invalid();}}
        light[i]=std::clamp(v.light,-256,256);
        if(v.screen[1]<triangle[low].screen[1]) {low=i;}
        if(v.screen[1]>triangle[high].screen[1]) {high=i;}
    }
    for(size_t axis=0;axis<2;++axis) {
        const auto bounds=std::minmax({triangle[0].screen[axis],triangle[1].screen[axis],triangle[2].screen[axis]});
        if(bounds.second-bounds.first>=32768) {Invalid();}
    }
    std::vector<Span> result;
    const auto y0=triangle[low].screen[1],y1=triangle[high].screen[1];
    if(y0==y1 || clip[0]>=clip[2] || clip[1]>=clip[3]) {return result;}
    const auto edge=[&](size_t start,int direction)->std::optional<Edge> {
        if(start==high) {return {};}
        const auto next=[&](size_t index) {return static_cast<size_t>((static_cast<int>(index)+direction+3)%3);};
        auto end=next(start);
        while(triangle[end].screen[1]==triangle[start].screen[1]) {
            start=end;if(start==high) {return {}; }end=next(start);
        }
        const auto dy=triangle[end].screen[1]-triangle[start].screen[1];
        if(dy<=0) {return {};}
        const auto dx=triangle[end].screen[0]-triangle[start].screen[0];
        const auto u=triangle[start].uv[0]*65536+32768,v=triangle[start].uv[1]*65536+32768,s=light[start]*65536;
        return Edge{end,direction,dy,dy,triangle[start].screen[0],dx/dy,
            (dx<0?-dx:dx)%dy,dx<0?1-dy:0,dx<0?-1:1,u,v,s,
            (triangle[end].uv[0]*65536-u)/dy,(triangle[end].uv[1]*65536-v)/dy,(light[end]*65536-s)/dy};
    };
    const auto advance=[&](std::optional<Edge> e)->std::optional<Edge> {
        if(!e) {return {};}
        if(!--e->remaining) {return edge(e->end,e->direction);}
        e->x=Signed32(int64_t(e->x)+e->q);
        e->u=Signed32(int64_t(e->u)+e->du);e->v=Signed32(int64_t(e->v)+e->dv);
        e->shade=Signed32(int64_t(e->shade)+e->dshade);
        e->error+=e->remainder;
        if(e->error>0) {e->x+=e->sign;e->error-=e->dy;}
        return e;
    };
    auto left=edge(low,-1),right=edge(low,1);
    for(auto y=y0;y<std::min(y1,clip[3]);++y) {
        if(!left || !right) {break;}
        const auto l=*left,r=*right;left=advance(left);right=advance(right);
        const auto width=r.x-l.x;
        if(width<=0 || y<clip[1] || r.x<=clip[0] || l.x>=clip[2]) {continue;}
        const auto du=HorizontalStep(int64_t(r.u)-l.u,width,16),dv=HorizontalStep(int64_t(r.v)-l.v,width,16);
        const auto raw_shade=HorizontalStep(int64_t(r.shade)-l.shade,width,8);
        const auto dshade=static_cast<uint16_t>(raw_shade< -32768 || raw_shade>32767 ? -32768 : raw_shade);
        const auto x0=std::max(l.x,clip[0]),x1=std::min(r.x,clip[2]);
        const int64_t skip=int64_t(x0)-l.x;
        result.push_back({y,x0,x1,Signed32(l.u+skip*du),Signed32(l.v+skip*dv),du,dv,
            static_cast<uint16_t>(Floor(l.shade,256)+skip*dshade),dshade});
    }
    return result;
}

Result Draw(const Bitmap& initial,const Bitmap& texture,const std::vector<Triangle>& triangles,const Clip& clip) {
    ValidateBitmap(initial);ValidateBitmap(texture);
    const Clip bounded{{std::max(0,clip[0]),std::max(0,clip[1]),std::min(initial.width,clip[2]),std::min(initial.height,clip[3])}};
    Result result{initial,0};
    for(const auto& triangle:triangles) {
        for(const auto& span:GenerateSpans(triangle,bounded)) {
            int64_t u=span.u,v=span.v;
            auto shade=span.shade;
            for(auto x=span.left;x<span.right;++x) {
                const auto offset=Floor(v,65536)*texture.width+Floor(u,65536);
                if(offset<0 || static_cast<uint64_t>(offset)>=texture.pixels.size()) {Invalid();}
                result.bitmap.pixels[static_cast<size_t>(span.y)*static_cast<size_t>(initial.width)+static_cast<size_t>(x)]=
                    static_cast<uint8_t>(texture.pixels[static_cast<size_t>(offset)]+(shade>>8));
                ++result.writes;u+=span.du;v+=span.dv;shade=static_cast<uint16_t>(shade+span.dshade);
                Signed32(u);Signed32(v);
            }
        }
    }
    return result;
}

}
