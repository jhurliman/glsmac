#pragma once
#include <cstdint>
#include <vector>

namespace gse::tests::restart_package_fixture {
// Literal selected IDs and factory order from RE-SMAC-RESTART-PACKAGE-001.
// Public fixture SHA256 df18f5493b6bb0dc52a70f160fedbf5e34242408a5ca3622ea17a016dca43f65.
// Inputs use owned synthetic catalogs; no original names/assets or oracle code.
struct Case { const char* name; int32_t turn, defender, balanced, mobile; std::vector<int32_t> order; };
inline const std::vector<Case> cases{
    {"gaians_turn1",1,2,2,-1,{0,1,0,2}},
    {"gaians_turn100",100,2,2,-1,{0,1,0,2,2,2,2}},
    {"hive_turn100",100,2,2,-1,{0,1,0,2,2,2,2}},
    {"univ_turn100",100,2,2,-1,{0,1,0,2,2,2,2}},
    {"morgan_turn100",100,64,64,-1,{0,1,0,64,64,64,64}},
    {"spartans_turn100",100,2,64,64,{0,1,0,2,64,64,2,64}},
    {"believe_turn100",100,2,2,-1,{0,1,0,2,2,2,2}},
    {"peace_turn100",100,2,2,-1,{0,1,0,2,2,2,2}},
    {"threshold_40",40,2,2,-1,{0,1,0,2}},
    {"threshold_41",41,2,2,-1,{0,1,0,2,2}},
    {"threshold_60",60,2,2,-1,{0,1,0,2,2}},
    {"threshold_61",61,2,2,-1,{0,1,0,2,2,2}},
    {"threshold_80",80,2,2,-1,{0,1,0,2,2,2}},
    {"threshold_81",81,2,2,-1,{0,1,0,2,2,2,2}},
    {"spartans_turn20",20,2,64,64,{0,1,0,2}},
    {"spartans_turn21",21,2,64,64,{0,1,0,2,64}},
    {"no_owned_technology",100,2,2,-1,{0,1,0,2,2,2,2}},
    {"all_owned_technology",100,2,2,-1,{0,1,0,2,2,2,2}},
    {"saved_custom_three_roles",100,90,91,92,{0,1,0,90,92,91,90,91}},
    {"saved_custom_equal_scores",100,90,90,-1,{0,1,0,90,90,90,90}},
    {"saved_fast_negative_score",100,90,90,-1,{0,1,0,90,90,90,90}},
    {"saved_ineligible_decoys",100,2,2,-1,{0,1,0,2,2,2,2}},
    {"mixed_psi_zero_cache",100,90,2,-1,{0,1,0,90,2,90,2}},
};
}
