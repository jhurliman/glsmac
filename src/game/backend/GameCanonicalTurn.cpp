#include "Game.h"
#include "faction/SmacRankingScore.h"

#include "Player.h"
#include "faction/Faction.h"
#include "slot/Slots.h"
#include "base/Base.h"
#include "base/SmacEcology.h"
#include "base/BaseManager.h"
#include "base/PopDef.h"
#include "event/Event.h"
#include "map/Map.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
#include "unit/SmacSelectionDistance.h"
#include "unit/SmacNativeAI.h"
#include "gc/Space.h"
#include "gse/value/Int.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Null.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include "util/FinallyGuard.h"
#include <set>
#include <algorithm>
#include <thread>
#include <deque>

namespace game::backend {
namespace {
bool IsActiveOwner(const faction::SmacSimulationState& state,int32_t owner) {
    return std::any_of(state.roster.begin(),state.roster.end(),[&](const auto& row){return row.simulation_id==owner && row.active;});
}
bool HasEcologyCalculationAuthority(const faction::SmacSimulationState& state,const settings::GlobalSettings& settings,const map::Map& world) {
    return world.GetWidth()>=8 && state.ecology_history && state.ecology_world && state.perihelion && settings.GetSmacEcologyRules();
}
void BeginOwnerEcologyAggregate(faction::SmacSimulationState& state,const settings::GlobalSettings& settings,const map::Map& world,int32_t owner) {
    // Legacy worlds without the calculation inputs retain historical words;
    // they cannot manufacture a newly calculated zero at an owner boundary.
    if(HasEcologyCalculationAuthority(state,settings,world)) {state.ecology_history->research_aggregate.at(owner)=0;}
}
void PostBaseEcologyAggregate(faction::SmacSimulationState& state,const settings::GlobalSettings& settings,const map::Map& world,const base::Base& base) {
    if(!HasEcologyCalculationAuthority(state,settings,world)) {return;}
    const auto owner=base.GetSmacResourceContext().owner_simulation_id;
    const auto& cache=base.GetSmacUpkeepCache();
    const auto* social=state.social.Find(owner);
    if(!cache || !cache->calculation.eco_damage || !social || social->phase!=faction::SmacFactionSocialState::Phase::Refreshed) {
        throw std::runtime_error("Ecology aggregate requires the committed resource pass and current PLANET");
    }
    // Research may have awarded technology. This consumer uses the retained
    // final resource pass, rather than silently forcing a new calculation.
    const auto contribution=base::SmacEcologyResearchContribution(*cache->calculation.eco_damage,social->current.at(7));
    auto& aggregate=state.ecology_history->research_aggregate.at(owner);
    const auto next=int64_t(aggregate)+contribution;
    if(next<INT32_MIN || next>INT32_MAX) {throw std::overflow_error("Ecology aggregate exceeds measured signed32");}
    aggregate=static_cast<int32_t>(next);
    // The mandatory CRT draw and downstream event producer remain a separate
    // pending integration. Posting alone is not a complete ecology handler.
}
}


void Game::ConfigureSmacBasePresentations(GSE_CALLABLE,const std::array<std::string,8>& roles) {
	bool configure=false;
	{
		std::lock_guard lock(m_admission_mutex);
		configure=m_state && m_owner_transition && m_owner_transition->phase==OwnerTransition::Phase::Running
			&& m_owner_transition->thread==std::this_thread::get_id() && m_active_admission
			&& m_owner_transition->admission==m_active_admission && !m_active_admission->cancelled
			&& m_active_admission->state==m_state && m_state->IsGameSessionActive(m_active_admission->admitted_binding)
			&& (m_owner_transition->stage==StartupStage::InitializeConfigure
				|| (m_owner_transition->stage==StartupStage::Start && m_state->IsMaster()
					&& m_active_admission->kind==WorldAdmission::Kind::INITIALIZATION
					&& !m_active_admission->previous && !m_active_admission->map_path));
	}
	if (!configure || gc_space!=GetGCSpace() || !gc_space->IsAccumulating() || !m_bm
		|| !m_bm->GetBases().empty() || m_current_turn.GetId()!=0) {
		throw std::runtime_error("Base presentation roles must be configured by startup before entities");
	}
	std::set<std::string> unique;
	for (const auto& role:roles) {
		if (role.empty() || role.size()>256 || role.find('\0')!=std::string::npos || !unique.insert(role).second) {
			throw std::runtime_error("Base presentation roles require eight distinct bounded definition IDs");
		}
	}
	if (m_smac_base_presentations && *m_smac_base_presentations!=roles) { throw std::runtime_error("Base presentation roles already configured"); }
	m_smac_base_presentations=roles;
}

const std::array<std::string,8>& Game::GetSmacBasePresentations() const {
	if (!m_smac_base_presentations || !m_bm) { throw std::runtime_error("Base presentation roles are not configured"); }
	for (size_t i=0;i<8;++i) {
		const auto* def=m_bm->GetPopDef(m_smac_base_presentations->at(i));
		if (!def || (i==0 ? !(def->m_flags & base::PopDef::PF_TILE_WORKER) || def->m_smac_specialist_id.has_value()
			: (def->m_flags & base::PopDef::PF_TILE_WORKER) || def->m_smac_specialist_id!=i-1)) {
			throw std::runtime_error("Configured Base presentation does not match its positional role");
		}
		m_bm->ValidateSmacPopDef(def);
	}
	return *m_smac_base_presentations;
}

void Game::RequireCanonicalStartupTurn( size_t turn_id ) const {
	if ( !m_map || !m_map->m_worker_knowledge ) { return; }
	RequireCanonicalTurn(turn_id);
	if (m_current_turn.GetId()!=0) { throw std::runtime_error("Canonical startup already reached its first turn"); }
}

void Game::RequireCanonicalTurn(size_t turn_id) const {
	if (!IsRunning() || !m_state || !m_map || !m_map->m_worker_knowledge || !m_map->m_worker_knowledge_ready
		|| m_canonical_turn_failed || m_canonical_turn_in_progress || m_canonical_continuation || !GetGCSpace()->IsAccumulating()
		|| m_current_turn.GetId()>=INT32_MAX || turn_id!=m_current_turn.GetId()+1) {
		throw std::runtime_error("Canonical transition requires its exact next turn and ready owning world");
	}
	RequireSmacLocalHumanOwner(m_slot);
	ValidateCurrentSimulation();
	RequireSmacGameplayActive();
	for (const auto& participant:m_simulation->value.roster) {
		if (participant.active) {(void)RequireSmacProductionOwner(&m_state->m_slots->GetSlot(participant.slot));}
	}
	const bool startup=m_current_turn.GetId()==0;
	if (startup) { RequireConventionalStartupClaim(); }
	else { m_um->RequireConventionalUnitsReady(); }
	const auto data=m_map->m_worker_knowledge->CopyData();
	for (int32_t observer=1;observer<=7;++observer) {
		if (data.observers[observer]) { (void)ProjectSmacWorkerKnowledge(*m_map->m_worker_knowledge,observer,nullptr,startup); }
	}
}

bool Game::QueueCanonicalCompleteTurn(GSE_CALLABLE,const std::string& name,const gse::value::object_properties_t& args) {
	if (name!="complete_turn" || !m_map || !m_map->m_worker_knowledge) { return false; }
	if (!args.empty() || m_current_turn.GetId()==0) { throw std::runtime_error("Canonical end turn takes no supplied state and requires a started turn"); }
	// Bind the command at enqueue time. Two clicks queued in one turn cannot
	// silently become two consecutive upkeep runs when the batch is dispatched.
	RequireCanonicalTurn(m_current_turn.GetId()+1);
	const gse::value::object_properties_t stamped{{"turn_id",VALUE(gse::value::Int,,m_current_turn.GetId()+1)}};
	AddEvent(new event::Event(this,event::Event::ES_LOCAL,m_slot_num,GSE_CALL,name,stamped));
	return true;
}

bool Game::HandleCanonicalStartupTurnEvent(GSE_CALLABLE,const std::string& name,
	const gse::value::object_properties_t& args,size_t caller,bool apply) {
	if ((name!="advance_turn" && name!="complete_turn") || !m_map || !m_map->m_worker_knowledge) { return false; }
	const auto turn=args.find("turn_id");
	if (caller!=m_slot_num || args.size()!=1 || turn==args.end() || !turn->second
		|| turn->second->type!=gse::VT_INT || static_cast<gse::value::Int*>(turn->second)->value<1
		|| (name=="complete_turn" && m_current_turn.GetId()==0)) {
		throw std::runtime_error("Canonical turn event requires its authenticated local next transition");
	}
	const auto next=static_cast<gse::value::Int*>(turn->second)->value;
	if (next>INT32_MAX) { throw std::runtime_error("Canonical turn exceeds the supported signed domain"); }
	if (m_canonical_turn_failed || m_canonical_turn_in_progress) { throw std::runtime_error("Canonical turn is failed or already in progress"); }
	// A genuine local click stamped in an earlier turn is harmless stale
	// input. Do not abort the batch or discard later valid commands.
	if (name=="complete_turn" && static_cast<size_t>(next)<=m_current_turn.GetId()) { return true; }
	RequireCanonicalTurn(static_cast<size_t>(next));
	if (apply) { CheckRW(GSE_CALL); AdvanceCanonicalTurn(GSE_CALL,static_cast<size_t>(next)); }
	return true;
}

std::optional<Game::CanonicalRanking> Game::PrepareCanonicalRankOrder() const {
    const auto& simulation=m_simulation->value;
    if(simulation.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {
        throw std::runtime_error("Ordinary ranking requires the admitted no-project campaign");
    }
    const auto& settings=m_state->m_settings.global;
    std::array<int32_t,7> scores{};uint8_t active=0;
    for(const auto& owner:simulation.roster) {
        if(!owner.active) {continue;}
        const auto& economy=simulation.economy.at(owner.simulation_id);
        // Old port frames did not own these inputs. A normal owner summary
        // establishes them; loading never manufactures a historical ranking.
        if(!economy.population_aggregate || !economy.design_counts) {return std::nullopt;}
        const auto* technology=simulation.technology.Find(owner.simulation_id);
        if(!technology) {throw std::runtime_error("Ranking faction lacks owned technology state");}
        scores[owner.simulation_id-1]=faction::CalculateSmacOrdinaryRankingScore(*technology,*economy.population_aggregate,
            *economy.design_counts,*settings.GetSmacTechnologyRules(),settings.GetSmacPredefinedUnitRules()->catalog,simulation.designs);
        active|=static_cast<uint8_t>(1u<<owner.simulation_id);
    }
    auto retained=simulation.ranking_scores;
    // Full rank selection consumes every positive score slot, including inactive
    // zero-score slots. Dominance later reads this same retained table.
    if(retained) {for(size_t i=1;i<retained->size();++i) {retained->at(i)=-1;}}
    std::array<int32_t,8> composite{};
    if(simulation.ranking_scores) {composite[0]=simulation.ranking_scores->at(0);}
    std::copy(scores.begin(),scores.end(),composite.begin()+1);
    return CanonicalRanking{faction::SmacRankOrder::Update(scores,active),retained,composite};
}

void Game::AdvanceCanonicalTurn(GSE_CALLABLE,size_t turn_id) {
	RequireCanonicalTurn(turn_id);
    if(m_simulation->value.faction_turn) {
        m_canonical_turn_in_progress=true;
        util::FinallyGuard finished([&]{m_canonical_turn_in_progress=false;});
        try {
            auto& cursor=*m_simulation->value.faction_turn;
            if(cursor.Current().phase==faction::SmacFactionTurn::Phase::OwnerCommand) {
                if(cursor.Current().owner!=cursor.Focus()) {throw std::runtime_error("Human end turn has no focused command ticket");}
                cursor.CompleteOwnerCommand(cursor.Current(),ActiveFactionMask());
            }
            ContinueFactionController(GSE_CALL);
        } catch(const std::exception& error) {FailCanonicalTurn(error.what());throw;}
        catch(...) {FailCanonicalTurn("Unknown faction controller failure");throw;}
        return;
    }
	// Session bindings choose the worker presentation explicitly: default
	// WORKER and TALENT both carry the tile-worker flag.
	(void)GetSmacBasePresentations();
	const auto records=m_bm->GetBaseRecordOrder();
	for (const auto id:records) {
		const auto* base=m_bm->GetBase(id);
		if (!base || !base->HasSmacResourceContext()) { throw std::runtime_error("Canonical campaign contains a legacy or missing Base"); }
        const auto owner=base->GetSmacResourceContext().owner_simulation_id;
        const auto row=std::find_if(m_simulation->value.roster.begin(),m_simulation->value.roster.end(),[&](const auto& p){return p.simulation_id==owner;});
        if(row!=m_simulation->value.roster.end() && row->active) {RequireSmacLocalHumanOwner(base->m_owner);}
	}
	// The original upkeep traverses one faction's live records at a time.
	// Ascending accepted simulation IDs are the explicit local human campaign
	// controller policy; saved record order remains authoritative within it.
	std::map<int32_t,const slot::Slot*> owners;
	for (const auto& participant:m_simulation->value.roster) {
		owners.emplace(participant.simulation_id,&m_state->m_slots->GetSlot(participant.slot));
	}
	CanonicalTurnContinuation continuation{turn_id};
	size_t count=0;
	for (const auto& owner:owners) {
		continuation.owners.push_back({owner.first,{}});
		for (const auto id:records) { if (m_bm->GetBase(id)->m_owner==owner.second) {continuation.owners.back().second.push_back(id);++count;} }
	}
	if (count!=records.size()) { throw std::runtime_error("Canonical upkeep Base owner is outside the admitted roster"); }
	m_canonical_turn_in_progress=true;
	util::FinallyGuard finished([&]{m_canonical_turn_in_progress=false;});
	try {
        const auto rank_order=PrepareCanonicalRankOrder();
        const auto perihelion=PrepareSmacPerihelionForTurn(turn_id);
		RefreshSimulationForTurn(turn_id);
        if(rank_order) {
            // Publish fresh composites before rank selection consumes its working
            // table and before advancing the turn. Never manufacture old rows.
            if(m_simulation->value.dominance_history) {
                if(m_simulation->value.dominance_native_keys) {
                    if(const auto composite=NativeDominanceScores(rank_order->composite)) {
                        m_simulation->value.dominance_history->Publish(GetTurnId(),*composite);
                    } // Unsupported projection preserves native history; ranking/upkeep still run.
                } else {m_simulation->value.dominance_history->Publish(GetTurnId(),rank_order->composite);}
            }
            m_simulation->value.ranking_scores=rank_order->scores;
            m_simulation->value.rank_order=rank_order->order;
        }
		// Rules such as the post-first-turn mineral deficit branch must observe
		// the incoming turn. UI notification remains after successful upkeep.
		m_current_turn.AdvanceTurn(turn_id);
		m_simulation->value.perihelion=perihelion;
		m_is_turn_complete=false;
		m_canonical_continuation=std::move(continuation);
        if(!m_canonical_continuation->owners.empty() && IsActiveOwner(m_simulation->value,m_canonical_continuation->owners.front().first)) {BeginOwnerEcologyAggregate(m_simulation->value,m_state->m_settings.global,*m_map,m_canonical_continuation->owners.front().first);}
		for (const auto& entry:m_um->GetUnits()) {
            // Positive-owner upkeep does not run the separate native controller.
            if(entry.second->IsNative()) {(void)m_um->QueryNativeUnit(entry.second->Native());continue;}
			if (!entry.second->IsConventional()) { throw std::runtime_error("Canonical upkeep contains an unsupported legacy Unit"); }
			if(!IsActiveOwner(m_simulation->value,entry.second->Conventional().owner_simulation_id)) {continue;}
			m_um->ResetConventionalSpent(GSE_CALL,entry.second);
		}
		ContinueCanonicalTurn(GSE_CALL);
	} catch (const std::exception& error) {
		FailCanonicalTurn(error.what()); throw;
	} catch (...) {
		FailCanonicalTurn("Unknown canonical upkeep failure"); throw;
	}
}

void Game::ContinueCanonicalTurn(GSE_CALLABLE) {
	if(!m_canonical_turn_in_progress || !m_canonical_continuation || m_research_request || m_colony_request) {
		throw std::runtime_error("Canonical continuation is not owned or still awaits a choice");
	}
	using Stage=CanonicalTurnContinuation::Stage;
	auto& next=*m_canonical_continuation;
    const auto previous_authority=m_smac_controller_owner;
    if(m_simulation->value.faction_turn) {m_smac_controller_owner=next.owners.at(next.owner_index).first;}
    util::FinallyGuard authority([&]{m_smac_controller_owner=previous_authority;});
	if(next.stage==Stage::Screen) {ValidateCanonicalColonyScreen();return;}
	while(next.owner_index<next.owners.size()) {
		const auto& owner=next.owners[next.owner_index];
        const auto participant=std::find_if(m_simulation->value.roster.begin(),m_simulation->value.roster.end(),[&](const auto& p){return p.simulation_id==owner.first;});
        if(participant==m_simulation->value.roster.end()) {throw std::runtime_error("Canonical owner is outside the admitted roster");}
        // An owner eliminated during its admitted pass still retires the
        // empty-owner tail before the existing human-ending break.
        if(!participant->active && m_simulation->value.human_defeat_request!=owner.first) {
            ++next.owner_index;next.base_index=0;next.stage=Stage::Base;
            if(next.owner_index<next.owners.size() && IsActiveOwner(m_simulation->value,next.owners[next.owner_index].first)) {BeginOwnerEcologyAggregate(m_simulation->value,m_state->m_settings.global,*m_map,next.owners[next.owner_index].first);}
            continue; // owned entities do not revive an eliminated controller
        }
		if(next.base_index==owner.second.size()) {
            if(m_simulation->value.faction_turn && GetTurnId()==1) {GrantFirstOwnerTechnologies(GSE_CALL,owner.first);}
			RefreshCanonicalPopulationAggregate(owner.first);
			// Native upkeep retires the shared route only AFTER this summary.
			m_map->InvalidateRegionalPathWorkspace();
			ResetCanonicalHurryMarkers(owner.first);
			SettleCanonicalEconomy(owner.first);
            for(const auto id:m_um->GetUnitRecordOrder()) {
                auto* actor=m_um->GetUnit(id);
                if(actor && actor->IsConventional() && actor->Conventional().owner_simulation_id==owner.first) {
                    m_um->ClearConventionalActivity(GSE_CALL,actor);
                }
            }
            if(m_simulation->value.faction_turn) {
                auto& cursor=*m_simulation->value.faction_turn;
                cursor.CompleteOwnerUpkeep(cursor.Current(),ActiveFactionMask());
                m_canonical_continuation.reset();
                ContinueFactionController(GSE_CALL);
                return;
            }
			++next.owner_index;next.base_index=0;next.stage=Stage::Base;
			if(m_simulation->value.HasEnding()) {break;} // finish this owner's tail, then the ending controller
            if(next.owner_index<next.owners.size() && IsActiveOwner(m_simulation->value,next.owners[next.owner_index].first)) {BeginOwnerEcologyAggregate(m_simulation->value,m_state->m_settings.global,*m_map,next.owners[next.owner_index].first);}
			continue;
		}
		auto* base=m_bm->GetBase(owner.second[next.base_index]);
		if(!base || base->GetSmacResourceContext().owner_simulation_id!=owner.first) {throw std::runtime_error("Canonical continuation Base identity changed");}
		if(next.stage==Stage::Base) {
			if(!BeginCanonicalBaseUpkeep(GSE_CALL)) {return;}
			if(next.stage==Stage::Base) {continue;} // starvation advanced the cursor
		}
		if(!ContinueCanonicalResearch(GSE_CALL)) {return;}
		if(next.stage!=Stage::Credit || !next.energy) {throw std::runtime_error("Canonical research omitted its credit stage");}
        PostBaseEcologyAggregate(m_simulation->value,m_state->m_settings.global,*m_map,*base);
		const auto notifications=PostCanonicalBaseMaintenance(*base,*next.energy);
		m_bm->RefreshBase(base);
		for(const auto& notification:notifications) {Message(notification);}
		if(next.screen_base) {
			next.stage=Stage::Screen;
			ValidateCanonicalColonyScreen();
			NotifyCanonicalColonyScreen(GSE_CALL);
			return; // credit is committed; close resumes after this exact Base
		}
		next.energy.reset();++next.base_index;next.stage=Stage::Base;
	}
	// The global territory producer follows all owner tails; read-only research
	// and suspended Base screens never refresh these retained counters.
	if(map::smac_territory::RefreshTerritoryAtGlobalTail(next.turn)) {
		std::optional<map::GlsmacTerritoryTopology> topology;
		auto knowledge=PrepareBaseTerritory(nullptr,nullptr,false,&topology);
		CommitBaseTerritory(knowledge,topology);
	}
	const auto turn_id=next.turn;
	m_canonical_continuation.reset();
	for(const auto& slot:m_state->m_slots->GetSlots()) {
		if(slot.GetState()==slot::Slot::SS_PLAYER) {slot.GetPlayer()->UncompleteTurn();}
	}
	auto fr=FrontendRequest(FrontendRequest::FR_TURN_ADVANCE);fr.data.turn_advance.turn_id=turn_id;AddFrontendRequest(fr);
	m_state->TriggerObject(this,"turn",ARGS_F(this) {
		{"year",VALUE(gse::value::Int,,m_current_turn.GetId()+2100)},
	}; });
	if(m_simulation->value.HasEnding()) {
		m_state->TriggerObject(this,"smac_ending",ARGS_F(this) { }; });
		return;
	}
	CheckTurnComplete();
	if(!m_is_turn_complete) {SetTurnStatus(turn::TS_TURN_ACTIVE);}
}

