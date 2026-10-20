#include "game/frontend/NativeTerrainRenderer.h"
#include "planet_geometry_input.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc,char** argv) {
    if(argc!=4) {return 1;}
    namespace native=game::frontend::native_terrain;
    game::native_terrain::World world;world.retained_geometry=true;world.viewer=3;
    const auto cells=PlanetCells(argv[1]);
    for(size_t i=0;i<cells.size();++i) {
        world.tiles[i].band=cells[i].band;world.tiles[i].landmarks=cells[i].landmarks;
        world.tiles[i].ordinary=world.tiles[i].material_admitted=true;
    }
    std::ifstream input(argv[2],std::ios::binary);
    const std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(input),{}};
    if(bytes.size()!=3200) {return 1;}
    native::ViewGeometry::Knowledge known(3200,false);
    for(size_t i=0;i<bytes.size();++i) {if(bytes[i]>1) {return 1;}known[i]=bytes[i]!=0;}
    const std::filesystem::path out(argv[3]);std::filesystem::create_directories(out);
    native::ViewGeometry geometry(world);std::optional<native::Frame> previous;
    native::Source source;source.loaded={1024,768,std::vector<uint8_t>(1024*768,9)};
    source.palette[194]={13,28,48};
    constexpr int cameras[4][2]={{22,38},{22,38},{24,38},{22,38}};
    for(int frame=0;frame<4;++frame) {
        const auto stem=out/std::to_string(frame);std::ofstream queries(stem.string()+".queries");
        const bool pan=frame>=2;
        const auto plan=geometry.Prepare(native::Layout(cameras[frame][0],cameras[frame][1],0,true),0,25,pan,true,[&](const auto& q) {
            queries<<int(q.kind)<<' '<<q.tile[0]<<' '<<q.tile[1]<<' '<<q.vertex<<' '<<q.result[0]<<' '<<q.result[1]<<'\n';
        },&known);
        std::ofstream lines(stem.string()+".lines");
        for(const auto& tile:plan.terrain) {if(tile.unknown) {
            for(unsigned edge=0;edge<4;++edge) {
                const auto& a=tile.outline[edge];const auto& b=tile.outline[(edge+1)%4];
                lines<<a[0]<<' '<<a[1]<<' '<<b[0]<<' '<<b[1]<<" 194\n";
            }
        }}
        const auto cache=geometry.CacheBytes();std::ofstream cache_file(stem.string()+".cache",std::ios::binary);
        cache_file.write(reinterpret_cast<const char*>(cache.data()),cache.size());
        // Partial exploration checks the full production geometry schedule only.
        // Placeholder art is not a valid known sea/material raster profile.
        if(std::any_of(known.begin(),known.end(),[](bool value){return value;})) {
            std::cout<<frame<<" query-only "<<plan.terrain.size()<<" "<<plan.objects.size()<<'\n';continue;
        }
        auto result=native::RenderPrepared(world,source,plan,0,0x60010ee3,65,pan?&*previous:nullptr);
        for(size_t i=3;i<result.rgba.size();i+=4) {if(result.rgba[i]!=255) {throw std::runtime_error("Unknown backing became transparent");}}
        std::ofstream pixels(stem.string()+".indexed",std::ios::binary);
        pixels.write(reinterpret_cast<const char*>(result.indexed.data()),result.indexed.size());
        std::cout<<frame<<' '<<plan.terrain.size()<<' '<<plan.objects.size()<<' '<<result.tiles<<'\n';
        previous=std::move(result);
    }
}
