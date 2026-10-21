#pragma once
namespace gse::tests::workshop_animation_fixture {
// Complete original rasters, SHA256-checked before reducing to FNV1a digests.
// Freeze52a360088d8c333fdbc2373ffb5028131add72d5d3d66afe4781b52fe9da767b.
// No captured camera, model transforms or original asset bytes are inputs.
struct Frame {const char* name;int chassis;uint32_t phase,next,yaw;uint64_t color,depth;};
inline constexpr Frame FRAMES[]{
{"inf_yaw_first_main",0,1,1,0x3d4ccccd,UINT64_C(0xe1d2303bd4a6c91a),UINT64_C(0xbd219244e6e85fdf)},
{"inf_yaw_next_main",0,1,1,0x3dcccccd,UINT64_C(0xb73982f7a4dbb68f),UINT64_C(0x604db4a0e0c96465)},
{"copter_phase14_main",6,14,15,0x3d4ccccd,UINT64_C(0x21e0b69973a41ef0),UINT64_C(0x03ced90f0ef19d8a)},
{"copter_phase15_main",6,15,16,0x3d4ccccd,UINT64_C(0x3c31cf3b073bf6db),UINT64_C(0x8b78f02eac25ccbc)},
};
}
