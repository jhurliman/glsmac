#pragma once
#include "NativeTerrainRenderer.h"

namespace game::frontend::smac_tile_preview {
// Independent NULL-view land paint at the measured console85/100 override.
// No geometry/light cache is read or mutated. Unknown memory, sea/coast and
// unmeasured feature profiles remain unavailable, rather than leaking live art.
std::optional<native_terrain::Frame> Paint(const ::game::native_terrain::World& world,
    const native_terrain::Source& source,int x,int y,uint16_t variation_base_low16,
    const smac_terrain_raster::Bitmap& backing,
    smac_terrain_raster::Point anchor={{2,1}},
    smac_terrain_raster::Clip clip={{0,0,89,117}});
}
