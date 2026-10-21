#pragma once
#include <vector>
#include <utility>
namespace gse::tests::retirement_history_fixture {
// Eight native clear/append/lookup controls, including all 4096 history bytes and all eight cursors.
// Verified manifest ce73e8db32e0eeeb0e420d10b644ddd70be4603947e74edce61718d62d98d7ee.
struct Frame {int kind,owner,start,step;bool human;std::array<uint8_t,8> tuple;std::array<int32_t,8> before_cursor,after_cursor;std::vector<std::pair<size_t,uint8_t>> patches;uint64_t after;bool found;};
inline const Frame FRAMES[]{
{0,1,19,37,true,{64,48,32,16,0,1,2,3},{0,17,0,0,0,0,0,0},{0,17,0,0,0,0,0,0},{},UINT64_C(0x23d37a2fe8bac125),false},
{0,7,19,37,true,{64,48,32,16,0,1,2,3},{0,17,0,0,0,0,0,63},{0,17,0,0,0,0,0,63},{},UINT64_C(0xd676d9c5dd1ac125),false},
{1,1,19,37,true,{64,48,32,16,0,1,2,3},{0,0,0,0,0,0,0,63},{0,1,0,0,0,0,0,63},{},UINT64_C(0x4aa0b59930d30961),false},
{1,1,19,37,true,{64,48,32,16,0,1,2,3},{0,63,0,0,0,0,0,63},{0,0,0,0,0,0,0,63},{},UINT64_C(0x1f265626f7b4f301),false},
{1,7,19,37,true,{64,48,32,16,0,1,2,3},{0,0,0,0,0,0,0,63},{0,0,0,0,0,0,0,0},{},UINT64_C(0xc118709791c73301),false},
{2,1,255,0,true,{64,48,32,16,0,1,2,3},{0,42,0,0,0,0,0,0},{0,42,0,0,0,0,0,0},{{536,64},{537,48},{538,32},{539,16},{540,0},{541,1},{542,2},{543,3}},UINT64_C(0x854ec64184db9429),true},
{2,1,255,0,false,{64,48,32,16,0,1,2,3},{0,42,0,0,0,0,0,0},{0,42,0,0,0,0,0,0},{{536,64},{537,48},{538,32},{539,16},{540,0},{541,1},{542,2},{543,3}},UINT64_C(0x854ec64184db9429),false},
{2,1,255,0,true,{64,48,32,16,0,1,2,4},{0,42,0,0,0,0,0,0},{0,42,0,0,0,0,0,0},{{536,64},{537,48},{538,32},{539,16},{540,0},{541,1},{542,2},{543,3}},UINT64_C(0x854ec64184db9429),false},
};
}
