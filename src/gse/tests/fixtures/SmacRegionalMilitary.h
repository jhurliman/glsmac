#pragma once
#include "game/backend/faction/SmacRegionalMilitaryState.h"
#include <vector>
namespace gse::tests::regional_military_fixture {
using Military=game::backend::faction::SmacRegionalMilitaryState;
// Native resolved-component/morale projection; excludes flags and cleanup admission.
// Public source SHA256 501f0e76475c3fd1046805cdd42d424ecf4adfd5453996be05f38f76e9e8ad1f
struct Case {const char* id; Military::Owner before,expected; std::vector<Military::Contribution> units;};
inline std::vector<Case> Cases(){std::vector<Case> cases;
{Case row{"scout_at_base",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"scout_offbase",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"scout_order_1",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,1,0});
cases.push_back(std::move(row));}
{Case row{"scout_order_2",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,2,2,0});
cases.push_back(std::move(row));}
{Case row{"scout_order_3",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,3,0});
cases.push_back(std::move(row));}
{Case row{"scout_order_4",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,4,0});
cases.push_back(std::move(row));}
{Case row{"scout_order_5",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,5,0});
cases.push_back(std::move(row));}
{Case row{"scout_order_8",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,8,0});
cases.push_back(std::move(row));}
{Case row{"scout_order_9",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,9,0});
cases.push_back(std::move(row));}
{Case row{"rover_at_base",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"rover_offbase",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"rover_order_1",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,1,0});
cases.push_back(std::move(row));}
{Case row{"rover_order_2",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,2,2,0});
cases.push_back(std::move(row));}
{Case row{"rover_order_3",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,3,0});
cases.push_back(std::move(row));}
{Case row{"rover_order_4",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,4,0});
cases.push_back(std::move(row));}
{Case row{"rover_order_5",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,5,0});
cases.push_back(std::move(row));}
{Case row{"rover_order_8",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,8,0});
cases.push_back(std::move(row));}
{Case row{"rover_order_9",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,9,0});
cases.push_back(std::move(row));}
{Case row{"former_at_base",{},{},{}};
row.units.push_back({1,9,0,1,0,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"former_offbase",{},{},{}};
row.units.push_back({1,9,0,1,0,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"former_order_1",{},{},{}};
row.units.push_back({1,9,0,1,0,1,2,1,0});
cases.push_back(std::move(row));}
{Case row{"former_order_2",{},{},{}};
row.units.push_back({1,9,0,1,0,1,2,2,0});
cases.push_back(std::move(row));}
{Case row{"former_order_3",{},{},{}};
row.units.push_back({1,9,0,1,0,1,2,3,0});
cases.push_back(std::move(row));}
{Case row{"former_order_4",{},{},{}};
row.units.push_back({1,9,0,1,0,1,2,4,0});
cases.push_back(std::move(row));}
{Case row{"former_order_5",{},{},{}};
row.units.push_back({1,9,0,1,0,1,2,5,0});
cases.push_back(std::move(row));}
{Case row{"former_order_8",{},{},{}};
row.units.push_back({1,9,0,1,0,1,2,8,0});
cases.push_back(std::move(row));}
{Case row{"former_order_9",{},{},{}};
row.units.push_back({1,9,0,1,0,1,2,9,0});
cases.push_back(std::move(row));}
{Case row{"colony_at_base",{},{},{}};
row.units.push_back({1,8,0,1,0,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"colony_offbase",{},{},{}};
row.units.push_back({1,8,0,1,0,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"colony_order_1",{},{},{}};
row.units.push_back({1,8,0,1,0,1,2,1,0});
cases.push_back(std::move(row));}
{Case row{"colony_order_2",{},{},{}};
row.units.push_back({1,8,0,1,0,1,2,2,0});
cases.push_back(std::move(row));}
{Case row{"colony_order_3",{},{},{}};
row.units.push_back({1,8,0,1,0,1,2,3,0});
cases.push_back(std::move(row));}
{Case row{"colony_order_4",{},{},{}};
row.units.push_back({1,8,0,1,0,1,2,4,0});
cases.push_back(std::move(row));}
{Case row{"colony_order_5",{},{},{}};
row.units.push_back({1,8,0,1,0,1,2,5,0});
cases.push_back(std::move(row));}
{Case row{"colony_order_8",{},{},{}};
row.units.push_back({1,8,0,1,0,1,2,8,0});
cases.push_back(std::move(row));}
{Case row{"colony_order_9",{},{},{}};
row.units.push_back({1,8,0,1,0,1,2,9,0});
cases.push_back(std::move(row));}
{Case row{"morale_2_0",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_2_1",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_2_2",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,1,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_2_3",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_2_4",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,3,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_2_5",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,4,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_2_6",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,5,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_2_7",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,6,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_2_255",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,6,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_2_0",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_2_1",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_2_2",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_2_3",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_2_7",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_2_8",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_2_99",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_11_0",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_11_1",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_11_2",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,1,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_11_3",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_11_4",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,3,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_11_5",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,4,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_11_6",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,5,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_11_7",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,6,0,0});
cases.push_back(std::move(row));}
{Case row{"morale_11_255",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,6,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_11_0",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_11_1",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_11_2",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_11_3",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_11_7",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_11_8",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"old_strategy_11_99",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"two_colonies_one_region",{},{},{}};
row.units.push_back({1,8,0,1,0,1,0,0,0});
row.units.push_back({1,8,0,1,0,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"colony_former_scout_rover",{},{},{}};
row.expected.units[1]=2;
row.units.push_back({1,3,0,1,1,1,1,0,0});
row.units.push_back({1,3,0,1,1,1,1,0,0});
row.units.push_back({1,9,0,1,0,1,1,0,0});
row.units.push_back({1,8,0,1,0,1,1,0,0});
cases.push_back(std::move(row));}
{Case row{"foreign_scout_excluded",{},{},{}};
cases.push_back(std::move(row));}
{Case row{"water_base_2",{},{},{}};
row.expected.attack[65]=1;
row.units.push_back({65,3,0,0,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"water_unbased_2",{},{},{}};
cases.push_back(std::move(row));}
{Case row{"water_base_11",{},{},{}};
row.expected.attack[65]=1;
row.units.push_back({65,3,0,0,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"water_unbased_11",{},{},{}};
cases.push_back(std::move(row));}
{Case row{"water_base_1",{},{},{}};
row.units.push_back({65,9,0,0,0,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"water_unbased_1",{},{},{}};
cases.push_back(std::move(row));}
{Case row{"water_base_0",{},{},{}};
row.units.push_back({65,8,0,0,0,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"water_unbased_0",{},{},{}};
cases.push_back(std::move(row));}
{Case row{"region_0",{},{},{}};
row.expected.units[0]=1;
row.units.push_back({0,3,0,1,1,1,1,0,0});
cases.push_back(std::move(row));}
{Case row{"region_62",{},{},{}};
row.expected.units[62]=1;
row.units.push_back({62,3,0,1,1,1,1,0,0});
cases.push_back(std::move(row));}
{Case row{"region_63",{},{},{}};
row.expected.units[63]=1;
row.units.push_back({63,3,0,1,1,1,1,0,0});
cases.push_back(std::move(row));}
{Case row{"region_64",{},{},{}};
row.expected.units[64]=1;
row.units.push_back({64,3,0,1,1,1,1,0,0});
cases.push_back(std::move(row));}
{Case row{"region_65",{},{},{}};
row.expected.units[65]=1;
row.units.push_back({65,3,0,1,1,1,1,0,0});
cases.push_back(std::move(row));}
{Case row{"region_127",{},{},{}};
row.expected.units[127]=1;
row.units.push_back({127,3,0,1,1,1,1,0,0});
cases.push_back(std::move(row));}
{Case row{"raw_strength_2_1_order_0",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=2;
row.units.push_back({1,3,0,1,2,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"raw_strength_2_1_order_2",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=2;
row.units.push_back({1,3,0,1,2,1,2,2,0});
cases.push_back(std::move(row));}
{Case row{"raw_strength_1_2_order_0",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,2,2,0,0});
cases.push_back(std::move(row));}
{Case row{"raw_strength_1_2_order_2",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,2,2,2,0});
cases.push_back(std::move(row));}
{Case row{"raw_strength_0_1_order_0",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,0,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"raw_strength_0_1_order_2",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,0,1,2,2,0});
cases.push_back(std::move(row));}
{Case row{"damage_does_not_scale",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"no_home_morale",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"mode0_riot_counterexample",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,2,1,1,0,0});
cases.push_back(std::move(row));}
{Case row{"social_morale_-4",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,0,0,0});
cases.push_back(std::move(row));}
{Case row{"social_morale_-2",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,1,0,0});
cases.push_back(std::move(row));}
{Case row{"social_morale_-1",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,3,0,1,1,1,1,0,0});
cases.push_back(std::move(row));}
{Case row{"social_morale_0",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"social_morale_1",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,3,0,0});
cases.push_back(std::move(row));}
{Case row{"social_morale_2",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,3,0,0});
cases.push_back(std::move(row));}
{Case row{"social_morale_3",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,4,0,0});
cases.push_back(std::move(row));}
{Case row{"social_morale_4",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,5,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_0",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,0,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_1",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,1,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_2",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,2,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_3",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_4",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,4,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_5",{},{},{}};
row.expected.units[1]=1;
row.units.push_back({1,5,0,1,0,0,0,0,1});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_6",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,6,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_7",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,7,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_8",{},{},{}};
row.expected.attack[1]=1;
row.units.push_back({1,8,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_9",{},{},{}};
row.expected.attack[1]=1;
row.units.push_back({1,9,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_10",{},{},{}};
row.expected.attack[1]=1;
row.units.push_back({1,10,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_11",{},{},{}};
row.expected.attack[1]=1;
row.units.push_back({1,11,0,1,1,1,3,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_plan_12",{},{},{}};
row.expected.attack[1]=1;
row.units.push_back({1,12,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_civilian_positive_attack_0",{},{},{}};
row.expected.attack[1]=2;
row.units.push_back({1,8,0,1,2,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_civilian_positive_attack_1",{},{},{}};
row.expected.attack[1]=2;
row.units.push_back({1,9,0,1,2,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_Base_marker_affiliation_0",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_Base_marker_affiliation_2",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_Base_marker_affiliation_7",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_Base_marker_affiliation_8",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_Base_marker_affiliation_15",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=1;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_chassis_domain_1",{},{},{}};
cases.push_back(std::move(row));}
{Case row{"diagnostic_chassis_domain_at_Base_1",{},{},{}};
row.expected.attack[1]=1;
row.units.push_back({1,3,1,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_chassis_domain_2",{},{},{}};
row.expected.attack[1]=1;
row.units.push_back({1,3,2,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_chassis_domain_at_Base_2",{},{},{}};
row.expected.attack[1]=1;
row.units.push_back({1,3,2,1,1,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_transported_scout",{},{},{}};
row.expected.attack[65]=1;
row.units.push_back({65,3,0,0,1,1,2,0,0});
row.units.push_back({65,7,1,0,0,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_large_strength",{},{},{}};
row.expected.units[1]=1;
row.expected.attack[1]=190;
row.units.push_back({1,3,0,1,127,127,6,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_native_probe_flag",{},{},{}};
row.units.push_back({1,11,0,1,0,1,2,0,0});
cases.push_back(std::move(row));}
{Case row{"diagnostic_counter_wrap",{},{},{}};
// Declared mid-phase injection; first watched word writes record before=65535.
row.before.units[1]=65535;row.before.attack[1]=65535;
row.units.push_back({1,3,0,1,1,1,2,0,0});
cases.push_back(std::move(row));}
return cases;}
}
