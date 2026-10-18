#pragma once
#include "game/backend/base/SmacEcology.h"
#include <vector>
namespace smac_bloom_fixture {
namespace b=game::backend::base;
// Literal sparse inputs and observations from RE-SMAC-ECOLOGY-SELECTOR-TERRAIN-001.
struct Cell {int32_t x,y;b::SmacBloomTile value;};
struct Draw {int32_t raw;uint32_t after;};
struct Case {const char* id;b::SmacWorkerRadiusMap map;int32_t base,damage;bool perihelion;uint32_t seed;std::vector<b::SmacBloomBase> bases; b::SmacBloomTile default_tile;std::vector<Cell> cells;std::vector<Draw> draws;b::SmacBloomSelection selected;};
inline const std::vector<Case> cases{
{"published_Planet_neighborhood",{{64,16},80,80,true},0,51,false,65u,{
{0,{64,16}}},{3,false,false,0},{
{63,9,{3,true,false,0}},
{65,9,{2,true,false,0}},
{62,10,{3,true,false,0}},
{64,10,{3,true,false,0}},
{61,11,{3,true,false,0}},
{63,11,{4,false,false,0}},
{67,11,{3,true,false,0}},
{60,12,{2,false,false,0}},
{62,12,{3,true,false,0}},
{64,12,{4,false,false,0}},
{66,12,{4,true,false,0}},
{68,12,{4,false,false,0}},
{61,13,{3,true,false,0}},
{63,13,{4,true,false,0}},
{65,13,{4,true,false,0}},
{67,13,{4,false,false,0}},
{69,13,{4,false,false,0}},
{60,14,{3,false,false,0}},
{62,14,{4,false,false,0}},
{64,14,{5,true,false,0}},
{66,14,{5,false,false,0}},
{68,14,{5,false,false,0}},
{70,14,{4,false,false,0}},
{57,15,{3,true,false,0}},
{59,15,{4,false,false,0}},
{61,15,{4,false,false,0}},
{63,15,{5,true,false,0}},
{65,15,{5,false,false,0}},
{67,15,{5,false,false,0}},
{69,15,{4,false,false,0}},
{71,15,{3,true,false,0}},
{58,16,{4,true,false,0}},
{60,16,{4,false,false,0}},
{62,16,{4,true,false,0}},
{64,16,{5,false,true,0}},
{66,16,{5,false,false,0}},
{68,16,{5,false,false,0}},
{70,16,{4,true,false,0}},
{57,17,{3,true,false,0}},
{59,17,{4,false,false,0}},
{61,17,{5,true,false,0}},
{63,17,{5,true,false,0}},
{65,17,{5,false,false,0}},
{67,17,{5,false,false,0}},
{69,17,{4,false,false,0}},
{58,18,{4,true,false,0}},
{60,18,{5,true,false,0}},
{62,18,{5,false,false,0}},
{64,18,{5,false,false,0}},
{66,18,{4,true,false,0}},
{68,18,{5,false,false,0}},
{70,18,{4,false,false,0}},
{59,19,{4,true,false,0}},
{61,19,{5,false,false,0}},
{63,19,{4,false,false,0}},
{65,19,{4,true,false,0}},
{67,19,{4,true,false,0}},
{69,19,{4,false,false,0}},
{60,20,{4,true,false,0}},
{62,20,{4,true,false,0}},
{64,20,{4,true,false,0}},
{66,20,{4,false,false,0}},
{68,20,{4,true,false,0}},
{61,21,{4,true,false,0}},
{63,21,{3,true,false,0}},
{65,21,{3,true,false,0}},
{67,21,{4,false,false,0}},
{64,22,{3,true,false,0}},
{66,22,{4,false,false,0}},
{63,23,{3,true,false,0}},
},{{250,16441856u},{18237,1195243715u},{2761,2328461434u},{26456,1733854549u},{23942,3716588228u},{4890,320521847u},{14066,921888606u},{5852,2531057833u},{2359,2302140616u},{24204,3733724267u},{29007,4048532866u},{30964,2029258301u}},{{63,19},17,39}},
{"flat_corner_clipping",{{0,0},80,80,false},0,51,false,65u,{
{0,{0,0}}},{3,false,false,0},{
{0,0,{3,false,true,0}},
{1,1,{3,true,false,0}},
},{{250,16441856u},{18237,1195243715u},{2761,2328461434u},{26456,1733854549u}},{{2,0},2,27}},
{"cylinder_corner_wrap_and_clipping",{{0,0},80,80,true},0,51,false,65u,{
{0,{0,0}}},{3,false,false,0},{
{0,0,{3,false,true,0}},
{1,1,{3,true,false,0}},
{79,1,{3,true,false,0}},
},{{250,16441856u},{18237,1195243715u},{2761,2328461434u},{26456,1733854549u},{23942,3716588228u},{4890,320521847u}},{{2,0},2,27}},
{"outer_tie_later_peer_wins",{{64,16},80,80,true},0,51,false,65u,{
{0,{64,16}},{1,{66,16}}},{3,false,false,0},{
{64,16,{3,false,true,0}},
{66,16,{3,false,true,0}},
},{{250,16441856u},{18237,1195243715u}},{{62,20},36,7}},
{"outer_tie_later_current_wins",{{64,16},80,80,true},1,51,false,65u,{
{0,{66,16}},{1,{64,16}}},{3,false,false,0},{
{64,16,{3,false,true,0}},
{66,16,{3,false,true,0}},
},{{250,16441856u},{18237,1195243715u}},{{65,11},25,7}},
};
}
