#include "Game.h"

#include "Player.h"
#include "slot/Slots.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/FacilityDef.h"
#include "faction/SmacResearch.h"
#include "map/Map.h"
#include "gc/Space.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Int.h"
#include "gse/value/Null.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include "util/FinallyGuard.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend {
namespace {
int32_t Checked(int64_t value) {
	if(value<INT32_MIN || value>INT32_MAX) { throw std::overflow_error("Canonical research exceeds signed32"); }
	return static_cast<int32_t>(value);
}
faction::SmacFactionTechnologyState& Owner(faction::SmacTechnologyState& state,int32_t owner) {
	for(auto& value:state.factions) { if(value.admission.simulation_id==owner) {return value;} }
	throw std::runtime_error("Research owner is not admitted");
}
}

int32_t Game::GetCanonicalResearchCost(int32_t owner,const faction::SmacTechnologyState& technology) const {
	ValidateCurrentSimulation();
	const auto* state=technology.Find(owner);
	const auto* social=m_simulation->value.social.Find(owner);
	const auto& global=m_state->m_settings.global;
	if(!state || !social || !social->intrinsic || !global.GetSmacResourceRules() || !m_map) {
		throw std::runtime_error("Research cost requires current owned history, intrinsic ratings and rules");
	}
	const slot::Slot* slot=nullptr;
	for(const auto& participant:m_simulation->value.roster) {
		if(participant.simulation_id==owner) {slot=&m_state->m_slots->GetSlot(participant.slot);}
	}
	RequireSmacLocalHumanOwner(slot);
	faction::SmacResearchCostInputs inputs{};
	inputs.score=state->score; inputs.discount=state->discount; inputs.deferred=state->deferred;
	for(const auto& other:technology.factions) {
		inputs.other_scores.at(other.admission.simulation_id-1)=other.score;
		inputs.other_deferred.at(other.admission.simulation_id-1)=other.deferred;
	}
	inputs.human=true;
	inputs.owner_difficulty=unit::SmacStartupDesignState::OriginalDifficulty(slot->GetPlayer()->GetDifficultyLevel());
	inputs.global_difficulty=m_simulation->value.designs.global_difficulty;
	inputs.turn=Checked(GetTurnId()); inputs.intrinsic_research=social->intrinsic->at(10);
	inputs.discovery_rate=global.GetSmacResourceRules()->discovery_rate;
	// The validated profile domain has no TECHCOST override. Its real definition
	// constructor writes100; ordinary local campaigns currently use no Stagnation.
	inputs.technology_cost_percent=100; inputs.stagnation=false;
	inputs.map_width=Checked(m_map->GetWidth()); inputs.map_height=Checked(m_map->GetHeight());
	inputs.cache=state->cached_threshold;
	return faction::CalculateSmacResearchCost(inputs);
}

void Game::GrantCanonicalResearch(int32_t owner,int32_t id,bool clear_labs) {
	if(!m_canonical_turn_in_progress || !m_canonical_continuation || GetTurnId()==0 || id<0 || id>=89) {
		throw std::runtime_error("Research acquisition requires the owning normal-turn continuation");
	}
	const auto& global=m_state->m_settings.global;
	const auto& catalog=*global.GetSmacTechnologyRules();
	auto next=m_simulation->value.technology;
	auto& target=Owner(next,owner);
	bool first=true;
	for(const auto& other:next.factions) { if(other.owned[id]) {first=false;} }
	if(target.target==id) {target.target.reset();}
	target.owned[id]=true; target.score=Checked(int64_t(target.score)+2);
	if(id==88) {target.future_count=Checked(int64_t(target.future_count)+1);}
	if(target.discount>0) {target.discount=std::max(0,target.discount-2);}
	if(first && (catalog.technologies[id].flags & 1)) {target.pending_secrets=true;}
	if(clear_labs) {target.labs=0;}
	target.cached_threshold.reset();
	target.cached_threshold=GetCanonicalResearchCost(owner,next);
	next.Validate(catalog,*global.GetSmacFactionStartupRules());
	std::optional<map::SmacWorkerKnowledge> revelation;
	if(catalog.technologies[id].flags & 8) {revelation=PrepareSmacTechnologyReveal(owner);}
	// The full owned replacement and map knowledge are staged before either
	// authority changes. No acquisition script can observe a partial grant.
	m_simulation->value.technology.factions.swap(next.factions);
	CommitSmacTechnologyReveal(std::move(revelation));
}

