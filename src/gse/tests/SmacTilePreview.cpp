#include "SmacTilePreview.h"
#include "game/frontend/SmacTilePreview.h"
#include "task/gsetests/GSETests.h"
namespace gse::tests {
namespace preview=game::frontend::smac_tile_preview;
namespace raster=game::frontend::smac_terrain_raster;
namespace native=game::frontend::native_terrain;
void AddSmacTilePreviewTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC tile preview independent NULL view flat geometry and retained backing",GT(){
        game::native_terrain::World world;
        for(auto& tile:world.tiles) {tile.band=3;tile.selected_known=tile.resource_ready=true;}
        native::Source source;source.loaded={1000,1000,std::vector<uint8_t>(1000000,77)};
        const raster::Bitmap background{1024,257,std::vector<uint8_t>(1024*257,73)};
        const auto a=preview::Paint(world,source,8,8,0xbf60,background,{{148,67}},{{146,66,235,183}});
        GT_ASSERT(a && a->writes==1600,"Measured four triangle1600-store preview missing");
        int left=1024,top=257,right=0,bottom=0;
        for(int y=0;y<257;++y) {for(int x=0;x<1024;++x) {
            const auto p=size_t(y)*1024+size_t(x);
            if(a->coverage[p]) {left=std::min(left,x);top=std::min(top,y);right=std::max(right,x+1);bottom=std::max(bottom,y+1);}
            else {GT_ASSERT(a->indexed[p]==73 && a->rgba[4*p+3]==0,"Preview cleared backing or invented alpha outside stores");}
        }}
        GT_ASSERT(left==148 && top==68 && right==228 && bottom==107,"Original painted bounds/top-tip rule differs");
        auto& tile=world.tiles[8*40+4];tile.band=4;tile.contour=87;tile.heights={{71,-38,53,99,-17}};tile.lighting={{-4,3,4,2,-2}};
        const auto b=preview::Paint(world,source,8,8,0xbf60,background,{{148,67}},{{146,66,235,183}});
        GT_ASSERT(b && b->indexed==a->indexed,"NULL-view preview borrowed slope geometry or lighting");
        const auto local=preview::Paint(world,source,8,8,0xbf60,{89,117,std::vector<uint8_t>(89*117,73)});
        GT_ASSERT(local && local->writes==1600,"Independent widget clip lost original stores");
        for(int y=0;y<117;++y) {for(int x=0;x<89;++x) {
            GT_ASSERT(local->indexed[size_t(y)*89+x]==a->indexed[size_t(y+66)*1024+x+146],"Widget path resized a framebuffer instead of translating the independent paint");
        }}
        GT_OK();
    });
    task->AddTest("SMAC tile preview remembered improvements and unavailable profile admission",GT(){
        game::native_terrain::World world;
        for(auto& tile:world.tiles) {tile.band=3;tile.selected_known=tile.resource_ready=true;}
        native::Source source;source.loaded={1000,1000,std::vector<uint8_t>(1000000,77)};
        const raster::Bitmap background{89,117,std::vector<uint8_t>(89*117,73)};
        auto& tile=world.tiles[8*40+4];
        const auto bare=preview::Paint(world,source,8,8,0,background);
        tile.items=32;const auto live=preview::Paint(world,source,8,8,0,background);
        GT_ASSERT(bare && live && live->writes==bare->writes && live->indexed==bare->indexed,"Live-only Fungus leaked into remembered preview");
        world.tiles[7*40+3].selected_known=false;world.tiles[7*40+3].items=32;
        const auto partial=preview::Paint(world,source,8,8,0,background);
        GT_ASSERT(partial && partial->indexed==bare->indexed,"Bare-land climate connectivity unnecessarily required neighbor improvement memory or leaked live Fungus");
        world.tiles[7*40+3].selected_known=true;
        tile.items=0;tile.selected_items=32;const auto remembered=preview::Paint(world,source,8,8,0,background);
        GT_ASSERT(remembered && remembered->writes>bare->writes,"Remembered Fungus bypassed original feature painter");
        tile.selected_known=false;GT_ASSERT(!preview::Paint(world,source,8,8,0,background),"Unknown center painted");
        tile.selected_known=true;world.tiles[7*40+3].selected_known=false;
        GT_ASSERT(!preview::Paint(world,source,8,8,0,background),"Unknown neighbor memory silently became live memory");
        world.tiles[7*40+3].selected_known=true;tile.band=2;
        GT_ASSERT(!preview::Paint(world,source,8,8,0,background),"Unmeasured sea admitted");
        tile.band=3;tile.selected_items=0x8000;
        GT_ASSERT(!preview::Paint(world,source,8,8,0,background),"Unmeasured preview feature admitted");
        GT_ASSERT(!preview::Paint(world,source,-1,8,0,background),"Invalid center wrapped into a different selected tile");
        GT_OK();
    });
}
}
