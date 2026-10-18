#pragma once
#include "game/backend/faction/SmacRegionalStrategy.h"
#include <vector>
// Literal original initial stores, SHA256 bcbcb576f542b4e2f7df7d3040c428e36196aa48ccd201338107e4543b321cbc
namespace gse::tests::regional_initial_fixture {
namespace f=game::backend::faction;
struct Case {const char* id;f::SmacRegionalStrategyContext context;f::SmacRegionalStrategyRegion region;uint8_t expected;};
inline std::vector<Case> Cases(){std::vector<Case> rows;
{Case row{"reused_good4_rating2_region1",{1,1,1024,1,1,0,false,true},{},8};
row.region.peers[1].bases=1;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=0;row.region.neutral_suitable=0;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"reused_good4_farmed1_region1",{1,1,1024,1,1,0,false,true},{},7};
row.region.peers[1].bases=1;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=1;row.region.neutral_suitable=0;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"reused_good4_rating1_region1",{1,1,1024,1,1,0,false,true},{},7};
row.region.peers[1].bases=1;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=1;row.region.owned_suitable=4;row.region.covered_suitable=0;row.region.neutral_suitable=0;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"reused_retained_flag8_region1",{1,1,1024,1,1,0,false,true},{},7};
row.region.peers[1].bases=1;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=0;row.region.neutral_suitable=0;row.region.flags=8;
rows.push_back(std::move(row));}
{Case row{"reused_two_owned_base_records_region1",{1,1,1024,2,1,0,false,true},{},7};
row.region.peers[1].bases=2;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=1;row.region.owned_suitable=0;row.region.covered_suitable=0;row.region.neutral_suitable=0;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"reused_pact_stale_base_presence_region1",{1,1,1024,1,1,0,false,true},{},7};
row.region.peers[1].bases=1;
row.region.peers[2].bases=1;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=0;row.region.neutral_suitable=0;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"reused_pact_regional_units_no_base_region1",{1,1,1024,1,1,0,false,true},{},8};
row.region.peers[1].bases=1;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=0;row.region.neutral_suitable=0;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_first_comparison_equal",{1,75,3200,4,0,0,false,false},{},8};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=20;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_first_comparison_strict",{1,75,3200,4,0,0,false,false},{},7};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=19;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_retained_bit256_set",{1,75,3200,4,0,0,false,true},{},8};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=19;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_N_five",{1,75,3200,5,0,0,false,false},{},7};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=20;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_world_tiles_3201",{1,75,3201,4,0,0,false,false},{},7};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=20;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_turn_76",{1,76,3200,4,0,0,false,false},{},7};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=20;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_dominance_one",{1,75,3200,4,0,0,true,false},{},7};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=20;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_presence_H_two",{1,75,3200,4,0,0,false,false},{},7};
row.region.peers[1].bases=3;
row.region.peers[2].bases=1;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=20;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_Explore_one",{1,75,3200,4,1,0,false,false},{},7};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=20;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_Build_one",{1,75,3200,4,0,1,false,false},{},8};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=20;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_signed_Build_six",{1,75,3200,4,0,6,false,false},{},8};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=20;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_signed_Explore_negative",{1,75,3200,4,-6,0,false,false},{},8};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=20;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_second_comparison_equal",{1,75,3200,4,0,0,false,false},{},8};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=16;row.region.covered_suitable=0;row.region.neutral_suitable=0;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_ceil_C_three",{1,75,3200,4,0,0,false,false},{},7};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=15;row.region.covered_suitable=0;row.region.neutral_suitable=0;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_ceil_C_two",{1,75,3200,4,0,0,false,false},{},8};
row.region.peers[1].bases=2;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=15;row.region.covered_suitable=0;row.region.neutral_suitable=0;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_ceil_C_four",{1,75,3200,4,0,0,false,false},{},7};
row.region.peers[1].bases=4;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=15;row.region.covered_suitable=0;row.region.neutral_suitable=0;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_R_three",{1,75,3200,4,0,0,false,false},{},7};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=3;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=19;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_R_four",{1,75,3200,4,0,0,false,false},{},8};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=4;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=19;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_G_minus_F_four",{1,75,3200,4,0,0,false,false},{},8};
row.region.peers[1].bases=1;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=0;row.region.neutral_suitable=1000;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_G_minus_F_three",{1,75,3200,4,0,0,false,false},{},7};
row.region.peers[1].bases=1;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=4;row.region.covered_suitable=1;row.region.neutral_suitable=1000;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_negative_G_minus_F_division",{1,75,3200,4,0,0,false,false},{},7};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=0;row.region.covered_suitable=5;row.region.neutral_suitable=0;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_C_nineteen",{1,75,3200,4,0,0,false,false},{},8};
row.region.peers[1].bases=19;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=1000;row.region.covered_suitable=0;row.region.neutral_suitable=10000;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_C_twenty",{1,75,3200,4,0,0,false,false},{},7};
row.region.peers[1].bases=20;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=1000;row.region.covered_suitable=0;row.region.neutral_suitable=10000;row.region.flags=0;
rows.push_back(std::move(row));}
{Case row{"fragment_empty_C_bypasses_pressure",{1,75,3200,4,0,0,false,false},{},8};
row.region.peers[1].bases=0;
row.region.peers[2].bases=1;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=1;row.region.owned_suitable=0;row.region.covered_suitable=5;row.region.neutral_suitable=20;row.region.flags=8;
rows.push_back(std::move(row));}
{Case row{"fragment_signed_N_negative",{1,75,3200,-1,0,0,false,false},{},8};
row.region.peers[1].bases=3;
row.region.peers[2].bases=0;
row.region.peers[3].bases=0;
row.region.peers[4].bases=0;
row.region.peers[5].bases=0;
row.region.peers[6].bases=0;
row.region.peers[7].bases=0;
row.region.maximum_rating=2;row.region.owned_suitable=10;row.region.covered_suitable=0;row.region.neutral_suitable=20;row.region.flags=0;
rows.push_back(std::move(row));}
return rows;}
}
