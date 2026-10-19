#pragma once
#include "SmacWaterCoast.h"
namespace game::frontend::smac_road_tube {
// Selected items are the viewer's remembered Road/Tube word, not live River
// items. Base admission belongs to the real matching record/visibility query.
struct Cell {uint8_t band;uint32_t selected_items;bool admitted_base;};
using CellAt=std::function<std::optional<Cell>(int,int)>;
std::vector<smac_water_coast::Pass> Select(int x,int y,
    const std::array<smac_terrain_raster::Point,5>& points,
    const std::array<int,5>& lighting,const CellAt& cells,uint32_t flags);
}
