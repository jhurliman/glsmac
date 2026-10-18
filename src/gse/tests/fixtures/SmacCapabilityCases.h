#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
namespace gse::tests::smac_capability_fixture {
struct Case {const char* name;std::vector<std::pair<std::string,int32_t>> overrides;std::array<int32_t,5> expected;bool foreign;};
// RE-SMAC-CAPABILITY-BINDINGS-001: original native cases; port array order is
// weapon/armor/land speed/PSI weapon/PSI armor. All inputs are explicit overrides.
inline std::vector<Case> Cases() {return {
// Bundled receipt SHA256: a53406af8f482eb272de9116d6c7558ad2e61753f1d27bef32106bd0fd2f025b
{"empty",{},{1,1,1,0,0},false},
{"native_eligible",{{"technology_58_owner_mask",2}},{1,1,1,12,6},false},
{"native_inactive",{{"technology_58_owner_mask",2},{"mind_worm_active_flags",0}},{1,1,1,0,0},false},
{"native_excluded",{{"technology_58_owner_mask",2},{"mind_worm_faction_exclusion_mask",2}},{1,1,1,0,0},false},
{"native_no_prerequisite",{{"mind_worm_technology_prerequisite",-1}},{1,1,1,12,6},false},
{"native_disabled_prerequisite",{{"mind_worm_technology_prerequisite",-2}},{1,1,1,0,0},false},
{"native_future_prerequisite",{{"mind_worm_technology_prerequisite",88},{"technology_88_owner_mask",2}},{1,1,1,0,0},false},
{"native_disabled_technology",{{"technology_58_owner_mask",2},{"technology_58_first_prerequisite",-2}},{1,1,1,0,0},false},
{"native_no_base_sea",{{"technology_58_owner_mask",2},{"mind_worm_chassis",3}},{1,1,1,12,6},false},
{"native_no_colony_rule",{{"technology_58_owner_mask",2},{"primary_rules",4194304}},{1,1,1,12,6},false},
{"native_synthetic_colony_plan_blocked",{{"technology_58_owner_mask",2},{"mind_worm_plan",8},{"primary_rules",4194304}},{1,1,1,0,0},false},
{"weapon98",{{"weapon_0_raw",98},{"weapon_0_technology",-1}},{98,1,1,0,0},false},
{"weapon99_filtered",{{"weapon_0_raw",99},{"weapon_0_technology",-1}},{1,1,1,0,0},false},
{"armor99_accepted",{{"armor_0_raw",99},{"armor_0_technology",-1}},{1,99,1,0,0},false},
{"armor127_accepted",{{"armor_0_raw",127},{"armor_0_technology",-1}},{1,127,1,0,0},false},
{"armor_negative128",{{"armor_0_raw",-128},{"armor_0_technology",-1}},{1,6,1,0,6},false},
{"weapon_negative128",{{"weapon_0_raw",-128},{"weapon_0_technology",-1}},{12,1,1,12,0},false},
{"chassis_raw3",{{"chassis_0_raw",3},{"chassis_0_technology",-1}},{1,1,3,0,0},false},
{"chassis_unsigned255",{{"chassis_0_raw",255},{"chassis_0_technology",-1}},{1,1,255,0,0},false},
{"chassis_zero_floor",{{"chassis_0_raw",0},{"chassis_0_technology",-1}},{1,1,1,0,0},false},
{"chassis_domain1_ignored",{{"chassis_0_raw",255},{"chassis_0_technology",-1},{"chassis_0_domain",1}},{1,1,1,0,0},false},
{"chassis_domain2_ignored",{{"chassis_0_raw",255},{"chassis_0_technology",-1},{"chassis_0_domain",2}},{1,1,1,0,0},false},
{"chassis_unowned_prerequisite",{{"chassis_0_raw",255},{"chassis_0_technology",77}},{1,1,1,0,0},false},
{"chassis_late_owned_prerequisite",{{"chassis_0_raw",255},{"chassis_0_technology",77},{"technology_77_owner_mask",2}},{1,1,255,0,0},false},
{"armor_unowned_prerequisite",{{"armor_0_raw",127},{"armor_0_technology",77}},{1,1,1,0,0},false},
{"armor_late_owned_prerequisite",{{"armor_0_raw",127},{"armor_0_technology",77},{"technology_77_owner_mask",2}},{1,127,1,0,0},false},
{"native_disabled_second_with_first_none",{{"technology_58_owner_mask",2},{"technology_58_first_prerequisite",-1},{"technology_58_second_prerequisite",-2}},{1,1,1,12,6},false},
{"native_disabled_second_with_first_present",{{"technology_58_owner_mask",2},{"technology_58_first_prerequisite",0},{"technology_58_second_prerequisite",-2}},{1,1,1,0,0},false},
{"own_psi_full_width",{{"weapon_0_raw",-1},{"weapon_0_technology",-1},{"armor_0_raw",-1},{"armor_0_technology",-1},{"prior_foreign_armor",70000},{"prior_foreign_weapon",90000}},{105000,60000,1,105000,60000},false},
{"foreign_full_width",{{"peer2_five_caches",70000}},{70000,70000,70000,70000,70000},true},
{"foreign_negative_ignored",{{"peer2_five_caches",-7}},{0,0,0,0,0},true},
};}
}
namespace gse::tests::smac_capability_fixture {
struct PsiCase {const char* name;std::array<int32_t,2> foreign,ratio;int32_t percent,owned,projects;std::array<int32_t,5> expected;};
inline std::vector<PsiCase> PsiCases() {return {
{"native_only",{9,8},{3,2},0,1,0,{1,1,1,12,6}},
{"components_only",{9,8},{3,2},0,6,0,{12,6,1,12,6}},
{"neither",{9,8},{3,2},0,0,0,{1,1,1,0,0}},
{"zero_prior",{0,0},{3,2},0,7,0,{1,1,1,0,0}},
{"odd_prior",{7,5},{3,2},0,7,0,{7,4,1,7,4}},
{"percent_rounding",{7,5},{3,2},25,7,0,{8,5,1,8,5}},
{"negative_percent",{7,5},{3,2},-25,7,0,{5,3,1,5,3}},
{"both_projects",{9,8},{3,2},0,7,3,{18,9,1,18,9}},
{"attack_project",{9,8},{3,2},0,7,1,{18,6,1,18,6}},
{"defense_project",{9,8},{3,2},0,7,2,{12,9,1,12,9}},
{"foreign_projects",{9,8},{3,2},0,7,0,{12,6,1,12,6}},
{"round_before_project",{7,5},{3,2},25,7,3,{12,7,1,12,7}},
{"ratio_control",{7,5},{5,4},0,7,0,{6,5,1,6,5}},
{"zero_estimate_floor",{1,0},{3,2},0,7,0,{1,1,1,0,0}},
{"negative_input",{-7,-5},{3,2},25,7,3,{1,1,1,-12,-7}},
};}
}