void Game::RunNativeCommands(GSE_CALLABLE) {
    namespace ai=unit::smac_native_ai;
    if(!IsSmacControllerOwner(0)) {throw std::runtime_error("Wildlife commands require native-controller authority");}
    const auto records=m_um->GetUnitRecordOrder();
    const ai::Geometry geometry{int32_t(m_map->GetWidth()),int32_t(m_map->GetHeight()),true};
    constexpr int32_t deltas[8][2]={{1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2}};
    const auto neighbor=[&](map::tile::Tile* source,int32_t direction)->map::tile::Tile* {
        const auto x=(int32_t(source->coord.x)+deltas[direction][0]+geometry.width)%geometry.width;
        const auto y=int32_t(source->coord.y)+deltas[direction][1];return y<0 || y>=geometry.height ? nullptr:m_map->GetTile(x,y);
    };
    // Planning sentry resets precede the complete command scan. Resetting a Worm
    // after an Isle readied it would incorrectly re-board it before beaching.
    for(const auto id:records) {
        auto* actor=m_um->GetUnit(id);
        if(actor && actor->IsNative() && !actor->Native().owner && (actor->Native().design==8 || actor->Native().design==9)) {m_um->ResetWildNativeSpent(GSE_CALL,actor);}
    }
    // The measured carrier pass precedes the land-native pass; both scan current
    // dense records descending and reacquire stable IDs after every mutation.
    for(const int32_t design:{9,8}) {for(auto record=records.rbegin();record!=records.rend();++record) {
        const auto id=*record;
        for(size_t step=0;step<12;++step) {
            auto* actor=m_um->GetUnit(id);
            if(!actor || !actor->IsNative() || actor->Native().owner || actor->Native().design!=design || actor->Native().order==1
                || m_um->QueryNativeUnit(actor->Native()).remaining_allowance<=0) {break;}
            if(actor->Native().flags&0x440u) {m_um->ExhaustWildNativeUnit(GSE_CALL,actor);break;}
            const auto before=actor->Native();auto* source=actor->GetTile();
            const auto knowledge=m_map->GetSmacWorkerKnowledge()->CopyData();
            const auto tile_query=[&](ai::Coordinate point) {
                const auto raw=m_map->CopyGlobalWildlifeTile(point,knowledge);const auto* tile=m_map->GetTile(point[0],point[1]);
                return ai::Tile{tile->is_water_tile,raw.Region(),raw.CachedOccupant(),knowledge.tiles.at(size_t(point[1])*(geometry.width/2)+point[0]/2).explored_by,raw.Items()};
            };
            uint8_t work=0;
            int32_t land_count=0;
            for(const auto& row:source->units) {
                const auto* member=row.second;
                if(member->IsNative()) {land_count+=member->Native().design==8;}
                else {const auto value=ResolveConventionalDesign(member->Conventional(),*m_simulation);land_count+=static_cast<int32_t>(m_state->m_settings.global.GetSmacPredefinedUnitRules()->catalog.chassis.chassis.at(value.chassis).triad)==0;}
            }
            if(design==9) {
                const auto& topology=m_map->GetTerritoryTopology();if(!topology) {throw std::runtime_error("Native carrier target requires retained regions");}
                std::array<int32_t,8> planet{};std::vector<ai::Base> bases;
                for(const auto base_id:m_bm->GetBaseRecordOrder()) {
                    const auto* base=m_bm->GetBase(base_id);const auto owner=base->GetSmacResourceContext().owner_simulation_id;
                    if(!base->m_native_target_resources) {throw std::runtime_error("Native target requires the Base's retained completed resource generation");}
                    const auto* social=m_simulation->value.social.Find(owner);
                    if(!social || !social->retained) {throw std::runtime_error("Native target requires retained owner PLANET");}planet.at(owner)=social->retained->at(7);
                    int32_t artifacts=0;
                    for(const auto& row:base->GetTile()->units) {if(row.second->IsConventional()) {artifacts+=ResolveConventionalDesign(row.second->Conventional(),*m_simulation).plan==12;}}
                    const auto& values=*base->m_native_target_resources;
                    bases.push_back({int32_t(bases.size()),{{int32_t(base->GetTile()->coord.x),int32_t(base->GetTile()->coord.y)}},GetCanonicalRegion(*base->GetTile(),*topology),uint8_t(owner),values[0],values[1],values[2],artifacts});
                }
                const ai::Coordinate point{{int32_t(source->coord.x),int32_t(source->coord.y)}};
                const auto selected=ai::ChooseBase(geometry,point,GetCanonicalRegion(*source,*topology),before.home_base,before.ecology_owner_tag,planet,bases);
                if(m_simulation->value.movement_history) {m_simulation->value.movement_history->RetainNearest(selected.metric);}
                if(selected.base<0) {m_um->ExhaustWildNativeUnit(GSE_CALL,actor);break;}
                work=m_um->BeginWildNativeCommand(GSE_CALL,actor);
                const auto& target=bases.at(selected.base);
                if(land_count) {
                const auto action=ai::CarrierStep(geometry,point,target,work,land_count,int32_t(before.stored_morale)+1,tile_query);
                if(action.action==ai::CarrierAction::Disband) {m_um->DisbandWildNativeUnit(GSE_CALL,actor);break;}
                if(action.action==ai::CarrierAction::ActivateStack) {m_um->ActivateWildNativeStack(GSE_CALL,actor,action.observation);break;}
                if(action.action==ai::CarrierAction::SeaAttack) {
                    auto* destination=m_map->GetTile((*action.point)[0],(*action.point)[1]);m_um->ValidateWildNativeMove(actor,destination);
                    m_um->SetWildNativeWaypoint(GSE_CALL,actor,0x58,destination);m_um->MoveWildNativeUnit(GSE_CALL,actor,destination);continue;
                }
                const auto directive=ai::CarrierDirective(geometry,point,target.point,tile_query);
                if(!directive.waypoint) {m_um->ExhaustWildNativeUnit(GSE_CALL,actor);break;}
                auto* goal=m_map->GetTile((*directive.waypoint)[0],(*directive.waypoint)[1]);m_um->SetWildNativeWaypoint(GSE_CALL,actor,24,goal);
                // Routing is the port controller's deterministic legal sea path,
                // separate from the measured target/directive competition.
                std::vector<int32_t> distance(size_t(geometry.width/2)*geometry.height,-1);std::deque<map::tile::Tile*> queue;
                const auto index=[&](const map::tile::Tile* tile){return size_t(tile->coord.y)*(geometry.width/2)+tile->coord.x/2;};
                if(goal->is_water_tile && !goal->base) {distance[index(goal)]=0;queue.push_back(goal);}
                while(!queue.empty()) {
                    auto* tile=queue.front();queue.pop_front();
                    for(int32_t direction=0;direction<8;++direction) {
                        auto* next=neighbor(tile,direction);if(!next || !next->is_water_tile || next->base || distance[index(next)]>=0) {continue;}
                        bool foreign=false;for(const auto& row:next->units) {foreign|=row.second->SimulationOwner()!=0;}if(foreign) {continue;}
                        distance[index(next)]=distance[index(tile)]+1;queue.push_back(next);
                    }
                }
                map::tile::Tile* destination=nullptr;int32_t best=INT32_MAX;
                for(int32_t direction=0;direction<8;++direction) {
                    auto* next=neighbor(source,direction);if(!next || distance[index(next)]<0 || distance[index(next)]>=best) {continue;}
                    try {m_um->ValidateWildNativeMove(actor,next);} catch(const std::runtime_error&) {continue;}
                    best=distance[index(next)];destination=next;
                }
                if(!destination || distance[index(source)]==0) {m_um->ExhaustWildNativeUnit(GSE_CALL,actor);break;}
                m_um->MoveWildNativeUnit(GSE_CALL,actor,destination);continue;
                }
            } else {(void)m_um->BeginWildNativeCommand(GSE_CALL,actor);}
            std::vector<ai::Candidate> candidates;std::array<map::tile::Tile*,8> destinations{};
            for(int32_t direction=0;direction<8;++direction) {
                auto* tile=neighbor(source,direction);destinations[direction]=tile;
                ai::Candidate candidate{direction,tile ? ai::Coordinate{{int32_t(tile->coord.x),int32_t(tile->coord.y)}}:ai::Coordinate{{0,0}},tile!=nullptr,false,0,0,{}};
                if(tile) {
                    const auto raw=m_map->CopyGlobalWildlifeTile(candidate.point,knowledge);candidate.sea=tile->is_water_tile;candidate.items=raw.Items();candidate.affiliation=raw.CachedOccupant();
                    for(const auto& row:tile->units) {
                        const auto* head=row.second;if(head->SourceStack() && head->SourceStack()->previous>=0) {continue;}
                        if(head->IsConventional()) {const auto context=ResolveConventionalUnit(head->Conventional(),head->m_owner);candidate.head=ai::Head{head->SimulationOwner(),head->Conventional().design_id,*context.offense_rating,0};}
                        else {candidate.head=ai::Head{head->Native().owner,head->Native().design,-1,head->Native().design==9 ? 1:head->Native().design==10 ? 2:0};}break;
                    }
                    try {m_um->ValidateWildNativeMove(actor,tile);} catch(const std::runtime_error&) {candidate.in_bounds=false;}
                }candidates.push_back(candidate);
            }
            const auto choice=ai::ChooseNeighbors(source->is_water_tile,design==9 ? 1:0,before.facing,int32_t(GetTurnId()),
                m_simulation->value.designs.global_difficulty,before.state,m_simulation->value.gameplay_random,candidates);
            m_simulation->value.gameplay_random=choice.random;
            if(design==9 && choice.best<99 && !land_count) {m_um->DisbandWildNativeUnit(GSE_CALL,actor);break;}
            if(choice.direction<0) {m_um->ExhaustWildNativeUnit(GSE_CALL,actor);break;}
            m_um->SetWildNativeWaypoint(GSE_CALL,actor,0x58,destinations.at(choice.direction));m_um->MoveWildNativeUnit(GSE_CALL,actor,destinations.at(choice.direction));
            actor=m_um->GetUnit(id);if(!actor || (actor->GetTile()==source && actor->Native().movement_spent==before.movement_spent)) {break;}
        }
    }}
    auto& native_status=m_simulation->value.faction_status.values[0];if(native_status) {*native_status&=~0x10000u;}
}

