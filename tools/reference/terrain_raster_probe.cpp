// Standalone original-asset comparison; no engine, graphics, or asset bundle.
#include "game/frontend/SmacTerrainRaster.h"
#include "gse/tests/fixtures/SmacTerrainRaster.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

int main(int argc,char** argv) {
    if(argc!=3) {throw std::invalid_argument("Expected loaded56x56 index crop and output prefix");}
    namespace raster=game::frontend::smac_terrain_raster;
    std::ifstream input(argv[1],std::ios::binary);
    raster::Bitmap texture{56,56,std::vector<uint8_t>(std::istreambuf_iterator<char>(input),{})};
    if(texture.pixels.size()!=3136) {throw std::invalid_argument("Invalid loaded texture crop");}
    const auto cases=gse::tests::smac_terrain_fixture::Cases();
    const raster::Bitmap canvas{1024,511,std::vector<uint8_t>(1024*511,250)};
    for(size_t i=0;i<4;++i) {
        std::vector<raster::Triangle> triangles;
        for(size_t j=0;j<4;++j) {triangles.push_back(cases[i*4+j].triangle);}
        const auto result=raster::Draw(canvas,texture,triangles,{0,0,1024,511});
        std::ofstream output(std::string(argv[2])+std::to_string(i)+".bin",std::ios::binary);
        output.write(reinterpret_cast<const char*>(result.bitmap.pixels.data()),static_cast<std::streamsize>(result.bitmap.pixels.size()));
        if(!output) {throw std::runtime_error("Could not write indexed frame");}
        std::cout<<i<<":"<<result.writes<<"\n";
    }
}
