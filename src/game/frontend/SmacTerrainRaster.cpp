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
    const auto denominator=static_cast<uint32_t>(width);
    uint64_t reciprocal=(uint64_t{1}<<63)/denominator,remainder=(uint64_t{1}<<63)%denominator;
    for(unsigned bit=0;bit<k;++bit) {
        remainder*=2;
        reciprocal<<=1;
        if(remainder>=denominator) {remainder-=denominator;reciprocal|=1;}
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
Polygon TrianglePolygon(const Triangle& triangle) {
    Polygon result;result.count=3;
    std::copy(triangle.begin(),triangle.end(),result.vertices.begin());return result;
}
Clip Intersection(const Clip& a,const Clip& b) {
    return {{std::max(a[0],b[0]),std::max(a[1],b[1]),std::min(a[2],b[2]),std::min(a[3],b[3])}};
}
}

bool Span::operator==(const Span& b) const {
    return std::tie(y,left,right,u,v,du,dv,shade,dshade)==std::tie(b.y,b.left,b.right,b.u,b.v,b.du,b.dv,b.shade,b.dshade);
}

std::vector<Span> GenerateSpans(const Triangle& triangle,const Clip& clip) {
    return GeneratePolygonSpans(TrianglePolygon(triangle),clip,true,{{-256,256}});
}

