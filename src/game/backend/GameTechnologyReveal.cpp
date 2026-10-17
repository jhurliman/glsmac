#include "Game.h"

#include "base/Base.h"
#include "base/BaseManager.h"
#include "map/Map.h"
#include "slot/Slots.h"
#include "gc/Space.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend {

bool Game::HasSmacTechnologyReveal( const int32_t owner ) const {
    ValidateCurrentSimulation();
    if ( owner < 1 || owner > 7 || !m_simulation || !m_map || !m_map->m_worker_knowledge
        || !m_map->m_worker_knowledge_ready ) {
        throw std::runtime_error("Technology revelation requires current positive-faction map knowledge");
    }
    const auto& roster=m_simulation->value.roster;
    const auto participant=std::find_if(roster.begin(),roster.end(),[&](const auto& item){return item.simulation_id==owner;});
    if ( participant==roster.end() ) { throw std::runtime_error("Technology revelation owner is not admitted"); }
    RequireSmacLocalHumanOwner(&m_state->m_slots->GetSlot(participant->slot));
    (void)ProjectSmacWorkerKnowledge(*m_map->m_worker_knowledge,owner);
    return bool(m_map->m_worker_knowledge->CopyData().all_map_revealed & (1u<<owner));
}

std::optional< map::SmacWorkerKnowledge > Game::PrepareSmacTechnologyReveal( const int32_t owner ) const {
    if ( !IsRunning() || !GetGCSpace()->IsAccumulating() ) {
        throw std::runtime_error("Technology revelation requires the running owning accumulation");
    }
    if ( HasSmacTechnologyReveal(owner) ) { return {}; }
    auto data=m_map->m_worker_knowledge->CopyData();
    const auto bit=uint8_t(1u<<owner);
    auto& remembered=data.remembered[owner]; remembered.resize(data.tiles.size());
    for ( int32_t y=0;y<data.height;++y ) { for ( int32_t x=y&1;x<data.width;x+=2 ) {
        const auto* tile=m_map->GetTile(x,y);
        const auto index=size_t(y)*(data.width/2)+x/2;
        data.tiles[index].explored_by |= bit;
        remembered[index]=map::SmacWorkerKnowledge::Improvements{
            tile->features,tile->terraforming,tile->base!=nullptr,!tile->units.empty()};
    } }
    for ( const auto& entry:m_bm->GetBases() ) {
        const auto id=entry.first; const auto* base=entry.second;
        if ( !base || base->m_pops.size()>127 ) { throw std::runtime_error("Technology revelation Base population is invalid"); }
        auto found=std::find_if(data.bases.begin(),data.bases.end(),[&](const auto& item){return item.id==id;});
        if(found==data.bases.end()) { data.bases.push_back({id,0,{}}); found=std::prev(data.bases.end()); }
        found->known_by |= bit;
        found->population[owner]=static_cast<uint8_t>(base->m_pops.size());
    }
    data.all_map_revealed |= bit;
    return map::SmacWorkerKnowledge::Create(data);
}

void Game::CommitSmacTechnologyReveal( std::optional< map::SmacWorkerKnowledge > prepared ) noexcept {
    CommitConventionalSight(prepared);
}

}