void Game::NotifyCanonicalResearch(GSE_CALLABLE) {
	m_state->TriggerObject(this,"research_choice",ARGS_F(this) { }; });
}

bool Game::RequestCanonicalResearch(GSE_CALLABLE,int32_t owner,bool bonus) {
	const auto* state=m_simulation->value.technology.Find(owner);
	if(!state) {throw std::runtime_error("Research choice has no current owner");}
	auto choices=faction::SmacManualResearchPool(*state,*m_state->m_settings.global.GetSmacTechnologyRules());
	if(choices.empty()) {return false;} // the measured no-choice answer awards nothing
	if(m_next_research_request==INT64_MAX) {throw std::overflow_error("Research request identity exhausted");}
	m_research_request=CanonicalResearchRequest{++m_next_research_request,owner,bonus,std::move(choices)};
	SetTurnStatus(turn::TS_PLEASE_WAIT);
	NotifyCanonicalResearch(GSE_CALL);
	return true;
}

bool Game::ContinueCanonicalResearch(GSE_CALLABLE) {
	using Stage=CanonicalTurnContinuation::Stage;
	auto& continuation=*m_canonical_continuation;
	const auto owner=continuation.owners.at(continuation.owner_index).first;
	if(continuation.stage==Stage::ResearchStart) {
		const auto* base=m_bm->GetBase(continuation.owners.at(continuation.owner_index).second.at(continuation.base_index));
		if(!base || base->GetSmacResourceContext().owner_simulation_id!=owner) {throw std::runtime_error("Research Base identity changed");}
		const auto* social=m_simulation->value.social.Find(owner);
		auto& saved=m_simulation->value.economy.at(owner);
		bool sphere=false;
		for(const auto& entry:base->m_facilities) {if(entry.second->m_smac_source_id==23) {sphere=true;}}
		const auto difficulty=unit::SmacStartupDesignState::OriginalDifficulty(base->m_owner->GetPlayer()->GetDifficultyLevel());
		const auto scaled=faction::ScaleSmacResearchSubmission(continuation.energy->tail.final.labs,social->current[10],
			Checked(GetTurnId()),difficulty,sphere,saved.research_fraction);
		auto& technology=Owner(m_simulation->value.technology,owner);
		const auto labs=scaled.increment>0?Checked(int64_t(technology.labs)+scaled.increment):technology.labs;
		const bool had_target=technology.target.has_value();
		const auto threshold=scaled.increment>0 && had_target?GetCanonicalResearchCost(owner,m_simulation->value.technology):0;
		technology.labs=labs; saved.research_fraction=scaled.fraction;
		continuation.stage=Stage::Credit;
		if(scaled.increment<=0) {return true;}
		continuation.stage=Stage::ResearchFinish;
		if(had_target) {
			if(labs>=threshold) {GrantCanonicalResearch(owner,*technology.target,true);}
			// Grant may replace the vector, so reacquire its owned record.
			auto& after=Owner(m_simulation->value.technology,owner);
			if(after.pending_secrets) {after.pending_secrets=false; continuation.stage=Stage::ResearchBonus;}
		}
	}
	if(continuation.stage==Stage::ResearchBonus) {
		const auto selected=Owner(m_simulation->value.technology,owner).target;
		if(selected) {GrantCanonicalResearch(owner,*selected,false);}
		else if(RequestCanonicalResearch(GSE_CALL,owner,true)) {return false;}
		continuation.stage=Stage::ResearchFinish;
	}
	if(continuation.stage==Stage::ResearchFinish) {
		const auto& technology=Owner(m_simulation->value.technology,owner);
		if(technology.labs>=0 && !technology.target && RequestCanonicalResearch(GSE_CALL,owner,false)) {return false;}
		continuation.stage=Stage::Credit;
	}
	return true;
}

