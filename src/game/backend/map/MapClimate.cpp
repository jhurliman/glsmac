#include "Map.h"
#include "game/backend/Game.h"
#include "gc/Space.h"
#include "SmacClimate.h"
#include "SmacLandmarkGeometry.h"
#include "SmacTerrainAuthority.h"
#include "SmacRetainedTerrain.h"
#include "SmacGlobalSpread.h"
#include "SmacGenerationRegions.h"
#include "SmacFinalGeneration.h"
#include "tile/Tile.h"
#include "util/FinallyGuard.h"
#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace game::backend::map {
namespace {
struct Material {uint16_t features,work;};
Material Decode(uint32_t items,uint32_t landmarks) {
    Material out{};
    for(const auto& bit:std::array<std::pair<uint32_t,uint16_t>,3>{{{0x80,tile::FEATURE_RIVER},{0x2000,tile::FEATURE_MONOLITH},{0x20,tile::FEATURE_XENOFUNGUS}}}) {
        if(items&bit.first) {out.features|=bit.second;}
    }
    if(!(landmarks&0x80000000u)) {
        for(const auto& bit:std::array<std::pair<uint32_t,uint16_t>,6>{{{4,tile::FEATURE_JUNGLE},{8,tile::FEATURE_URANIUM},{0x400,tile::FEATURE_GEOTHERMAL},
            {2,tile::FEATURE_VOLCANO},{1,tile::FEATURE_GARLAND_CRATER},{0x800,tile::FEATURE_FOSSIL_FIELD_RIDGE}}}) {if(landmarks&bit.first) {out.features|=bit.second;}}
    }
    for(const auto& bit:std::array<std::pair<uint32_t,uint16_t>,13>{{{4,tile::TERRAFORMING_ROAD},{8,tile::TERRAFORMING_MAG_TUBE},{0x10,tile::TERRAFORMING_MINE},
        {0x40,tile::TERRAFORMING_SOLAR},{0x8000,tile::TERRAFORMING_FARM},{0x80000,tile::TERRAFORMING_SOIL_ENRICHER},{0x200000,tile::TERRAFORMING_FOREST},
        {0x400000,tile::TERRAFORMING_CONDENSER},{0x800000,tile::TERRAFORMING_MIRROR},{0x1000000,tile::TERRAFORMING_BOREHOLE},
        {0x80000000u,tile::TERRAFORMING_SENSOR},{0x800,tile::TERRAFORMING_BUNKER},{0x40000,tile::TERRAFORMING_AIRBASE}}}) {if(items&bit.first) {out.work|=bit.second;}}
    return out;
}
struct Physical {
    tile::Tile* tile;
    uint16_t moisture,rockiness,bonus,features,work;
    bool water;
    void Apply() const noexcept {
        tile->moisture=moisture;tile->rockiness=rockiness;tile->bonus=bonus;tile->features=features;tile->terraforming=work;tile->is_water_tile=water;
    }
};
Physical Capture(tile::Tile* t) {return {t,t->moisture,t->rockiness,t->bonus,t->features,t->terraforming,t->is_water_tile};}
}
struct Map::PreparedClimateTerrain::Data {
    Map* owner;
    GlsmacResourceMapPolicy original_policy,policy;
    SmacWorkerKnowledge original_knowledge,knowledge;
    GlsmacTerritoryTopology original_topology,topology;
    std::vector<std::string> tile_preimages;
    std::vector<Physical> before,after;
    std::map<tile::elevation_t*,tile::elevation_t> old_heights,heights;
    bool committed=false,territory_complete=false,validated=false;
    TerrainRenderUpdate render;
};
Map::PreparedClimateTerrain::PreparedClimateTerrain()=default;
Map::PreparedClimateTerrain::~PreparedClimateTerrain()=default;
smac_climate::World Map::CopyClimateTerrain(int32_t turn,const std::array<uint32_t,8>& status) const {
    // This existing live projection merges current work/features, occupancy and
    // remembered material into retained native words without revealing terrain.
    const auto input=CopyGlobalSpreadInput(turn,status);
    return retained_terrain::ExportClimateTerrain(input.tiles,input.geometry);
}
std::unique_ptr<Map::PreparedClimateTerrain> Map::PrepareClimateTerrain(const smac_climate::World& world,int32_t offset,int32_t extra,std::optional<SmacWorkerKnowledge> entities) {
    if(!m_game->GetGCSpace()->IsAccumulating() || m_global_spread_query_active || !m_resource_policy || !m_resource_policy->NativeTerrainSource()
        || !m_worker_knowledge || !m_worker_knowledge_ready || !m_territory_topology || !m_territory_topology->Areas() || !m_territory_topology->SiteRatings()) {
        throw std::runtime_error("Climate terrain requires its owning live native world");
    }
    auto policy=m_resource_policy->WithClimateTerrain(world,offset,extra);
    const auto source=policy.NativeTerrainSource();const auto width=int32_t(GetWidth()),height=int32_t(GetHeight());
    smac_generation::State regional{std::vector<smac_generation::Record>(world.tiles.size()),{},SmacGameplayRandomState(0,0)};
    regional.topology=world.horizontal_wrap?0:1;
    for(int32_t y=0;y<height;++y) {for(int32_t x=y&1;x<width;x+=2) {regional.records[size_t(y)*(width/2)+x/2]=source->At(x,y).bytes;}}
    smac_generation::Regions regions;regions.summary_initial=0;
    for(size_t r=0;r<128;++r) {for(unsigned k=0;k<4;++k) {regions.records[r*28+k]=uint8_t(uint32_t(m_territory_topology->Areas()->at(r))>>(8*k));}}
    smac_generation::RebuildRegions(regional,{width,height},regions);
    smac_generation::ClimatePostpass(regional,{width,height},regions);
    policy=policy.WithClimatePostpass(regional);
    std::vector<bool> sea,fertile;std::vector<uint8_t> details;
    sea.reserve(world.tiles.size());fertile.reserve(world.tiles.size());details.reserve(world.tiles.size());
    for(const auto& t:world.tiles) {const bool water=t.elevation<3;sea.push_back(water);
        fertile.push_back(!water && t.moisture>0 && t.rockiness<2 && !(t.items&0x20u));}
    for(const auto& t:regional.records) {details.push_back(t[15]);}
    auto topology=m_territory_topology->Rebuild(sea).WithTerrainPostpass(sea,fertile).WithLandmarkDetails(std::move(details));
    for(size_t i=0;i<regional.records.size();++i) {if(topology.Regions().at(i)!=regional.records[i][3]) {throw std::logic_error("Climate region producers disagree");}}
    auto knowledge=entities?entities->CopyData():m_worker_knowledge->CopyData();
    if(knowledge.width!=width || knowledge.height!=height || knowledge.horizontal_wrap!=world.horizontal_wrap) {
        throw std::invalid_argument("Climate candidate entity knowledge geometry differs");
    }
    const auto retained_before=m_resource_policy->NativeTerrainSource()->ExportClimateTerrain();
    auto prepared=std::unique_ptr<PreparedClimateTerrain>(new PreparedClimateTerrain);
    prepared->data=std::make_unique<PreparedClimateTerrain::Data>(PreparedClimateTerrain::Data{
        this,*m_resource_policy,std::move(policy),*m_worker_knowledge,*m_worker_knowledge,*m_territory_topology,std::move(topology),{},{},{},{},{},false,false,false});
    auto& d=*prepared->data;d.render.invalidate_native_geometry=true;
    const auto height_at=[&](int x,int y){
        y=std::clamp(y,0,height-1);if(world.horizontal_wrap) {x=(x%width+width)%width;}else {x=std::clamp(x,0,width-1);}
        if((x&1)!=(y&1)) {x=x+1<width?x+1:x-1;}
        return (world.At(x,y)->contour-60)*50+25;
    };
    const auto corner=[&](int x,int y,int dx,int dy){
        if(dx) {return (height_at(x,y)+height_at(x+2*dx,y)+height_at(x+dx,y-1)+height_at(x+dx,y+1))/4;}
        return (height_at(x,y)+height_at(x,y+2*dy)+height_at(x-1,y+dy)+height_at(x+1,y+dy))/4;
    };
    std::vector<SmacOccupancyCache::Cell> occupancy;occupancy.reserve(world.tiles.size());
    for(int y=0;y<height;++y) {for(int x=y&1;x<width;x+=2) {
        const auto index=size_t(y)*(width/2)+x/2;const auto& next=world.tiles[index];auto* live=GetTile(x,y);
        const auto material=Decode(next.items,next.landmarks);auto after=Capture(live);
        occupancy.push_back({uint8_t(next.cached_occupant),uint8_t(next.items&3u)});
        after.moisture=tile::MOISTURE_ARID+next.moisture;after.rockiness=tile::ROCKINESS_FLAT+next.rockiness;
        after.features=material.features;after.work=material.work;after.water=next.elevation<3;
        const auto bonus=d.policy.Bonus(*live);
        after.bonus=bonus==resource::smac::ResourceBonus::Nutrients?tile::BONUS_NUTRIENT:bonus==resource::smac::ResourceBonus::Minerals?tile::BONUS_MINERALS:
            bonus==resource::smac::ResourceBonus::Energy?tile::BONUS_ENERGY:tile::BONUS_NONE;
        d.tile_preimages.push_back(live->Serialize().ToString());d.before.push_back(Capture(live));d.after.push_back(after);
        for(auto* ptr:live->elevation.corners) {d.old_heights.emplace(ptr,*ptr);}
        d.old_heights.emplace(live->elevation.center,*live->elevation.center);
        for(const auto& point:std::array<std::pair<tile::elevation_t*,int>,4>{{{live->elevation.left,corner(x,y,-1,0)},{live->elevation.top,corner(x,y,0,-1)},
            {live->elevation.right,corner(x,y,1,0)},{live->elevation.bottom,corner(x,y,0,1)}}}) {
            d.heights[point.first]=point.second; // same ordered shared-corner writes as LoadNativeTerrain
        }
        for(unsigned viewer=1;viewer<8;++viewer) {
            const auto native_before=retained_before.tiles[index].remembered_items[viewer-1];
            const auto memory=next.remembered_items[viewer-1];
            if(memory!=native_before) {
                auto& row=knowledge.remembered[viewer];if(row.empty()) {row.resize(world.tiles.size());}
                // Cross-category setter writes all positive memories, independent
                // of exploration. It does not reveal a previously unseen square.
                auto remembered=Decode(memory,0);
                constexpr uint16_t material_features=tile::FEATURE_RIVER|tile::FEATURE_MONOLITH|tile::FEATURE_XENOFUNGUS;
                if(row[index]) {remembered.features|=row[index]->features&~material_features;}
                row[index]=SmacWorkerKnowledge::Improvements{remembered.features,remembered.work,bool(memory&1u),bool(memory&2u)};
            }
        }
    }}
    for(const auto& p:d.after) {
        const auto* t=p.tile;d.heights[t->elevation.center]=(d.heights.at(t->elevation.left)+d.heights.at(t->elevation.top)+d.heights.at(t->elevation.right)+d.heights.at(t->elevation.bottom))/4;
    }
    knowledge.occupancy=SmacOccupancyCache::Create(width,height,world.horizontal_wrap,occupancy);
    d.knowledge=SmacWorkerKnowledge::Create(knowledge);
    return prepared;
}
void Map::CheckClimateTerrain(const PreparedClimateTerrain& prepared) const {
    if(!prepared.data || prepared.data->owner!=this || prepared.data->committed || m_global_spread_query_active) {throw std::invalid_argument("Invalid climate terrain transaction");}
    const auto& d=*prepared.data;
    if(!m_resource_policy || !(*m_resource_policy==d.original_policy) || !m_worker_knowledge || !(*m_worker_knowledge==d.original_knowledge)
        || !m_territory_topology || !(*m_territory_topology==d.original_topology)) {throw std::runtime_error("Stale climate authority preimage");}
    for(size_t i=0;i<d.before.size();++i) {if(d.before[i].tile->Serialize().ToString()!=d.tile_preimages[i]) {throw std::runtime_error("Stale climate tile preimage");}}
}
void Map::ValidateClimateTerrain(PreparedClimateTerrain& prepared) {
    CheckClimateTerrain(prepared);
    if(!prepared.data->territory_complete) {throw std::runtime_error("Climate publication requires completed entity and territory consumers");}
    prepared.data->validated=true;
}
void Map::QueryClimateTerrain(const PreparedClimateTerrain& prepared,const std::function<void()>& query) {
    CheckClimateTerrain(prepared);if(!query) {throw std::invalid_argument("Climate query requires a consumer");}
    const auto& d=*prepared.data;
    auto policy=std::optional<GlsmacResourceMapPolicy>{d.policy};auto knowledge=std::optional<SmacWorkerKnowledge>{d.knowledge};auto topology=std::optional<GlsmacTerritoryTopology>{d.topology};
    std::unique_ptr<smac_terrain::LandmarkGeometry> geometry;std::shared_ptr<const SmacTerrainAuthority> source;
    const auto wrap=m_native_geometry_wrap;
    util::FinallyGuard restore([&]{
        for(const auto& [p,value]:d.old_heights) {*p=value;}for(const auto& p:d.before) {p.Apply();}
        m_resource_policy.swap(policy);m_worker_knowledge.swap(knowledge);m_territory_topology.swap(topology);
        m_native_geometry=std::move(geometry);m_native_geometry_source=std::move(source);m_native_geometry_wrap=wrap;m_global_spread_query_active=false;
    });
    geometry=std::move(m_native_geometry);source=std::move(m_native_geometry_source);m_global_spread_query_active=true;
    m_resource_policy.swap(policy);m_worker_knowledge.swap(knowledge);m_territory_topology.swap(topology);
    for(const auto& [p,value]:d.heights) {*p=value;}for(const auto& p:d.after) {p.Apply();}query();
}
void Map::CompleteClimateTerritory(PreparedClimateTerrain& prepared,SmacWorkerKnowledge knowledge,GlsmacTerritoryTopology topology) {
    if(!prepared.data || prepared.data->owner!=this || prepared.data->committed || m_global_spread_query_active) {throw std::invalid_argument("Invalid climate territory completion");}
    auto& d=*prepared.data;std::vector<bool> sea;for(const auto& p:d.after) {sea.push_back(p.water);}topology.ValidateTerrain(sea);
    const auto data=knowledge.CopyData();if(data.width!=int32_t(GetWidth()) || data.height!=int32_t(GetHeight()) || data.horizontal_wrap!=topology.Wraps()) {throw std::invalid_argument("Climate territory geometry differs");}
    d.knowledge=std::move(knowledge);d.topology=std::move(topology);d.territory_complete=true;d.validated=false;
}
void Map::CommitClimateTerrain(PreparedClimateTerrain& prepared) noexcept {
    assert(prepared.data && prepared.data->owner==this && prepared.data->territory_complete && prepared.data->validated && !prepared.data->committed);
    auto& d=*prepared.data;
    static_assert(noexcept(*m_resource_policy=std::move(d.policy)) && noexcept(*m_worker_knowledge=std::move(d.knowledge)) && noexcept(*m_territory_topology=std::move(d.topology)));
    for(const auto& [p,value]:d.heights) {*p=value;}for(const auto& p:d.after) {p.Apply();}
    *m_resource_policy=std::move(d.policy);*m_worker_knowledge=std::move(d.knowledge);*m_territory_topology=std::move(d.topology);
    m_native_geometry.reset();m_native_geometry_source.reset();InvalidateRegionalPathWorkspace();m_native_only_presentation=true;d.committed=true;
}
const TerrainRenderUpdate& Map::GetClimateTerrainPayload(const PreparedClimateTerrain& prepared) const {
    if(!prepared.data || prepared.data->owner!=this) {throw std::invalid_argument("Foreign climate terrain payload");}
    return prepared.data->render;
}
}
