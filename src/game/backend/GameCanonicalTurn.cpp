#include "Game.h"

#include "Player.h"
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
#include "util/FinallyGuard.h"

namespace game::backend {

void Game::RequireCanonicalStartupTurn( size_t turn_id ) const {
	if ( !m_map || !m_map->m_worker_knowledge ) { return; }
	RequireCanonicalTurn(turn_id);
	if (m_current_turn.GetId()!=0) { throw std::runtime_error("Canonical startup already reached its first turn"); }
}

void Game::RequireCanonicalTurn(size_t turn_id) const {
	if (!IsRunning() || !m_state || !m_map || !m_map->m_worker_knowledge || !m_map->m_worker_knowledge_ready
		|| m_canonical_turn_failed || m_canonical_turn_in_progress || !GetGCSpace()->IsAccumulating()
		|| m_current_turn.GetId()>=INT32_MAX || turn_id!=m_current_turn.GetId()+1) {
		throw std::runtime_error("Canonical transition requires its exact next turn and ready owning world");
	}
	RequireSmacLocalHumanOwner(m_slot);
	ValidateCurrentSimulation();
	for (const auto& participant:m_simulation->value.roster) {
		RequireSmacLocalHumanOwner(&m_state->m_slots->GetSlot(participant.slot));
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
	RequireCanonicalTurn(static_cast<size_t>(next));
	if (apply) { CheckRW(GSE_CALL); AdvanceCanonicalTurn(GSE_CALL,static_cast<size_t>(next)); }
	return true;
}

void Game::AdvanceCanonicalTurn(GSE_CALLABLE,size_t turn_id) {
	RequireCanonicalTurn(turn_id);
	// Resolve semantic Pop roles once, with no display-name inference. These
	// are already the registered authority used by founding/manual allocation.
	base::Base::SmacAllocationRoles roles;
	for (const auto& entry:m_bm->GetBasePopDefs()) {
		const auto* def=entry.second;
		m_bm->ValidateSmacPopDef(def);
		if (def->m_flags & base::PopDef::PF_TILE_WORKER) {
			if (!roles.worker.empty()) { throw std::runtime_error("Canonical upkeep has ambiguous worker presentation"); }
			roles.worker=entry.first;
		} else if (def->m_smac_specialist_id) {
			auto& role=roles.specialists.at(*def->m_smac_specialist_id);
			if (!role.empty()) { throw std::runtime_error("Canonical upkeep has ambiguous specialist presentation"); }
			role=entry.first;
		}
	}
	if (roles.worker.empty()) { throw std::runtime_error("Canonical upkeep lacks its worker role"); }
	for (const auto& role:roles.specialists) { if(role.empty()) { throw std::runtime_error("Canonical upkeep lacks a specialist role"); } }
	const auto order=m_bm->GetBaseRecordOrder();
	for (const auto id:order) {
		const auto* base=m_bm->GetBase(id);
		if (!base || !base->HasSmacResourceContext()) { throw std::runtime_error("Canonical campaign contains a legacy or missing Base"); }
		RequireSmacLocalHumanOwner(base->m_owner);
	}
	m_canonical_turn_in_progress=true;
	util::FinallyGuard finished([&]{m_canonical_turn_in_progress=false;});
	try {
		RefreshSimulationForTurn(turn_id);
		// Rules such as the post-first-turn mineral deficit branch must observe
		// the incoming turn. UI notification remains after successful upkeep.
		m_current_turn.AdvanceTurn(turn_id);
		m_is_turn_complete=false;
		for (const auto& entry:m_um->GetUnits()) {
			if (!entry.second->IsConventional()) { throw std::runtime_error("Canonical upkeep contains an unsupported legacy Unit"); }
			m_um->ResetConventionalSpent(GSE_CALL,entry.second);
		}
		for (const auto id:order) {
			auto* base=m_bm->GetBase(id);
			const auto result=base->ProcessSmacUpkeep(GSE_CALL,roles,[&](int32_t design){
				return ResolveSmacProductionPresentation(base->m_owner,design);
			},unit::SmacMoraleMode::Mode0);
			if (!result.final_calculation) { throw std::runtime_error("Canonical upkeep omitted its final resource pass"); }
			m_bm->RefreshBase(base);
		}
		for (const auto& slot:m_state->m_slots->GetSlots()) {
			if (slot.GetState()==slot::Slot::SS_PLAYER) { slot.GetPlayer()->UncompleteTurn(); }
		}
		auto fr=FrontendRequest(FrontendRequest::FR_TURN_ADVANCE); fr.data.turn_advance.turn_id=turn_id; AddFrontendRequest(fr);
		m_state->TriggerObject(this,"turn",ARGS_F(this) {
			{"year",VALUE(gse::value::Int,,m_current_turn.GetId()+2100)},
		}; });
		CheckTurnComplete();
		if (!m_is_turn_complete) { SetTurnStatus(turn::TS_TURN_ACTIVE); }
	} catch (const std::exception& error) {
		FailCanonicalTurn(error.what()); throw;
	} catch (...) {
		FailCanonicalTurn("Unknown canonical upkeep failure"); throw;
	}
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

}
