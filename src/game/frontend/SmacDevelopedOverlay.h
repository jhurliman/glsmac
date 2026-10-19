#pragma once
#include "SmacTerrainRaster.h"
#include "SmacLandMaterial.h"

namespace game::frontend::smac_developed_overlay {
using Crop=smac_land_material::Crop;
using Palette=smac_land_material::Palette;
struct Selection {Crop crop;bool shadow=false;};
// Caller resolves nutrients from live faction/tile state. Admission uses the
// viewer-selected word, while Borehole variant selection uses the live word.
std::vector<Selection> Select(uint32_t selected,uint8_t band,int64_t nutrients,
    uint32_t live_landmarks,uint32_t flags=0x21,int zoom=0);
struct Sprite {smac_terrain_raster::Bitmap bitmap;smac_terrain_raster::Point offset;};
// Input already uses the original loaded index rebase (+10 modulo256).
Sprite Decode(const smac_terrain_raster::Bitmap& atlas,const Crop& crop);
std::array<uint8_t,256> ShadowLookup(const Palette& active_palette);
}
