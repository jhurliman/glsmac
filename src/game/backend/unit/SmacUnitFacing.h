#pragma once
#include "game/backend/map/tile/Tile.h"
#include <stdexcept>

namespace game::backend::unit {
// Original direction codes; use actual neighbour links so the horizontal seam
// has the same authority as ordinary adjacency, without guessed coordinate wrap.
inline uint8_t SmacFacingForStep(const map::tile::Tile& source,const map::tile::Tile& destination) {
    const map::tile::Tile* neighbours[]={source.NE,source.E,source.SE,source.S,source.SW,source.W,source.NW,source.N};
    for(uint8_t direction=0;direction<8;++direction) {if(neighbours[direction]==&destination) {return direction;}}
    throw std::runtime_error("Facing requires a current adjacent destination");
}
}