void Game::RunComputerCommands(GSE_CALLABLE,int32_t owner) {
    if(!IsSmacControllerOwner(owner) || m_simulation->value.IsHuman(owner)) {throw std::runtime_error("Computer commands require private nonhuman authority");}
    const auto& simulation=m_simulation->value;
    auto& slot=m_state->m_slots->GetSlot(std::find_if(simulation.roster.begin(),simulation.roster.end(),[&](const auto& row){return row.simulation_id==owner;})->slot);
    using Package=unit::ConventionalUnitContext::Package;
    using Work=unit::ConventionalUnitState::WorkOrder;
    std::vector<size_t> bases;
    std::array<size_t,3> counts{};
    for(const auto id:m_bm->GetBaseRecordOrder()) {if(m_bm->GetBase(id)->m_owner==&slot) {bases.push_back(id);}}
    for(const auto id:m_um->GetUnitRecordOrder()) {
        const auto* actor=m_um->GetUnit(id);
        if(actor->IsConventional() && actor->m_owner==&slot) {++counts.at(static_cast<size_t>(ResolveConventionalUnit(actor->Conventional(),&slot).package));}
    }
    // A modest growing empire: garrison, one developer per Base, then one
    // expansion party at a time. The real selection transaction charges losses
    // and rejects unavailable technology, population or component packages.
    for(const auto id:bases) {
        auto* base=m_bm->GetBase(id);
        const auto& production=base->GetProductionState();if(!production || !production->queue) {continue;}
        const bool developer=counts.at(static_cast<size_t>(Package::Former))<bases.size();
        const bool expand=!developer && bases.size()<5 && !counts.at(static_cast<size_t>(Package::Colony)) && base->GetPopulation()>=2;
        const bool garrison=std::any_of(base->GetTile()->units.begin(),base->GetTile()->units.end(),[&](const auto& row){
            return row.second->IsConventional() && row.second->m_owner==&slot
                && ResolveConventionalUnit(row.second->Conventional(),&slot).package==Package::Combat;
        });
        int32_t desired=!garrison?2:developer?1:expand?0:2;
        auto item=MakeSmacProductionItem(&slot,base::ProductionItemKind::Unit,desired);
        if(!IsSmacUnitProductionEligible(*base,item)) {desired=2;item=MakeSmacProductionItem(&slot,base::ProductionItemKind::Unit,desired);}
        if(IsSmacUnitProductionEligible(*base,item) && !(production->current_item==item)) {
            const auto expected=base::SerializeProductionState(production).ToString();
            base->SelectSmacProduction(GSE_CALL,desired,expected,true);
        }
    }
    const auto distance=[&](const map::tile::Tile* a,const map::tile::Tile* b) {
        return unit::selection_distance::Metric({{int32_t(a->coord.x),int32_t(a->coord.y)}},{{int32_t(b->coord.x),int32_t(b->coord.y)}},int32_t(m_map->GetWidth()),true);
    };
    // Stable record IDs and owned world/Unit state are the complete persistent
    // policy state. Saving at command boundaries reproduces every later choice.
    const auto actors=m_um->GetUnitRecordOrder();
    for(const auto id:actors) {
        auto* actor=m_um->GetUnit(id);
        if(!actor || !actor->IsConventional() || actor->m_owner!=&slot || !actor->HasMovesLeft()) {continue;}
        const auto package=ResolveConventionalUnit(actor->Conventional(),&slot).package;
        if(actor->Conventional().IsHeld()) {continue;}
        for(size_t step=0;step<12 && (actor=m_um->GetUnit(id)) && actor->HasMovesLeft();++step) {
            auto* source=actor->GetTile();
            if(package==Package::Colony) {
                bool legal=true;try {m_bm->ValidateSmacFounding(*actor);} catch(const std::runtime_error&) {legal=false;}
                if(legal) {m_bm->FoundSmacBase(GSE_CALL,*actor,GetSmacBasePresentations());break;}
            }
            if(package==Package::Former) {
                const auto order=actor->Conventional().work_order;
                if(order!=Work::None) {
                    bool legal=true;
                    try {m_um->ValidateConventionalWork(actor,order,slot.GetIndex(),true);} catch(const std::runtime_error&) {legal=false;}
                    if(legal) {m_um->SetConventionalWork(GSE_CALL,actor,order,slot.GetIndex(),true);break;}
                    // Global flora or a newly founded Base can invalidate a
                    // persisted job. Use the existing explicit cancel prefix,
                    // which retains work history, then seek another legal job.
                    m_um->SetConventionalWork(GSE_CALL,actor,Work::None,slot.GetIndex(),false);
                }
                bool worked=false;
                std::vector<Work> jobs;
                if(source->moisture==map::tile::MOISTURE_ARID && !(source->terraforming&(map::tile::TERRAFORMING_FARM|map::tile::TERRAFORMING_SOLAR|map::tile::TERRAFORMING_MINE))) {jobs={Work::Forest};}
                else {jobs={Work::Farm,Work::Solar,Work::Road};}
                for(const auto job:jobs) {
                    bool legal=true;try {m_um->ValidateConventionalWork(actor,job,slot.GetIndex(),false);} catch(const std::runtime_error&) {legal=false;}
                    if(legal && !source->base) {m_um->SetConventionalWork(GSE_CALL,actor,job,slot.GetIndex(),false);m_um->SetConventionalWork(GSE_CALL,actor,job,slot.GetIndex(),true);worked=true;break;}
                }
                if(worked) {break;}
            }
            if(package==Package::Combat && source->base && source->base->m_owner==&slot) {
                bool first=true;
                for(const auto& resident:source->units) {
                    if(resident.first>=id) {break;}
                    if(resident.second->IsConventional() && resident.second->m_owner==&slot
                        && ResolveConventionalUnit(resident.second->Conventional(),&slot).package==Package::Combat) {first=false;break;}
                }
                if(first) {m_um->ApplyHumanOrder(GSE_CALL,actor,unit::UnitManager::HumanOrderCommand::Skip,slot.GetIndex());break;}
            }
            const auto knowledge=m_map->GetSmacWorkerKnowledge()->CopyData();
            const auto bit=uint8_t(1u<<owner);
            const auto index=[&](const map::tile::Tile* t){return size_t(t->coord.y)*(knowledge.width/2)+t->coord.x/2;};
            map::tile::Tile* target=nullptr;int32_t nearest=INT32_MAX;
            if(package==Package::Combat) {
                for(const auto& observation:knowledge.bases) {
                    if(!(observation.known_by&bit)) {continue;}
                    auto* base=m_bm->GetBase(observation.id);
                    if(!base || base->m_owner==&slot) {continue;}
                    const auto d=distance(source,base->GetTile());if(d<nearest) {nearest=d;target=base->GetTile();}
                }
            }
            std::vector<std::pair<int64_t,map::tile::Tile*>> choices;
            for(auto* tile:source->neighbours) {
                if(!tile || !(knowledge.tiles.at(index(tile)).explored_by&bit)) {continue;}
                int64_t score=0;
                if(target) {score+=1000*(distance(source,target)-distance(tile,target));}
                for(const auto* adjacent:tile->neighbours) {if(adjacent && !(knowledge.tiles.at(index(adjacent)).explored_by&bit)) {score+=100;}}
                int32_t own_distance=INT32_MAX;
                for(const auto base_id:bases) {if(const auto* base=m_bm->GetBase(base_id)) {own_distance=std::min(own_distance,distance(tile,base->GetTile()));}}
                if(package==Package::Colony && own_distance!=INT32_MAX) {score+=100*std::min(own_distance,5);}
                if(package==Package::Former && own_distance!=INT32_MAX) {score-=100*std::abs(own_distance-1);}
                if(package==Package::Former && !tile->base && !tile->features && !(tile->terraforming&map::tile::TERRAFORMING_FOREST)) {
                    const auto complete=map::tile::TERRAFORMING_FARM|map::tile::TERRAFORMING_SOLAR|map::tile::TERRAFORMING_ROAD;
                    if((tile->terraforming&complete)!=complete) {score+=300;}
                }
                // Break equally useful exploration directions without RNG or
                // inspecting undisclosed enemy entities/terrain elsewhere.
                score+=int64_t((id*17+GetTurnId()*7+index(tile)*13)%31);
                choices.push_back({score,tile});
            }
            std::stable_sort(choices.begin(),choices.end(),[](const auto& a,const auto& b){return a.first>b.first;});
            bool moved=false;
            for(const auto& choice:choices) {
                bool legal=true;try {m_um->ValidateConventionalMove(actor,choice.second,slot.GetIndex());} catch(const std::runtime_error&) {legal=false;}
                if(!legal) {continue;}
                const auto before=actor->Conventional();
                m_um->MoveConventionalUnit(GSE_CALL,actor,choice.second,slot.GetIndex());
                actor=m_um->GetUnit(id);moved=true;
                if(!actor || (actor->GetTile()==source && actor->Conventional()==before)) {moved=false;}
                break;
            }
            if(!moved) {break;}
        }
    }
}

