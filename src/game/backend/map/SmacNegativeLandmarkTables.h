#pragma once
#include <array>
#include <cstdint>
namespace game::backend::map::smac_terrain::landmark_tables {
// Bounded preceding mapping bytes from the pinned original SMAC executable.
inline constexpr std::array<int,8> NEGATIVE_DETAILS={{-48,-46,-45,-30,-28,-26,-8,-1}};
inline constexpr std::array<std::array<uint8_t,5>,8> NEGATIVE_MAPPING={{
    {{4,3,4,3,5}},
    {{4,3,7,1,8}},
    {{1,1,1,1,1}},
    {{2,2,5,2,7}},
    {{10,2,12,0,6}},
    {{13,0,1,1,1}},
    {{7,3,7,3,12}},
    {{3,13,3,14,0}}
}};
}
