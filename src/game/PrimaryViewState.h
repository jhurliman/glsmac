#pragma once
#include <array>
#include <cstdint>
#include <stdexcept>

namespace game::ui_view {
using Cursor=std::array<int32_t,2>;
// Reopen reads the independent saved cursor, not a saved view camera or roster.
// A valid cursor is not normalized for tile parity or exploration.
inline Cursor ReopenCamera(Cursor cursor,Cursor world) {
    if(world[0]<=0 || world[1]<=0) {throw std::invalid_argument("Invalid primary view world dimensions");}
    if(cursor[0]>=0 && cursor[0]<world[0] && cursor[1]>=0 && cursor[1]<world[1]) {return cursor;}
    Cursor center{{world[0]/2,world[1]/2}};center[0]^=(center[0]^center[1])&1;return center;
}
}
