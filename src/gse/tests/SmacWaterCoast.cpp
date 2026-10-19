#include "SmacWaterCoast.h"
#include "fixtures/SmacWaterCoast.h"
#include "game/frontend/NativeTerrainRenderer.h"
#include "game/frontend/SmacWaterCoastTables.h"
#include "game/backend/map/SmacTerrainGeometry.h"
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
void AddSmacWaterCoastTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC water coast producer preserves native contour samples and ordered commands",GT(){
        size_t commands=0,samples=0;
        for(const auto& c:water_coast_fixture::Contexts()) {
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
            const auto actual=water::Select(c.x,c.y,points,at);
            GT_ASSERT(actual.size()==c.expected.size(),"Water command count differs: "+std::string(c.name));
            for(size_t i=0;i<actual.size();++i) GT_ASSERT(Equal(actual[i],c.expected[i]),"Water/coast polygon/material/order differs: "+std::string(c.name));
            commands+=actual.size();
            for(const auto& s:c.samples) {
                const auto sample=water::Sample(s[0],s[1],s[2],s[3],at);
                GT_ASSERT(sample==s[4] && water::tables::depth_lookup[sample]==s[5],"Native contour/depth sample differs: "+std::string(c.name));++samples;
            }
        }
        GT_ASSERT(commands==250 && samples==760,"Missing native water/coast contexts");GT_OK();
    });
    task->AddTest("SMAC water coast raster generates7714 original spans including threshold exits",GT(){
        size_t count=0;
        for(const auto& f:water_coast_fixture::Frames()) {for(const auto& draw:f.draws) {
            const auto& p=draw.pass;
            const bool skip=p.shader==raster::Shader::WaterDepth16 && std::all_of(p.polygon.vertices.begin(),p.polygon.vertices.end(),[](const auto& v){return v.light>=16;});
            const auto spans=skip?std::vector<raster::Span>{}:raster::GeneratePolygonSpans(p.polygon,f.clip,p.shader!=raster::Shader::KeyedUnlit,p.shade_limits);
            GT_ASSERT(spans.size()==draw.spans && Digest(spans)==draw.span_hash,"Original water/coast spans differ: "+std::string(f.name));count+=spans.size();
        }}
        GT_ASSERT(count==7714,"Missing native water/coast raster coverage");GT_OK();
    });
    task->AddTest("SMAC water coast shaders distinguish depth threshold from key and mask literals",GT(){
        raster::Bitmap background{8,8,std::vector<uint8_t>(64,250)},source{8,8,std::vector<uint8_t>(64,9)},mask{8,8,std::vector<uint8_t>(64,0)};
        raster::Command command;command.texture=&source;command.shader=raster::Shader::WaterDepth16;command.shade_limits={-256,256};
        command.polygon={{{{{{0,0}},{{0,0}},0},{{{8,0}},{{7,0}},0},{{{8,8}},{{7,7}},0},{{{0,8}},{{0,7}},0}}},4};
        auto drawn=raster::DrawBatch(background,{command},{0,0,8,8},true);
        GT_ASSERT(drawn.writes==64 && drawn.bitmap.pixels[0]==9 && drawn.coverage[0],"Water treated source9 as a transparent key");
        for(auto& vertex:command.polygon.vertices) vertex.light=16;
        drawn=raster::DrawBatch(background,{command},{0,0,8,8},true);
        GT_ASSERT(!drawn.attempts && !drawn.writes && !drawn.coverage[0],"Water depth16 did not exit before sampling");
        for(auto& vertex:command.polygon.vertices) vertex.light=-1;
        drawn=raster::DrawBatch(background,{command},{0,0,8,8});
        GT_ASSERT(drawn.writes==64 && drawn.bitmap.pixels[0]==8,"Water depth lost signed byte wrap");
        command.shader=raster::Shader::CoastMapped;command.mask=&mask;command.mask_mapping[0]=0;
        drawn=raster::DrawBatch(background,{command},{0,0,8,8},true);
        GT_ASSERT(!drawn.writes && !drawn.coverage[0],"Coast class0 did not preserve destination");
        command.mask_mapping[0]=1;drawn=raster::DrawBatch(background,{command},{0,0,8,8});
        GT_ASSERT(drawn.writes==64 && drawn.bitmap.pixels[0]==8,"Coast selector1 lost shaded source");
        command.mask_mapping[0]=9;drawn=raster::DrawBatch(background,{command},{0,0,8,8},true);
        GT_ASSERT(drawn.writes==64 && drawn.bitmap.pixels[0]==9 && drawn.coverage[0],"Coast literal was shaded or keyed");
        mask.pixels[0]=53;bool rejected=false;
        try {(void)raster::DrawBatch(background,{command},{0,0,8,8});}catch(const std::invalid_argument&){rejected=true;}
        GT_ASSERT(rejected && background.pixels[0]==250,"Invalid mask class escaped an owned partial frame");GT_OK();
    });
}
}
