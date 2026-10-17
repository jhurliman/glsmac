#include "Game.h"

#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/SmacWorkedSquareCollection.h"
#include "map/Map.h"
#include "map/SmacTerritoryRules.h"
#include "unit/UnitManager.h"
#include "gc/Space.h"
#include <set>
#include <type_traits>

namespace game::backend {

Game::StartingBaseKnowledge Game::PrepareStartingBaseKnowledge( const base::Base& candidate ) const {
    RequireConventionalStartupClaim();
    ValidateCurrentSimulation();
    if ( !GetGCSpace()->IsAccumulating() || !m_map || !m_bm || !m_um || !m_map->GetResourcePolicy()
        || !candidate.m_resource_context || candidate.GetGame() != this || m_bm->GetBase(candidate.m_id)
        || !candidate.GetTile() || candidate.GetTile()->base ) {
        throw std::runtime_error("Starting Base requires a private candidate and generated resource semantics");
    }
    const auto& rules = m_state->m_settings.global.GetSmacTerritoryRules();
    if ( !rules ) { throw std::runtime_error("Starting Base requires frozen territory rules"); }
    const auto owner = candidate.m_resource_context->owner_simulation_id;
    const auto sea = m_map->TerritorySea();
    auto topology = m_map->m_territory_topology;
    if ( topology ) { topology->ValidateTerrain(sea); }
    else { topology = map::GlsmacTerritoryTopology::Build(m_map->GetWidth(),m_map->GetHeight(),true,sea); }
    map::SmacWorkerKnowledge::Data data;
    if ( m_map->m_worker_knowledge ) {
        if ( !m_map->m_worker_knowledge_ready ) { throw std::runtime_error("Starting Base cannot replace pending restored knowledge"); }
        data = m_map->m_worker_knowledge->CopyData();
        for ( int32_t observer=1; observer<=7; ++observer ) {
            if ( data.observers[observer] ) { (void)ProjectSmacWorkerKnowledge(*m_map->m_worker_knowledge,observer,nullptr,true); }
        }
    } else {
        if ( !m_bm->GetBases().empty() || !m_um->GetUnits().empty() ) {
            throw std::runtime_error("Initial worker knowledge must precede all startup entities");
        }
        // Explicit new GLSMAC local campaign policy. No inherited map reveal,
        // diplomatic grants, convoy orders, or remembered terrain are invented.
        data.width = m_map->GetWidth(); data.height = m_map->GetHeight();
        data.horizontal_wrap = true; data.ordinary_local_mode = true; data.all_map_revealed = 0;
        data.tiles.resize(sea.size(),{0,std::nullopt});
        for ( const auto& participant : m_simulation->value.roster ) {
            data.observers[participant.simulation_id] = map::SmacWorkerKnowledge::Observer{false,{},uint8_t(0)};
            data.remembered[participant.simulation_id].resize(sea.size());
        }
    }
    if ( !data.observers[owner] ) { throw std::runtime_error("Starting owner has no initial observer"); }
    const base::SmacWorkerRadiusMap radius{{static_cast<int32_t>(candidate.GetTile()->coord.x),static_cast<int32_t>(candidate.GetTile()->coord.y)},data.width,data.height,true};
    auto& remembered = data.remembered[owner];
    if ( remembered.empty() ) { remembered.resize(sea.size()); }
    // FIRST-WORKER-MAP: all 21 ordered visits occur before the Base marker and
    // before ordinary Scout sight. Resolve performs one X adjustment then clips.
    for ( int32_t index=0; index<=20; ++index ) {
        const auto coordinate = base::SmacWorkedSquareCollection::Resolve(radius,index);
        if ( !coordinate ) { continue; }
        const auto* tile = m_map->GetTile((*coordinate)[0],(*coordinate)[1]);
        const size_t position = size_t((*coordinate)[1])*(data.width/2)+(*coordinate)[0]/2;
        data.tiles[position].explored_by |= uint8_t(1u<<owner);
        remembered[position] = map::SmacWorkerKnowledge::Improvements{tile->features,tile->terraforming,tile->base!=nullptr,!tile->units.empty()};
    }
    auto knowledge = map::SmacWorkerKnowledge::Create(data);
    map::smac_territory::Input territory{data.width,data.height,true,rules->BorderDistance(),{}, {}};
    territory.tiles.reserve(sea.size());
    for ( size_t i=0; i<sea.size(); ++i ) { territory.tiles.push_back({sea[i],topology->Regions()[i]}); }
    std::set<size_t> seen;
    for ( const auto id : m_bm->GetBaseRecordOrder() ) {
        const auto* existing = m_bm->GetBase(id);
        if ( !existing || !seen.insert(id).second ) { throw std::runtime_error("Starting Base record order differs"); }
        const auto context = existing->GetSmacResourceContext();
        territory.bases.push_back({{static_cast<int32_t>(existing->GetTile()->coord.x),static_cast<int32_t>(existing->GetTile()->coord.y)},context.owner_simulation_id});
    }
    if ( seen.size()!=m_bm->GetBases().size() ) { throw std::runtime_error("Starting Base order is incomplete"); }
    territory.bases.push_back({radius.center,owner});
    knowledge = map::smac_territory::ReplaceClaims(knowledge,territory);
    auto frame = ProjectSmacWorkerKnowledge(knowledge,owner,nullptr,true).CopyData();
    frame.bases.push_back({owner,radius.center,0});
    const auto unavailable = map::SmacWorkerMapFrame::Create(frame).Evaluate(frame.bases.size()-1).unavailable;
    return {std::move(knowledge),std::move(topology),unavailable};
}

void Game::CommitStartingBaseKnowledge( StartingBaseKnowledge& prepared ) noexcept {
    static_assert(std::is_nothrow_swappable_v<decltype(prepared.knowledge)> && std::is_nothrow_swappable_v<decltype(prepared.topology)>);
    m_map->m_worker_knowledge.swap(prepared.knowledge);
    m_map->m_territory_topology.swap(prepared.topology);
    m_map->m_worker_knowledge_ready = true;
}

}
