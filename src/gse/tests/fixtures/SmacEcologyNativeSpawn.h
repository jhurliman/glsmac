#pragma once
#include <vector>
// Frozen RE-SMAC-ECOLOGY-ORDERED-SPAWN-001, six original spawn-stage controls.
namespace smac_native_spawn_fixture {
struct Call {int32_t design,x,y,sweep_attempts,special_counter,result;};
struct Born {int32_t design,x,y,tag;};
struct Case {const char* name;int32_t history,region,count,population;bool absent,no_fungus;int32_t sweep_attempts,special_counter;std::vector<Call> calls;std::vector<Born> born;};
inline const std::vector<Case> cases{
{"land_H7",7,1,0,3,false,false,7,0,{{8,40,40,1,0,0},{8,41,39,2,0,1},{8,42,40,3,0,2},{8,41,41,4,0,3},{8,40,42,5,0,4},{8,39,41,6,0,5},{8,38,40,7,0,6}},{{8,40,40,1},{8,41,39,1},{8,42,40,1},{8,41,41,1},{8,40,42,1},{8,39,41,1},{8,38,40,1}}},
{"land_H9",9,1,0,3,false,false,9,0,{{8,40,40,1,0,0},{8,41,39,2,0,1},{8,42,40,3,0,2},{8,41,41,4,0,3},{8,40,42,5,0,4},{8,39,41,6,0,5},{8,38,40,7,0,6},{8,39,39,8,0,7},{8,40,38,9,0,8}},{{8,40,40,1},{8,41,39,1},{8,42,40,1},{8,41,41,1},{8,40,42,1},{8,39,41,1},{8,38,40,1},{8,39,39,1},{8,40,38,1}}},
{"sea_region_present_H9",9,65,0,3,false,false,2,9,{{10,40,40,1,1,0},{9,41,39,2,2,1},{9,40,40,2,9,2}},{{10,40,40,1},{9,41,39,1},{9,40,40,0}}},
{"sea_region_absent_H9",9,65,0,3,true,false,2,9,{{10,40,40,1,1,0},{9,41,39,2,2,1},{10,40,40,2,9,2}},{{10,40,40,1},{9,41,39,1},{10,40,40,0}}},
{"fallback_population2_H12",12,1,0,2,false,true,0,0,{{8,40,40,0,0,0},{8,40,40,0,0,1}},{{8,40,40,0},{8,40,40,0}}},
{"capacity_full_H9",9,1,2048,3,false,false,9,0,{{8,40,40,1,0,-1},{8,41,39,2,0,-1},{8,42,40,3,0,-1},{8,41,41,4,0,-1},{8,40,42,5,0,-1},{8,39,41,6,0,-1},{8,38,40,7,0,-1},{8,39,39,8,0,-1},{8,40,38,9,0,-1}},{}}
};
}
