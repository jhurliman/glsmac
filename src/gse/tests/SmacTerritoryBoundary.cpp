#include "SmacTerritoryBoundary.h"
#include "fixtures/SmacTerritoryBoundary.h"
#include "game/frontend/NativeTerrainRenderer.h"
#include "game/backend/map/SmacTerrainGeometry.h"
#include "task/gsetests/GSETests.h"
#include <algorithm>
#include <stdexcept>

namespace gse::tests {
namespace {
namespace boundary=game::frontend::smac_territory_boundary;
namespace raster=game::frontend::smac_terrain_raster;
uint64_t Digest(const std::vector<raster::Span>& spans) {
    uint64_t result=14695981039346656037ULL;
    for(const auto& s:spans) {for(const auto n:{s.y,s.left,s.right,s.u,s.v,s.du,s.dv,int(s.shade),int(s.dshade)}) {
        for(int shift=0;shift<32;shift+=8) {result=(result^static_cast<uint8_t>(static_cast<uint32_t>(n)>>shift))*1099511628211ULL;}
    }}return result;
}
}
void AddSmacTerritoryBoundaryTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC territory boundary reproduces31 native owner region and topology contexts",GT(){
        size_t count=0,commands=0;
        constexpr std::array<raster::Point,8> offsets{{{{1,-1}},{{2,0}},{{1,1}},{{0,2}},{{-1,1}},{{-2,0}},{{-1,-1}},{{0,-2}}}};
        for(const auto& c:boundary_fixture::Contexts()) {
            const boundary::CellAt at=[&](int x,int y)->std::optional<boundary::Cell> {
                if(y<0 || y>=80) return {};
                if(c.wrap) x=(x%80+80)%80;
                if(x<0 || x>=80) return {};
                uint8_t band=c.band;int region=1,owner=c.owner;
                if(x==c.x && y==c.y) {band=c.center_band;}
                else {for(size_t i=0;i<offsets.size();++i) {
                    if(x==(c.x+offsets[i][0]+80)%80 && y==c.y+offsets[i][1] && c.different[i]) {region=c.other_region;owner=-1;}
                }}
                for(const auto& patch:c.patch) {if(x==(c.x+patch[0]+80)%80 && y==c.y+patch[1]) {band=static_cast<uint8_t>(patch[2]);}}
                return boundary::Cell{band,static_cast<uint8_t>(region),owner};
            };
            const auto view=game::frontend::native_terrain::View(c.x,c.y,c.wrap);
            const auto center=std::find_if(view.begin(),view.end(),[&](const auto& p){return p.tile==raster::Point{c.x,c.y};});
            GT_ASSERT(center!=view.end(),"Native boundary camera omitted center");
            const auto h=game::backend::map::smac_terrain::OrdinaryHeights(c.x,c.y,[&](int x,int y)->std::optional<uint8_t>{const auto cell=at(x,y);return cell?std::optional<uint8_t>{cell->band}:std::nullopt;});
            const auto x=center->flat_center[0],y=center->flat_center[1];
            const std::array<raster::Point,5> points{{{{x,y-h[0]}},{{x-48,y-h[1]}},{{x,y-24-h[2]}},{{x+48,y-h[3]}},{{x,y+24-h[4]}}}};
            boundary::Context context;context.viewer=3;context.relations.fill(c.relation);
            const auto actual=boundary::Select(c.x,c.y,points,at,context,c.flags);
            GT_ASSERT(actual.has_value()==!c.expected.empty(),"Native boundary admission differs: "+std::string(c.name));
            if(actual) {
                const auto& expected=c.expected.front();
                GT_ASSERT(actual->mapping==expected.mapping && actual->shade_limits==expected.shade_limits && actual->polygon.count==expected.polygon.count,"Boundary mapping differs: "+std::string(c.name));
                for(size_t i=0;i<4;++i) {GT_ASSERT(actual->polygon.vertices[i].screen==expected.polygon.vertices[i].screen && actual->polygon.vertices[i].uv==expected.polygon.vertices[i].uv && actual->polygon.vertices[i].light==0,"Boundary polygon differs: "+std::string(c.name));}
                ++commands;
            }++count;
        }
        GT_ASSERT(count==31 && commands==24,"Missing native territory contexts");GT_OK();
    });
    task->AddTest("SMAC territory boundary reproduces5024 native composed spans with half texel bias",GT(){
        size_t count=0;
        for(const auto& f:boundary_fixture::Frames()) {for(const auto& draw:f.draws) {
            const auto& p=draw.pass;
            const bool skip=p.shader==raster::Shader::WaterDepth16 && std::all_of(p.polygon.vertices.begin(),p.polygon.vertices.end(),[](const auto& v){return v.light>=16;});
            const auto spans=skip?std::vector<raster::Span>{}:raster::GeneratePolygonSpans(p.polygon,f.clip,p.shader!=raster::Shader::KeyedUnlit,p.shade_limits);
            GT_ASSERT(spans.size()==draw.spans && Digest(spans)==draw.span_hash,"Native boundary spans differ: "+std::string(f.name));count+=spans.size();
        }}GT_ASSERT(count==5024,"Missing native boundary spans");GT_OK();
    });
    task->AddTest("SMAC territory boundary quantizes common colors with stable ties and reserved identities",GT(){
        game::frontend::smac_land_material::Palette colors{};
        for(auto& color:colors) {color={100,0,0};}colors[10]={0,0,0};colors[11]={2,0,0};
        const auto tie=boundary::Row(colors,11,50),zero=boundary::Row(colors,11,0);
        GT_ASSERT(tie[10]==10 && zero[20]==12,"Boundary lost first nearest-color tie or duplicate-color quantization");
        for(size_t i=0;i<256;++i) {if(i<10 || i>245) GT_ASSERT(tie[i]==i && zero[i]==i,"Boundary remapped a reserved palette entry");}
        const auto percentages=boundary::ReadOpacities("#OTHER\n1,test\n#BOUNDARYPERCENTS\r\n0, unused\r\n50, gaian\n60, hive ;note\n50, university\n50, morgan\n60, spartan\n50, believer\n70, peace\n#END\n");
        GT_ASSERT((percentages==std::array<int,8>{{0,50,60,50,50,60,50,70}}),"Boundary opacities ignored system section");GT_OK();
    });
    task->AddTest("SMAC territory boundary enforces visibility relations and counts identity stores",GT(){
        boundary::Context context;context.viewer=3;
        GT_ASSERT(boundary::Owner(3,context)==3 && boundary::Owner(0,context)==-1 && boundary::Owner(2,context)==-1,"Boundary owner query leaked foreign or unclaimed information");
        context.relations[2]=0x08000008;GT_ASSERT(boundary::Owner(2,context)==2,"Boundary rejected admitted foreign owner");
        const boundary::CellAt at=[](int x,int y)->std::optional<boundary::Cell>{return boundary::Cell{3,1,(x==40 && y==40)?3:-1};};
        const std::array<raster::Point,5> points{{{{4,4}},{{0,4}},{{4,0}},{{8,4}},{{4,8}}}};
        context.visible=false;GT_ASSERT(!boundary::Select(40,40,points,at,context,0x10001),"Boundary ignored current visibility gate");
        context.visibility_bypass=true;GT_ASSERT(boundary::Select(40,40,points,at,context,0x10001),"Boundary ignored viewer capability bypass");
        context.override_mode=true;GT_ASSERT(!boundary::Select(40,40,points,at,context,0x10001),"Boundary ignored renderer override");
        raster::OwnerClut clut{};for(auto& row:clut) {for(size_t i=0;i<256;++i) {row[i]=static_cast<uint8_t>(i);}}
        raster::Bitmap initial{8,8,std::vector<uint8_t>(64,255)},classes{8,8,std::vector<uint8_t>(64,1)};
        raster::Command command;command.texture=&classes;command.shader=raster::Shader::OwnerClut;command.owner_clut=&clut;command.owner_mapping[1]=4;
        command.polygon={{{{{{0,0}},{{0,0}},0},{{{8,0}},{{7,0}},0},{{{8,8}},{{7,7}},0},{{{0,8}},{{0,7}},0}}},4};
        const auto painted=raster::DrawBatch(initial,{command},{{0,0,8,8}},true);
        GT_ASSERT(painted.writes==64 && painted.bitmap.pixels==initial.pixels && std::all_of(painted.coverage.begin(),painted.coverage.end(),[](auto n){return n!=0;}),"Reserved identity-valued boundary store lost coverage");GT_OK();
    });
}
}
