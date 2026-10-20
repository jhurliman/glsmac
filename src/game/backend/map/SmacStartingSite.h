#pragma once
#include "SmacSiteGeometry.h"
#include "game/backend/SmacGameplayRandomState.h"
#include <functional>
#include <optional>
#include <vector>

namespace game::backend::map::smac_start {
using smac_site::Coordinate;using smac_site::Geometry;
struct Tile {uint8_t elevation,moisture,rockiness,region,landmark_detail;bool fungus,occupied;};
struct Base {int64_t id;Coordinate point;};
struct Unit {int32_t owner;Coordinate point;};
struct Region {int32_t area,wet;uint8_t presence;};
struct Input {
    Geometry geometry;int32_t scale,ocean,difficulty;bool human,look_first;
    // Ocean is the retained coverage category (native0x9b952c), not tile density.
    std::array<Region,128> regions;
    std::vector<Base> bases; // native record order; later records win distance ties
    std::vector<Unit> units;
};
using Query=std::function<Tile(Coordinate)>;
// Caller owns effective exclusion/cache/miss semantics and staged cache writes.
using Rating=std::function<int32_t(Coordinate)>;
enum class Outcome {Selected,Limit,LowElevation,Fungus,Rocky,InitialDistance,HighElevation,Occupied,Polar,GoodTiles,RelaxedDistance,EarlyRegionArea,RegionDensity,RegionArea,SharedRegionArea,SiteRating,Landmark};
const char* OutcomeName(Outcome value);
struct Attempt {int32_t number;Coordinate point;uint32_t crt;int32_t good,bucket,distance;int64_t nearest;uint8_t region,presence;std::optional<int32_t> rating;Outcome outcome;};
struct Refinement {size_t index;Coordinate point;int32_t score,previous;};
struct Result {std::optional<Coordinate> center,site;SmacGameplayRandomState random;size_t attempts=0,rating_reads=0;};
using Observe=std::function<void(const Attempt&)>;
using ObserveRefinement=std::function<void(const Refinement&)>;
using ObserveDraw=std::function<void(uint32_t,uint32_t,uint32_t,uint16_t)>; // caller,before,after,value
// Turn-zero ordinary search and final refinement. Never reseeds, changes input
// RNG, manufactures a cached rating or chooses a fixed candidate coordinate.
Result Select(const Input& input,const SmacGameplayRandomState& initial,const Query& query,const Rating& rating,
    const Observe& observe={},const ObserveRefinement& refinement={},const std::function<bool()>& cancelled={},const ObserveDraw& draws={});
}