uint8_t Game::ActiveFactionMask() const {
    uint8_t mask=0;
    for(const auto& owner:m_simulation->value.roster) {if(owner.active) {mask|=uint8_t(1u<<owner.simulation_id);}}
    return mask;
}

void Game::PublishCanonicalCommandTurn(GSE_CALLABLE) {
    m_is_turn_complete=false;
    for(const auto& slot:m_state->m_slots->GetSlots()) {
        if(slot.GetState()==slot::Slot::SS_PLAYER) {slot.GetPlayer()->UncompleteTurn();}
    }
    auto fr=FrontendRequest(FrontendRequest::FR_TURN_ADVANCE);
    fr.data.turn_advance.turn_id=GetTurnId();AddFrontendRequest(fr);
    m_state->TriggerObject(this,"turn",ARGS_F(this) {
        {"year",VALUE(gse::value::Int,,m_current_turn.GetId()+2100)},
    }; });
    if(m_simulation->value.HasEnding()) {m_state->TriggerObject(this,"smac_ending",ARGS_F(this) { }; });}
    else {SetTurnStatus(turn::TS_TURN_ACTIVE);}
}

void Game::ContinueFactionController(GSE_CALLABLE) {
    using Phase=faction::SmacFactionTurn::Phase;
    if(!m_canonical_turn_in_progress || !m_simulation || !m_simulation->value.faction_turn || m_canonical_continuation) {
        throw std::runtime_error("Faction dispatch requires the owned ready controller");
    }
    auto& state=m_simulation->value;
    auto& cursor=*state.faction_turn;
    while(true) {
        if(state.HasEnding()) {PublishCanonicalCommandTurn(GSE_CALL);return;}
        switch(cursor.Current().phase) {
        case Phase::Global: {
            const auto turn=GetTurnId()+1;
            const auto rank=PrepareCanonicalRankOrder();
            const auto perihelion=PrepareSmacPerihelionForTurn(turn);
            RefreshSimulationForTurn(turn);
            // RefreshSimulationForTurn publishes the next cursor and seals
            // raw starting Units before the scalar year becomes observable.
            m_current_turn.AdvanceTurn(turn);state.perihelion=perihelion;
            if(rank) {state.ranking_scores=rank->scores;state.rank_order=rank->order;}
            m_is_turn_complete=false;break;
        }
        case Phase::WildlifeUpkeep:
            RunSmacWildlifeTurn(GSE_CALL);
            cursor.CompleteWildlifeUpkeep(cursor.Current());break;
        case Phase::WildlifeCommand: {
            const auto previous=m_smac_controller_owner;m_smac_controller_owner=0;
            util::FinallyGuard authority([&]{m_smac_controller_owner=previous;});
            RunNativeCommands(GSE_CALL);
            cursor.CompleteWildlifeCommand(cursor.Current(),ActiveFactionMask());break;
        }
        case Phase::OwnerPending: {
            const auto owner=cursor.Current().owner;
            if(!(ActiveFactionMask()&(1u<<owner))) {cursor.SkipInactiveOwner(cursor.Current(),ActiveFactionMask());break;}
            auto social=state.social;auto next_cursor=cursor;
            social.RefreshOwnerForFirstCommand(owner,*m_state->m_settings.global.GetSmacTechnologyRules(),*m_state->m_settings.global.GetSmacFactionStartupRules());
            next_cursor.BeginOwnerUpkeep(next_cursor.Current(),ActiveFactionMask());
            state.social=std::move(social);cursor=std::move(next_cursor);
            const auto previous=m_smac_controller_owner;m_smac_controller_owner=owner;
            util::FinallyGuard authority([&]{m_smac_controller_owner=previous;});
            BeginOwnerEcologyAggregate(state,m_state->m_settings.global,*m_map,owner);
            for(const auto id:m_um->GetUnitRecordOrder()) {
                auto* unit=m_um->GetUnit(id);
                if(unit->IsConventional() && unit->Conventional().owner_simulation_id==owner) {m_um->ResetConventionalSpent(GSE_CALL,unit);}
            }
            CanonicalTurnContinuation continuation{GetTurnId()};
            for(const auto& participant:state.roster) {continuation.owners.push_back({participant.simulation_id,{}});}
            std::sort(continuation.owners.begin(),continuation.owners.end(),[](const auto& a,const auto& b){return a.first<b.first;});
            for(size_t i=0;i<continuation.owners.size();++i) {
                auto& row=continuation.owners[i];if(row.first==owner) {continuation.owner_index=i;}
                for(const auto id:m_bm->GetBaseRecordOrder()) {
                    const auto* base=m_bm->GetBase(id);
                    if(!base || !base->HasSmacResourceContext()) {throw std::runtime_error("Faction upkeep encountered a noncanonical Base");}
                    if(base->GetSmacResourceContext().owner_simulation_id==row.first) {row.second.push_back(id);}
                }
            }
            m_canonical_continuation=std::move(continuation);
            ContinueCanonicalTurn(GSE_CALL);return;
        }
        case Phase::OwnerCommand: {
            const auto owner=cursor.Current().owner;
            if(owner==cursor.Focus()) {PublishCanonicalCommandTurn(GSE_CALL);return;}
            const auto previous=m_smac_controller_owner;m_smac_controller_owner=owner;
            util::FinallyGuard authority([&]{m_smac_controller_owner=previous;});
            RunComputerCommands(GSE_CALL,owner);
            cursor.CompleteOwnerCommand(cursor.Current(),ActiveFactionMask());break;
        }
        case Phase::Tail:
            if(map::smac_territory::RefreshTerritoryAtGlobalTail(GetTurnId())) {
                std::optional<map::GlsmacTerritoryTopology> topology;
                auto knowledge=PrepareBaseTerritory(nullptr,nullptr,false,&topology);CommitBaseTerritory(knowledge,topology);
            }
            cursor.CompleteRound(cursor.Current());break;
        case Phase::OwnerUpkeep:throw std::runtime_error("Faction upkeep lost its saved continuation");
        }
    }
}

