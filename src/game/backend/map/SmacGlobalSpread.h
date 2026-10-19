#pragma once
#include "SmacPlanetMap.h"
#include "SmacSiteGeometry.h"
#include "game/backend/SmacGameplayRandomState.h"

namespace game::backend::map::smac_spread {
using smac_site::Geometry;using smac_site::Coordinate;
struct Input {
    Geometry geometry;
    int32_t tile_count,turn;
    std::array<uint32_t,8> observer_status{};
    std::vector<SmacPlanetMap::Tile> tiles;
};
struct Change {
    int32_t iteration;Coordinate source,target;uint32_t feature;int32_t score;
    SmacPlanetMap::Tile before,after;
};
struct Result {
    SmacGameplayRandomState random;
    std::vector<SmacPlanetMap::Tile> tiles;
    std::vector<Coordinate> samples;
    std::vector<Change> changes;
};
// The forest/kelp part of global537790, before wildlife generation. Uses staged
// current terrain, so a feature created early can spread in a later sample.
// Does not perform wildlife/global maintenance or publish rendering updates.
Result Prepare(const Input& input,const SmacGameplayRandomState& initial);
}
