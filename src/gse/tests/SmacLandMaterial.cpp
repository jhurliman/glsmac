#include "SmacLandMaterial.h"
#include "fixtures/SmacLandMaterial.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>
#include <algorithm>

namespace gse::tests {
namespace material=game::frontend::smac_land_material;
void AddSmacLandMaterialTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC land material separates remembered center from live neighbor items",GT(){
        size_t checked=0;
        for(const auto& c:land_material_fixture::Cases()) {
            if(std::string(c.name)!="fungus_connected_NW" && std::string(c.name)!="forest_connected_NW") {continue;}
            const auto original=land_material_fixture::Lookup(c);
            const material::TileAt live=[&](int x,int y) {
                auto tile=original(x,y);
                if(tile && material::Point{{x,y}}==c.coord) {tile->items=0;}
                return tile;
            };
            const auto got=material::Select(c.coord,live,c.seed,c.center.items);
            GT_ASSERT(got.size()==c.expected.size(),"Remembered center lost original connected feature");
            for(size_t i=0;i<got.size();++i) {
                const auto& a=got[i];const auto& expected=c.expected[i];
                GT_ASSERT(a.role==expected.role && a.crop==expected.crop && a.uv==expected.uv,
                    "Connected material used memory instead of live neighbor inputs");
            }
            const auto bare=material::Select(c.coord,original,c.seed,uint32_t{0});
            GT_ASSERT(std::none_of(bare.begin(),bare.end(),[](const auto& pass) {
                return pass.role==material::Role::Forest || pass.role==material::Role::Fungus;
            }),"Live center bypassed explicit remembered absence");
            ++checked;
        }
        GT_ASSERT(checked==2,"Missing original Fungus/Forest connectivity controls");GT_OK();
    });
    task->AddTest("SMAC land material reproduces193 ordered native texture calls",GT(){
        size_t count=0;
        for(const auto& c:land_material_fixture::Cases()) {
            const auto got=material::Select(c.coord,land_material_fixture::Lookup(c),c.seed);
            GT_ASSERT(got.size()==c.expected.size(),"Native material pass count: "+std::string(c.name));
            for(size_t i=0;i<got.size();++i) {
                const auto& a=got[i];const auto& b=c.expected[i];
                GT_ASSERT(a.role==b.role && a.fan==b.fan && a.crop==b.crop && a.uv==b.uv && a.vertices==b.vertices && a.flags==b.flags,
                    "Native material pass mismatch: "+std::string(c.name));
            }
            count+=got.size();
        }
        GT_ASSERT(count==193,"Missing native selector/composition receipts");
        for(const auto& c:land_material_fixture::Cases()) {if(std::string(c.name)=="moist_sea_NW") {
            const auto at=land_material_fixture::Lookup(c);
            const auto polluted=[&](int x,int y) {
                auto tile=at(x,y);
                if(tile && material::Point{{x,y}}!=c.coord) {tile->rockiness=3;if(tile->band<3) {tile->moisture=3;}}
                return tile;
            };
            const auto got=material::Select(c.coord,polluted,c.seed);
            GT_ASSERT(got.size()==c.expected.size(),"Unused neighbor fields change native pass count");
            for(size_t i=0;i<got.size();++i) {GT_ASSERT(got[i].crop==c.expected[i].crop && got[i].uv==c.expected[i].uv,"Unused sea moisture or neighbor rockiness changes connectivity");}
        }}
        GT_OK();
    });
    task->AddTest("SMAC land material palette follows loaded indices and preserves active reserved colors",GT(){
        material::Palette common{},active{};
        for(auto& color:active) {color={17,19,23};}
        common[0]={172,129,81};common[46]={96,51,45};common[48]={79,41,41};common[235]={193,173,26};
        const auto palette=material::CommonGamma100(common,active);
        GT_ASSERT(palette[10]==common[0] && palette[245]==common[235],"Common palette range is offset or uses the embedded texture palette");
        GT_ASSERT(palette[0]==active[0] && palette[9]==active[9] && palette[246]==active[246] && palette[255]==active[255],"Invented reserved palette colors");
        const auto loaded=material::LoadedIndex(48);
        const auto shade=material::FinalIndex(loaded,-2,false);
        GT_ASSERT(loaded==58 && shade && *shade==56,"Shade applied before loaded-index rebase");
        const auto rgba=material::ResolveRgba({*shade,loaded,9},palette);
        const std::vector<uint8_t> expected{96,51,45,255,79,41,41,255,17,19,23,255};
        GT_ASSERT(rgba==expected,"RGB lookup did not follow the final wrapped game index");
        GT_ASSERT(!material::FinalIndex(9,2,true) && material::FinalIndex(10,-2,true)==8 && material::LoadedIndex(255)==9,"Key compare occurs after shade or loaded-index wrap is lost");
        common[0]={255,236,235};const auto capped=material::CommonGamma100(common,active);
        const material::Rgb cap{235,235,235};GT_ASSERT(capped[10]==cap,"Gamma100 channel cap differs");
        bool rejected=false;
        try {material::Select({40,40},[](int,int)->std::optional<material::Tile>{return {};},0);}catch(const std::invalid_argument&) {rejected=true;}
        GT_ASSERT(rejected,"Selector bypasses caller admission for absent terrain");
        GT_OK();
    });
}
}
