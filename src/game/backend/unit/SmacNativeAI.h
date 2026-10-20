#pragma once
#include "game/backend/map/SmacSiteGeometry.h"
#include "game/backend/SmacGameplayRandomState.h"
#include <array>
#include <functional>
#include <optional>
#include <vector>

namespace game::backend::unit::smac_native_ai {
using Coordinate=map::smac_site::Coordinate;
using Geometry=map::smac_site::Geometry;
struct Base {
    int32_t id;Coordinate point;uint8_t region,owner;
    int32_t field_c0,field_c4,field48,role12_count;
};
struct TargetTerm {int32_t base,distance,denominator,metric;};
struct Target {int32_t base=-1,metric=9999;std::vector<TargetTerm> terms;};
Target ChooseBase(Geometry geometry,Coordinate point,uint8_t region,int32_t home,uint8_t tag,
    const std::array<int32_t,8>& planet,const std::vector<Base>& bases);
struct Head {int32_t owner,design,weapon_offense,triad;};
struct Candidate {
    int32_t direction;Coordinate point;bool in_bounds,sea;uint32_t items;uint8_t affiliation;
    std::optional<Head> head;
};
enum class Rejection {None,Bounds,LandToWater,SeaToLand,BeachingIntoPositiveUnit,NegativePredefinedWeapon,AirMismatch};
struct Score {
    int32_t direction;Rejection rejection;int32_t base_bonus=0,features=0,item_2000_bonus=0,jitter=0,heading=0,total=0;
};
struct Competition {
    SmacGameplayRandomState random;int32_t level,best=0,direction=-1;std::vector<Score> terms;
};
// Healthy normal competition only: later path-derived heading, damaged-unit
// hesitation and human-target pillaging retain separate caller policies.
Competition ChooseNeighbors(bool current_sea,int32_t triad,uint8_t facing,int32_t turn,int32_t difficulty,
    uint32_t state,const SmacGameplayRandomState& random,const std::vector<Candidate>& candidates);
enum class UpkeepAction {ClearSentry,Sentry,Delete};
struct Upkeep {uint32_t state;int32_t order;std::vector<UpkeepAction> actions;};
// Planning-stage scalar decisions, after the distinct Unit maintenance phase.
// Execute sentry/deletion through canonical publishers, not scalar writes alone.
Upkeep PlanUpkeep(int32_t triad,bool sea,bool ordinary_base,uint32_t flags,int32_t order,
    uint32_t state,int32_t stack_transport_roles,bool human=false);
bool CommandPassAdmits(int32_t priority,int32_t role,int32_t triad,int32_t damage,int32_t owner,
    bool first_pass_helper,int32_t land_triad_count,int32_t weapon,bool ability16);
struct Repeat {bool finish;int32_t retries;bool exhaust;};
Repeat CommandRepeat(int32_t result,int32_t count_before,int32_t count_after,int32_t order,int32_t triad,int32_t retries);
struct Tile {bool sea;uint8_t region,affiliation,explored;uint32_t items;};
using TileQuery=std::function<Tile(Coordinate)>;
struct Shore {Coordinate point;int32_t count;};
struct Directive {std::optional<Coordinate> waypoint;bool direct=false;std::vector<Shore> terms;};
// A waypoint means native order24/status99. No waypoint leaves the existing
// order alone. Query callbacks read one detached current transaction.
Directive CarrierDirective(Geometry geometry,Coordinate point,Coordinate target,const TileQuery& tile,
    int32_t source_base=-1,const std::function<bool(int32_t,uint8_t)>& water_connected={});
enum class CarrierAction {NormalCompetition,Disband,SeaAttack,ActivateStack,Directive};
struct Carrier {CarrierAction action;int32_t direction=-1;std::optional<Coordinate> point;uint8_t observation=0;};
Carrier CarrierStep(Geometry geometry,Coordinate point,const Base& target,int32_t work_after_increment,
    int32_t land_triad_count,int32_t capacity,const TileQuery& tile);
}
