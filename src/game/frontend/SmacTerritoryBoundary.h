#pragma once
#include "SmacTerrainRaster.h"
#include "SmacLandMaterial.h"
#include <functional>
#include <optional>
#include <string_view>

namespace game::frontend::smac_territory_boundary {
namespace raster=smac_terrain_raster;
using Mapping=std::array<uint8_t,10>;
struct Cell {uint8_t band,region;int owner;};
using CellAt=std::function<std::optional<Cell>(int,int)>;
struct Context {
    int viewer=0;
    std::array<uint32_t,8> relations{};
    bool visible=true,visibility_bypass=false,scenario_bypass=false,override_mode=false;
};
int Owner(int raw,const Context& context);
struct Pass {raster::Polygon polygon;std::array<int32_t,2> shade_limits;Mapping mapping;};
std::optional<Pass> Select(int x,int y,const std::array<raster::Point,5>& points,
    const CellAt& cells,const Context& context,uint32_t flags);
raster::Bitmap Mask(const raster::Bitmap& loaded_rainfall);
std::array<uint8_t,256> Row(const smac_land_material::Palette& colors,uint8_t target,int opacity);
raster::OwnerClut Rows(const smac_land_material::Palette& colors,const std::array<uint8_t,8>& targets,const std::array<int,8>& opacities);
std::array<int,8> ReadOpacities(std::string_view system);
}
