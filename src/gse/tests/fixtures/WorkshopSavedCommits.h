#pragma once
namespace gse::tests::workshop_commit_fixture {
// Seven complete native saved-pool digests, freeze f8c5706cb48dfbab7581793f1f4e1371c4ec0c1ff1e17cddfc7262f860b480db.
struct Case {const char* name;game::backend::unit::SmacWorkshopDraft draft;int selected;uint64_t digest;};
inline const Case CASES[]{
{"inf_equal_laser_synth",{2,0,1,1,0,1,""},127,UINT64_C(0xc596e86cbffc1f04)},
{"rover_impact_synth",{2,1,2,1,0,1,""},127,UINT64_C(0xf1134ad9b4db93fe)},
{"inf_fusion_guard",{2,0,1,2,0,2,""},127,UINT64_C(0x3c6b0d163d98f4d8)},
{"inf_clean",{2,0,1,1,65536,1,""},127,UINT64_C(0x5f81ee61c1c0e2dc)},
{"inf_artillery",{2,0,2,2,32768,1,""},127,UINT64_C(0xb0959b22b951661c)},
{"inf_clean_trance",{2,0,1,1,327680,2,""},127,UINT64_C(0x0b5803447dbb1da3)},
{"long_amphib_artillery",{2,0,11,7,32776,4,""},127,UINT64_C(0x63026cb7f06f8c60)},
};
}
