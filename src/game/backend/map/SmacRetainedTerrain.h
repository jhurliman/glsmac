#pragma once
#include "SmacPlanetMap.h"
#include "SmacSiteGeometry.h"

namespace game::backend::map::retained_terrain {
using Cell=SmacPlanetMap::Tile;
using Cells=std::vector<Cell>;
// Shared raw-record publication kernels. Results own copies; no source or live
// registry is modified. Container headers and generated metadata stay with the
// caller. All unrelated raw fields and ordered journal preconditions survive.
Cells WithLiveGlobalSpread(const Cells&,smac_site::Geometry,const smac_spread::Input&,const smac_spread::Result&);
Cells WithGlobalSpread(const Cells&,smac_site::Geometry,const smac_spread::Input&,const smac_spread::Result&);
Cells WithStartingBalance(const Cells&,smac_site::Geometry,const smac_balance::Result&);
Cells WithBloomTerrain(const Cells&,smac_site::Geometry,const base::SmacBloomTerrainPlan&);
smac_climate::World ExportClimateTerrain(const Cells&,smac_site::Geometry);
Cells WithClimateTerrain(const Cells&,smac_site::Geometry,const smac_climate::World&);
}
