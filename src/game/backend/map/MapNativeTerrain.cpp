#include "Map.h"
#include "SmacLandmarkGeometry.h"
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
        // The retained Planet path uses the same bounded authored negative-detail
        // mappings as the view renderer; rejecting them mixes legacy/native fans.
        auto geometry=std::make_unique<smac_terrain::LandmarkGeometry>(80,80,wrap,std::move(cells),false,true);
        m_native_geometry=std::move(geometry);m_native_geometry_source=source;m_native_geometry_wrap=wrap;
    }
    return m_native_geometry.get();
}
std::unique_ptr<::game::native_terrain::World> Map::CopyNativeTerrain(bool include_geometry) const {
    const auto* planet=m_resource_policy?m_resource_policy->OriginalPlanet():nullptr;
    if(!planet || GetWidth()!=80 || GetHeight()!=80 || !m_territory_topology) {return {};}
    auto result=std::make_unique<::game::native_terrain::World>();
    result->horizontal_wrap=m_territory_topology->Wraps();
    result->retained_geometry=true;
    result->resource_pattern=planet->ResourcePattern();
    result->automatic_land_resources=m_resource_policy->AutomaticLandResources();
    // The original console passes a NULL view: producing its material inputs
    // must not create, refresh or traverse the shared height/face-light cache.
    auto* geometry=include_geometry?GetNativeGeometry():nullptr;
    const auto band_at=[&](int x,int y)->std::optional<uint8_t> {
        if(y<0 || y>=80) {return std::nullopt;}
        if(result->horizontal_wrap) {x=(x%80+80)%80;}
        if(x<0 || x>=80) {return std::nullopt;}
        return planet->At(x,y).Climate()>>5;
    };
    const auto knowledge=m_worker_knowledge && m_worker_knowledge_ready?std::optional<SmacWorkerKnowledge::Data>{m_worker_knowledge->CopyData()}:std::nullopt;
    if(knowledge && (knowledge->width!=80 || knowledge->height!=80 || knowledge->tiles.size()!=3200 ||
        knowledge->horizontal_wrap!=result->horizontal_wrap)) {throw std::runtime_error("Native terrain snapshot has invalid knowledge geometry");}
    if(knowledge) {
        result->exploration_ready=true;result->revealed_by=knowledge->all_map_revealed;
        for(unsigned owner=1;owner<8;++owner) {if(knowledge->observers[owner]) {result->observers_present|=static_cast<uint8_t>(1u<<owner);}}
    }
    for(int y=0;y<80;++y) {for(int x=y&1;x<80;x+=2) {
        const auto& original=planet->At(x,y);
        const auto* live=GetTile(x,y);
        auto& output=result->tiles[static_cast<size_t>(y)*40+x/2];
        output.band=original.Climate()>>5;
        const auto index=static_cast<size_t>(y)*40+x/2;
        output.region=m_territory_topology->Regions().at(index);
        if(knowledge) {output.owner=static_cast<int8_t>(knowledge->tiles[index].claimant.value_or(-1));output.explored_by=knowledge->tiles[index].explored_by;}
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
        // Resource/site flags belong to the current retained native policy,
        // not explored improvement memory. Monolith presence is live terrain.
        output.resource_items=(original.Items()&~0x2000u)|((live->features&tile::FEATURE_MONOLITH)?0x2000u:0u);
        output.resource_ready=true;
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
        // Passive memberships add no independent terrain art. Preserve their
        // words: live Monolith items and the Borehole Cluster variant are
        // composed separately, rather than erased with the admission mask.
        // Geometry and named art retain their own bounds below.
        constexpr uint32_t passive_memberships=0x10u|0x20u|0x80u|0x1000u|0x4000u;
        const auto unsupported_landmark_materials=landmarks&~(0xff400000u|0xb03u|4u|8u|0x40u|0x400u|0x2000u|passive_memberships);
        const auto art_detail=landmarks>>24;
        const bool unsupported_art_detail=((landmarks&0x103u) && art_detail>=49 && art_detail<128) ||
            ((landmarks&0x80002000u)==0x2000u && art_detail>8);
        // Unsupported later improvements retain the existing GPU terrain.
        // Road/Tube now compose from the viewer's separately copied memory.
        // Pholus is height-only in this prefix. Its live adapter feature must
        // not veto the now-proven original profile; an unrelated port-only
        // ridge still lacks the retained source authority needed for admission.
        const auto unsupported_ridge=(landmarks&0x800u) && result->retained_geometry
            ?0u:static_cast<unsigned>(tile::FEATURE_FOSSIL_FIELD_RIDGE);
        const bool quad_geometry=output.band<3 || output.heights || result->retained_geometry;
        const auto unsupported_quads=((landmarks&2u) && quad_geometry?0u:static_cast<unsigned>(tile::FEATURE_VOLCANO)) |
            ((landmarks&1u) && quad_geometry?0u:static_cast<unsigned>(tile::FEATURE_GARLAND_CRATER));
        const auto unsupported_jungle=(landmarks&4u)?0u:static_cast<unsigned>(tile::FEATURE_JUNGLE);
        const auto unsupported_dunes=(landmarks&0x40u)?0u:static_cast<unsigned>(tile::FEATURE_DUNES);
        const auto unsupported_features=unsupported_jungle|unsupported_quads|unsupported_ridge|unsupported_dunes;
        const auto unsupported_work=live->terraforming&~((1u<<13)-1);
        output.material_admitted=!unsupported_art_detail && !unsupported_landmark_materials && !(live->features&unsupported_features) && !unsupported_work;
        output.ordinary=!output.deformed && output.material_admitted;
    }}
    return result;
}
}
