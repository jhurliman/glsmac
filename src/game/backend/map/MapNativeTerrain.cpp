#include "Map.h"
#include "game/NativeTerrainWorld.h"
#include "SmacPlanetMap.h"
#include "SmacNativeTerrainItems.h"
#include "tile/Tile.h"

namespace game::backend::map {
std::unique_ptr<::game::native_terrain::World> Map::CopyNativeTerrain() const {
    const auto* planet=m_resource_policy?m_resource_policy->OriginalPlanet():nullptr;
    if(!planet || GetWidth()!=80 || GetHeight()!=80 || !m_territory_topology) {return {};}
    auto result=std::make_unique<::game::native_terrain::World>();
    result->horizontal_wrap=m_territory_topology->Wraps();
    const auto knowledge=m_worker_knowledge && m_worker_knowledge_ready?std::optional<SmacWorkerKnowledge::Data>{m_worker_knowledge->CopyData()}:std::nullopt;
    if(knowledge && knowledge->tiles.size()!=3200) {throw std::runtime_error("Native territory snapshot has invalid claim geometry");}
    for(int y=0;y<80;++y) {for(int x=y&1;x<80;x+=2) {
        const auto& original=planet->At(x,y);
        const auto* live=GetTile(x,y);
        auto& output=result->tiles[static_cast<size_t>(y)*40+x/2];
        output.band=original.Climate()>>5;
        const auto index=static_cast<size_t>(y)*40+x/2;
        output.region=m_territory_topology->Regions().at(index);
        if(knowledge) {output.owner=static_cast<int8_t>(knowledge->tiles[index].claimant.value_or(-1));}
        if(knowledge && knowledge->occupancy) {
            const auto& cell=knowledge->occupancy->At({x,y});
            output.center_known=true;output.base_present=bool(cell.presence&1u);output.cached_affiliation=cell.affiliation;
        }
        output.contour=original.Contour();
        output.moisture=live->moisture?live->moisture-1:0;
        output.rockiness=live->rockiness?live->rockiness-1:0;
        output.items=NativeTerrainItems(live->terraforming,live->features);
        output.items|=original.Items()&0x200u; // retained raw extra-River conditional
        output.landmarks=original.Landmarks();
        // These landmark-specific geometric branches still need translation.
        const auto landmarks=original.Landmarks();
        // Only these low memberships select land height/shading profiles.
        // The upper byte is shared detail;0x400000 is dirty bookkeeping.
        // Negative detail suppresses extra shade, not the height profiles.
        output.deformed=output.band>=3 && (landmarks&0xb03u);
        // Keep unimplemented material memberships separate from geometry.
        // Preserve the prior Jungle/Uranium/Geothermal admission here; live
        // feature gates below still reject unsupported special land paint.
        // Geometry-only0x200/0x800 have no sea profile. Memberships
        //1/2/0x100 also select special art and stay excluded on sea.
        const uint32_t sea_memberships=output.band<3?0xd0u:0u;
        const auto unsupported_landmark_materials=landmarks&~(0xff400000u|0xa00u|4u|8u|0x400u|sea_memberships);
        // Unsupported later improvements retain the existing GPU terrain.
        // Road/Tube now compose from the viewer's separately copied memory.
        const auto unsupported_features=tile::FEATURE_JUNGLE|tile::FEATURE_VOLCANO|
            tile::FEATURE_GARLAND_CRATER|tile::FEATURE_FOSSIL_FIELD_RIDGE|tile::FEATURE_DUNES;
        const auto unsupported_work=live->terraforming&~((1u<<13)-1);
        output.ordinary=!output.deformed && !unsupported_landmark_materials && !(live->features&unsupported_features) && !unsupported_work;
    }}
    return result;
}
}