void Game::RequireSmacGameplayActive() const {
    if(m_first_base_request) {throw std::runtime_error("Finish naming the first Base before issuing gameplay commands");}
	if(m_simulation && m_simulation->value.HasEnding()) {throw std::runtime_error("Campaign has an ending request");}
}

gse::Value* Game::GetSmacEnding(GSE_CALLABLE) const {
	if(!IsRunning() || !m_simulation || !m_simulation->value.HasEnding()) {return VALUE(gse::value::Null);}
	ValidateCurrentSimulation();
	const bool conquest=m_simulation->value.conquest_winner.has_value();
    const auto owner=conquest ? *m_simulation->value.conquest_winner:*m_simulation->value.human_defeat_request;
	const auto& roster=m_simulation->value.roster;
	const auto found=std::find_if(roster.begin(),roster.end(),[&](const auto& row){return row.simulation_id==owner;});
	if(found==roster.end() || found->active!=conquest) {throw std::runtime_error("Ending request owner differs from current saved roster");}
	auto* player=m_state->m_slots->GetSlot(found->slot).GetPlayer();
	const auto name=player->GetFaction()->m_name;
	const bool local=m_slot && found->slot==static_cast<int32_t>(m_slot->GetIndex());
	return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
		{"kind",VALUE(gse::value::String,,conquest ? "conquest":"human_defeat")}, {"owner_simulation_id",VALUE(gse::value::Int,,owner)},
		{"faction_name",VALUE(gse::value::String,,name)}, {"local",VALUE(gse::value::Bool,,local)}
	}));
}

void Game::FailCanonicalTurn(const std::string& error) {
	m_canonical_turn_failed=true;
	std::lock_guard guard(m_admission_mutex);
	if (m_active_admission && !m_active_admission->cancelled) {
		m_active_admission->error=error;
		m_active_admission->stage=StartupStage::Failure;
	}
	// Per-Base transactions are committed in saved order. Until whole-turn
	// staging exists, a later failure terminally retires this world; neither
	// a retry nor a snapshot of the partially advanced world is permitted.
}

void Game::PrepareCanonicalOwnershipContinuation() {
    if(!m_canonical_turn_in_progress || !m_canonical_continuation) {throw std::runtime_error("Ownership continuation has no active turn");}
    const auto count=m_bm->GetBaseRecordOrder().size();
    for(auto& owner:m_canonical_continuation->owners) {owner.second.reserve(count);}
}
void Game::CompleteCanonicalOwnershipChange(GSE_CALLABLE,size_t base_id) {
    auto& next=*m_canonical_continuation;const auto old_owner=next.owners.at(next.owner_index).first;
    const auto& records=m_bm->GetBaseRecordOrder();
    for(auto& owner:next.owners) {
        if(owner.second.capacity()<records.size()) {throw std::runtime_error("Ownership continuation storage was not prepared");}
        owner.second.clear();
    }
    size_t cursor=0;bool before=true,found=false;
    for(const auto id:records) {
        const auto* base=m_bm->GetBase(id);if(!base) {throw std::runtime_error("Ownership continuation lost a live Base record");}
        if(id==base_id) {before=false;found=true;}
        const auto owner=base->GetSmacResourceContext().owner_simulation_id;
        const auto row=std::find_if(next.owners.begin(),next.owners.end(),[&](const auto& candidate){return candidate.first==owner;});
        if(row==next.owners.end()) {throw std::runtime_error("Transferred Base has no retained owner traversal");}
        row->second.push_back(id);if(before && owner==old_owner) {++cursor;}
    }
    if(!found) {throw std::runtime_error("Transferred Base disappeared from its stable record");}
    next.base_index=cursor;next.energy.reset();next.stage=CanonicalTurnContinuation::Stage::Base;
    if(next.screen_base) {
        if(*next.screen_base!=base_id || !next.screen_request_id) {throw std::runtime_error("Transferred Colony Screen identity differs");}
        next.screen_after_transfer=true;next.stage=CanonicalTurnContinuation::Stage::Screen;
        ValidateCanonicalColonyScreen();NotifyCanonicalColonyScreen(GSE_CALL);
    }
}

