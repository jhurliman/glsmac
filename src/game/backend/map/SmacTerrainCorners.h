#pragma once

namespace game::backend::map::smac_terrain {
// W,N,E,S shared-vertex contributors in native incident-face order.
inline constexpr int CORNERS[4][4][2] = {
    {{-2,0},{-1,-1},{0,0},{-1,1}},
    {{-1,-1},{0,-2},{1,-1},{0,0}},
    {{0,0},{1,-1},{2,0},{1,1}},
    {{-1,1},{0,0},{1,1},{0,2}}
};
}
