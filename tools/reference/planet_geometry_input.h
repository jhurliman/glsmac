#pragma once
#include "game/backend/map/SmacLandmarkGeometry.h"
#include <fstream>
#include <iterator>
using game::backend::map::smac_terrain::LandmarkGeometry;
inline std::vector<LandmarkGeometry::Cell> PlanetCells(const std::string& path) {
    std::ifstream file(path,std::ios::binary);std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
    if(bytes.size()!=15+0xaa4+3200*44+128) {throw std::runtime_error("Unexpected pinned Planet size");}
    std::vector<LandmarkGeometry::Cell> cells;
    for(size_t i=0;i<3200;++i) {
        const auto offset=2739+44*i;
        const uint32_t word=uint32_t(bytes[offset+12])|(uint32_t(bytes[offset+13])<<8)|
            (uint32_t(bytes[offset+14])<<16)|(uint32_t(bytes[offset+15])<<24);
        cells.push_back({uint8_t(bytes[offset]>>5),word});
    }
    return cells;
}