// One entry for ordinary upkeep and the saved Colony-choice continuation.
bool Game::BeginCanonicalBaseUpkeep(GSE_CALLABLE) {
	if(!m_canonical_turn_in_progress || !m_canonical_continuation || m_colony_request || m_research_request) {
		throw std::runtime_error("Colony upkeep requires its owning active turn");
	}
	auto& next=*m_canonical_continuation;
	if(next.stage!=CanonicalTurnContinuation::Stage::Base || next.colony || next.energy) {
		throw std::runtime_error("Colony upkeep cannot replay a completed prefix");
	}
	const auto& owner=next.owners.at(next.owner_index);
	auto* base=m_bm->GetBase(owner.second.at(next.base_index));
	if(!base || base->GetSmacResourceContext().owner_simulation_id!=owner.first) {
		throw std::runtime_error("Colony upkeep Base identity changed");
	}
	base::Base::SmacAllocationRoles roles;
	const auto& configured=GetSmacBasePresentations();roles.worker=configured[0];
	for(size_t i=0;i<7;++i) {roles.specialists[i]=configured[i+1];}
    PrepareCanonicalOwnershipContinuation();
	auto result=base->BeginSmacUpkeep(GSE_CALL,roles,[&](int32_t design){
		return ResolveSmacProductionPresentation(base->m_owner,design);
	},unit::SmacMoraleMode::Mode0);
    if(result.colony_pause && !m_simulation->value.IsHuman(owner.first)) {
        // A population-one computer Base retains its city rather than opening
        // a human abandonment dialog. The existing deferred-production tail
        // commits its resource accounting; command planning changes the item.
        result=base->ResumeSmacColonyUpkeep(GSE_CALL,*result.colony_pause,base::SmacColonyChoice::Delay,roles,[&](int32_t design){
            return ResolveSmacProductionPresentation(base->m_owner,design);
        });
    }
    if(result.ownership_changed) {CompleteCanonicalOwnershipChange(GSE_CALL,base->m_id);return m_canonical_continuation->stage!=CanonicalTurnContinuation::Stage::Screen;}
	if(result.needs_former_owner) {next.stage=CanonicalTurnContinuation::Stage::FormerOwner;RequestCanonicalFormerOwner(GSE_CALL);return false;}
	if(result.colony_pause) {
		next.colony=result.colony_pause;next.stage=CanonicalTurnContinuation::Stage::Colony;
		RequestCanonicalColony(GSE_CALL);return false;
	}
	if(result.removed_base) {++next.base_index;return true;}
	if(!result.final_calculation) {throw std::runtime_error("Colony upkeep omitted its final resource pass");}
	next.energy=result.retained_energy ? *result.retained_energy:CalculateCanonicalBaseEnergy(*base,*result.final_calculation);
	next.stage=CanonicalTurnContinuation::Stage::ResearchStart;
	return true;
}

void Game::ValidateCanonicalColony() const {
	if(!m_canonical_continuation || !m_colony_request || m_research_request || m_canonical_turn_failed || !IsRunning()) {
		throw std::runtime_error("Colony choice has no active suspended turn");
	}
	const auto& next=*m_canonical_continuation;const auto& request=*m_colony_request;
	if(next.stage!=CanonicalTurnContinuation::Stage::Colony || !next.colony || next.energy
		|| next.turn!=GetTurnId() || request.turn!=next.turn || request.id<1
		|| next.owner_index>=next.owners.size()) {throw std::runtime_error("Colony choice stage differs");}
	const auto& owner=next.owners[next.owner_index];
	if(request.owner!=owner.first || next.base_index>=owner.second.size() || request.base!=owner.second[next.base_index]) {
		throw std::runtime_error("Colony choice owner or Base identity differs");
	}
	ValidateCurrentSimulation();
	const auto* base=m_bm->GetBase(request.base);
	if(!base || base->GetSmacResourceContext().owner_simulation_id!=request.owner) {
		throw std::runtime_error("Colony choice Base no longer exists for its owner");
	}
	RequireSmacLocalHumanOwner(base->m_owner);
	base->ValidateSmacColonyPause(*next.colony);
}

void Game::RequestCanonicalColony(GSE_CALLABLE) {
	if(!m_canonical_turn_in_progress || !m_canonical_continuation || m_colony_request || m_research_request
		|| m_next_colony_request==INT64_MAX) {throw std::runtime_error("Colony request cannot be published");}
	const auto& next=*m_canonical_continuation;const auto& owner=next.owners.at(next.owner_index);
	m_colony_request=CanonicalColonyRequest{++m_next_colony_request,owner.first,owner.second.at(next.base_index),next.turn};
	ValidateCanonicalColony();
	SetTurnStatus(turn::TS_PLEASE_WAIT);
	NotifyCanonicalColony(GSE_CALL);
}

void Game::NotifyCanonicalColony(GSE_CALLABLE) {
	m_state->TriggerObject(this,"colony_choice",ARGS_F(this) { }; });
}

gse::Value* Game::GetPendingSmacColony(GSE_CALLABLE) const {
	if(!m_colony_request || m_canonical_turn_failed) {return VALUE(gse::value::Null);}
    if(m_canonical_continuation->stage==CanonicalTurnContinuation::Stage::FormerOwner) {
        ValidateCanonicalFormerOwner();const auto& request=*m_colony_request;const auto* base=m_bm->GetBase(request.base);
        gse::value::array_elements_t choices;
        choices.push_back(VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
            {"id",VALUE(gse::value::Int,,0)},{"name",VALUE(gse::value::String,,"Native life")}})));
        for(const auto& participant:m_simulation->value.roster) {
            auto* player=m_state->m_slots->GetSlot(participant.slot).GetPlayer();
            choices.push_back(VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
                {"id",VALUE(gse::value::Int,,participant.simulation_id)},{"name",VALUE(gse::value::String,,player->GetFaction()->m_name)}})));
        }
        return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
            {"kind",VALUE(gse::value::String,,"former_owner")},{"request_id",VALUE(gse::value::Int,,request.id)},
            {"base_id",VALUE(gse::value::Int,,request.base)},{"base_name",VALUE(gse::value::String,,base->m_name)},
            {"owner_slot",VALUE(gse::value::Int,,base->m_owner->GetIndex())},{"choices",VALUE(gse::value::Array,,choices)}}));
    }
	ValidateCanonicalColony();
	const auto& request=*m_colony_request;const auto* base=m_bm->GetBase(request.base);
	const gse::value::object_properties_t properties{
		{"request_id",VALUE(gse::value::Int,,request.id)}, {"base_id",VALUE(gse::value::Int,,request.base)},
		{"base_name",VALUE(gse::value::String,,base->m_name)}, {"owner_slot",VALUE(gse::value::Int,,base->m_owner->GetIndex())}
	};
	return VALUE(gse::value::Object,,GSE_CALL_NOGC,properties);
}

bool Game::HandleSmacColonyEvent(GSE_CALLABLE,const std::string& name,
	const gse::value::object_properties_t& args,size_t caller,bool apply) {
	if(name=="choose_former_faction") {return HandleSmacFormerOwnerEvent(GSE_CALL,name,args,caller,apply);}
	if(name!="choose_colony") {return false;}
	const auto request=args.find("request_id"),choice=args.find("choice");
	if(caller!=m_slot_num || args.size()!=2 || request==args.end() || choice==args.end()
		|| !request->second || request->second->type!=gse::VT_INT || !choice->second || choice->second->type!=gse::VT_STRING) {
		throw std::runtime_error("Colony choice requires its authenticated request and explicit answer");
	}
	const auto id=static_cast<gse::value::Int*>(request->second)->value;
	const auto& answer=static_cast<gse::value::String*>(choice->second)->value;
	if(id<1 || (answer!="delay" && answer!="screen" && answer!="accept")) {throw std::runtime_error("Invalid Colony choice");}
	if(m_canonical_turn_failed || m_canonical_turn_in_progress) {throw std::runtime_error("Colony choice is not currently available");}
	if(!m_colony_request || m_colony_request->id!=id) {return true;} // stale or duplicate answer
	ValidateCanonicalColony();
	// Joined with the distinct prepared abandonment provider before the default
	// controller uses BeginCanonicalBaseUpkeep. Never manufacture a substitute.
	if(answer=="accept" && m_um->GetUnits().size()<2048) {m_bm->ValidateSmacAbandonment(*m_bm->GetBase(m_colony_request->base));}
	if(!apply) {return true;}
	if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {throw std::runtime_error("Colony choice requires its owning accumulation");}
	m_canonical_turn_in_progress=true;
	util::FinallyGuard finished([&]{m_canonical_turn_in_progress=false;});
	try {
		auto& next=*m_canonical_continuation;const auto pending=*m_colony_request;
		auto* base=m_bm->GetBase(pending.base);
		base::Base::SmacAllocationRoles roles;const auto& configured=GetSmacBasePresentations();roles.worker=configured[0];
		for(size_t i=0;i<7;++i) {roles.specialists[i]=configured[i+1];}
		if(answer=="accept" && m_um->GetUnits().size()<2048) {
			// Prepare traversal storage before the provider publishes and retires the
			// old Base. Its returned stable successor owns the remaining tail.
			auto remaining=next.owners.at(next.owner_index).second;
			remaining.erase(remaining.begin()+next.base_index);
			const auto mode=next.colony->morale_mode;
			const auto design=*base->GetProductionState()->current_item.smac_source_id;
			auto* definition=ResolveSmacProductionPresentation(base->m_owner,design);
			const auto abandoned=m_bm->AbandonSmacBase(GSE_CALL,*base,definition,mode);
			next.owners[next.owner_index].second.swap(remaining);
			next.colony.reset();m_colony_request.reset();
			if(abandoned.successor_id) {
				if(!abandoned.successor_cache || next.base_index>=next.owners[next.owner_index].second.size()
					|| next.owners[next.owner_index].second[next.base_index]!=*abandoned.successor_id) {
					throw std::runtime_error("Abandonment compacted successor differs from the suspended traversal");
				}
				auto* successor=m_bm->GetBase(*abandoned.successor_id);
				if(!successor) {throw std::runtime_error("Abandonment successor disappeared");}
                PrepareCanonicalOwnershipContinuation();
				const auto tail=successor->ResumeSmacAbandonmentTail(GSE_CALL,*abandoned.successor_cache,roles,mode);
                if(tail.needs_former_owner) {
                    next.former_abandonment_tail=mode;next.stage=CanonicalTurnContinuation::Stage::FormerOwner;
                    RequestCanonicalFormerOwner(GSE_CALL);return true;
                }
                if(tail.ownership_changed) {
                    CompleteCanonicalOwnershipChange(GSE_CALL,successor->m_id);ContinueCanonicalTurn(GSE_CALL);
                    if(!m_colony_request) {NotifyCanonicalColony(GSE_CALL);}return true;
                }
				if(tail.removed_base || tail.completed_unit || !tail.final_calculation) {
					throw std::runtime_error("Abandonment successor produced an unsupported tail");
				}
				next.energy=tail.retained_energy ? *tail.retained_energy:CalculateCanonicalBaseEnergy(*successor,*tail.final_calculation);
				next.stage=CanonicalTurnContinuation::Stage::ResearchStart;
			} else {next.stage=CanonicalTurnContinuation::Stage::Base;}
			ContinueCanonicalTurn(GSE_CALL);
			if(!m_colony_request) {NotifyCanonicalColony(GSE_CALL);}
			return true;
		}
        PrepareCanonicalOwnershipContinuation();
		const auto selected=answer=="accept" ? base::SmacColonyChoice::Accept : answer=="screen" ? base::SmacColonyChoice::Screen:base::SmacColonyChoice::Delay;
		const auto result=base->ResumeSmacColonyUpkeep(GSE_CALL,*next.colony,selected,roles,[&](int32_t design){
			return ResolveSmacProductionPresentation(base->m_owner,design);
		});
        if(result.ownership_changed) {
            next.colony.reset();m_colony_request.reset();
            if(selected==base::SmacColonyChoice::Screen) {next.screen_base=pending.base;next.screen_request_id=pending.id;}
            CompleteCanonicalOwnershipChange(GSE_CALL,pending.base);
            if(next.stage!=CanonicalTurnContinuation::Stage::Screen) {ContinueCanonicalTurn(GSE_CALL);}
            if(!m_colony_request) {NotifyCanonicalColony(GSE_CALL);}return true;
        }
        if(result.needs_former_owner) {
            next.former_colony_choice=selected;next.stage=CanonicalTurnContinuation::Stage::FormerOwner;
            m_colony_request.reset();RequestCanonicalFormerOwner(GSE_CALL);return true;
        }
		if(result.removed_base) {
			// Population starvation retired the old Base. Its address and Screen
			// request cannot survive into the continuation.
			next.colony.reset();m_colony_request.reset();++next.base_index;
			next.stage=CanonicalTurnContinuation::Stage::Base;
			ContinueCanonicalTurn(GSE_CALL);if(!m_colony_request) {NotifyCanonicalColony(GSE_CALL);}return true;
		}
		if(result.completed_unit || !result.final_calculation) {
			throw std::runtime_error("Deferred Colony tail produced an invalid completion");
		}
		next.energy=result.retained_energy ? *result.retained_energy:CalculateCanonicalBaseEnergy(*base,*result.final_calculation);
		next.colony.reset();next.stage=CanonicalTurnContinuation::Stage::ResearchStart;
		if(selected==base::SmacColonyChoice::Screen) {next.screen_base=pending.base;next.screen_request_id=pending.id;}
		m_colony_request.reset();
		ContinueCanonicalTurn(GSE_CALL);
		if(!m_colony_request) {NotifyCanonicalColony(GSE_CALL);}
	} catch(const std::exception& error) {FailCanonicalTurn(error.what());throw;}
	catch(...) {FailCanonicalTurn("Unknown Colony continuation failure");throw;}
	return true;
}

