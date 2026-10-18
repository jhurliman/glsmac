#pragma once
#include "game/backend/resource/SmacSiteSignal.h"
#include <array>
namespace gse::tests::smac_site_signal_fixture {
struct Case {const char* name;game::backend::resource::smac::SiteSignalInputs input;uint8_t expected;};
// Bundled resolved-site-pod-query-cases.json SHA256: 1f5f67dc11555ca2485b7a0e1e2a1d313cb024bb67559d9e550016b5b34393d4
inline const std::array<Case,36> cases{{
{"zero_pattern",{0,0,game::backend::resource::smac::AltitudeBand::Land3,0u,false,0,11,61},0},
{"force_with_zero_pattern",{0,0,game::backend::resource::smac::AltitudeBand::Land3,268435456u,false,0,11,61},1},
{"monolith_suppresses_force",{0,0,game::backend::resource::smac::AltitudeBand::Land3,268443648u,false,0,11,61},0},
{"special_marker_suppresses_force",{0,0,game::backend::resource::smac::AltitudeBand::Land3,269484032u,false,1,11,61},0},
{"automatic_ignores_force",{0,0,game::backend::resource::smac::AltitudeBand::Land3,268435456u,true,1,11,61},0},
{"automatic_explicit_bit_a",{0,0,game::backend::resource::smac::AltitudeBand::Land3,67108864u,true,1,11,61},2},
{"automatic_explicit_bit_b",{0,0,game::backend::resource::smac::AltitudeBand::Land3,134217728u,true,1,11,61},2},
{"automatic_explicit_both",{0,0,game::backend::resource::smac::AltitudeBand::Land3,201326592u,true,1,11,61},2},
{"automatic_monolith_suppresses_explicit",{0,0,game::backend::resource::smac::AltitudeBand::Land3,201334784u,true,1,11,61},0},
{"automatic_marker_suppresses_explicit",{0,0,game::backend::resource::smac::AltitudeBand::Land3,202375168u,true,1,11,61},0},
{"explicit_bits_ignored_in_pattern_mode",{0,0,game::backend::resource::smac::AltitudeBand::Land3,201326592u,false,0,11,61},0},
{"force_on_deep_water",{0,0,game::backend::resource::smac::AltitudeBand::Trench,268435456u,false,0,11,61},1},
{"primary_land_pattern_match",{0,0,game::backend::resource::smac::AltitudeBand::Land3,0u,false,16,11,61},2},
{"no_primary_on_water",{0,0,game::backend::resource::smac::AltitudeBand::Shelf,0u,false,16,11,61},0},
{"fungus_does_not_suppress",{5,1,game::backend::resource::smac::AltitudeBand::Land3,32u,false,1,11,61},0},
{"base_radius_does_not_suppress",{5,1,game::backend::resource::smac::AltitudeBand::Land3,4096u,false,1,11,61},0},
{"farm_does_not_suppress",{5,1,game::backend::resource::smac::AltitudeBand::Land3,32768u,false,1,11,61},0},
{"base_and_unit_do_not_suppress",{5,1,game::backend::resource::smac::AltitudeBand::Land3,3u,false,1,11,61},0},
{"forced_class1_trench",{5,1,game::backend::resource::smac::AltitudeBand::Trench,268435456u,false,1,11,61},1},
{"explicit_class2_trench",{5,1,game::backend::resource::smac::AltitudeBand::Trench,67108864u,true,1,11,61},2},
{"coefficient_0",{0,16,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,0,0},0},
{"coefficient_1",{1,17,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,0,0},0},
{"coefficient_2",{78,0,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,0,0},0},
{"coefficient_3",{1,79,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,0,0},0},
{"coefficient_4",{0,16,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,-11,61},0},
{"coefficient_5",{1,17,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,-11,61},0},
{"coefficient_6",{78,0,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,-11,61},0},
{"coefficient_7",{1,79,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,-11,61},0},
{"coefficient_8",{0,16,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,11,-61},0},
{"coefficient_9",{1,17,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,11,-61},0},
{"coefficient_10",{78,0,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,11,-61},0},
{"coefficient_11",{1,79,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,11,-61},0},
{"coefficient_12",{0,16,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,2147483647,-2147483648},0},
{"coefficient_13",{1,17,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,2147483647,-2147483648},0},
{"coefficient_14",{78,0,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,2147483647,-2147483648},0},
{"coefficient_15",{1,79,game::backend::resource::smac::AltitudeBand::Land3,0u,false,-1,2147483647,-2147483648},0},
}};
}
