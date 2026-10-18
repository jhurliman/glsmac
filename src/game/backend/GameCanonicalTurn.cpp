#include "Game.h"
#include "faction/SmacRankingScore.h"

#include "Player.h"
#include "faction/Faction.h"
#include "slot/Slots.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/PopDef.h"
#include "event/Event.h"
#include "map/Map.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
#include "gc/Space.h"
#include "gse/value/Int.h"
#include "gse/value/Bool.h"
#include "gse/value/Null.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include "util/FinallyGuard.h"
#include <set>
#include <algorithm>
#include <thread>

namespace game::backend {

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
		if (participant.active) {RequireSmacLocalHumanOwner(&m_state->m_slots->GetSlot(participant.slot));}
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
    return CanonicalRanking{faction::SmacRankOrder::Update(scores,active),retained};
}

void Game::AdvanceCanonicalTurn(GSE_CALLABLE,size_t turn_id) {
	RequireCanonicalTurn(turn_id);
	// Session bindings choose the worker presentation explicitly: default
	// WORKER and TALENT both carry the tile-worker flag.
	(void)GetSmacBasePresentations();
	const auto records=m_bm->GetBaseRecordOrder();
	for (const auto id:records) {
		const auto* base=m_bm->GetBase(id);
		if (!base || !base->HasSmacResourceContext()) { throw std::runtime_error("Canonical campaign contains a legacy or missing Base"); }
		RequireSmacLocalHumanOwner(base->m_owner);
	}
	// The original upkeep traverses one faction's live records at a time.
	// Ascending accepted simulation IDs are the explicit local human campaign
	// controller policy; saved record order remains authoritative within it.
	std::map<int32_t,const slot::Slot*> owners;
	for (const auto& participant:m_simulation->value.roster) {
		if (participant.active) {owners.emplace(participant.simulation_id,&m_state->m_slots->GetSlot(participant.slot));}
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
            m_simulation->value.ranking_scores=rank_order->scores;
            m_simulation->value.rank_order=rank_order->order;
        }
		// Rules such as the post-first-turn mineral deficit branch must observe
		// the incoming turn. UI notification remains after successful upkeep.
		m_current_turn.AdvanceTurn(turn_id);
		m_simulation->value.perihelion=perihelion;
		m_is_turn_complete=false;
		m_canonical_continuation=std::move(continuation);
		for (const auto& entry:m_um->GetUnits()) {
			if (!entry.second->IsConventional()) { throw std::runtime_error("Canonical upkeep contains an unsupported legacy Unit"); }
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
	if(next.stage==Stage::Screen) {ValidateCanonicalColonyScreen();return;}
	while(next.owner_index<next.owners.size()) {
		const auto& owner=next.owners[next.owner_index];
		if(next.base_index==owner.second.size()) {
			RefreshCanonicalPopulationAggregate(owner.first);
			// Native upkeep retires the shared route only AFTER this summary.
			m_map->InvalidateRegionalPathWorkspace();
			SettleCanonicalEconomy(owner.first);
			++next.owner_index;next.base_index=0;next.stage=Stage::Base;
			if(m_simulation->value.human_defeat_request) {break;} // finish this owner's tail, then the ending controller
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
	if(m_simulation->value.human_defeat_request) {
		m_state->TriggerObject(this,"smac_ending",ARGS_F(this) { }; });
		return;
	}
	CheckTurnComplete();
	if(!m_is_turn_complete) {SetTurnStatus(turn::TS_TURN_ACTIVE);}
}

void Game::RequireSmacGameplayActive() const {
	if(m_simulation && m_simulation->value.human_defeat_request) {throw std::runtime_error("Campaign has a human-defeat ending request");}
}

gse::Value* Game::GetSmacEnding(GSE_CALLABLE) const {
	if(!IsRunning() || !m_simulation || !m_simulation->value.human_defeat_request) {return VALUE(gse::value::Null);}
	ValidateCurrentSimulation();
	const auto owner=*m_simulation->value.human_defeat_request;
	const auto& roster=m_simulation->value.roster;
	const auto found=std::find_if(roster.begin(),roster.end(),[&](const auto& row){return row.simulation_id==owner;});
	if(found==roster.end() || found->active) {throw std::runtime_error("Ending request owner differs from current saved roster");}
	auto* player=m_state->m_slots->GetSlot(found->slot).GetPlayer();
	const auto name=player->GetFaction()->m_name;
	const bool local=m_slot && found->slot==static_cast<int32_t>(m_slot->GetIndex());
	return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
		{"kind",VALUE(gse::value::String,,"human_defeat")}, {"owner_simulation_id",VALUE(gse::value::Int,,owner)},
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
	const auto result=base->BeginSmacUpkeep(GSE_CALL,roles,[&](int32_t design){
		return ResolveSmacProductionPresentation(base->m_owner,design);
	},unit::SmacMoraleMode::Mode0);
	if(result.colony_pause) {
		next.colony=result.colony_pause;next.stage=CanonicalTurnContinuation::Stage::Colony;
		RequestCanonicalColony(GSE_CALL);return false;
	}
	if(result.removed_base) {++next.base_index;return true;}
	if(!result.final_calculation) {throw std::runtime_error("Colony upkeep omitted its final resource pass");}
	next.energy=CalculateCanonicalBaseEnergy(*base,*result.final_calculation);
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
	if(answer=="accept") {m_bm->ValidateSmacAbandonment(*m_bm->GetBase(m_colony_request->base));}
	if(!apply) {return true;}
	if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {throw std::runtime_error("Colony choice requires its owning accumulation");}
	m_canonical_turn_in_progress=true;
	util::FinallyGuard finished([&]{m_canonical_turn_in_progress=false;});
	try {
		auto& next=*m_canonical_continuation;const auto pending=*m_colony_request;
		auto* base=m_bm->GetBase(pending.base);
		base::Base::SmacAllocationRoles roles;const auto& configured=GetSmacBasePresentations();roles.worker=configured[0];
		for(size_t i=0;i<7;++i) {roles.specialists[i]=configured[i+1];}
		if(answer=="accept") {
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
				const auto tail=successor->ResumeSmacAbandonmentTail(GSE_CALL,*abandoned.successor_cache,roles,mode);
				if(tail.removed_base || tail.completed_unit || !tail.final_calculation) {
					throw std::runtime_error("Abandonment successor produced an unsupported tail");
				}
				next.energy=CalculateCanonicalBaseEnergy(*successor,*tail.final_calculation);
				next.stage=CanonicalTurnContinuation::Stage::ResearchStart;
			} else {next.stage=CanonicalTurnContinuation::Stage::Base;}
			ContinueCanonicalTurn(GSE_CALL);
			if(!m_colony_request) {NotifyCanonicalColony(GSE_CALL);}
			return true;
		}
		const auto selected=answer=="screen"?base::SmacColonyChoice::Screen:base::SmacColonyChoice::Delay;
		const auto result=base->ResumeSmacColonyUpkeep(GSE_CALL,*next.colony,selected,roles,[&](int32_t design){
			return ResolveSmacProductionPresentation(base->m_owner,design);
		});
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
		next.energy=CalculateCanonicalBaseEnergy(*base,*result.final_calculation);
		next.colony.reset();next.stage=CanonicalTurnContinuation::Stage::ResearchStart;
		if(selected==base::SmacColonyChoice::Screen) {next.screen_base=pending.base;next.screen_request_id=pending.id;}
		m_colony_request.reset();
		ContinueCanonicalTurn(GSE_CALL);
		if(!m_colony_request) {NotifyCanonicalColony(GSE_CALL);}
	} catch(const std::exception& error) {FailCanonicalTurn(error.what());throw;}
	catch(...) {FailCanonicalTurn("Unknown Colony continuation failure");throw;}
	return true;
}

void Game::ValidateCanonicalColonyScreen() const {
	if(!m_canonical_continuation || m_colony_request || m_research_request || m_canonical_turn_failed || !IsRunning()) {
		throw std::runtime_error("Colony screen has no active suspended turn");
	}
	const auto& next=*m_canonical_continuation;
	if(next.stage!=CanonicalTurnContinuation::Stage::Screen || next.colony || !next.energy
		|| !next.screen_base || !next.screen_request_id || *next.screen_request_id<1 || next.turn!=GetTurnId()
		|| next.owner_index>=next.owners.size()) {throw std::runtime_error("Colony screen stage differs");}
	const auto& owner=next.owners[next.owner_index];
	if(next.base_index>=owner.second.size() || *next.screen_base!=owner.second[next.base_index]) {
		throw std::runtime_error("Colony screen traversal identity differs");
	}
	ValidateCurrentSimulation();
	const auto* base=m_bm->GetBase(*next.screen_base);
	if(!base || base->GetSmacResourceContext().owner_simulation_id!=owner.first) {
		throw std::runtime_error("Colony screen Base no longer exists for its owner");
	}
	RequireSmacLocalHumanOwner(base->m_owner);
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
		{"base_name",VALUE(gse::value::String,,base->m_name)}, {"owner_slot",VALUE(gse::value::Int,,base->m_owner->GetIndex())}
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
		++next.base_index;next.stage=CanonicalTurnContinuation::Stage::Base;
		NotifyCanonicalColonyScreen(GSE_CALL);
		ContinueCanonicalTurn(GSE_CALL);
	} catch(const std::exception& error) {FailCanonicalTurn(error.what());throw;}
	catch(...) {FailCanonicalTurn("Unknown Colony screen continuation failure");throw;}
	return true;
}

bool Game::QueueCanonicalColonyScreenEdit(GSE_CALLABLE,const std::string& name,
	const gse::value::object_properties_t& args) {
	if(!m_canonical_continuation || m_canonical_continuation->stage!=CanonicalTurnContinuation::Stage::Screen) {return false;}
	if(name!="work_base_tile" && name!="unwork_base_tile" && name!="select_smac_production") {return false;}
	ValidateCanonicalColonyScreen();
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
	} else if(command=="select_smac_production" && HandleSmacProductionEvent(GSE_CALL,command,payload,caller,apply)) {return true;}
	throw std::runtime_error("Colony screen permits only native worker and production edits");
}

}
