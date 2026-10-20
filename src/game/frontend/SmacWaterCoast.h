#pragma once
#include "SmacTerrainRaster.h"
#include <functional>
#include <optional>

namespace game::frontend::smac_water_coast {
namespace raster=smac_terrain_raster;
struct Cell {uint8_t band,contour;uint32_t items=0;};
// Caller owns doubled-X topology; missing cells remain missing, not sea.
using CellAt=std::function<std::optional<Cell>(int,int)>;
using Mapping=std::array<uint8_t,53>;
uint8_t Sample(int x,int y,unsigned sector,unsigned mode,const CellAt& at);
std::array<uint8_t,4> CoastPatterns(int x,int y,const CellAt& at);
Mapping CoastMapping(unsigned pattern,int x,int y);
struct Pass {
    std::array<int,4> crop;
    raster::Polygon polygon;
    raster::Shader shader;
    std::array<int32_t,2> shade_limits;
    Mapping mapping{};
};
// Sea: overlay/base pairs in W,N,E,S order. Land: coast continuation only,
// appended after the separately measured land-material/feature prefix.
// Connectivity reads live neighbor items; only center admission uses the
// separately selected/remembered word when supplied.
std::vector<Pass> Select(int x,int y,const std::array<raster::Point,5>& points,const CellAt& at,
    std::optional<uint32_t> center_items=std::nullopt);
// Loaded (+10) RAINFALL.PCX. Reconstructs both original56x56 class masks;
// ordinary coast painting uses mask0. No palette/RGBA approximation.
std::array<raster::Bitmap,2> CoastMasks(const raster::Bitmap& rainfall);
}
