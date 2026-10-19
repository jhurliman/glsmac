#pragma once
#include "game/NativeTerrainWorld.h"
#include "SmacTerrainRaster.h"
#include "SmacLandMaterial.h"
#include <memory>

namespace game::frontend::native_terrain {
using Point=smac_terrain_raster::Point;
struct Placement {Point tile,flat_center;uint32_t record_offset;};
// Original80x80/1024x511 ordinary zoom0 full redraw, including padded rows.
std::vector<Placement> View(int camera_x,int camera_y,bool horizontal_wrap);
struct Source {
    smac_terrain_raster::Bitmap loaded;
    smac_land_material::Palette palette;
};
struct Frame {
    std::vector<uint8_t> rgba;
    uint64_t attempts=0,writes=0;
    size_t tiles=0;
};
// Explicit reference allocation-base low16; never a coordinate hash or a
// gameplay RNG stream. Offset translates a visible copy without changing UVs.
Frame Render(const ::game::native_terrain::World& world,const Source& source,
    const std::vector<Placement>& placements,uint16_t variation_base_low16,Point offset={{0,0}});
}
