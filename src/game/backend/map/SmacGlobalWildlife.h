#pragma once
#include "SmacGlobalSpread.h"
#include <functional>

namespace game::backend::map::smac_wildlife {
using smac_site::Coordinate;using smac_site::Geometry;
struct Base {int32_t id;Coordinate point;};
struct Input {
    Geometry geometry;
    int32_t turn,life,difficulty,voice_marker,unit_count;
    uint32_t rules,secondary;
    std::array<int32_t,128> region_areas{};
    std::array<uint32_t,8> observer_status{},native_diplomacy{};
    std::array<uint32_t,2> locust_technology_masks{};
    std::vector<Base> bases; // native record order; nearest ties choose the last
};
struct Hooks {
    // Every hook reads/writes the same detached transaction. No live publication,
    // script callbacks or yields: the caller discards that transaction on error.
    // Query must see factory sight/occupancy effects from earlier births.
    std::function<SmacPlanetMap::Tile(Coordinate)> tile;
    std::function<int32_t(Coordinate)> site;
    // Actual staged constructor, including its own RNG and full-pool behavior.
    // Returns native dense Unit index, or -1 for the original factory failure.
    std::function<int32_t(int32_t,Coordinate,SmacGameplayRandomState&)> create;
    std::function<int32_t(int32_t)> capacity;
    // Native order1, payload-1, waypointY0=0; not the port's Hold order2.
    std::function<void(int32_t,SmacGameplayRandomState&)> sentry,promote_stack;
};
struct Action {
    enum class Kind {Create,Sentry,PromoteStack};
    Kind kind;int32_t unit,design;Coordinate point;
};
struct Result {
    SmacGameplayRandomState random;
    int32_t unit_count;
    std::vector<Coordinate> sea_samples,land_samples;
    std::vector<Action> actions;
};
// Reachable stock global537790 fauna policy, after flora. The owning factory
// supplies full constructor/order/stack semantics; these are not guessed births.
Result Produce(const Input& input,const SmacGameplayRandomState& initial,const Hooks& hooks);
}
