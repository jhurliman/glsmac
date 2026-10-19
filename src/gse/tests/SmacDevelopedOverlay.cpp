#include "SmacDevelopedOverlay.h"
#include "fixtures/SmacDevelopedOverlay.h"
#include "fixtures/SmacBaselessNutrients.h"
#include "game/backend/map/SmacNativeTerrainItems.h"
#include "game/backend/resource/SmacTileNutrients.h"
#include "game/backend/resource/SmacBaseSquareYield.h"
#include "game/frontend/NativeTerrainRenderer.h"
#include "task/gsetests/GSETests.h"
#include <algorithm>
#include <limits>
namespace gse::tests {
void AddSmacDevelopedOverlayTests(task::gsetests::GSETests* task) {
    namespace overlay=game::frontend::smac_developed_overlay;
    namespace raster=game::frontend::smac_terrain_raster;
    task->AddTest("SMAC developed overlays bind20 native base-less and contrasting Base queries",GT(){
        using namespace game::backend::resource::smac;
        const TileNutrientRules rules{1,2,1,1,2,2,0,-1};
        for(const auto& c:baseless_nutrient_fixture::cases) {
            const auto has=[&](uint32_t flag){return bool(c.items&flag);};
            const TileNutrientInputs input{
                {AltitudeBand::Land3,static_cast<Moisture>(c.moisture),Rockiness::Flat,has(0x20000000)?ResourceBonus::Nutrients:ResourceBonus::None,
                    false,has(0x200000),has(0x20),has(0x1000000),has(0x1000)},
                {has(0x8000),has(0x10),has(0x80000),has(0x400000)},
                {true,true,bool(c.technology),c.capability,c.planet,bool(c.event&512),bool(c.event&1024)}
            };
            TileNutrientYield result;
            if(c.base<0) {result=CalculateBaselessTileNutrients(rules,2,has(1),uint8_t(c.marker),input);}
            else if(has(1) && c.marker<8) {
                const auto yields=CalculateBaseSquareYield({{2,1,1},{2,2,2},{1,1,1}},
                    {{true,input.terrain.resource_bonus,{false,false,false},false},{false,true,false},
                    {0,false,1,false,false},{{bool(c.event&512),false,false},{bool(c.event&1024),false,false},false,false}});
                result={yields[0],0};
            }else {result=CalculateTileNutrients(rules,input);}
            GT_ASSERT(result.nutrients==c.result && result.technology_capped_excess==c.excess,"Native base-less context or contrasting Base result differs");
        }
        GT_OK();
    });
    task->AddTest("SMAC developed overlays map all live and remembered improvement fields",GT(){
        using namespace game::backend::map;
        const auto all=NativeTerrainItems((1u<<13)-1,tile::FEATURE_RIVER|tile::FEATURE_XENOFUNGUS);
        GT_ASSERT(all==0x81ec88fcu,"Native improvement mapping dropped or aliased a field");
        GT_ASSERT(overlay::ReplacesLegacySprite("FarmSea") && overlay::ReplacesLegacySprite("Sensor")
            && !overlay::ReplacesLegacySprite("Monolith") && !overlay::ReplacesLegacySprite("NutrientBonusLand"),"Native visibility consumed an unimplemented object family");GT_OK();
    });
    task->AddTest("SMAC native terrain composes selected improvements with live nutrients and indexed shadows",GT(){
        namespace native=game::frontend::native_terrain;
        ::game::native_terrain::World world;
        for(auto& tile:world.tiles) {tile.band=3;tile.ordinary=true;}
        auto& tile=world.tiles[40*40+20];tile.developed_ready=tile.selected_known=true;tile.items=0x40;
        native::Source source;source.loaded={1024,768,std::vector<uint8_t>(1024*768,10)};
        source.ter1={1024,768,std::vector<uint8_t>(1024*768,9)};
        source.coast_mask={56,56,std::vector<uint8_t>(56*56,0)};
        for(size_t i=0;i<256;++i) {source.palette[i]={uint8_t(i),0,0};source.shadow_lookup[i]=40;}
        const auto ink=[&](int x,int y,int dx,int dy,uint8_t color){source.ter1.pixels[size_t(y+dy)*1024+x+dx]=color;};
        ink(607,127,25,17,30);ink(708,316,48,25,0);
        ink(822,642,48,25,60);ink(822,453,48,25,70);
        const std::vector<native::Placement> view{{{{40,40}},{{576,288}},44*(40*40+20)}};
        const auto draw=[&](){return native::Render(world,source,view,0);};
        GT_ASSERT(draw().rgba[(281*1024+553)*4]!=30,"Live Solar bypassed remembered admission");
        tile.selected_items=0x40;const auto solar=draw();
        GT_ASSERT(solar.rgba[(281*1024+553)*4]==30 && solar.developed_tiles[40*40+20],"Solar lost native crop, projector anchor or visibility handoff");
        tile.heights=std::array<int,5>{{-12,-3,-3,-15,-16}};
        GT_ASSERT(draw().rgba[(291*1024+553)*4]==30,"Negative landmark mean truncated toward zero at live sprite anchor");
        tile.heights.reset();
        tile.selected_items|=0x80000000u;GT_ASSERT(draw().rgba[(289*1024+576)*4]==40,"Sensor shadow lost terrain destination index");
        tile.selected_items=0x88000;tile.nutrients=6;GT_ASSERT(draw().rgba[(289*1024+576)*4]==70,"Live high-yield Soil patch lost last draw order");
        tile.nutrients=4;GT_ASSERT(draw().rgba[(289*1024+576)*4]==60,"Soil Farm tier incorrectly appended extra patch");
        tile.developed_ready=false;const auto unavailable=draw();
        GT_ASSERT(!unavailable.developed_tiles[40*40+20] && unavailable.rgba[(289*1024+576)*4]!=60,"Unavailable nutrient context suppressed legacy sprite or invented a tier");GT_OK();
    });
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
