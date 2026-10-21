#pragma once
#include "SmacTerrainRaster.h"
#include "SmacLandMaterial.h"
#include <string_view>

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
bool ReplacesLegacySprite(std::string_view name);
enum class ShadowPrecision {Binary64,Extended64};
// Native initialization inherits x87 precision. Keep both measured profiles;
// the ordinary initialized-strip caller currently declares027f (binary64).
std::array<uint8_t,256> ShadowLookup(const Palette& active_palette,ShadowPrecision precision=ShadowPrecision::Binary64);
}