std::vector<Span> GeneratePolygonSpans(const Polygon& polygon,const Clip& clip,bool shaded,std::array<int32_t,2> shade_limits) {
    const size_t count=polygon.count;
    if((count!=3 && count!=4) || shade_limits[0]< -256 || shade_limits[1]>256 || shade_limits[0]>shade_limits[1]) {Invalid();}
    const auto& vertices=polygon.vertices;
    const int bias=shaded?32768:0;
    std::array<int32_t,4> light{};
    size_t low=0,high=0;
    for(size_t i=0;i<count;++i) {
        const auto& v=vertices[i];
        for(auto coordinate:v.screen) {if(coordinate<=-32765 || coordinate>=32767) {Invalid();}}
        for(auto coordinate:v.uv) {if(coordinate< -8192 || coordinate>=8192) {Invalid();}}
        light[i]=shaded?std::clamp(v.light,shade_limits[0],shade_limits[1]):0;
        if(v.screen[1]<vertices[low].screen[1]) {low=i;}
        if(v.screen[1]>vertices[high].screen[1]) {high=i;}
    }
    for(size_t axis=0;axis<2;++axis) {
        auto minimum=vertices[0].screen[axis],maximum=minimum;
        for(size_t i=1;i<count;++i) {minimum=std::min(minimum,vertices[i].screen[axis]);maximum=std::max(maximum,vertices[i].screen[axis]);}
        if(maximum-minimum>=32768) {Invalid();}
    }
    std::vector<Span> result;
    const auto y0=vertices[low].screen[1],y1=vertices[high].screen[1];
    if(y0==y1 || clip[0]>=clip[2] || clip[1]>=clip[3]) {return result;}
    const auto edge=[&](size_t start,int direction)->std::optional<Edge> {
        if(start==high) {return {};}
        const auto next=[&](size_t index) {return static_cast<size_t>((static_cast<int>(index)+direction+static_cast<int>(count))%static_cast<int>(count));};
        auto end=next(start);
        while(vertices[end].screen[1]==vertices[start].screen[1]) {
            start=end;if(start==high) {return {}; }end=next(start);
        }
        const auto dy=vertices[end].screen[1]-vertices[start].screen[1];
        if(dy<=0) {return {};}
        const auto dx=vertices[end].screen[0]-vertices[start].screen[0];
        const auto u=vertices[start].uv[0]*65536+bias,v=vertices[start].uv[1]*65536+bias,s=light[start]*65536;
        return Edge{end,direction,dy,dy,vertices[start].screen[0],dx/dy,
            (dx<0?-dx:dx)%dy,dx<0?1-dy:0,dx<0?-1:1,u,v,s,
            (vertices[end].uv[0]*65536-u)/dy,(vertices[end].uv[1]*65536-v)/dy,(light[end]*65536-s)/dy};
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
    ValidateBitmap(texture);
    std::vector<Command> commands;commands.reserve(triangles.size());
    for(const auto& triangle:triangles) {commands.push_back({TrianglePolygon(triangle),&texture,Shader::OpaqueShaded,clip,{{-256,256}}});}
    return DrawBatch(initial,commands,clip);
}

Result DrawBatch(const Bitmap& initial,const std::vector<Command>& commands,const Clip& clip,bool track_coverage) {
    ValidateBitmap(initial);
    for(const auto& command:commands) {
        if(!command.texture || (command.shader!=Shader::OpaqueShaded && command.shader!=Shader::KeyedShaded && command.shader!=Shader::KeyedUnlit && command.shader!=Shader::WaterDepth16 && command.shader!=Shader::CoastMapped && command.shader!=Shader::OwnerClut && command.shader!=Shader::Sprite && command.shader!=Shader::SpriteShadow)) {Invalid();}
        ValidateBitmap(*command.texture);
        if(command.shader==Shader::OwnerClut && (!command.owner_clut || std::any_of(command.owner_mapping.begin(),command.owner_mapping.end(),[](auto selector){return selector>8;}))) {Invalid();}
        if(command.shader==Shader::CoastMapped) {
            if(!command.mask) {Invalid();}
            ValidateBitmap(*command.mask);
            if(command.mask->width!=command.texture->width || command.mask->height!=command.texture->height) {Invalid();}
        }
        if(command.shader==Shader::Sprite || command.shader==Shader::SpriteShadow) {
            for(auto coordinate:command.sprite_origin) {if(coordinate < -32768 || coordinate > 32767) {Invalid();}}
            if(command.shader==Shader::SpriteShadow && !command.shadow_lookup) {Invalid();}
        }else if(command.polygon.count!=3 && command.polygon.count!=4) {Invalid();}
    }
    const auto bounded=Intersection(clip,{{0,0,initial.width,initial.height}});
    Result result{initial,0};
    if(track_coverage) {result.coverage.resize(initial.pixels.size());}
    for(const auto& command:commands) {
        const auto& texture=*command.texture;
        if(command.shader==Shader::Sprite || command.shader==Shader::SpriteShadow) {
            const auto visible=Intersection(Intersection(command.clip,bounded),{{command.sprite_origin[0],command.sprite_origin[1],
                Signed32(int64_t(command.sprite_origin[0])+texture.width),Signed32(int64_t(command.sprite_origin[1])+texture.height)}});
            for(int y=visible[1];y<visible[3];++y) {for(int x=visible[0];x<visible[2];++x) {
                const auto src=static_cast<size_t>(y-command.sprite_origin[1])*static_cast<size_t>(texture.width)+static_cast<size_t>(x-command.sprite_origin[0]);
                const auto dst=static_cast<size_t>(y)*static_cast<size_t>(initial.width)+static_cast<size_t>(x);
                const auto ink=texture.pixels[src];++result.attempts;
                if(ink==9) {continue;}
                result.bitmap.pixels[dst]=command.shader==Shader::SpriteShadow && ink==0?(*command.shadow_lookup)[result.bitmap.pixels[dst]]:ink;
                ++result.writes;if(track_coverage) {result.coverage[dst]=1;}
            }}
            continue;
        }
        const bool shaded=command.shader!=Shader::KeyedUnlit;
        const bool keyed=command.shader==Shader::KeyedShaded || command.shader==Shader::KeyedUnlit;
        if(command.shader==Shader::WaterDepth16 && std::all_of(command.polygon.vertices.begin(),
            command.polygon.vertices.begin()+command.polygon.count,[](const auto& vertex){return vertex.light>=16;})) {continue;}
        // Native6393d0 interpolates coast depth without the ordinary terrain
        // shade caps, even while the caller globals remain[-4,+4].
        const auto limits=command.shader==Shader::CoastMapped?std::array<int32_t,2>{{-256,256}}:command.shade_limits;
        for(const auto& span:GeneratePolygonSpans(command.polygon,Intersection(command.clip,bounded),shaded,limits)) {
            int64_t u=span.u,v=span.v;
            auto shade=span.shade;
            for(auto x=span.left;x<span.right;++x) {
                const auto offset=Floor(v,65536)*texture.width+Floor(u,65536);
                if(offset<0 || static_cast<uint64_t>(offset)>=texture.pixels.size()) {Invalid();}
                const auto loaded=texture.pixels[static_cast<size_t>(offset)];
                ++result.attempts;
                int signed_shade=shade>>8;if(signed_shade>=128) {signed_shade-=256;}
                uint8_t mapped=0;
                if(command.shader==Shader::CoastMapped) {
                    const auto cls=command.mask->pixels[static_cast<size_t>(offset)];
                    if(cls>=command.mask_mapping.size()) {Invalid();}
                    mapped=command.mask_mapping[cls];
                }
                uint8_t owner_selector=0;
                if(command.shader==Shader::OwnerClut) {
                    if(loaded>=command.owner_mapping.size()) {Invalid();}
                    owner_selector=command.owner_mapping[loaded];
                }
                const bool store=(!keyed || loaded!=9) &&
                    (command.shader!=Shader::WaterDepth16 || signed_shade<16) &&
                    (command.shader!=Shader::CoastMapped || mapped!=0) &&
                    (command.shader!=Shader::OwnerClut || owner_selector!=0);
                if(store) {
                    result.bitmap.pixels[static_cast<size_t>(span.y)*static_cast<size_t>(initial.width)+static_cast<size_t>(x)]=
                        command.shader==Shader::OwnerClut?(*command.owner_clut)[owner_selector-1][result.bitmap.pixels[static_cast<size_t>(span.y)*static_cast<size_t>(initial.width)+static_cast<size_t>(x)]]:
                        command.shader==Shader::CoastMapped && mapped>1?mapped:static_cast<uint8_t>(loaded+(shaded?signed_shade:0));
                    ++result.writes;
                    if(track_coverage) {result.coverage[static_cast<size_t>(span.y)*static_cast<size_t>(initial.width)+static_cast<size_t>(x)]=1;}
                }
                u+=span.du;v+=span.dv;shade=static_cast<uint16_t>(shade+span.dshade);
                Signed32(u);Signed32(v);
            }
        }
    }
    return result;
}

}
