#pragma once
#include <cstdint>
#include <vector>
namespace smac_bloom_class_fixture {
struct Case {const char* id;int32_t turn,before,after,rockiness,result_rockiness;uint32_t landmarks,result_landmarks,seed,result_seed,dirty;std::vector<int32_t> draws;};
inline const std::vector<Case> cases{
{"sea_raise_no_rock_draw",90,1,2,1,1,0u,0u,0u,0u,4u,{}},
{"unchanged_land_no_draw_no_metadata",90,3,3,2,2,16777346u,16777346u,0u,0u,4u,{}},
{"turn_zero_no_draw_no_metadata",0,6,3,2,2,16777346u,16777346u,0u,0u,4u,{}},
{"land_lower_draw_changes_rock",90,6,3,2,1,0u,4194304u,0u,2531011u,5u,{38}},
{"land_lower_draw_zero_keeps_rock",90,6,3,2,2,0u,0u,2u,2959037u,4u,{45}},
{"sea_landmark_0x0",90,1,2,1,1,0u,0u,0u,0u,4u,{}},
{"sea_landmark_0x80",90,1,2,1,1,128u,4278190080u,0u,0u,4u,{}},
{"sea_landmark_0x1000082",90,1,2,1,1,16777346u,4261412866u,0u,0u,4u,{}},
{"sea_landmark_0x7f000082",90,1,2,1,1,2130706562u,2147483650u,0u,0u,4u,{}},
{"sea_landmark_0x80000082",90,1,2,1,1,2147483778u,2147483650u,0u,0u,4u,{}},
{"sea_landmark_0xff000082",90,1,2,1,1,4278190210u,4278190082u,0u,0u,4u,{}},
};
}
