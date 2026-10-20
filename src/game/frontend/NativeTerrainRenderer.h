#pragma once
#include "game/NativeTerrainWorld.h"
#include "NativeTerrainView.h"
#include "NativeViewGeometry.h"
#include "SmacTerrainRaster.h"
#include "SmacLandMaterial.h"
#include "SmacTerritoryBoundary.h"
#include <memory>

namespace game::frontend::native_terrain {
// Original80x80/1024x511 ordinary zoom0 full redraw, including padded rows.
std::vector<Placement> View(int camera_x,int camera_y,bool horizontal_wrap);
struct Source {
    smac_terrain_raster::Bitmap loaded;
    smac_terrain_raster::Bitmap ter1{};
    std::array<uint8_t,256> shadow_lookup{};
    smac_land_material::Palette palette;
    smac_terrain_raster::Bitmap coast_mask{},boundary_mask{};
    smac_terrain_raster::OwnerClut owner_clut{};
    std::array<std::string,8> faction_files{};
};
struct Frame {
    std::vector<uint8_t> rgba,indexed,coverage;
    uint64_t attempts=0,writes=0;
    size_t tiles=0;
    bool opaque_background=false;
    std::array<bool,3200> developed_tiles{},landmark_tiles{},resource_tiles{};
};
// Explicit reference allocation-base low16; never a coordinate hash or a
// gameplay RNG stream. Offset translates a visible copy without changing UVs.
// Initial indexed backdrop is explicit for controlled replay; coverage still
// identifies submitted stores rather than making the initial plane opaque.
Frame Render(const ::game::native_terrain::World& world,const Source& source,
    const std::vector<Placement>& placements,uint16_t variation_base_low16,Point offset={{0,0}},uint32_t display_flags=0x10421,uint8_t initial_index=0);
// Render an owned view schedule. Projector anchors are consumed directly;
// they must not be reconstructed from later height-cache observations.
Frame RenderPrepared(const ::game::native_terrain::World& world,const Source& source,
    const ViewGeometry::Plan& plan,uint16_t variation_base_low16,
    uint32_t display_flags=0x10421,uint8_t initial_index=0,const Frame* previous=nullptr);
}
