#include "SmacRoadTube.h"
#include "fixtures/SmacRoadTube.h"
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
void AddSmacRoadTubeTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC road tube producer preserves61 native selected-item and Base contexts",GT(){
        size_t count=0,commands=0;
        constexpr std::array<raster::Point,8> offsets{{{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}}}};
        for(const auto& c:road_tube_fixture::Contexts()) {
            const game::frontend::smac_road_tube::CellAt at=[&](int x,int y)->std::optional<game::frontend::smac_road_tube::Cell> {
                if(y<0 || y>=80) return {};
                if(c.wrap) x=(x%80+80)%80;
                if(x<0 || x>=80) return {};
                uint8_t band=c.band;uint32_t items=0;bool base=false;
                if(x==c.x && y==c.y) {band=c.center_band;items=c.selected;}
                else {
                    for(const auto& patch:c.patch) {if(x==(c.x+patch[0]+80)%80 && y==c.y+patch[1]) {band=static_cast<uint8_t>(patch[2]);items=static_cast<uint32_t>(patch[4]);}}
                    for(const auto& row:c.remembered) {const auto& delta=offsets[row[0]];if(x==(c.x+delta[0]+80)%80 && y==c.y+delta[1]) {items=static_cast<uint32_t>(row[1]);}}
                }
                for(const auto& row:c.bases) {if(x==(c.x+row[0]+80)%80 && y==c.y+row[1]) {base=::game::native_terrain::AdmitsBase(3,row[2],static_cast<uint8_t>(row[3]));}}
                return game::frontend::smac_road_tube::Cell{band,items,base};
            };
            const auto view=game::frontend::native_terrain::View(c.x,c.y,c.wrap);
            const auto center=std::find_if(view.begin(),view.end(),[&](const auto& p){return p.tile==raster::Point{c.x,c.y};});
            GT_ASSERT(center!=view.end(),"Native road camera omitted center");
            const auto bands=[&](int x,int y)->std::optional<uint8_t>{const auto cell=at(x,y);return cell?std::optional<uint8_t>{cell->band}:std::nullopt;};
            const auto h=game::backend::map::smac_terrain::OrdinaryHeights(c.x,c.y,bands);
            const auto x=center->flat_center[0],y=center->flat_center[1];
            const std::array<raster::Point,5> points{{{{x,y-h[0]}},{{x-48,y-h[1]}},{{x,y-24-h[2]}},{{x+48,y-h[3]}},{{x,y+24-h[4]}}}};
            std::array<int,5> lighting{};if(c.center_band>=3) {const auto light=game::backend::map::smac_terrain::OrdinaryLighting(c.x,c.y,bands);GT_ASSERT(light,"Road context lighting unavailable");lighting=*light;}
            const auto actual=game::frontend::smac_road_tube::Select(c.x,c.y,points,lighting,at,c.flags);
            GT_ASSERT(actual.size()==c.expected.size(),"Native road count differs: "+std::string(c.name));
            for(size_t i=0;i<actual.size();++i) {GT_ASSERT(Equal(actual[i],c.expected[i]),"Native road geometry/order differs: "+std::string(c.name));}
            ++count;commands+=actual.size();
        }
        GT_ASSERT(count==61 && commands==86,"Missing native Road/Tube contexts");GT_OK();
    });
    task->AddTest("SMAC road tube raster preserves10364 native composed spans",GT(){
        size_t count=0;
        for(const auto& f:road_tube_fixture::Frames()) {for(const auto& draw:f.draws) {
            const auto& p=draw.pass;
            const bool skip=p.shader==raster::Shader::WaterDepth16 && std::all_of(p.polygon.vertices.begin(),p.polygon.vertices.end(),[](const auto& v){return v.light>=16;});
            const auto spans=skip?std::vector<raster::Span>{}:raster::GeneratePolygonSpans(p.polygon,f.clip,p.shader!=raster::Shader::KeyedUnlit,p.shade_limits);
            GT_ASSERT(spans.size()==draw.spans && Digest(spans)==draw.span_hash,"Original Road/Tube composed spans differ: "+std::string(f.name));count+=spans.size();
        }}
        GT_ASSERT(count==10364,"Missing native Road/Tube raster coverage");GT_OK();
    });

}
}
