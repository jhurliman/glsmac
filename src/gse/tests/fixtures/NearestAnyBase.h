#pragma once
#include "game/backend/SmacMovementHistory.h"
#include <array>

namespace gse::tests::nearest_any_fixture {
// RE-SMAC-NEAREST-ANY-BASE-001 synthetic records, literal expected outputs.
// Source research fixture SHA256 0ce8640e79c9f1e57cf3ac47a5387d6541ed8c90e26a7bfa53fb69f99b243123.
// Record IDs below are their original zero-based order; owners are not inputs
// to this all-owner query. Cases04/05 differ only in deliberately ignored owner.
struct Case { int x, y, width; bool wrap; std::vector< game::backend::SmacMovementHistory::BasePosition > bases; int expected, distance; };
inline const std::array< Case, 14 > cases{{
	{17,17,128,true,{},-1,777},
	{17,17,128,true,{{0,16,16}},0,1},
	{17,17,128,true,{{0,12,12},{1,18,18}},1,1},
	{17,17,128,true,{{0,16,16},{1,18,18}},1,1},
	{17,17,128,true,{{0,16,16},{1,18,18}},1,1},
	{17,17,128,true,{{0,18,18},{1,16,16}},1,1},
	{17,17,128,true,{{0,16,16},{1,17,17}},1,0},
	{0,0,128,true,{{0,126,0},{1,10,0}},0,1},
	{0,0,128,false,{{0,126,0},{1,10,0}},1,7},
	{0,0,128,true,{{0,0,62},{1,0,10}},1,7},
	{0,0,128,true,{{0,8,0}},0,6},
	{17,17,128,true,{{0,18,18},{1,22,22}},0,1},
	{0,0,32000,false,{{0,9999,9999}},0,9999},
	{0,0,32000,false,{{0,10000,10000}},-1,777},
}};
}
