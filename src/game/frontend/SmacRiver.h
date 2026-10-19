#pragma once
#include "SmacWaterCoast.h"

namespace game::frontend::smac_river {
// Explicit native display flags;0x400 enables River. Live items are independent
// of any remembered material fields. Caller supplies doubled-X topology.
std::vector<smac_water_coast::Pass> Select(int x,int y,
    const std::array<smac_terrain_raster::Point,5>& points,
    const std::array<int,5>& lighting,const smac_water_coast::CellAt& live,uint32_t flags);
}
