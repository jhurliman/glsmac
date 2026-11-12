#include "Map.h"
#include "game/backend/Game.h"
#include "SmacTerrainAuthority.h"
#include "SmacClimate.h"
#include "SmacNativeTerrainItems.h"
#include "SmacGlobalSpread.h"
#include "SmacLandmarkGeometry.h"
#include "game/backend/base/SmacEcology.h"
#include "tile/Tile.h"
#include "util/FinallyGuard.h"
#include <stdexcept>

namespace game::backend::map {
base::SmacBloomTerrainCell Map::CopyEcologyBloomCell(std::array<int32_t,2> point,const SmacWorkerKnowledge::Data& knowledge) const {
    auto raw=CopyGlobalWildlifeTile(point,knowledge);
    const auto* live=GetTile(point[0],point[1]);
    if(live->rockiness<tile::ROCKINESS_FLAT || live->rockiness>tile::ROCKINESS_ROCKY) {
        throw std::runtime_error("Ecology requires current ordinary native rockiness");
    }
    const auto material=NativeTerrainItems(live->terraforming,live->features)
        |((live->features&tile::FEATURE_MONOLITH)?0x2000u:0u);
    return {int32_t(raw.Climate()>>5),int32_t(live->rockiness-tile::ROCKINESS_FLAT),
        (raw.Items()&~smac_spread::LIVE_MATERIAL_MASK)|material,raw.Landmarks(),raw.CachedOccupant()};
}
struct Map::PreparedEcologyBloom::Data {
    Map* owner;
    GlsmacResourceMapPolicy original_policy,policy;
    SmacWorkerKnowledge original_knowledge,knowledge;
    GlsmacTerritoryTopology original_topology,topology;
    std::vector<base::SmacBloomTerrainDelta> preimages;
    TerrainAppearances original_appearance,appearance;
    std::unique_ptr<PreparedTerrainUpdate> render;
    bool committed=false;
};
Map::PreparedEcologyBloom::PreparedEcologyBloom()=default;
Map::PreparedEcologyBloom::~PreparedEcologyBloom()=default;
std::unique_ptr<Map::PreparedEcologyBloom> Map::PrepareEcologyBloom(const base::SmacBloomTerrainPlan& plan,std::optional<uint8_t> viewer,std::optional<int32_t> climate_extra) {
    m_game->RequireWorkerKnowledgeMutation();
    if(m_global_spread_query_active || !m_resource_policy || !m_resource_policy->NativeTerrainSource()
        || !m_worker_knowledge || !m_worker_knowledge_ready || !m_territory_topology || !m_territory_topology->SiteRatings()) {
        throw std::runtime_error("Ecology terrain requires current native authority, knowledge and site history");
    }
    auto knowledge=m_worker_knowledge->CopyData();
    auto policy=m_resource_policy->WithBloomTerrain(plan,viewer);
    if(climate_extra) {
        const auto native=policy.NativeTerrainSource();
        policy=policy.WithClimateTerrain(native->ExportClimateTerrain(),native->Info().sea_offset,*climate_extra);
    }
    auto topology=*m_territory_topology;
    TerrainAppearances old,appearance;
    constexpr uint16_t removed=tile::TERRAFORMING_MINE|tile::TERRAFORMING_SOLAR|tile::TERRAFORMING_FARM
        |tile::TERRAFORMING_SOIL_ENRICHER|tile::TERRAFORMING_FOREST;
    constexpr uint16_t landmark_features=tile::FEATURE_JUNGLE|tile::FEATURE_URANIUM|tile::FEATURE_GEOTHERMAL
        |tile::FEATURE_VOLCANO|tile::FEATURE_GARLAND_CRATER|tile::FEATURE_FOSSIL_FIELD_RIDGE;
    for(const auto& delta:plan.Changes()) {
        if(!(CopyEcologyBloomCell(delta.coordinate,knowledge)==delta.before)) {throw std::runtime_error("Ecology terrain live preimage is stale");}
        const auto& next=delta.after;auto* live=GetTile(delta.coordinate[0],delta.coordinate[1]);
        auto features=uint16_t(live->features&~(tile::FEATURE_XENOFUNGUS|landmark_features));
        if(next.items&0x20u) {features|=tile::FEATURE_XENOFUNGUS;}
        if(!(next.landmarks&0x80000000u)) {
            for(const auto& bit:std::array<std::pair<uint32_t,uint16_t>,6>{{{4,tile::FEATURE_JUNGLE},{8,tile::FEATURE_URANIUM},
                {0x400,tile::FEATURE_GEOTHERMAL},{2,tile::FEATURE_VOLCANO},{1,tile::FEATURE_GARLAND_CRATER},{0x800,tile::FEATURE_FOSSIL_FIELD_RIDGE}}}) {
                if(next.landmarks&bit.first) {features|=bit.second;}
            }
        }
        auto work=uint16_t(live->terraforming&~removed);
        for(const auto& bit:std::array<std::pair<uint32_t,uint16_t>,5>{{{0x10,tile::TERRAFORMING_MINE},{0x40,tile::TERRAFORMING_SOLAR},
            {0x8000,tile::TERRAFORMING_FARM},{0x80000,tile::TERRAFORMING_SOIL_ENRICHER},{0x200000,tile::TERRAFORMING_FOREST}}}) {
            if(next.items&bit.first) {work|=bit.second;}
        }
        old.emplace(live,TerrainAppearance{live->features,live->terraforming,live->rockiness});
        appearance.emplace(live,TerrainAppearance{features,work,static_cast<tile::rockiness_t>(next.rockiness+tile::ROCKINESS_FLAT)});
        topology=topology.WithSiteRatings(topology.SiteRatings()->InvalidateLocal(delta.coordinate));
    }
    if(viewer && plan.FungusCell()) {
        const auto point=*plan.FungusCell();const auto* tile=GetTile(point[0],point[1]);
        auto& row=knowledge.remembered.at(*viewer);if(row.empty()) {row.resize(knowledge.tiles.size());}
        const auto next=appearance.at(tile);const auto before=CopyEcologyBloomCell(point,m_worker_knowledge->CopyData());
        row.at(size_t(point[1])*(GetWidth()/2)+point[0]/2)=SmacWorkerKnowledge::Improvements{
            next.features,next.terraforming,bool(before.items&1u),bool(before.items&2u)};
    }
    auto remembered=SmacWorkerKnowledge::Create(knowledge);
    auto render=appearance.empty()?std::unique_ptr<PreparedTerrainUpdate>{}:PrepareTerrainAppearanceUpdate(appearance);
    auto prepared=std::unique_ptr<PreparedEcologyBloom>(new PreparedEcologyBloom);
    prepared->data=std::make_unique<PreparedEcologyBloom::Data>(PreparedEcologyBloom::Data{
        this,*m_resource_policy,std::move(policy),*m_worker_knowledge,std::move(remembered),*m_territory_topology,std::move(topology),
        plan.Changes(),std::move(old),std::move(appearance),std::move(render),false});
    return prepared;
}
const TerrainRenderUpdate* Map::GetEcologyBloomTerrainPayload(const PreparedEcologyBloom& prepared) const {
    if(!prepared.data || prepared.data->owner!=this) {throw std::invalid_argument("Foreign ecology terrain transaction");}
    return prepared.data->render?&GetTerrainUpdatePayload(*prepared.data->render):nullptr;
}
void Map::ValidateEcologyBloom(const PreparedEcologyBloom& prepared) const {
    if(!prepared.data || prepared.data->owner!=this || prepared.data->committed || m_global_spread_query_active) {
        throw std::invalid_argument("Invalid ecology terrain transaction");
    }
    const auto& d=*prepared.data;
    if(!m_resource_policy || !m_worker_knowledge || !m_territory_topology || !(*m_resource_policy==d.original_policy)
        || !(*m_worker_knowledge==d.original_knowledge) || !(*m_territory_topology==d.original_topology)) {
        throw std::runtime_error("Stale ecology terrain policy, knowledge or topology");
    }
    const auto knowledge=m_worker_knowledge->CopyData();
    for(const auto& delta:d.preimages) {
        if(!(CopyEcologyBloomCell(delta.coordinate,knowledge)==delta.before)) {throw std::runtime_error("Stale ecology terrain live preimage");}
    }
}
void Map::CommitEcologyBloom(PreparedEcologyBloom& prepared) {
    ValidateEcologyBloom(prepared);auto& d=*prepared.data;
    static_assert(noexcept(*m_resource_policy=std::move(d.policy)) && noexcept(*m_worker_knowledge=std::move(d.knowledge))
        && noexcept(*m_territory_topology=std::move(d.topology)));
    for(const auto& [tile,next]:d.appearance) {
        auto* live=GetTile(tile->coord.x,tile->coord.y);live->features=next.features;live->terraforming=next.terraforming;live->rockiness=next.rockiness;
    }
    *m_resource_policy=std::move(d.policy);*m_worker_knowledge=std::move(d.knowledge);*m_territory_topology=std::move(d.topology);
    m_native_geometry.reset();m_native_geometry_source.reset();InvalidateRegionalPathWorkspace();
    if(d.render) {CommitOrdinaryTerrainUpdate(*d.render);}d.committed=true;
}
void Map::QueryEcologyBloom(const PreparedEcologyBloom& prepared,const std::function<void()>& query) {
    ValidateEcologyBloom(prepared);if(!query) {throw std::invalid_argument("Ecology terrain query requires a consumer");}
    const auto& d=*prepared.data;
    auto policy=std::optional<GlsmacResourceMapPolicy>{d.policy};auto knowledge=std::optional<SmacWorkerKnowledge>{d.knowledge};
    auto topology=std::optional<GlsmacTerritoryTopology>{d.topology};
    std::unique_ptr<smac_terrain::LandmarkGeometry> geometry;std::shared_ptr<const SmacTerrainAuthority> source;
    const auto wrap=m_native_geometry_wrap;bool applied=false;
    util::FinallyGuard restore([&]{
        if(!applied) {return;}
        for(const auto& [tile,old]:d.original_appearance) {auto* live=GetTile(tile->coord.x,tile->coord.y);
            live->features=old.features;live->terraforming=old.terraforming;live->rockiness=old.rockiness;}
        m_resource_policy.swap(policy);m_worker_knowledge.swap(knowledge);m_territory_topology.swap(topology);
        m_native_geometry=std::move(geometry);m_native_geometry_source=std::move(source);m_native_geometry_wrap=wrap;
        m_global_spread_query_active=false;
    });
    applied=true;m_global_spread_query_active=true;geometry=std::move(m_native_geometry);source=std::move(m_native_geometry_source);
    m_resource_policy.swap(policy);m_worker_knowledge.swap(knowledge);m_territory_topology.swap(topology);
    for(const auto& [tile,next]:d.appearance) {auto* live=GetTile(tile->coord.x,tile->coord.y);
        live->features=next.features;live->terraforming=next.terraforming;live->rockiness=next.rockiness;}
    query();
}
}
