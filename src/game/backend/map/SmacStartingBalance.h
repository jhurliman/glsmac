#pragma once
#include "SmacSiteGeometry.h"
#include "game/backend/SmacGameplayRandomState.h"
#include <functional>
#include <optional>
#include <vector>

namespace game::backend::map::smac_balance {
using smac_site::Coordinate;using smac_site::Geometry;
struct Tile {uint8_t elevation,rain,rockiness,region;uint32_t items;};
struct Unit {int64_t id;int32_t owner;Coordinate point;};
struct Input {
    Geometry geometry;int32_t difficulty;uint8_t human_mask;
    std::array<int32_t,128> areas;
    std::vector<Unit> units;
    // Roles of configured faction UNIT descriptors, not a live Unit census.
    std::array<std::vector<uint8_t>,8> configured_unit_roles;
};
struct Score {int32_t owner;Coordinate point;uint8_t region;int32_t area,spacing,space,terrain_count,terrain,region_term,value;};
struct Deficit {int32_t owner,maximum,credit,value;std::array<std::optional<int32_t>,8> scores;};
struct Write {int32_t owner;Coordinate point;uint32_t before,mask,after;};
struct Pod {int32_t owner;Coordinate point;};
struct Result {
    SmacGameplayRandomState random;
    std::vector<Score> scores;std::vector<Deficit> deficits;
    std::vector<Pod> pods;std::vector<Write> writes;
};
using Query=std::function<Tile(Coordinate)>;
// Resolve the original site query against the staged, current items word.
using Site=std::function<int32_t(Coordinate,uint32_t)>;
// Detached plan only: the owning startup transaction creates real Units and
// publishes ordered live terrain changes. No fixed positions or packages.
Result Prepare(const Input& input,const SmacGameplayRandomState& initial,const Query& query,const Site& site);
}
