#pragma once
#include <array>
#include <cstdint>
namespace gse::tests::smac_diplomatic_warning_fixture {
// Literal19 named original-x86 cases RE-SMAC-DIPLO-WARN-001.
struct Case {const char* id;uint32_t relation,guards;bool proceed;int warning;bool pass;};
inline constexpr std::array<Case,19> Cases{{
    {"DIPLO-WARN-001",0x0u,0x7u,true,0,true},
    {"DIPLO-WARN-002",0x10u,0x7u,true,0,true},
    {"DIPLO-WARN-003",0x3u,0x7u,true,1,false},
    {"DIPLO-WARN-004",0x2u,0x7u,true,2,true},
    {"DIPLO-WARN-005",0x2u,0x7u,false,2,false},
    {"DIPLO-WARN-006",0x4u,0x7u,true,3,true},
    {"DIPLO-WARN-007",0x4u,0x7u,false,3,false},
    {"DIPLO-WARN-008",0x7u,0x7u,true,1,false},
    {"DIPLO-WARN-009",0x6u,0x7u,true,2,true},
    {"DIPLO-WARN-010",0x17u,0x7u,true,0,true},
    {"DIPLO-WARN-011",0x0u,0x7u,true,0,true},
    {"DIPLO-WARN-012",0x2u,0x4u,true,0,true},
    {"DIPLO-WARN-013",0x5u,0x4u,true,1,false},
    {"DIPLO-WARN-014",0x10004u,0x7u,true,4,true},
    {"DIPLO-WARN-015",0x10004u,0x7u,false,4,false},
    {"DIPLO-WARN-016",0x10004u,0x10007u,true,0,true},
    {"DIPLO-WARN-017",0x4u,0x10007u,false,3,false},
    {"DIPLO-WARN-018",0x2u,0x10007u,false,2,false},
    {"DIPLO-WARN-019",0x4u,0x7u,true,3,true},
}};
}
