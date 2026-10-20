#include "game/frontend/NativeTerrainRenderer.h"
#include "planet_geometry_input.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
int main(int argc,char** argv) {
    if(argc!=3) {throw std::invalid_argument("Expected Planet and output directory");}
    namespace native=game::frontend::native_terrain;
    const auto planet=PlanetCells(argv[1]);const std::filesystem::path directory(argv[2]);
    std::filesystem::create_directories(directory);
    int groups;std::cin>>groups;
    for(int group=0;group<groups;++group) {
        int w,h,vw,vh,z,wrap,known_count;std::cin>>w>>h>>vw>>vh>>z>>wrap>>known_count;
        game::native_terrain::World world(w,h);world.horizontal_wrap=wrap;world.retained_geometry=true;
        for(size_t i=0;i<world.tiles.size();++i) {
            world.tiles[i].band=w==80?planet.at(i).band:4;
            world.tiles[i].landmarks=w==80?planet.at(i).landmarks:0;
        }
        native::ViewGeometry::Knowledge known(world.tiles.size(),false);
        for(int i=0;i<known_count;++i) {int x,y;std::cin>>x>>y;known.at(size_t(y)*size_t(w/2)+size_t(x)/2)=true;}
        native::ViewGeometry geometry(world);std::optional<native::Frame> previous;
        native::Source source;source.loaded={1024,768,std::vector<uint8_t>(1024*768,9)};source.palette[194]={13,28,48};
        for(int frame=0;frame<3;++frame) {
            int x,y;std::cin>>x>>y;const auto stem=directory/(std::to_string(group)+"-"+std::to_string(frame));
            std::ofstream queries(stem.string()+".queries");
            const auto view=native::Layout(x,y,z,wrap,{{w,h}},{{vw,vh}});
            const auto plan=geometry.Prepare(view,z,25,frame!=0,true,[&](const auto& q) {
                queries<<int(q.kind)<<' '<<q.tile[0]<<' '<<q.tile[1]<<' '<<q.vertex<<' '<<q.result[0]<<' '<<q.result[1]<<'\n';
            },&known);
            const auto cache=geometry.CacheBytes();std::ofstream cache_file(stem.string()+".cache",std::ios::binary);
            cache_file.write(reinterpret_cast<const char*>(cache.data()),cache.size());
            if(known_count) {continue;}
            auto result=native::RenderPrepared(world,source,plan,0,0x60010ee3,65,frame?&*previous:nullptr);
            if(result.dimensions!=view.viewport || result.indexed.size()!=size_t(vw)*size_t(vh)) {throw std::runtime_error("Bitmap dimensions differ");}
            for(size_t i=3;i<result.rgba.size();i+=4) {if(result.rgba[i]!=255) {throw std::runtime_error("Unknown bitmap became transparent");}}
            std::ofstream pixels(stem.string()+".indexed",std::ios::binary);pixels.write(reinterpret_cast<const char*>(result.indexed.data()),result.indexed.size());
            previous=std::move(result);
        }
    }
    if(!std::cin) {throw std::runtime_error("Truncated input");}
}
