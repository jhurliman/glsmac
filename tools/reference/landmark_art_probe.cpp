#include "game/frontend/SmacLandmarkArt.h"
#include <iostream>
#include <stdexcept>
int main() {
    unsigned count;std::cin>>count;
    for(unsigned c=0;c<count;++c) {
        uint32_t word;bool land;std::array<game::frontend::smac_terrain_raster::Point,5> points{};
        std::array<int,5> lighting{};std::cin>>word>>land;
        for(auto& point:points) {std::cin>>point[0]>>point[1];}
        for(auto& light:lighting) {std::cin>>light;}
        if(!std::cin) {throw std::runtime_error("Truncated landmark input");}
        const auto passes=game::frontend::smac_landmark_art::Quads(word,land,points,lighting);
        std::cout<<passes.size();
        for(const auto& pass:passes) {
            for(auto value:pass.crop) {std::cout<<' '<<value;}
            std::cout<<' '<<(pass.shader==game::frontend::smac_terrain_raster::Shader::KeyedUnlit)
                <<' '<<pass.shade_limits[0]<<' '<<pass.shade_limits[1];
            for(const auto& v:pass.polygon.vertices) {
                std::cout<<' '<<v.screen[0]<<' '<<v.screen[1]<<' '<<v.uv[0]<<' '<<v.uv[1]<<' '<<v.light;
            }
        }
        std::cout<<'\n';
    }
}
