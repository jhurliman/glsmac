#pragma once
#include "game/backend/map/SmacMinimap.h"
#include <array>
namespace gse::tests::minimap_fixture {
using namespace game::backend::map::smac_minimap;
struct Case {const char* name; Input input; std::optional<Color> expected;};
inline MapColors Colors() {return MapColors{{
    Color{249,{{255,0,0}}},
    Color{241,{{0,235,0}}},
    Color{252,{{0,0,255}}},
    Color{255,{{255,255,255}}},
    Color{251,{{255,255,0}}},
    Color{65,{{0,0,0}}},
    Color{102,{{235,92,0}}},
    Color{235,{{164,176,232}}},
}};}
// Literal original-x86 cases, not derived expectations.
inline const std::array<Case,28> cases{{
    {"physical_band0",{0,0,3,-1,0x80000000u,0x0u,true,false,false,false},Color{190,{{14,37,75}}}},
    {"physical_band1",{1,0,3,-1,0x80000000u,0x0u,true,false,false,false},Color{190,{{14,37,75}}}},
    {"physical_band2",{2,0,3,-1,0x80000000u,0x0u,true,false,false,false},Color{190,{{14,37,75}}}},
    {"physical_band3",{3,0,3,-1,0x80000000u,0x0u,true,false,false,false},Color{22,{{80,65,51}}}},
    {"physical_band4",{4,0,3,-1,0x80000000u,0x0u,true,false,false,false},Color{20,{{93,76,59}}}},
    {"physical_band5",{5,0,3,-1,0x80000000u,0x0u,true,false,false,false},Color{18,{{106,87,67}}}},
    {"physical_band6",{6,0,3,-1,0x80000000u,0x0u,true,false,false,false},Color{16,{{120,98,74}}}},
    {"physical_band7",{7,0,3,-1,0x80000000u,0x0u,true,false,false,false},Color{14,{{133,109,82}}}},
    {"rainfall_0",{3,0,3,-1,0x80000010u,0x0u,true,false,false,false},Color{149,{{212,208,116}}}},
    {"rainfall_1",{3,1,3,-1,0x80000010u,0x0u,true,false,false,false},Color{151,{{152,176,76}}}},
    {"rainfall_2",{3,2,3,-1,0x80000010u,0x0u,true,false,false,false},Color{153,{{88,140,44}}}},
    {"rainfall_3",{3,3,3,-1,0x80000010u,0x0u,true,false,false,false},Color{155,{{216,224,235}}}},
    {"political_own",{3,0,3,3,0x80010000u,0x0u,true,false,false,false},Color{255,{{255,255,255}}}},
    {"political_unclaimed",{3,0,3,-1,0x80010000u,0x0u,true,false,false,false},Color{22,{{80,65,51}}}},
    {"political_foreign_unrecognized",{3,0,3,2,0x80010000u,0x0u,true,false,false,false},Color{22,{{80,65,51}}}},
    {"political_foreign_contact_only",{3,0,3,2,0x80010000u,0x8u,true,false,false,false},Color{22,{{80,65,51}}}},
    {"political_foreign_other_bit_only",{3,0,3,2,0x80010000u,0x8000000u,true,false,false,false},Color{22,{{80,65,51}}}},
    {"political_foreign_recognized",{3,0,3,2,0x80010000u,0x8000008u,true,false,false,false},Color{252,{{0,0,255}}}},
    {"political_unknown",{3,0,3,3,0x80010000u,0x0u,false,false,false,false},std::nullopt},
    {"political_reveal",{3,0,3,3,0x80010000u,0x0u,false,false,true,false},Color{255,{{255,255,255}}}},
    {"political_editor_foreign",{3,0,3,2,0x80010000u,0x0u,true,false,false,true},Color{252,{{0,0,255}}}},
    {"physical_unknown",{3,0,3,-1,0x80000000u,0x0u,false,false,false,false},std::nullopt},
    {"physical_rule100_unknown",{3,0,3,-1,0x80000000u,0x0u,false,true,false,false},Color{22,{{80,65,51}}}},
    {"political_survey_unknown",{3,0,3,3,0x80010000u,0x0u,false,true,false,false},Color{22,{{80,65,51}}}},
    {"political_reveal_foreign_unrecognized",{3,0,3,2,0x80010000u,0x0u,false,false,true,false},Color{22,{{80,65,51}}}},
    {"political_sea_own",{2,0,3,3,0x80010000u,0x0u,true,false,false,false},Color{190,{{14,37,75}}}},
    {"rainfall_sea",{2,2,3,-1,0x80000010u,0x0u,true,false,false,false},Color{190,{{14,37,75}}}},
    {"political_native_claim_zero",{3,0,3,0,0x80010000u,0x0u,true,false,false,false},Color{22,{{80,65,51}}}},
}};
}
