#include "gse/tests/fixtures/SmacTerrainPolygonRaster.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

int main(int argc,char** argv) {
    if(argc!=3) {throw std::invalid_argument("Expected loaded texture directory and output prefix");}
    namespace raster=game::frontend::smac_terrain_raster;
    const auto frames=gse::tests::polygon_raster_fixture::Frames();
    const raster::Bitmap canvas{1024,511,std::vector<uint8_t>(1024*511,250)};
    for(size_t frame=0;frame<frames.size();++frame) {
        std::vector<raster::Bitmap> textures;textures.reserve(frames[frame].draws.size());
        std::vector<raster::Command> commands;
        for(size_t i=0;i<frames[frame].draws.size();++i) {
            const auto& draw=frames[frame].draws[i];
            std::ifstream source(std::string(argv[1])+"/texture-"+std::to_string(frame)+"-"+std::to_string(i)+".bin",std::ios::binary);
            textures.push_back({draw.crop[2],draw.crop[3],std::vector<uint8_t>(std::istreambuf_iterator<char>(source),{})});
        }
        for(size_t i=0;i<frames[frame].draws.size();++i) {
            const auto& draw=frames[frame].draws[i];commands.push_back({draw.polygon,&textures[i],draw.shader,draw.clip});
        }
        const auto result=raster::DrawBatch(canvas,commands,frames[frame].clip);
        std::ofstream output(std::string(argv[2])+std::to_string(frame)+".bin",std::ios::binary);
        output.write(reinterpret_cast<const char*>(result.bitmap.pixels.data()),static_cast<std::streamsize>(result.bitmap.pixels.size()));
        if(!output) {throw std::runtime_error("Could not write indexed frame");}
        std::cout<<frame<<":"<<result.attempts<<":"<<result.writes<<"\n";
    }
}
