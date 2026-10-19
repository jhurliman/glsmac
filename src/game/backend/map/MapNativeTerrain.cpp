#include "Map.h"
#include "game/NativeTerrainWorld.h"
#include "SmacPlanetMap.h"
#include "SmacNativeTerrainItems.h"
#include "SmacTerrainLighting.h"
#include "tile/Tile.h"

namespace game::backend::map {
smac_terrain::LandmarkGeometry* Map::GetNativeGeometry() const {
    const auto source=m_resource_policy?m_resource_policy->OriginalPlanetSource():nullptr;
    if(!source) {
        m_native_geometry.reset();m_native_geometry_source.reset();return nullptr;
    }
    const bool wrap=m_territory_topology?m_territory_topology->Wraps():true;
    if(!m_native_geometry || source!=m_native_geometry_source || wrap!=m_native_geometry_wrap) {
        std::vector<smac_terrain::LandmarkGeometry::Cell> cells;
        cells.reserve(3200);
        for(int y=0;y<80;++y) {for(int x=y&1;x<80;x+=2) {
            const auto& tile=source->At(x,y);
            cells.push_back({uint8_t(tile.Climate()>>5),tile.Landmarks()});
        }}
        auto geometry=std::make_unique<smac_terrain::LandmarkGeometry>(80,80,wrap,std::move(cells));
        m_native_geometry=std::move(geometry);m_native_geometry_source=source;m_native_geometry_wrap=wrap;
    }
    return m_native_geometry.get();
}
std::unique_ptr<::game::native_terrain::World> Map::CopyNativeTerrain() const {
    const auto* planet=m_resource_policy?m_resource_policy->OriginalPlanet():nullptr;
    if(!planet || GetWidth()!=80 || GetHeight()!=80 || !m_territory_topology) {return {};}
    auto result=std::make_unique<::game::native_terrain::World>();
    result->horizontal_wrap=m_territory_topology->Wraps();
    auto* geometry=GetNativeGeometry();
    const auto band_at=[&](int x,int y)->std::optional<uint8_t> {
        if(y<0 || y>=80) {return std::nullopt;}
        if(result->horizontal_wrap) {x=(x%80+80)%80;}
        if(x<0 || x>=80) {return std::nullopt;}
        return planet->At(x,y).Climate()>>5;
    };
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
        // Retained landmark memberships select the shared native height profiles.
        const auto landmarks=original.Landmarks();
        // Only these low memberships select land height/shading profiles.
        // The upper byte is shared detail;0x400000 is dirty bookkeeping.
        // Negative detail suppresses extra shade, not the height profiles.
        output.deformed=output.band>=3 && (landmarks&0xb03u);
        if(geometry && output.band>=3) {
            std::array<int,5> heights{};bool valid=true;
            for(unsigned v=0;v<5;++v) {
                const auto height=geometry->Height(x,y,v);
                if(!height) {valid=false;break;}heights[v]=*height;
            }
            auto light=smac_terrain::OrdinaryLighting(x,y,band_at);
            if(valid && light) {
                for(unsigned v=0;v<5;++v) {
                    const auto extra=geometry->ExtraShade(x,y,v);
                    if(!extra) {valid=false;break;}(*light)[v]+=*extra;
                }
                if(valid) {output.heights=heights;output.lighting=*light;output.deformed=false;}
            }
        }
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
