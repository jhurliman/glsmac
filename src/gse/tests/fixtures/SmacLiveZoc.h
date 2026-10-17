#pragma once
#include <cstdint>
namespace gse::tests::zoc_fixture {
// Literal projection of RE-SMAC-LAND-ZOC-001. No modeled cost/control oracle.
// SHA256 dcd0fd7eae568195a87244e1946d73530f510b4a5deae7c5dcd88fe35a59de9d
struct Case { const char* name; int sx,sy,dx,dy; int ex[2],ey[2],enemies; bool rover,pact,treaty,road,base,moved; int claim; uint8_t newly_revealed; };
inline constexpr Case CASES[] = {
  {"source_only", 32, 16, 30, 16, {33,0}, {15,0}, 1, false, false, false, false, false, true, -1, 0},
  {"destination_only", 30, 16, 32, 16, {33,0}, {15,0}, 1, false, false, false, false, false, true, -1, 1},
  {"both_endpoints", 32, 16, 34, 16, {33,0}, {15,0}, 1, false, false, false, false, false, false, -1, 0},
  {"pact_both", 32, 16, 34, 16, {33,0}, {15,0}, 1, false, true, false, false, false, true, -1, 0},
  {"treaty_both", 32, 16, 34, 16, {33,0}, {15,0}, 1, false, false, true, false, false, false, -1, 0},
  {"vendetta_rovers_both", 32, 16, 34, 16, {33,0}, {15,0}, 1, true, false, false, false, false, false, -1, 0},
  {"own_territory_both", 24, 16, 26, 16, {25,0}, {15,0}, 1, false, false, false, false, false, false, 1, 0},
  {"own_base_source_exemption", 16, 16, 18, 16, {17,0}, {15,0}, 1, false, false, false, false, true, true, 1, 0},
  {"different_units_cover_endpoints", 32, 16, 34, 16, {31,35}, {15,15}, 2, false, false, false, false, false, false, -1, 2},
  {"connected_roads_still_block", 32, 16, 34, 16, {33,0}, {15,0}, 1, false, false, false, true, false, false, -1, 0},
};
}