gse::Value* Game::GetPendingSmacResearch(GSE_CALLABLE) const {
	if(!m_research_request || m_canonical_turn_failed) {return VALUE(gse::value::Null);}
	ValidateCurrentSimulation();
	const auto& request=*m_research_request;
	const slot::Slot* owner=nullptr;
	for(const auto& participant:m_simulation->value.roster) {
		if(participant.simulation_id==request.owner) {owner=&m_state->m_slots->GetSlot(participant.slot);}
	}
	if(!owner) {throw std::runtime_error("Research request owner is outside the current roster");}
	RequireSmacLocalHumanOwner(owner);
	gse::value::array_elements_t choices;
	const auto& catalog=*m_state->m_settings.global.GetSmacTechnologyRules();
	constexpr size_t axes[]{3,1,2,0};
	for(const auto id:request.choices) {
		const auto& row=catalog.technologies[id]; size_t category=0;
		for(size_t i=1;i<4;++i) {if(row.priorities[axes[i]]>row.priorities[axes[category]]) {category=i;}}
		gse::value::object_properties_t properties{
			{"id",VALUE(gse::value::Int,,id)}, {"name",VALUE(gse::value::String,,row.name)},
			{"category",VALUE(gse::value::Int,,category)}
		};
		choices.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,properties));
	}
	gse::value::object_properties_t properties{
		{"request_id",VALUE(gse::value::Int,,request.id)}, {"owner_slot",VALUE(gse::value::Int,,owner->GetIndex())},
		{"owner_name",VALUE(gse::value::String,,owner->GetPlayer()->GetPlayerName())},
		{"bonus",VALUE(gse::value::Bool,,request.bonus)}, {"choices",VALUE(gse::value::Array,,choices)}
	};
	return VALUE(gse::value::Object,,GSE_CALL_NOGC,properties);
}

bool Game::HandleSmacResearchEvent(GSE_CALLABLE,const std::string& name,
	const gse::value::object_properties_t& args,size_t caller,bool apply) {
	if(name!="choose_research") {return false;}
	const auto request=args.find("request_id"),choice=args.find("technology_id");
	if(caller!=m_slot_num || args.size()!=2 || request==args.end() || choice==args.end()
		|| !request->second || request->second->type!=gse::VT_INT || !choice->second || choice->second->type!=gse::VT_INT) {
		throw std::runtime_error("Research selection requires an authenticated request and technology identity");
	}
	const auto request_id=static_cast<gse::value::Int*>(request->second)->value;
	const auto selected=static_cast<gse::value::Int*>(choice->second)->value;
	if(request_id<1 || selected<0 || selected>=89) {throw std::runtime_error("Invalid research selection identity");}
	if(m_canonical_turn_failed || m_canonical_turn_in_progress) {throw std::runtime_error("Research selection is not currently available");}
	if(!m_research_request || m_research_request->id!=request_id) {return true;} // retired/duplicate button
	if(!m_canonical_continuation || !IsRunning()) {throw std::runtime_error("Research selection has no suspended turn");}
	ValidateCurrentSimulation(); RequireSmacLocalHumanOwner(m_slot);
	const auto& pending=*m_research_request;
	if(std::find(pending.choices.begin(),pending.choices.end(),selected)==pending.choices.end()) {throw std::runtime_error("Technology was not offered by this request");}
	const auto pool=faction::SmacManualResearchPool(*m_simulation->value.technology.Find(pending.owner),*m_state->m_settings.global.GetSmacTechnologyRules());
	if(pool!=pending.choices) {throw std::runtime_error("Research choice context changed while suspended");}
	if(!apply) {return true;}
	if(!GetGCSpace()->IsAccumulating()) {throw std::runtime_error("Research selection requires the owning accumulation");}
	m_canonical_turn_in_progress=true;
	util::FinallyGuard finished([&]{m_canonical_turn_in_progress=false;});
	try {
		const auto owner=pending.owner; const bool bonus=pending.bonus;
		if(bonus) {GrantCanonicalResearch(owner,static_cast<int32_t>(selected),false);}
		else {Owner(m_simulation->value.technology,owner).target=static_cast<int32_t>(selected);}
		m_canonical_continuation->stage=bonus?CanonicalTurnContinuation::Stage::ResearchFinish:CanonicalTurnContinuation::Stage::Credit;
		m_research_request.reset();
		ContinueCanonicalTurn(GSE_CALL);
		if(!m_research_request) {NotifyCanonicalResearch(GSE_CALL);}
	} catch(const std::exception& error) {FailCanonicalTurn(error.what()); throw;}
	catch(...) {FailCanonicalTurn("Unknown research continuation failure"); throw;}
	return true;
}

}
