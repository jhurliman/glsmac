#include "Game.h"

#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/SmacWorkedSquareCollection.h"
#include "map/Map.h"
#include "map/SmacTerritoryRules.h"
#include "unit/UnitManager.h"
#include "Player.h"
#include "slot/Slot.h"
#include "gc/Space.h"
#include <set>
#include <thread>
#include <type_traits>

namespace game::backend {

bool Game::HandleCanonicalStartupEvent(GSE_CALLABLE,const std::string& name,
    const gse::value::object_properties_t& args,size_t caller,bool apply) {
    if(name!="spawn_starting_smac_base" && name!="spawn_starting_conventional_unit") {return false;}
    // Enqueue happens in create_world before GS_RUNNING. Only this exact
    // claimed new-world Start can queue these native commands; dispatch then
    // uses the stricter ordinary raw-startup producer guard.
    bool claimed=false;
    {
        std::lock_guard lock(m_admission_mutex);
        claimed=m_state && m_state->IsMaster() && m_owner_transition
            && m_owner_transition->phase==OwnerTransition::Phase::Running
            && m_owner_transition->thread==std::this_thread::get_id()
            && m_owner_transition->stage==StartupStage::Start && m_active_admission
            && m_owner_transition->admission==m_active_admission && !m_active_admission->cancelled
            && m_active_admission->kind==WorldAdmission::Kind::INITIALIZATION
            && !m_active_admission->previous && !m_active_admission->map_path && !m_active_admission->campaign_world
            && m_active_admission->state==m_state && m_state->IsGameSessionActive(m_active_admission->admitted_binding);
    }
    if(!claimed || caller!=m_slot_num || m_current_turn.GetId()!=0 || gc_space!=GetGCSpace()
        || !gc_space->IsAccumulating() || m_canonical_turn_failed || m_canonical_turn_in_progress) {
        throw std::runtime_error("Canonical startup commands require the authenticated claimed local Start");
    }
    const auto owner_arg=args.find("owner"),tile_arg=args.find("tile");
    if(args.size()!=2 || owner_arg==args.end() || tile_arg==args.end() || !owner_arg->second || !tile_arg->second) {
        throw std::runtime_error("Canonical startup command requires only owner and tile");
    }
    auto* player=Player::Unwrap(owner_arg->second);
    auto* tile=map::tile::Tile::Unwrap(tile_arg->second);
    auto* owner=player->GetSlot();
    RequireSmacLocalHumanOwner(owner);
    if(owner->GetPlayer()!=player || !m_map || tile->tiles!=m_map->GetTilesPtr() || tile->is_water_tile) {
        throw std::runtime_error("Canonical startup owner or land tile is not current");
    }
    if(apply) {
        CheckRW(GSE_CALL); RequireConventionalStartupClaim();
        if(name=="spawn_starting_smac_base") {
            const auto& configured=GetSmacBasePresentations();
            std::array<std::string,7> specialists;
            for(size_t i=0;i<7;++i) {specialists[i]=configured[i+1];}
            m_bm->SpawnStartingSmacBase(GSE_CALL,owner,tile,configured[0],specialists);
        } else {
            const auto raw=PrepareStartingConventionalUnit(owner,unit::SmacMoraleMode::Mode0);
            auto* definition=ResolveSmacProductionPresentation(owner,raw.design_id);
            m_um->SpawnStartingConventionalUnit(GSE_CALL,definition,owner,tile,unit::SmacMoraleMode::Mode0);
        }
    }
    return true;
}

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
    else { topology = m_map->PrepareFreshTerritoryTopology(); }
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
    if(topology->SiteRatings()) {
        std::vector<map::smac_site::Coordinate> centers;centers.reserve(territory.bases.size());
        for(const auto& existing:territory.bases) {centers.push_back(existing.coordinate);}
        topology=m_map->PrepareSiteRatingRefill(topology->WithSiteRatings(topology->SiteRatings()->FoundBase(radius.center)),centers,GetTurnId());
    }
    topology=m_map->PrepareTerritorySummary(*topology,knowledge);
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