void Game::ValidateCanonicalFormerOwner() const {
    if(!m_canonical_continuation || !m_colony_request || m_research_request || m_canonical_turn_failed || !IsRunning()) {
        throw std::runtime_error("Former faction question has no current suspended turn");
    }
    const auto& next=*m_canonical_continuation;const auto& request=*m_colony_request;
    if(next.stage!=CanonicalTurnContinuation::Stage::FormerOwner || next.energy || next.screen_base || next.screen_request_id
        || next.turn!=GetTurnId() || request.turn!=next.turn || next.owner_index>=next.owners.size()) {
        throw std::runtime_error("Former faction question checkpoint differs");
    }
    const auto& owner=next.owners[next.owner_index];
    if(next.base_index>=owner.second.size() || owner.first!=request.owner || owner.second[next.base_index]!=request.base
        || bool(next.colony)!=bool(next.former_colony_choice) || (next.colony && next.former_abandonment_tail)) {throw std::runtime_error("Former faction question identity differs");}
    const auto* base=m_bm->GetBase(request.base);
    if(!base || base->GetSmacResourceContext().owner_simulation_id!=owner.first || base->GetSmacResourceContext().FormerOwner()) {
        throw std::runtime_error("Former faction question no longer needs an answer");
    }
    RequireSmacLocalHumanOwner(base->m_owner);
    if(next.former_abandonment_tail && (!base->GetSmacUpkeepCache() || (*next.former_abandonment_tail!=unit::SmacMoraleMode::Mode0 && *next.former_abandonment_tail!=unit::SmacMoraleMode::Mode1))) {
        throw std::runtime_error("Former faction question lost its committed successor cache");
    }
    if(next.colony) {
        if(*next.former_colony_choice!=base::SmacColonyChoice::Delay && *next.former_colony_choice!=base::SmacColonyChoice::Screen
            && !(*next.former_colony_choice==base::SmacColonyChoice::Accept && m_um->GetUnits().size()>=2048)) {
            throw std::runtime_error("Former faction question lost the accepted Colony continuation");
        }
        base->ValidateSmacColonyPause(*next.colony);
    }
}
void Game::RequestCanonicalFormerOwner(GSE_CALLABLE) {
    if(!m_canonical_turn_in_progress || !m_canonical_continuation || m_colony_request || m_research_request || m_next_colony_request==INT64_MAX) {
        throw std::runtime_error("Former faction request cannot be published");
    }
    const auto& next=*m_canonical_continuation;const auto& owner=next.owners.at(next.owner_index);
    m_colony_request=CanonicalColonyRequest{++m_next_colony_request,owner.first,owner.second.at(next.base_index),next.turn};
    ValidateCanonicalFormerOwner();SetTurnStatus(turn::TS_PLEASE_WAIT);NotifyCanonicalColony(GSE_CALL);
}
bool Game::HandleSmacFormerOwnerEvent(GSE_CALLABLE,const std::string& name,const gse::value::object_properties_t& args,size_t caller,bool apply) {
    if(name!="choose_former_faction") {return false;}
    const auto request=args.find("request_id"),former=args.find("former_owner");
    if(caller!=m_slot_num || args.size()!=2 || request==args.end() || former==args.end() || !request->second || !former->second
        || request->second->type!=gse::VT_INT || former->second->type!=gse::VT_INT) {
        throw std::runtime_error("Former faction requires its authenticated request and explicit answer");
    }
    const auto id=static_cast<gse::value::Int*>(request->second)->value;
    const auto answer=static_cast<gse::value::Int*>(former->second)->value;
    if(id<1 || answer< -1 || answer>7) {throw std::runtime_error("Invalid former faction answer");}
    if(m_canonical_turn_in_progress || m_canonical_turn_failed) {throw std::runtime_error("Former faction question is not currently available");}
    if(!m_colony_request || m_colony_request->id!=id) {return true;}
    ValidateCanonicalFormerOwner();
    if(!apply || answer==-1) {return true;} // Cancel retains the exact checkpoint and question.
    if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {throw std::runtime_error("Former faction answer requires its owning accumulation");}
    auto& next=*m_canonical_continuation;const auto pending=*m_colony_request;auto* base=m_bm->GetBase(pending.base);
    base::Base::SmacAllocationRoles roles;const auto& configured=GetSmacBasePresentations();roles.worker=configured[0];
    for(size_t i=0;i<7;++i) {roles.specialists[i]=configured[i+1];}
    m_canonical_turn_in_progress=true;bool publication=false;
    util::FinallyGuard finished([&]{m_canonical_turn_in_progress=false;});
    try {
        PrepareCanonicalOwnershipContinuation();
        const auto result=base->ProcessSmacProductionAndPopulation(GSE_CALL,roles,[&](int32_t design){
            return ResolveSmacProductionPresentation(base->m_owner,design);
        },next.colony ? next.colony->morale_mode:next.former_abandonment_tail.value_or(unit::SmacMoraleMode::Mode0),true,!next.colony && !next.former_abandonment_tail,
            next.colony ? &*next.colony:nullptr,next.former_abandonment_tail ? &*base->m_smac_upkeep_cache:nullptr,static_cast<int32_t>(answer),&publication,next.former_colony_choice==base::SmacColonyChoice::Accept);
        if(result.needs_former_owner || result.colony_pause || (!result.removed_base && !result.final_calculation)) {
            throw std::runtime_error("Former faction answer did not complete its Base attempt");
        }
        if(next.former_colony_choice && *next.former_colony_choice==base::SmacColonyChoice::Screen && !result.removed_base) {
            next.screen_base=pending.base;next.screen_request_id=pending.id;
        }
        next.colony.reset();next.former_colony_choice.reset();next.former_abandonment_tail.reset();m_colony_request.reset();
        if(result.ownership_changed) {
            CompleteCanonicalOwnershipChange(GSE_CALL,pending.base);
            if(next.stage!=CanonicalTurnContinuation::Stage::Screen) {ContinueCanonicalTurn(GSE_CALL);}
            if(!m_colony_request) {NotifyCanonicalColony(GSE_CALL);}return true;
        }
        if(result.removed_base) {++next.base_index;next.stage=CanonicalTurnContinuation::Stage::Base;}
        else {
            next.energy=result.retained_energy ? *result.retained_energy:CalculateCanonicalBaseEnergy(*base,*result.final_calculation);
            next.stage=CanonicalTurnContinuation::Stage::ResearchStart;
        }
        ContinueCanonicalTurn(GSE_CALL);if(!m_colony_request) {NotifyCanonicalColony(GSE_CALL);}
    } catch(const std::exception& error) {
        // A failed unpublished preparation leaves this explicit question retryable.
        // Errors after authority publication retain the existing terminal policy.
        if(publication) {FailCanonicalTurn(error.what());}else {NotifyCanonicalColony(GSE_CALL);}throw;
    } catch(...) {if(publication) {FailCanonicalTurn("Unknown former faction continuation failure");}throw;}
    return true;
}

