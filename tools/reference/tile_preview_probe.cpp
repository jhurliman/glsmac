#include "game/frontend/SmacTilePreview.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace native=game::frontend::native_terrain;
std::vector<uint8_t> Read(const std::string& file) {
    std::ifstream in(file,std::ios::binary);if(!in) {throw std::runtime_error("Missing owned preview input");}
    return {std::istreambuf_iterator<char>(in),{}};
}
int main(int argc,char** argv) {
    if(argc!=2) {return 1;}const std::string dir=argv[1];
    int tw,th,sw,sh;std::ifstream in(dir+"/input.txt");if(!(in>>tw>>th>>sw>>sh)) {return 1;}
    native::Source source;source.loaded={tw,th,Read(dir+"/texture.bin")};source.ter1={sw,sh,Read(dir+"/ter1.bin")};
    const auto shadow=Read(dir+"/shadow.bin");if(shadow.size()!=256) {return 1;}
    std::copy(shadow.begin(),shadow.end(),source.shadow_lookup.begin());
    const uint32_t live[]={0,0,4,32,0x20000400,0,32},selected[]={0,0,4,32,0x20000400,32,0};
    for(int i=0;i<7;++i) {
        game::native_terrain::World world;world.viewer=3;
        for(auto& tile:world.tiles) {tile.band=3;tile.contour=60;tile.selected_known=tile.center_known=tile.resource_ready=true;}
        auto& center=world.tiles[8*40+4];center.items=center.resource_items=live[i];center.selected_items=selected[i];
        if(i==1) {
            center.band=4;center.contour=87;
            world.tiles[7*40+3].contour=63;
            world.tiles[7*40+4].band=5;world.tiles[7*40+4].contour=103;
            world.tiles[9*40+4].band=4;world.tiles[9*40+4].contour=89;
            world.tiles[9*40+3].contour=73;
        }
        const game::frontend::smac_terrain_raster::Bitmap backing{1024,257,std::vector<uint8_t>(1024*257,73)};
        const auto frame=game::frontend::smac_tile_preview::Paint(world,source,8,8,0xbf60,backing,{{148,67}},{{146,66,235,183}});
        if(!frame) {throw std::runtime_error("Measured preview rejected");}
        std::ofstream out(dir+"/frame-"+std::to_string(i)+".bin",std::ios::binary);
        out.write(reinterpret_cast<const char*>(frame->indexed.data()),frame->indexed.size());
        std::cout<<i<<' '<<frame->writes<<'\n';
        // Actual retained Planet records carry this inert detail/dirty word.
        // Preserve it; it must not veto the independent ordinary painter.
        for(auto& tile:world.tiles) {tile.landmarks=0xff400000u;}
        const auto retained=game::frontend::smac_tile_preview::Paint(world,source,8,8,0xbf60,backing,{{148,67}},{{146,66,235,183}});
        if(!retained || retained->indexed!=frame->indexed || retained->coverage!=frame->coverage) {return 5;}
        // Same actual source must keep unknown centers, unknown neighbor
        // memory, unsupported sea and unmeasured features unavailable.
        center.selected_known=false;if(game::frontend::smac_tile_preview::Paint(world,source,8,8,0,backing)) {return 2;}
        center.selected_known=true;world.tiles[7*40+3].selected_known=false;
        const auto partial=game::frontend::smac_tile_preview::Paint(world,source,8,8,0,backing);
        if(bool(partial)==(i==2 || i==3 || i==5)) {return 3;}
        world.tiles[7*40+3].selected_known=true;center.band=2;
        if(game::frontend::smac_tile_preview::Paint(world,source,8,8,0,backing)) {return 4;}
    }
}
