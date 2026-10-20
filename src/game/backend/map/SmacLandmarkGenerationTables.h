#pragma once
#include "SmacShapingOffsets.h"
namespace game::backend::map::smac_generation::landmark_tables {
// Immutable original ordered footprint: reuse the established shaping rings.
inline constexpr std::array<std::array<int32_t,2>,64> OUTER{{
{{0,-16}},
{{1,-15}},
{{2,-14}},
{{3,-13}},
{{4,-12}},
{{5,-11}},
{{6,-10}},
{{7,-9}},
{{8,-8}},
{{9,-7}},
{{10,-6}},
{{11,-5}},
{{12,-4}},
{{13,-3}},
{{14,-2}},
{{15,-1}},
{{16,0}},
{{15,1}},
{{14,2}},
{{13,3}},
{{12,4}},
{{11,5}},
{{10,6}},
{{9,7}},
{{8,8}},
{{7,9}},
{{6,10}},
{{5,11}},
{{4,12}},
{{3,13}},
{{2,14}},
{{1,15}},
{{0,16}},
{{-1,15}},
{{-2,14}},
{{-3,13}},
{{-4,12}},
{{-5,11}},
{{-6,10}},
{{-7,9}},
{{-8,8}},
{{-9,7}},
{{-10,6}},
{{-11,5}},
{{-12,4}},
{{-13,3}},
{{-14,2}},
{{-15,1}},
{{-16,0}},
{{-15,-1}},
{{-14,-2}},
{{-13,-3}},
{{-12,-4}},
{{-11,-5}},
{{-10,-6}},
{{-9,-7}},
{{-8,-8}},
{{-7,-9}},
{{-6,-10}},
{{-5,-11}},
{{-4,-12}},
{{-3,-13}},
{{-2,-14}},
{{-1,-15}},
}};
constexpr auto Offsets() {
    std::array<std::array<int32_t,2>,289> out{};
    for(size_t i=0;i<225;++i) {out[i]=smac_shaping::OFFSETS[i];}
    for(size_t i=0;i<64;++i) {out[225+i]=OUTER[i];}
    return out;
}
inline constexpr auto OFFSETS=Offsets();
inline constexpr std::array<size_t,13> RIDGE{{47,44,24,20,8,7,0,5,4,17,23,35,45}};
}
