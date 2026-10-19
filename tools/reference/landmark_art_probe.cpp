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
        uint32_t flags;std::cin>>flags;
        uint32_t items;std::cin>>items;
        std::array<std::optional<game::frontend::smac_landmark_art::Neighbor>,8> neighbors{};
        for(auto& neighbor:neighbors) {
            unsigned band;uint32_t landmarks;std::cin>>band>>landmarks;
            neighbor={{uint8_t(band),landmarks}};
        }
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
        for(const auto& marker:{game::frontend::smac_landmark_art::EarlyMarker(word,flags),
                               game::frontend::smac_landmark_art::Nexus(word,flags)}) {
            std::cout<<' '<<bool(marker);
            if(marker) {for(auto value:*marker) {std::cout<<' '<<value;}}
        }
        const auto jungle=game::frontend::smac_landmark_art::Jungle(word,land,points,neighbors);
        std::cout<<' '<<bool(jungle);
        if(jungle) {
            for(auto value:jungle->crop) {std::cout<<' '<<value;}
            for(const auto& v:jungle->polygon.vertices) {
                std::cout<<' '<<v.screen[0]<<' '<<v.screen[1]<<' '<<v.uv[0]<<' '<<v.uv[1];
            }
        }
        const auto dunes=game::frontend::smac_landmark_art::Dunes(word,land,items,flags);
        std::cout<<' '<<bool(dunes);
        if(dunes) {for(auto value:*dunes) {std::cout<<' '<<value;}}
        std::cout<<'\n';
    }
}
