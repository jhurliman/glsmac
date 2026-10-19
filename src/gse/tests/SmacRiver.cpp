#include "SmacRiver.h"
#include "fixtures/SmacRiver.h"
#include "game/frontend/NativeTerrainRenderer.h"
#include "game/frontend/SmacWaterCoastTables.h"
#include "game/backend/map/SmacTerrainGeometry.h"
#include "game/backend/map/SmacTerrainLighting.h"
#include "task/gsetests/GSETests.h"
#include <algorithm>
#include <stdexcept>

namespace gse::tests {
namespace {
namespace water=game::frontend::smac_water_coast;
namespace raster=game::frontend::smac_terrain_raster;
bool Equal(const water::Pass& a,const water::Pass& b) {
    if(a.crop!=b.crop || a.shader!=b.shader || a.shade_limits!=b.shade_limits || a.mapping!=b.mapping || a.polygon.count!=b.polygon.count) return false;
    for(size_t i=0;i<a.polygon.count;++i) {
        const auto& x=a.polygon.vertices[i];const auto& y=b.polygon.vertices[i];
        if(x.screen!=y.screen || x.uv!=y.uv || x.light!=y.light) return false;
    }
    return true;
}
uint64_t Digest(const std::vector<raster::Span>& spans) {
    uint64_t result=14695981039346656037ULL;
    for(const auto& s:spans) {
        for(const auto n:{s.y,s.left,s.right,s.u,s.v,s.du,s.dv,int(s.shade),int(s.dshade)}) {
            for(int shift=0;shift<32;shift+=8) {result=(result^static_cast<uint8_t>(static_cast<uint32_t>(n)>>shift))*1099511628211ULL;}
        }
    }
    return result;
}
}
void AddSmacRiverTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC river producer preserves33 native live-item adjacency and draw-order contexts",GT(){
        size_t commands=0,contexts=0;
        for(const auto& c:river_fixture::Contexts()) {
            const water::CellAt at=[&](int x,int y)->std::optional<water::Cell> {
                if(y<0 || y>=80) return {};
                if(c.wrap) x=(x%80+80)%80;
                if(x<0 || x>=80) return {};
                if(x==c.x && y==c.y) return water::Cell{c.center_band,c.center_contour,c.items};
                for(const auto& patch:c.patch) {
                    if(x==(c.x+patch[0]+80)%80 && y==c.y+patch[1]) return water::Cell{static_cast<uint8_t>(patch[2]),static_cast<uint8_t>(patch[3]),static_cast<uint32_t>(patch[4])};
                }
                return water::Cell{c.band,c.contour};
            };
            const auto view=game::frontend::native_terrain::View(c.x,c.y,c.wrap);
            const auto center=std::find_if(view.begin(),view.end(),[&](const auto& p){return p.tile==raster::Point{c.x,c.y};});
            GT_ASSERT(center!=view.end(),"Native water camera omitted center");
            const auto h=game::backend::map::smac_terrain::OrdinaryHeights(c.x,c.y,[&](int x,int y)->std::optional<uint8_t>{const auto cell=at(x,y);return cell?std::optional<uint8_t>{cell->band}:std::nullopt;});
            const auto x=center->flat_center[0],y=center->flat_center[1];
            const std::array<raster::Point,5> points{{{{x,y-h[0]}},{{x-48,y-h[1]}},{{x,y-24-h[2]}},{{x+48,y-h[3]}},{{x,y+24-h[4]}}}};
            game::backend::map::smac_terrain::Lighting lighting{};
            if(c.center_band>=3) {
                const auto light=game::backend::map::smac_terrain::OrdinaryLighting(c.x,c.y,[&](int tx,int ty)->std::optional<uint8_t>{const auto cell=at(tx,ty);return cell?std::optional<uint8_t>{cell->band}:std::nullopt;});
                GT_ASSERT(light.has_value(),"River context lighting missing");lighting=*light;
            }
            const auto actual=game::frontend::smac_river::Select(c.x,c.y,points,lighting,at,c.flags);
            GT_ASSERT(actual.size()==c.expected.size(),"River command count differs: "+std::string(c.name));
            for(size_t i=0;i<actual.size();++i) GT_ASSERT(Equal(actual[i],c.expected[i]),"River polygon/material/order differs: "+std::string(c.name));
            commands+=actual.size();
            ++contexts;
        }
        GT_ASSERT(commands==30 && contexts==33,"Missing native River contexts");GT_OK();
    });
    task->AddTest("SMAC river raster preserves5103 native composed spans",GT(){
        size_t count=0;
        for(const auto& f:river_fixture::Frames()) {for(const auto& draw:f.draws) {
            const auto& p=draw.pass;
            const bool skip=p.shader==raster::Shader::WaterDepth16 && std::all_of(p.polygon.vertices.begin(),p.polygon.vertices.end(),[](const auto& v){return v.light>=16;});
            const auto spans=skip?std::vector<raster::Span>{}:raster::GeneratePolygonSpans(p.polygon,f.clip,p.shader!=raster::Shader::KeyedUnlit,p.shade_limits);
            GT_ASSERT(spans.size()==draw.spans && Digest(spans)==draw.span_hash,"Original River composed spans differ: "+std::string(f.name));count+=spans.size();
        }}
        GT_ASSERT(count==5103,"Missing native River raster coverage");GT_OK();
    });

}
}
