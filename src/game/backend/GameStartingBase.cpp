#include "Game.h"

#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/SmacWorkedSquareCollection.h"
#include "map/Map.h"
#include "map/SmacPlanetMap.h"
#include "map/SmacTerritoryRules.h"
#include "map/SmacStartingSite.h"
#include "map/SmacStartingBalance.h"
#include "resource/SmacSiteSignal.h"
#include "map/SmacRestartPreprocessing.h"
#include "unit/Unit.h"
#include "slot/Slots.h"
#include <algorithm>
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
    if(name!="spawn_starting_smac_factions" && name!="spawn_starting_smac_base" && name!="spawn_starting_conventional_unit") {return false;}
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
    if(name=="spawn_starting_smac_factions") {
        if(!args.empty() || !m_simulation || !m_map || !m_map->GetResourcePolicy() || !m_bm->GetBases().empty() || !m_um->GetUnits().empty()) {
            throw std::runtime_error("Native faction startup requires an empty admitted world and no supplied state");
        }
        const auto& global=m_state->m_settings.global;const auto ocean=global.map.GetSmacOceanCoverage();
        const auto& rules=global.GetSmacCampaignRules();
        // The legacy no-rule-word port profile explicitly creates an immediate
        // HQ/Scout. It is not relabeled original Standard during this transition.
        if(!ocean || (rules && (rules->primary&0x10))) {throw std::runtime_error("Native immediate-HQ startup requires known ocean category and Look First off");}
        std::vector<const faction::SmacSimulationParticipant*> participants;
        for(const auto& p:m_simulation->value.roster) {
            if(!p.active) {continue;}
            RequireSmacLocalHumanOwner(&m_state->m_slots->GetSlot(p.slot));participants.push_back(&p);
        }
        std::sort(participants.begin(),participants.end(),[](const auto* a,const auto* b){return a->simulation_id<b->simulation_id;});
        if(!apply) {return true;}
        CheckRW(GSE_CALL);RequireConventionalStartupClaim();
        const auto& configured=GetSmacBasePresentations();std::array<std::string,7> specialists;
        for(size_t i=0;i<7;++i) {specialists[i]=configured[i+1];}
        auto topology=m_map->GetTerritoryTopology() ? *m_map->GetTerritoryTopology():m_map->PrepareFreshTerritoryTopology();
        if(!topology.Areas() || !topology.Postpass() || !topology.SiteRatings() || !topology.LandmarkDetails()) {
            throw std::runtime_error("Native starting sites require owned geography, wet counts, rating history and landmark detail");
        }
        // Original preparation clears imported ratings once; subsequent searches
        // retain exclusion/cache history produced by the preceding Base factory.
        topology=m_map->PrepareSiteRatingRefill(topology.WithSiteRatings(topology.SiteRatings()->ClearRatings()),{},0);
        m_map->m_territory_topology=std::move(topology);
        for(const auto* participant:participants) {
            auto* owner=&m_state->m_slots->GetSlot(participant->slot);RequireSmacLocalHumanOwner(owner);
            auto prepared=*m_map->GetTerritoryTopology();
            map::smac_start::Input input{};
            input.geometry={prepared.Width(),prepared.Height(),prepared.Wraps()};
            input.scale=map::smac_restart::ScaleFromGeometry(input.geometry);input.ocean=*ocean;
            input.difficulty=m_simulation->value.designs.global_difficulty;input.human=m_simulation->value.IsHuman(participant->simulation_id);
            input.look_first=false; // admitted immediate-HQ profile above
            for(size_t r=0;r<128;++r) {
                input.regions[r]={prepared.Areas()->at(r),prepared.Postpass()->fertility.at(r),0};
                for(const auto& peer:m_simulation->value.economy) {
                    if(!peer.second.regional_base_counts) {throw std::runtime_error("Starting site lacks retained regional Base presence");}
                    if(peer.second.regional_base_counts->at(r)) {input.regions[r].presence|=uint8_t(1u<<peer.first);}
                }
            }
            std::vector<map::smac_site::Coordinate> centers;
            for(const auto id:m_bm->GetBaseRecordOrder()) {const auto* base=m_bm->GetBase(id);
                const map::smac_site::Coordinate p{{static_cast<int32_t>(base->GetTile()->coord.x),static_cast<int32_t>(base->GetTile()->coord.y)}};
                input.bases.push_back({static_cast<int64_t>(id),p});centers.push_back(p);
            }
            for(const auto& row:m_um->GetUnits()) {const auto* unit=row.second;
                if(!unit->IsConventional()) {throw std::runtime_error("Starting site encountered an unexpected nonconventional Unit");}
                input.units.push_back({unit->Conventional().owner_simulation_id,{{static_cast<int32_t>(unit->GetTile()->coord.x),static_cast<int32_t>(unit->GetTile()->coord.y)}}});
            }
            const auto index=[&](auto p){return size_t(p[1])*(input.geometry.width/2)+p[0]/2;};
            const auto selected=map::smac_start::Select(input,m_simulation->value.gameplay_random,[&](auto p) {
                const auto* tile=m_map->GetTile(p[0],p[1]);
                return map::smac_start::Tile{static_cast<uint8_t>(m_map->GetResourcePolicy()->Altitude(*tile)),
                    static_cast<uint8_t>(tile->is_water_tile ? 0:tile->moisture-map::tile::MOISTURE_ARID),
                    static_cast<uint8_t>(tile->is_water_tile ? 0:tile->rockiness-map::tile::ROCKINESS_FLAT),
                    prepared.Regions().at(index(p)),prepared.LandmarkDetails()->at(index(p)),bool(tile->features&map::tile::FEATURE_XENOFUNGUS),tile->base || !tile->units.empty()};
            },[&](auto p) {
                if(prepared.SiteRatings()->Excluded(p)) {return 0;}
                if(const auto cached=prepared.SiteRatings()->Rating(p)) {return int32_t(cached);}
                prepared=m_map->PrepareSiteRatingQuery(prepared,centers,0,p);return int32_t(prepared.SiteRatings()->Rating(p));
            },{},{},[&] {
                std::lock_guard lock(m_admission_mutex);
                return !m_active_admission || m_active_admission->cancelled || !m_owner_transition
                    || m_owner_transition->admission!=m_active_admission;
            });
            if(!selected.site) {throw std::runtime_error("Native faction starting-site search refused its candidate budget");}
            m_simulation->value.gameplay_random=selected.random;m_map->m_territory_topology=std::move(prepared);
            auto* tile=m_map->GetTile((*selected.site)[0],(*selected.site)[1]);
            m_bm->SpawnStartingSmacBase(GSE_CALL,owner,tile,configured[0],specialists);
            const auto raw=PrepareStartingConventionalUnit(owner,unit::SmacMoraleMode::Mode0);
            m_um->SpawnStartingConventionalUnit(GSE_CALL,ResolveSmacProductionPresentation(owner,raw.design_id),owner,tile,unit::SmacMoraleMode::Mode0);
        }
        const auto& geography=*m_map->GetTerritoryTopology();
        const auto& policy=*m_map->GetResourcePolicy();const auto* source=policy.OriginalPlanet();
        map::smac_balance::Input balance{};balance.geometry={geography.Width(),geography.Height(),geography.Wraps()};
        balance.difficulty=m_simulation->value.designs.global_difficulty;balance.human_mask=m_simulation->value.human_membership;
        balance.areas=*geography.Areas();
        // Supported stock faction descriptors do not contain UNIT bonuses.
        // The profile parser rejects unsupported UNIT descriptors rather than
        // inferring credits from the Units just created.
        for(const auto& row:m_um->GetUnits()) {const auto* u=row.second;
            balance.units.push_back({static_cast<int64_t>(u->m_id),u->Conventional().owner_simulation_id,
                {{static_cast<int32_t>(u->GetTile()->coord.x),static_cast<int32_t>(u->GetTile()->coord.y)}}});
        }
        const auto compensation=map::smac_balance::Prepare(balance,m_simulation->value.gameplay_random,[&](auto p) {
            const auto* t=m_map->GetTile(p[0],p[1]);
            const auto at=size_t(p[1])*(balance.geometry.width/2)+p[0]/2;
            if(source) {const auto& raw=source->At(p[0],p[1]);return map::smac_balance::Tile{uint8_t(raw.Climate()>>5),uint8_t((raw.Climate()>>3)&3),uint8_t(raw.bytes[5]>>6),geography.Regions()[at],raw.Items()};}
            const uint32_t items=(t->features&map::tile::FEATURE_RIVER ? 0x80u:0)
                |(t->features&map::tile::FEATURE_MONOLITH ? 0x2000u:0)|(t->features&map::tile::FEATURE_UNITY_POD ? 0x10000000u:0);
            return map::smac_balance::Tile{uint8_t(policy.Altitude(*t)),uint8_t(t->is_water_tile ? 0:t->moisture-map::tile::MOISTURE_ARID),
                uint8_t(t->is_water_tile ? 0:t->rockiness-map::tile::ROCKINESS_FLAT),geography.Regions()[at],items};
        },[&](auto p,uint32_t items) {
            if(!source) {return int32_t(policy.SiteSignal(*m_map->GetTile(p[0],p[1])));}
            return int32_t(resource::smac::ResolveSiteSignal({p[0],p[1],static_cast<resource::smac::AltitudeBand>(source->At(p[0],p[1]).Climate()>>5),
                items,policy.AutomaticLandResources(),source->ResourcePattern(),11,61}));
        });
        auto compensated_policy=compensation.writes.empty() ? m_map->m_resource_policy
            :std::optional<map::GlsmacResourceMapPolicy>(policy.WithStartingBalance(compensation));
        for(const auto& pod:compensation.pods) {
            auto participant=std::find_if(participants.begin(),participants.end(),[&](const auto* p){return p->simulation_id==pod.owner;});
            if(participant==participants.end()) {throw std::runtime_error("Compensation Pod has no admitted owner");}
            auto* owner=&m_state->m_slots->GetSlot((*participant)->slot);
            m_um->SpawnStartingConventionalUnit(GSE_CALL,ResolveSmacProductionPresentation(owner,0),owner,
                m_map->GetTile(pod.point[0],pod.point[1]),unit::SmacMoraleMode::Mode0,true);
        }
        m_map->m_resource_policy=std::move(compensated_policy);m_simulation->value.gameplay_random=compensation.random;
        for(const auto& w:compensation.writes) {
            auto* t=m_map->GetTile(w.point[0],w.point[1]);
            if(m_map->GetResourcePolicy()->SiteSignal(*t)) {t->features|=map::tile::FEATURE_UNITY_POD;}
            else {t->features&=~map::tile::FEATURE_UNITY_POD;}
        }
        return true;
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
        auto occupancy=map::SmacOccupancyCache::Blank(data.width,data.height,true);
        if(const auto* original=m_map->GetResourcePolicy()->OriginalPlanet()) {
            std::vector<map::SmacOccupancyCache::Cell> cells;cells.reserve(sea.size());
            for(int32_t y=0;y<data.height;++y) {for(int32_t x=y&1;x<data.width;x+=2) {cells.push_back({original->At(x,y).CachedOccupant(),0});}}
            occupancy=map::SmacOccupancyCache::Create(data.width,data.height,true,cells);
        }
        data.occupancy=std::move(occupancy);

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
    if(data.occupancy) {data.occupancy->FoundBase(radius.center,owner);}
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
