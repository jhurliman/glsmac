#pragma once
namespace gse::tests::workshop_stock_visual_fixture {
// Complete native output digests only; no source assets or captured centers.
// Verified freeze a080004f9344c4d24341f467f2ce45af951568986c4aebfc7698bbc6b2454219.
struct Frame {const char* name;int chassis,weapon,armor,reactor,kind;uint64_t color,depth;size_t draws,records;};
inline constexpr Frame FRAMES[]{
{"inf_former_main",0,15,0,1,-1,UINT64_C(0x4002893630def339),UINT64_C(0x3fa5a8b44870a9e4),33,238672},
{"inf_former_weapon",0,15,0,1,1,UINT64_C(0x425c412ee2d37341),UINT64_C(0x0d52b5cf0473a19f),17,130689},
{"inf_colony_main",0,14,0,1,-1,UINT64_C(0xf0cc0bb7cee4e3d5),UINT64_C(0xde5d449599452814),20,187357},
{"inf_probe_main",0,18,0,1,-1,UINT64_C(0x610f6a7f958784c6),UINT64_C(0x66c1daedf0da2998),67,60982},
{"foil_laser_synth_main",3,1,1,1,-1,UINT64_C(0x4f396aba5f20f7ba),UINT64_C(0xff8a8515a3690697),23,166451},
{"cruiser_laser_plasma_main",4,1,2,2,-1,UINT64_C(0xe7d48344c704d3e3),UINT64_C(0xa4e33d4be6d98db3),36,199929},
{"needlejet_laser_synth_main",5,1,1,1,-1,UINT64_C(0x5d1e44301448bcc1),UINT64_C(0x2c599c1e363a3cd5),27,180109},
{"hover_laser_synth_main",2,1,1,1,-1,UINT64_C(0x04a748174346d586),UINT64_C(0x738e01661bee07bc),19,111222},
{"copter_laser_synth_main",6,1,1,1,-1,UINT64_C(0xbf0717fc45d45f61),UINT64_C(0x2a52a2508ef92032),36,176317},
{"grav_laser_synth_main",7,1,1,1,-1,UINT64_C(0xb8f4cccd3e678d43),UINT64_C(0x6bd90dd2e4a73d46),25,140795},
{"missile_conventional_main",8,20,0,1,-1,UINT64_C(0x90e6d9b560231473),UINT64_C(0xcd3c010ac0ce4094),13,99111},
{"inf_transport_weapon",0,16,0,1,1,UINT64_C(0x8da538e22254653c),UINT64_C(0xcaf6647ecc24b15d),1,35534},
{"inf_supply_weapon",0,17,0,1,1,UINT64_C(0xdd0037048efea4cd),UINT64_C(0x0502b6ee0f1f5f4f),1,18736},
{"inf_artifact_weapon",0,19,0,1,1,UINT64_C(0x980917fbe8ee1faa),UINT64_C(0x39259818bdf0277e),1,22033},
};
}
