#include "Map.h"
#include "SmacGlobalSpread.h"
#include "SmacNativeTerrainItems.h"
#include "tile/Tile.h"
#include <stdexcept>
#include <utility>

namespace game::backend::map {
smac_spread::Input Map::CopyGlobalSpreadInput(int32_t turn,const std::array<uint32_t,8>& status) const {
    const auto* planet=m_resource_policy?m_resource_policy->OriginalPlanet():nullptr;
    if(!planet || GetWidth()!=80 || GetHeight()!=80 || !m_territory_topology
        || !m_worker_knowledge || !m_worker_knowledge_ready || turn<0) {
        throw std::runtime_error("Global flora requires current installed Planet terrain and ready knowledge");
    }
    const auto knowledge=m_worker_knowledge->CopyData();
    const auto wrap=m_territory_topology->Wraps();
    if(knowledge.width!=80 || knowledge.height!=80 || knowledge.horizontal_wrap!=wrap
        || knowledge.tiles.size()!=3200) {
        throw std::runtime_error("Global flora knowledge geometry differs from current map");
    }
    smac_spread::Input result{{80,80,wrap},3200,turn,status,{}};
    result.tiles.reserve(3200);
    // Only flora's material inputs are projected. This is not an authoritative
    // 44-byte gameplay plane: site, occupancy and other retained words below
    // must not be used for wildlife or published as current entity authority.
    constexpr auto material=smac_spread::LIVE_MATERIAL_MASK;
    const auto items=[](uint16_t features,uint16_t work) {
        return NativeTerrainItems(work,features)|((features&tile::FEATURE_MONOLITH)?0x2000u:0u);
    };
    const auto put=[](SmacPlanetMap::Tile& tile,size_t offset,uint32_t value) {
        for(size_t k=0;k<4;++k) {tile.bytes[offset+k]=uint8_t(value>>(8*k));}
    };
    const auto word=[](const SmacPlanetMap::Tile& tile,size_t offset) {
        uint32_t value=0;for(size_t k=0;k<4;++k) {value|=uint32_t(tile.bytes[offset+k])<<(8*k);}return value;
    };
    for(int y=0;y<80;++y) {for(int x=y&1;x<80;x+=2) {
        const size_t index=size_t(y)*40+x/2;
        auto output=planet->At(x,y);const auto* live=GetTile(x,y);
        if(!live || live->moisture<1 || live->moisture>4 || live->rockiness<1 || live->rockiness>4
            || (live->terraforming&~uint16_t(0x1fff))) {
            throw std::runtime_error("Global flora requires admitted live moisture, rockiness and improvements");
        }
        output.bytes[0]=uint8_t((output.bytes[0]&~0x18u)|((live->moisture-1)<<3));
        output.bytes[5]=uint8_t((output.bytes[5]&0x3fu)|((live->rockiness-1)<<6));
        output.bytes[4]=knowledge.tiles[index].explored_by;
        put(output,8,(output.Items()&~material)|items(live->features,live->terraforming));
        for(size_t owner=1;owner<=7;++owner) {
            const auto& row=knowledge.remembered[owner];
            const bool needed=(status[owner]&0x200u) || (output.bytes[4]&(1u<<owner));
            if(row.empty() || !row.at(index)) {
                if(needed) {throw std::runtime_error("Global flora cannot substitute current terrain for missing observer memory");}
                continue;
            }
            const auto& memory=*row[index];const auto offset=12+4*owner;
            put(output,offset,(word(output,offset)&~material)|items(memory.features,memory.terraforming));
        }
        result.tiles.push_back(output);
    }}
    return result;
}

struct Map::PreparedGlobalSpread::Data {
    Map* owner;
    smac_spread::Input before;
    GlsmacResourceMapPolicy original_policy,policy;
    SmacWorkerKnowledge original_knowledge,knowledge;
    TerrainAppearances original_appearance,appearance;
    SmacGameplayRandomState random;
    std::unique_ptr<PreparedTerrainUpdate> render;
    bool committed=false;
};
Map::PreparedGlobalSpread::PreparedGlobalSpread()=default;
Map::PreparedGlobalSpread::~PreparedGlobalSpread()=default;
std::unique_ptr<Map::PreparedGlobalSpread> Map::PrepareGlobalSpread(int32_t turn,
    const std::array<uint32_t,8>& status,const SmacGameplayRandomState& random) {
    auto before=CopyGlobalSpreadInput(turn,status);
    const auto plan=smac_spread::Prepare(before,random);
    auto policy=m_resource_policy->WithLiveGlobalSpread(before,plan);
    auto knowledge=m_worker_knowledge->CopyData();TerrainAppearances original,appearance;
    const auto word=[](const SmacPlanetMap::Tile& t,size_t offset) {
        uint32_t value=0;for(size_t k=0;k<4;++k) {value|=uint32_t(t.bytes[offset+k])<<(8*k);}return value;
    };
    for(const auto& change:plan.changes) {
        auto* tile=GetTile(change.target[0],change.target[1]);
        const auto feature=change.feature==0x200000u?tile::TERRAFORMING_FOREST:tile::TERRAFORMING_FARM;
        original.emplace(tile,TerrainAppearance{tile->features,tile->terraforming,tile->rockiness});
        appearance[tile]={uint16_t(tile->features&~tile::FEATURE_XENOFUNGUS),uint16_t(tile->terraforming|feature),tile->rockiness};
        const auto index=size_t(change.target[1])*40+change.target[0]/2;
        for(size_t owner=1;owner<=7;++owner) {
            const auto offset=12+4*owner;
            if(word(change.before,offset)==word(change.after,offset)) {continue;}
            auto& row=knowledge.remembered[owner];
            if(row.empty() || !row.at(index)) {continue;}
            // The packed retained plane owns partial unexplored history. Never
            // manufacture a full port memory record from its few changed bits.
            row[index]->features&=~tile::FEATURE_XENOFUNGUS;
            row[index]->terraforming|=feature;
        }
    }
    auto next=SmacWorkerKnowledge::Create(knowledge);
    auto render=appearance.empty()?std::unique_ptr<PreparedTerrainUpdate>{}:PrepareTerrainAppearanceUpdate(appearance);
    auto prepared=std::unique_ptr<PreparedGlobalSpread>(new PreparedGlobalSpread);
    prepared->data=std::make_unique<PreparedGlobalSpread::Data>(PreparedGlobalSpread::Data{
        this,std::move(before),*m_resource_policy,std::move(policy),*m_worker_knowledge,std::move(next),
        std::move(original),std::move(appearance),plan.random,std::move(render),false});
    return prepared;
}
const SmacGameplayRandomState& Map::GetGlobalSpreadRandom(const PreparedGlobalSpread& prepared) const {
    if(!prepared.data || prepared.data->owner!=this) {throw std::invalid_argument("Foreign global flora transaction");}
    return prepared.data->random;
}
const TerrainRenderUpdate* Map::GetGlobalSpreadTerrainPayload(const PreparedGlobalSpread& prepared) const {
    if(!prepared.data || prepared.data->owner!=this) {throw std::invalid_argument("Foreign global flora transaction");}
    return prepared.data->render?&GetTerrainUpdatePayload(*prepared.data->render):nullptr;
}
void Map::CommitGlobalSpread(PreparedGlobalSpread& prepared) {
    if(!prepared.data || prepared.data->owner!=this || prepared.data->committed) {throw std::invalid_argument("Invalid global flora transaction");}
    auto& data=*prepared.data;
    if(!m_resource_policy || !m_worker_knowledge || !(*m_resource_policy==data.original_policy)
        || !(*m_worker_knowledge==data.original_knowledge)) {throw std::runtime_error("Stale global flora policy or knowledge");}
    const auto current=CopyGlobalSpreadInput(data.before.turn,data.before.observer_status);
    for(size_t i=0;i<current.tiles.size();++i) {
        if(current.tiles[i].bytes!=data.before.tiles[i].bytes) {throw std::runtime_error("Stale global flora terrain input");}
    }
    for(const auto& [tile,old]:data.original_appearance) {
        if(tile->features!=old.features || tile->terraforming!=old.terraforming || tile->rockiness!=old.rockiness) {
            throw std::runtime_error("Stale global flora physical appearance");
        }
    }
    // All allocations, stale checks and render preparation finish above.
    for(const auto& [tile,next]:data.appearance) {
        auto* live=GetTile(tile->coord.x,tile->coord.y);live->features=next.features;live->terraforming=next.terraforming;
    }
    static_assert(noexcept(*m_resource_policy=std::move(data.policy)),"Flora policy publication must not throw");
    static_assert(noexcept(*m_worker_knowledge=std::move(data.knowledge)),"Flora knowledge publication must not throw");
    *m_resource_policy=std::move(data.policy);*m_worker_knowledge=std::move(data.knowledge);
    if(data.render) {CommitOrdinaryTerrainUpdate(*data.render);}
    data.committed=true;
}
}
