#pragma once
namespace gse::tests::workshop_upgrade_strings {
// Original5935c0 scalar strings; pinned executable SHA256 in the behavior receipt.
struct Case {const char* id;uint32_t chassis,weapon,armor,mask;uint8_t reactor;int32_t domain;const char* expected;};
inline constexpr std::array<Case,11> Cases{{
{"plain",0,0,0,0,1,0,"1-1-1"},
{"fusion",0,0,0,0,2,0,"1-1-1*2"},
{"singularity",0,0,0,0,4,0,"1-1-1*4"},
{"decorations",0,0,0,1444664,2,0,"<1^~xe!>-<1+t>-1*2"},
{"psi_both",0,12,9,1444664,2,0,"Psi-Psi-1*2"},
{"special_weapon",0,13,0,1444664,1,0,"*-<1+t>-1"},
{"equipment",0,14,0,0,1,0,"0-1-1"},
{"sea",3,0,0,0,2,1,"(1)-1-4 Sea*2"},
{"air",5,0,0,0,2,2,"1-1-12 Air*2"},
{"psi_sea",3,0,9,0,2,1,"1-Psi-4 Sea*2"},
{"artillery",0,0,0,32768,1,0,"(1)-1-1"}
}};
}
