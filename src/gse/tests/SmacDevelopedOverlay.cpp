#include "SmacDevelopedOverlay.h"
#include "fixtures/SmacDevelopedOverlay.h"
#include "task/gsetests/GSETests.h"
#include <algorithm>
#include <limits>
namespace gse::tests {
void AddSmacDevelopedOverlayTests(task::gsetests::GSETests* task) {
    namespace overlay=game::frontend::smac_developed_overlay;
    namespace raster=game::frontend::smac_terrain_raster;
    task->AddTest("SMAC developed overlays preserve60 native selected-item producers",GT(){
        size_t cases=0,picks=0;
        for(const auto& c:developed_overlay_fixture::Contexts()) {
            const auto result=overlay::Select(c.items,c.band,c.nutrients,c.landmarks,c.flags,c.zoom);
            GT_ASSERT(result.size()==c.expected.size(),"Native developed-overlay count differs");
            for(size_t i=0;i<result.size();++i) {GT_ASSERT(result[i].crop==c.expected[i].crop && result[i].shadow==c.expected[i].shadow,"Native crop/order/shadow differs");}
            ++cases;picks+=result.size();
        }
        GT_ASSERT(cases==60 && picks==81,"Missing native improvement contexts");
        GT_ASSERT(overlay::Select(0x8000,3,INT64_MIN,0)[0].crop==overlay::Crop({923,453,100,62}),"Minimum nutrient tier overflowed");
        GT_ASSERT(overlay::Select(0x88000,3,INT64_MAX,0).size()==2,"Maximum nutrient tier overflowed");
        GT_ASSERT(overlay::Select(0xffffffffu,3,6,0,0x21,-8).empty(),"Improvement zoom gate ignored");
        GT_ASSERT(overlay::Select(0x8000,2,0,0,1,-8).size()==1,"Kelp incorrectly inherited zoom gate");GT_OK();
    });
    task->AddTest("SMAC developed overlays trim loaded keys while retaining literal zero",GT(){
        raster::Bitmap atlas{100,62,std::vector<uint8_t>(6200,9)};
        atlas.pixels[122]=0;atlas.pixels[325]=42;atlas.pixels[6198]=6;atlas.pixels[6199]=7;
        const auto sprite=overlay::Decode(atlas,{{0,0,100,62}});
        GT_ASSERT(sprite.offset==raster::Point({22,1}) && sprite.bitmap.width==4 && sprite.bitmap.height==3,"Overlay trim lost native offset/size");
        GT_ASSERT(sprite.bitmap.pixels.front()==0 && sprite.bitmap.pixels.back()==42,"Source zero treated as a key");
        std::fill(atlas.pixels.begin(),atlas.pixels.end(),6);const auto empty=overlay::Decode(atlas,{{0,0,100,62}});
        GT_ASSERT(empty.bitmap.pixels.empty() && empty.bitmap.width==0 && empty.bitmap.height==0,"Empty source created opaque sprite");GT_OK();
    });
    task->AddTest("SMAC developed overlays compose clipped indexed shadows after literal stores",GT(){
        const raster::Bitmap initial{6,4,std::vector<uint8_t>(24,20)},sprite{3,2,{9,0,42,0,9,55}};
        std::array<uint8_t,256> shadow{};shadow.fill(71);
        raster::Command plain;plain.texture=&sprite;plain.shader=raster::Shader::Sprite;plain.sprite_origin={1,1};
        auto shaded=plain;shaded.shader=raster::Shader::SpriteShadow;shaded.shadow_lookup=&shadow;
        const auto result=raster::DrawBatch(initial,{plain,shaded},{{2,1,4,3}},true);
        GT_ASSERT(result.attempts==8 && result.writes==6,"Clipped sprite counts differ");
        GT_ASSERT(result.bitmap.pixels[8]==71 && result.bitmap.pixels[9]==42 && result.bitmap.pixels[15]==55,"Shadow did not use indexed prior destination");
        GT_ASSERT(result.bitmap.pixels[14]==20 && !result.coverage[14] && result.coverage[8],"Key/coverage order differs");
        plain.sprite_origin={-1,-1};const auto clipped=raster::DrawBatch(initial,{plain},{{0,0,6,4}});
        GT_ASSERT(clipped.attempts==2 && clipped.writes==1 && clipped.bitmap.pixels[1]==55 && clipped.bitmap.pixels[0]==20,"Clipping restarted source coordinates");
        shaded.shadow_lookup=nullptr;bool rejected=false;
        try {(void)raster::DrawBatch(initial,{plain,shaded},{{0,0,6,4}});}catch(const std::invalid_argument&) {rejected=true;}
        GT_ASSERT(rejected && initial.pixels[0]==20,"Missing lookup accepted or input mutated");GT_OK();
    });
}
}