void Game::ValidateCanonicalColonyScreen() const {
	if(!m_canonical_continuation || m_colony_request || m_research_request || m_canonical_turn_failed || !IsRunning()) {
		throw std::runtime_error("Colony screen has no active suspended turn");
	}
	const auto& next=*m_canonical_continuation;
	if(next.stage!=CanonicalTurnContinuation::Stage::Screen || next.colony || (next.screen_after_transfer ? bool(next.energy):!next.energy)
		|| !next.screen_base || !next.screen_request_id || *next.screen_request_id<1 || next.turn!=GetTurnId()
		|| next.owner_index>=next.owners.size()) {throw std::runtime_error("Colony screen stage differs");}
	const auto& owner=next.owners[next.owner_index];
	if(!next.screen_after_transfer && (next.base_index>=owner.second.size() || *next.screen_base!=owner.second[next.base_index])) {
		throw std::runtime_error("Colony screen traversal identity differs");
	}
	ValidateCurrentSimulation();
	const auto* base=m_bm->GetBase(*next.screen_base);
	if(!base || (!next.screen_after_transfer && base->GetSmacResourceContext().owner_simulation_id!=owner.first)) {
		throw std::runtime_error("Colony screen Base no longer exists for its owner");
	}
	if(!next.screen_after_transfer) {RequireSmacLocalHumanOwner(base->m_owner);}
}

void Game::NotifyCanonicalColonyScreen(GSE_CALLABLE) {
	// The published Screen is a stable edit boundary. Immediate UI callbacks
	// may query its real production choices, but only enqueue stamped commands.
	const bool active=m_canonical_turn_in_progress;m_canonical_turn_in_progress=false;
	util::FinallyGuard restore([&]{m_canonical_turn_in_progress=active;});
	m_state->TriggerObject(this,"colony_screen",ARGS_F(this) {
		{"pending",GetPendingSmacColonyScreen(GSE_CALL)},
	};});
}

gse::Value* Game::GetPendingSmacColonyScreen(GSE_CALLABLE) const {
	if(!m_canonical_continuation || m_canonical_continuation->stage!=CanonicalTurnContinuation::Stage::Screen
		|| m_canonical_turn_failed) {return VALUE(gse::value::Null);}
	ValidateCanonicalColonyScreen();
	const auto& next=*m_canonical_continuation;const auto* base=m_bm->GetBase(*next.screen_base);
	const gse::value::object_properties_t properties{
		{"request_id",VALUE(gse::value::Int,,*next.screen_request_id)}, {"base_id",VALUE(gse::value::Int,,base->m_id)},
		{"base_name",VALUE(gse::value::String,,base->m_name)}, {"owner_slot",VALUE(gse::value::Int,,base->m_owner->GetIndex())},
        {"read_only",VALUE(gse::value::Bool,,next.screen_after_transfer)}
	};
	return VALUE(gse::value::Object,,GSE_CALL_NOGC,properties);
}

bool Game::HandleSmacColonyScreenClose(GSE_CALLABLE,const std::string& name,
	const gse::value::object_properties_t& args,size_t caller,bool apply) {
	if(name!="close_colony_screen") {return false;}
	const auto request=args.find("request_id");
	if(caller!=m_slot_num || args.size()!=1 || request==args.end() || !request->second || request->second->type!=gse::VT_INT
		|| static_cast<gse::value::Int*>(request->second)->value<1) {throw std::runtime_error("Colony screen close requires its authenticated request");}
	if(m_canonical_turn_failed || m_canonical_turn_in_progress) {throw std::runtime_error("Colony screen is not currently available");}
	const auto id=static_cast<gse::value::Int*>(request->second)->value;
	if(!m_canonical_continuation || m_canonical_continuation->stage!=CanonicalTurnContinuation::Stage::Screen
		|| m_canonical_continuation->screen_request_id!=id) {return true;}
	ValidateCanonicalColonyScreen();
	if(!apply) {return true;}
	if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {throw std::runtime_error("Colony screen close requires its owning accumulation");}
	m_canonical_turn_in_progress=true;
	util::FinallyGuard finished([&]{m_canonical_turn_in_progress=false;});
	try {
		auto& next=*m_canonical_continuation;
		next.screen_base.reset();next.screen_request_id.reset();next.energy.reset();
		if(!next.screen_after_transfer) {++next.base_index;}next.screen_after_transfer=false;next.stage=CanonicalTurnContinuation::Stage::Base;
		NotifyCanonicalColonyScreen(GSE_CALL);
		ContinueCanonicalTurn(GSE_CALL);
	} catch(const std::exception& error) {FailCanonicalTurn(error.what());throw;}
	catch(...) {FailCanonicalTurn("Unknown Colony screen continuation failure");throw;}
	return true;
}

bool Game::QueueCanonicalColonyScreenEdit(GSE_CALLABLE,const std::string& name,
	const gse::value::object_properties_t& args) {
	if(!m_canonical_continuation || m_canonical_continuation->stage!=CanonicalTurnContinuation::Stage::Screen) {return false;}
	if(name!="work_base_tile" && name!="unwork_base_tile" && name!="select_smac_production" && name!="commit_smac_workshop" && name!="select_smac_workshop" && name!="open_smac_workshop" && name!="preset_smac_workshop" && name!="retire_smac_workshop" && name!="upgrade_smac_workshop" && name!="obsolete_smac_workshop" && name!="open_smac_workshop_rename" && name!="answer_smac_workshop_rename" && name!="open_smac_specialist" && name!="select_smac_specialist" && name!="cancel_smac_specialist" && name!="open_smac_rename" && name!="commit_smac_rename" && name!="cancel_smac_rename" && name!="edit_smac_queue" && name!="hurry_smac_production" && name!="apply_smac_governor") {return false;}
	ValidateCanonicalColonyScreen();
    if(m_canonical_continuation->screen_after_transfer) {throw std::runtime_error("Close this screen to continue after the Base changed faction");}
	const gse::value::object_properties_t stamped{
		{"request_id",VALUE(gse::value::Int,,*m_canonical_continuation->screen_request_id)},
		{"name",VALUE(gse::value::String,,name)}, {"args",VALUE(gse::value::Object,,GSE_CALL_NOGC,args)}
	};
	HandleCanonicalColonyScreenEdit(GSE_CALL,stamped,m_slot_num,false);
	// Only this producer stamps an edit. Ordinary commands extracted before a
	// question stay fenced, including when a choice opens Screen in their batch.
	AddEvent(new event::Event(this,event::Event::ES_LOCAL,m_slot_num,GSE_CALL,"_colony_screen_edit",stamped));
	return true;
}

bool Game::HandleCanonicalColonyScreenEdit(GSE_CALLABLE,const gse::value::object_properties_t& args,size_t caller,bool apply) {
	const auto request=args.find("request_id"),name=args.find("name"),data=args.find("args");
	if(caller!=m_slot_num || args.size()!=3 || request==args.end() || name==args.end() || data==args.end()
		|| !request->second || request->second->type!=gse::VT_INT || !name->second || name->second->type!=gse::VT_STRING
		|| !data->second || data->second->type!=gse::VT_OBJECT || static_cast<gse::value::Int*>(request->second)->value<1) {
		throw std::runtime_error("Malformed Colony screen edit");
	}
	if(m_canonical_turn_failed || m_canonical_turn_in_progress || m_canonical_screen_edit_in_progress) {throw std::runtime_error("Colony screen edit is not currently available");}
	// Check the request before dereferencing potentially retired Base/tile wrappers.
	if(!m_canonical_continuation || m_canonical_continuation->stage!=CanonicalTurnContinuation::Stage::Screen
		|| m_canonical_continuation->screen_request_id!=static_cast<gse::value::Int*>(request->second)->value) {return true;}
	ValidateCanonicalColonyScreen();
    if(m_canonical_continuation->screen_after_transfer) {throw std::runtime_error("Close this screen to continue after the Base changed faction");}
	const auto& command=static_cast<gse::value::String*>(name->second)->value;
	const auto& payload=static_cast<gse::value::Object*>(data->second)->value;
	const auto target=payload.find("base");
	if(target==payload.end() || !target->second || target->second->Deref()->type!=gse::VT_OBJECT
		|| static_cast<gse::value::Object*>(target->second->Deref())->object_class!=base::Base::WRAP_CLASS
		|| base::Base::Unwrap(target->second)!=m_bm->GetBase(*m_canonical_continuation->screen_base)) {
		throw std::runtime_error("Colony screen edit requires its current Base");
	}
	if(apply) {
		if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {throw std::runtime_error("Colony screen edit requires its owning accumulation");}
		m_canonical_screen_edit_in_progress=true;
	}
	util::FinallyGuard finish([&]{if(apply) {m_canonical_screen_edit_in_progress=false;}});
	if(command=="work_base_tile" || command=="unwork_base_tile") {
		if(HandleSmacWorkerEvent(GSE_CALL,command,payload,caller,apply)) {return true;}
	} else if((command=="select_smac_production" || command=="commit_smac_workshop" || command=="select_smac_workshop" || command=="open_smac_workshop" || command=="preset_smac_workshop" || command=="retire_smac_workshop" || command=="upgrade_smac_workshop" || command=="obsolete_smac_workshop" || command=="open_smac_workshop_rename" || command=="answer_smac_workshop_rename") && HandleSmacProductionEvent(GSE_CALL,command,payload,caller,apply)) {return true;}
	if((command=="open_smac_specialist" || command=="select_smac_specialist" || command=="cancel_smac_specialist") && HandleSmacSpecialistEvent(GSE_CALL,command,payload,caller,apply)) {return true;}
	if(command=="apply_smac_governor" && HandleSmacGovernorEvent(GSE_CALL,command,payload,caller,apply)) {return true;}
	if(command=="hurry_smac_production" && HandleSmacHurryEvent(GSE_CALL,command,payload,caller,apply)) {return true;}
	if(command=="edit_smac_queue" && HandleSmacQueueEvent(GSE_CALL,command,payload,caller,apply)) {return true;}
	if((command=="open_smac_rename" || command=="commit_smac_rename" || command=="cancel_smac_rename") && HandleSmacRenameEvent(GSE_CALL,command,payload,caller,apply)) {return true;}
	throw std::runtime_error("Colony screen permits only native Base management edits");
}

}
