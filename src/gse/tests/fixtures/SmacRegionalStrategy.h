#pragma once
#include "game/backend/faction/SmacRegionalStrategy.h"
#include <vector>
// RE-SMAC-REGIONAL-STRATEGY-002 source SHA256 78fcb574252ba162cac2d200d0cc0d74a1fdcafce6ca4a1c37b04b22713cbbe8
namespace gse::tests::regional_strategy_fixture {
namespace f=game::backend::faction;
struct Query {int32_t peer,mode;bool result;};
struct Case {const char* id;f::SmacRegionalStrategyContext context;f::SmacRegionalStrategyRegion region;uint8_t old_code,expected;std::vector<Query> queries;};
inline std::vector<Case> Cases(){std::vector<Case> rows;
{Case row{"war_live_adjacent_base_weak",{1,1,1024,1,1,0,false,true},{},7,2,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
{Case row{"war_stale_presence_no_live_base",{1,1,1024,1,1,0,false,true},{},7,7,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
row.queries.push_back({2,0,false});
row.queries.push_back({2,1,false});
rows.push_back(std::move(row));}
{Case row{"war_live_adjacent_base_strong",{1,1,1024,1,1,0,false,true},{},7,0,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,2,2,0};
row.region.peers[2]={1,1,1,24u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
{Case row{"no_contact",{1,1,1024,1,1,0,false,true},{},7,7,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,16u};
rows.push_back(std::move(row));}
{Case row{"truce",{1,1,1024,1,1,0,false,true},{},7,7,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,25u};
rows.push_back(std::move(row));}
{Case row{"war_state_400",{1,1,1024,1,1,0,false,true},{},7,2,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,1032u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
{Case row{"war_state_800",{1,1,1024,1,1,0,false,true},{},7,2,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,2056u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
{Case row{"war_treaty_native_bilateral_policy",{1,1,1024,1,1,0,false,true},{},7,2,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,26u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
{Case row{"no_cached_peer_base_shortcut",{1,1,1024,1,1,0,false,true},{},7,2,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={0,2,2,24u};
rows.push_back(std::move(row));}
{Case row{"no_cached_peer_base_zero_units_skip",{1,1,1024,1,1,0,false,true},{},7,7,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={0,0,2,24u};
rows.push_back(std::move(row));}
{Case row{"no_cached_peer_base_five_attack_equality",{1,1,1024,1,1,0,false,true},{},7,0,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,5,0};
row.region.peers[2]={0,1,1,24u};
rows.push_back(std::move(row));}
{Case row{"no_cached_peer_base_five_attack_exceeded",{1,1,1024,1,1,0,false,true},{},7,7,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,6,0};
row.region.peers[2]={0,1,1,24u};
rows.push_back(std::move(row));}
{Case row{"owned_base_zero",{1,1,1024,0,1,0,false,true},{},7,0,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={0,1,0,0};
row.region.peers[2]={0,2,2,24u};
rows.push_back(std::move(row));}
{Case row{"owned_cached_base_zero_native_route_gate",{1,1,1024,0,1,0,false,true},{},7,0,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={0,1,0,0};
row.region.peers[2]={1,2,2,24u};
row.queries.push_back({2,0,false});
row.queries.push_back({2,1,false});
rows.push_back(std::move(row));}
{Case row{"stale_same_region_peer_count_actual_other_region",{1,1,1024,1,1,0,false,true},{},7,7,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
row.queries.push_back({2,0,false});
row.queries.push_back({2,1,false});
rows.push_back(std::move(row));}
{Case row{"pact_without_reachable_war_bypassed",{1,1,1024,1,1,0,false,true},{},7,7,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,3,3,40u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
{Case row{"war_take0_clears_pact_take2",{1,1,1024,1,1,0,false,true},{},7,0,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,2,2,0};
row.region.peers[2]={1,3,3,40u};
row.region.peers[3]={0,1,1,24u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
{Case row{"pact_zero_with_reachable_war",{1,1,1024,1,1,0,false,true},{},7,0,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,2,2,0};
row.region.peers[2]={1,1,3,40u};
row.region.peers[3]={0,1,1,24u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
{Case row{"two_war_peers_code2_after_code0",{1,1,1024,1,1,0,false,true},{},7,2,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,2,2,0};
row.region.peers[2]={0,3,3,24u};
row.region.peers[3]={0,1,1,24u};
rows.push_back(std::move(row));}
{Case row{"third_party_ai_territory_broad_route",{1,1,1024,1,1,0,false,true},{},7,2,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
row.queries.push_back({2,0,false});
row.queries.push_back({2,1,true});
rows.push_back(std::move(row));}
{Case row{"third_party_human_territory_blocks_both",{1,1,1024,1,1,0,false,true},{},7,7,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
row.queries.push_back({2,0,false});
row.queries.push_back({2,1,false});
rows.push_back(std::move(row));}
{Case row{"two_peer_components_two_failed_paths_per_mode",{1,1,1024,1,1,0,false,true},{},7,7,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={2,2,2,24u};
row.queries.push_back({2,0,false});
row.queries.push_back({2,1,false});
rows.push_back(std::move(row));}
{Case row{"one_peer_component_one_failed_path_per_mode",{1,1,1024,1,1,0,false,true},{},7,7,{}};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={2,2,2,24u};
row.queries.push_back({2,0,false});
row.queries.push_back({2,1,false});
rows.push_back(std::move(row));}
{Case row{"small_area_product49_fresh",{1,2,1024,7,1,0,false,true},{},7,2,{}};
row.region.stored_area=499;row.region.maximum_rating=1;row.region.peers[1]={7,1,0,0};
row.region.peers[2]={7,2,2,24u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
{Case row{"small_area_product50_stagger",{1,2,1024,5,1,0,false,true},{},7,7,{}};
row.region.stored_area=499;row.region.maximum_rating=1;row.region.peers[1]={5,1,0,0};
row.region.peers[2]={10,2,2,24u};
rows.push_back(std::move(row));}
{Case row{"area499_fresh",{1,2,1024,1,1,0,false,true},{},7,2,{}};
row.region.stored_area=499;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
{Case row{"area500_stagger_old7",{1,2,1024,1,1,0,false,true},{},7,7,{}};
row.region.stored_area=500;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
rows.push_back(std::move(row));}
{Case row{"area500_stagger_old2",{1,2,1024,1,1,0,false,true},{},2,2,{}};
row.region.stored_area=500;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
rows.push_back(std::move(row));}
{Case row{"area500_stagger_old3",{1,2,1024,1,1,0,false,true},{},3,7,{}};
row.region.stored_area=500;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
rows.push_back(std::move(row));}
{Case row{"area500_phase_zero",{1,7,1024,1,1,0,false,true},{},7,2,{}};
row.region.stored_area=500;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
{Case row{"area999_phase_zero",{1,7,1024,1,1,0,false,true},{},7,2,{}};
row.region.stored_area=999;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
{Case row{"area1000_new_phase_nonzero",{1,7,1024,1,1,0,false,true},{},7,7,{}};
row.region.stored_area=1000;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
rows.push_back(std::move(row));}
{Case row{"area1000_phase_zero",{1,14,1024,1,1,0,false,true},{},7,2,{}};
row.region.stored_area=1000;row.region.maximum_rating=1;row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,24u};
row.queries.push_back({2,0,true});
rows.push_back(std::move(row));}
return rows;}
struct Opening {const char* id;f::SmacRegionalStrategyRegion region;int32_t total_bases;uint8_t old_code,expected;};
inline std::vector<Opening> Openings(){std::vector<Opening> rows;
{Opening row{"occupied_zero_strategy",{},1,0,7};
row.region.peers[1]={1,1,0,0};
row.region.peers[2]={0,0,0,0u};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.owned_suitable=0;row.region.covered_suitable=0;
rows.push_back(std::move(row));}
{Opening row{"empty_owner",{},0,0,8};
row.region.peers[1]={0,0,0,0};
row.region.peers[2]={0,0,0,0u};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.owned_suitable=0;row.region.covered_suitable=0;
rows.push_back(std::move(row));}
{Opening row{"two_actual_scouts",{},1,0,7};
row.region.peers[1]={1,2,0,0};
row.region.peers[2]={0,0,0,0u};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.owned_suitable=0;row.region.covered_suitable=0;
rows.push_back(std::move(row));}
{Opening row{"two_owned_base_records",{},2,0,7};
row.region.peers[1]={2,1,0,0};
row.region.peers[2]={0,0,0,0u};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.owned_suitable=0;row.region.covered_suitable=0;
rows.push_back(std::move(row));}
{Opening row{"regional_reset_canaries",{},1,99,7};
row.region.peers[1]={1,1,0,0};
row.region.peers[2]={0,0,0,0u};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.owned_suitable=0;row.region.covered_suitable=0;
rows.push_back(std::move(row));}
{Opening row{"good4_rating2",{},1,0,8};
row.region.peers[1]={1,1,0,0};
row.region.peers[2]={0,0,0,0u};
row.region.stored_area=1024;row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=0;
rows.push_back(std::move(row));}
{Opening row{"good4_farmed1",{},1,0,7};
row.region.peers[1]={1,1,0,0};
row.region.peers[2]={0,0,0,0u};
row.region.stored_area=1024;row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=1;
rows.push_back(std::move(row));}
{Opening row{"good4_rating1",{},1,0,7};
row.region.peers[1]={1,1,0,0};
row.region.peers[2]={0,0,0,0u};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.owned_suitable=4;row.region.covered_suitable=0;
rows.push_back(std::move(row));}
{Opening row{"retained_flag8",{},1,0,7};
row.region.peers[1]={1,1,0,0};
row.region.peers[2]={0,0,0,0u};
row.region.stored_area=1024;row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=0;
row.region.flags=8;
rows.push_back(std::move(row));}
{Opening row{"factory_controlled_laser_prototype",{},1,7,7};
row.region.peers[1]={1,2,1,0};
row.region.peers[2]={0,0,0,0u};
row.region.stored_area=1024;row.region.maximum_rating=1;row.region.owned_suitable=0;row.region.covered_suitable=0;
rows.push_back(std::move(row));}
{Opening row{"pact_regional_units_no_base",{},1,0,8};
row.region.peers[1]={1,1,0,0};
row.region.peers[2]={0,2,2,40u};
row.region.stored_area=1024;row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=0;
rows.push_back(std::move(row));}
{Opening row{"pact_stale_base_presence",{},1,0,7};
row.region.peers[1]={1,1,0,0};
row.region.peers[2]={1,2,2,40u};
row.region.stored_area=1024;row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=0;
rows.push_back(std::move(row));}
{Opening row{"vendetta_foreign2_own_scout1",{},1,0,2};
row.region.peers[1]={1,1,0,0};
row.region.peers[2]={0,2,2,24u};
row.region.stored_area=1024;row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=0;
rows.push_back(std::move(row));}
{Opening row{"vendetta_foreign1_own_scout_and_laser",{},1,0,0};
row.region.peers[1]={1,2,1,0};
row.region.peers[2]={0,1,1,24u};
row.region.stored_area=1024;row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=0;
rows.push_back(std::move(row));}
return rows;}
}
