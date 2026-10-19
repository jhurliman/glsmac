#include "SmacTerrainLighting.h"
#include "fixtures/SmacTerrainLighting.h"
#include "task/gsetests/GSETests.h"
#include <limits>

namespace gse::tests {
void AddSmacTerrainLightingTests(task::gsetests::GSETests* task) {
    namespace native=game::backend::map::smac_terrain;
    task->AddTest("SMAC ordinary lighting matches196 native slope lookup entries",GT(){
        size_t index=0;
        for(unsigned face=0;face<4;++face) {for(int a=-3;a<=3;++a) {for(int b=-3;b<=3;++b) {
            GT_ASSERT(native::FaceLighting(a,b,face)==terrain_light_fixture::TABLE[index++],"Native signed face lookup differs");
        }}}
        GT_ASSERT(!native::FaceLighting(-4,0,0) && !native::FaceLighting(0,4,0) && !native::FaceLighting(0,0,4),"Unsupported face selector silently clamped");
        GT_OK();
    });
    task->AddTest("SMAC ordinary lighting matches16 native incident face stencils",GT(){
        for(const auto& c:terrain_light_fixture::Cases()) {
            const auto at=[&](int x,int y)->std::optional<uint8_t> {
                if(y<0 || y>=80 || (c.flat && (x<0 || x>=80))) {return {};}
                x=(x+80)%80;
                for(const auto& band:c.bands) {if(band[0]==x && band[1]==y) {return static_cast<uint8_t>(band[2]);}}
                return {};
            };
            GT_ASSERT(native::OrdinaryLighting(c.x,c.y,at)==c.expected,"Native incident face lighting: "+std::string(c.name));
        }
        const auto sea=[](int,int)->std::optional<uint8_t>{return 2;};
        const auto steep=[](int x,int y)->std::optional<uint8_t>{return x==40 && y==40?7:3;};
        GT_ASSERT(!native::OrdinaryLighting(40,40,sea) && !native::OrdinaryLighting(40,40,steep),"Sea or unproved steep profile got fabricated light");
        GT_ASSERT(!native::OrdinaryLighting(std::numeric_limits<int>::max(),0,sea),"Unsafe coordinate arithmetic accepted");
        GT_OK();
    });
}
}
