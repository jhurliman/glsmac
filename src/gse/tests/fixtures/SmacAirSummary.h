#pragma once
#include "game/backend/faction/SmacAirSummary.h"
namespace gse::tests::smac_air_fixture {
// Original summary-inputs manifest660ff6192f6816f884f95e1fa4b1df45143b7cac78b7ffc37104c21f0f28a71a.
struct Attack {int32_t offense,peer_defense,bonus;bool project;int32_t expected;};
inline const std::array<Attack,8> Attacks{{
{4,5,0,false,4},
{-1,5,0,false,7},
{-1,5,25,false,8},
{-1,5,25,true,12},
{-1,70000,0,false,105000},
{-1,-5,25,true,-12},
{-1,0,0,false,0},
{-1,5,-50,false,3},
}};
struct Counter {const char* name;game::backend::faction::SmacAirSummaryUnit unit;uint32_t relation;int32_t peer_defense,bonus;game::backend::faction::SmacAirSummary expected;};
inline const std::array<Counter,18> Counters{{
{"foreign_equal",{2,2,0,4,false},0x8u,4,0,{1,0,0}},
{"foreign_below",{2,2,0,4,false},0x8u,5,0,{0,0,0}},
{"no_contact",{2,2,0,4,false},0x0u,4,0,{0,0,0}},
{"excluded_rel1",{2,2,0,4,false},0x9u,4,0,{0,0,0}},
{"excluded_rel2",{2,2,0,4,false},0xau,4,0,{0,0,0}},
{"rel_override",{2,2,0,4,false},0x29u,4,0,{1,0,0}},
{"foreign_plan4",{2,2,4,4,false},0x8u,4,0,{0,0,0}},
{"foreign_special",{2,2,0,4,true},0x8u,4,0,{0,0,0}},
{"foreign_land",{2,0,0,4,false},0x8u,4,0,{0,0,0}},
{"foreign_psi",{2,2,0,-1,false},0x8u,4,0,{1,0,0}},
{"foreign_psi_discount",{2,2,0,-1,false},0x8u,4,-50,{0,0,0}},
{"own_air0",{1,2,0,4,false},0x8u,4,0,{0,1,0}},
{"own_air4",{1,2,4,4,false},0x8u,4,0,{0,0,1}},
{"own_air1",{1,2,1,4,false},0x8u,4,0,{0,0,0}},
{"own_special_air4",{1,2,4,4,true},0x8u,4,0,{0,0,1}},
{"own_native_locust",{1,2,4,-1,false},0x8u,4,0,{0,0,1}},
{"own_native_mindworm",{1,0,1,-1,false},0x8u,4,0,{0,0,0}},
{"foreign_native_locust",{2,2,4,-1,false},0x8u,4,0,{0,0,0}},
}};
}
