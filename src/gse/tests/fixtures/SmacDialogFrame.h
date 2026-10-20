#pragma once
#include <array>
#include <cstdint>
namespace gse::tests::smac_frame_fixture {
// Original indexed buffers, manifest23bffe9407b3618a1f3322b768424fb337c22527eccc26cb80b8ab9217a066cb.
struct Case {const char* name;int w,h;std::array<int,5> call;std::array<int,4> clip;bool background7;uint64_t hash;};
inline constexpr std::array<Case,11> Cases{{
    {"outer9",80,60,{9,0,0,73,51},{0,0,80,60},false,0x1026ca0a1624d425ull},
    {"caption3",80,40,{3,4,4,70,28},{0,0,80,40},false,0xbb45bd10811824cfull},
    {"caption6",80,40,{6,7,7,67,25},{0,0,80,40},false,0x7f9779c43c1ed4b1ull},
    {"body0",84,64,{0,9,11,71,51},{0,0,84,64},false,0xaded50e940190663ull},
    {"row27",80,45,{27,0,19,75,35},{0,0,80,45},false,0x089ec898a2988661ull},
    {"row28",80,45,{28,0,19,75,35},{0,0,80,45},false,0x923f55989ac32bf1ull},
    {"row29",80,45,{29,0,0,75,16},{0,0,80,45},false,0x1dd3980a2d086251ull},
    {"outer9-offset",91,75,{9,11,13,79,61},{0,0,91,75},false,0xd8691403750d06f2ull},
    {"outer9-clipped",70,50,{9,-3,-2,51,39},{5,7,47,36},false,0x018791fe9b836b47ull},
    {"outer9-tiny",30,31,{9,8,9,18,20},{0,0,30,31},false,0xfa7772871eb1c12bull},
    {"outer9-background7",80,60,{9,0,0,73,51},{0,0,80,60},true,0xe7d2a9754f9a27c3ull},
}};
}
