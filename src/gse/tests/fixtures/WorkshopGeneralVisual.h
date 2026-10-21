#pragma once
namespace gse::tests::workshop_general_visual_fixture {
// Fifteen complete native output digests only; original model/table operands are not runtime inputs.
// Verified source manifest b655640abdfa715169e8395d0ca170d30ed43c71fde108d2a372d078afa97cf4.
struct Frame {const char* name;int chassis,weapon,armor,reactor,kind;uint64_t color,depth;size_t draws,records;};
inline constexpr Frame FRAMES[]{
{"inf_laser_synth_fission_main",0,1,1,1,-1,UINT64_C(0xddb2c02b6c3c5173),UINT64_C(0x3b4fbdd983590afc),71,111597},
{"inf_laser_synth_fission_chassis",0,1,1,1,0,UINT64_C(0x2097ebc20e211bf1),UINT64_C(0xd4f0251e8f28e278),64,42810},
{"inf_laser_synth_fission_weapon",0,1,1,1,1,UINT64_C(0x9b79a74529c3d0ca),UINT64_C(0xc0c406af129de9c2),5,54953},
{"inf_laser_synth_fission_shielding",0,1,1,1,2,UINT64_C(0x12ad5bc8c5c3ad55),UINT64_C(0xc53de15396d87a55),1,9161},
{"inf_laser_synth_fission_reactor",0,1,1,1,3,UINT64_C(0xa11ad9ee87aca2aa),UINT64_C(0x1395c1ac9ae32933),1,4673},
{"inf_laser_plasma_fusion_main",0,1,2,2,-1,UINT64_C(0x427d5a420c470e44),UINT64_C(0xb43063638366568e),71,112190},
{"inf_laser_plasma_fusion_chassis",0,1,2,2,0,UINT64_C(0x2097ebc20e211bf1),UINT64_C(0xd4f0251e8f28e278),64,42810},
{"inf_laser_plasma_fusion_weapon",0,1,2,2,1,UINT64_C(0x9b79a74529c3d0ca),UINT64_C(0xc0c406af129de9c2),5,54953},
{"inf_laser_plasma_fusion_shielding",0,1,2,2,2,UINT64_C(0x99bf6fecd75cd085),UINT64_C(0xc53de15396d87a55),1,9161},
{"inf_laser_plasma_fusion_reactor",0,1,2,2,3,UINT64_C(0x42fc140c582c6f55),UINT64_C(0x64e5387174366a72),1,5266},
{"rover_impact_synth_fission_main",1,2,1,1,-1,UINT64_C(0xa124adb791f0272a),UINT64_C(0xfbb9f0ff420e4aa4),58,176588},
{"rover_impact_synth_fission_chassis",1,2,1,1,0,UINT64_C(0x539ab75422598f77),UINT64_C(0x42f0ad56f1df91de),23,101657},
{"rover_impact_synth_fission_weapon",1,2,1,1,1,UINT64_C(0x16e6e1434ac56b2d),UINT64_C(0x84e6ad5680fcf6aa),26,13594},
{"rover_impact_synth_fission_shielding",1,2,1,1,2,UINT64_C(0x5435508bad788fd2),UINT64_C(0x6d1e4ea0f4f9ec60),8,48758},
{"rover_impact_synth_fission_reactor",1,2,1,1,3,UINT64_C(0xa7095b33f0b97ba3),UINT64_C(0xa148a3a3b90686ae),1,12198},
};
}
