#include "Game.h"

#include "Player.h"
#include "slot/Slots.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/FacilityDef.h"
#include "faction/SmacResearch.h"
#include "faction/SmacResearchValue.h"
#include "faction/SmacAutomaticResearch.h"
#include "faction/SmacDominance.h"
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

void Game::GrantCanonicalResearch(int32_t owner,int32_t id,bool clear_labs,bool defer_threshold) {
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
	if(!defer_threshold) {target.cached_threshold.reset();target.cached_threshold=GetCanonicalResearchCost(owner,next);}
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

faction::SmacHumanResearchInputs Game::PrepareCanonicalHumanResearchInputs(int32_t owner,bool blind) const {
    ValidateCurrentSimulation();
    if(!m_simulation || !m_map) {throw std::runtime_error("Research valuation requires a bound world");}
    const auto& state=m_simulation->value;const auto& global=m_state->m_settings.global;
    const auto found=std::find_if(state.roster.begin(),state.roster.end(),[&](const auto& row){return row.simulation_id==owner;});
    const auto* participant=found==state.roster.end() ? nullptr:&*found;
    if(!participant || !participant->active) {throw std::runtime_error("Research valuation has no active participant");}
    RequireSmacLocalHumanOwner(&m_state->m_slots->GetSlot(participant->slot));
    const auto* social=state.social.Find(owner);const auto* profile=global.GetSmacFactionStartupRules()->Find(participant->faction_id);
    const auto& topology=m_map->GetTerritoryTopology();
    if(!global.IsOrdinarySmacCampaign() || state.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain
        || !state.strategy || !state.strategy->count(owner) || !state.capabilities || !state.regional_strategy
        || !state.retained_base_totals || !state.ecology_history || !state.ecology_world || !state.victory_history
        || !social || !social->intrinsic || !profile || !topology || !topology->Summary() || !topology->Areas()) {
        throw std::runtime_error("Human research valuation lacks retained ordinary campaign inputs");
    }
    const auto& technologies=*global.GetSmacTechnologyRules();const auto& catalog=global.GetSmacPredefinedUnitRules()->catalog;
    const auto resolve=[](const settings::SmacTechnologyReference& ref)->int32_t {
        using Kind=settings::SmacTechnologyReference::Kind;
        if(ref.kind==Kind::NONE) {return -1;}if(ref.kind==Kind::DISABLED) {return -2;}
        if(ref.kind==Kind::TECHNOLOGY_ID && ref.technology_id) {return *ref.technology_id;}
        throw std::runtime_error("Research valuation has an unresolved prerequisite");
    };
    faction::SmacHumanResearchInputs input{};input.faction_id=owner;input.blind=blind;
    for(size_t i=0;i<input.technologies.size();++i) {
        const auto& row=technologies.technologies[i];auto& out=input.technologies[i];out.ratings=row.priorities;out.flags=row.flags;
        for(size_t j=0;j<2;++j) {out.prerequisites[j]=resolve(technologies.ResolveToken(row.prerequisites[j]));}
        for(const auto& peer:state.technology.factions) {if(peer.owned[i]) {out.possession|=uint32_t(1)<<peer.admission.simulation_id;}}
    }
    input.facility_prerequisites.fill(-1);
    for(size_t i=1;i<input.facility_prerequisites.size();++i) {
        input.facility_prerequisites[i]=resolve(technologies.ResolveToken(global.GetSmacProductionRules()->facilities.at(i-1).prerequisite));
    }
    for(size_t i=0;i<2;++i) {input.reactor_prerequisites[i]=resolve(technologies.ResolveToken(catalog.reactors.reactors.at(i+1).prerequisite));}
    // Positional original records: predefined Formers1 and Foil chassis3.
    // Resolve their current catalog tokens instead of hardcoding technology IDs.
    input.former_prerequisite=resolve(catalog.units.at(1).prerequisite);
    input.foil_prerequisite=resolve(technologies.ResolveToken(catalog.chassis.chassis.at(3).prerequisite));
    for(size_t i=0;i<input.weapons.size();++i) {const auto& row=catalog.weapons.weapons.at(i);
        const auto raw=static_cast<uint8_t>(row.offense);
        input.weapons[i]={raw<=127 ? int32_t(raw):int32_t(raw)-256,resolve(technologies.ResolveToken(row.prerequisite))};}
    input.priorities=state.strategy->at(owner).priorities;input.base_count=state.retained_base_totals->at(owner);
    input.intrinsic_planet=social->intrinsic->at(7);input.intrinsic_probe=social->intrinsic->at(8);
    // Frozen ordinary faction admission has no PSI scalar writer (constructor0).
    input.psi_bonus=0;
    for(const auto& bonus:profile->bonuses) {if(bonus.kind==settings::SmacFactionStartupBonus::Kind::POPULATION) {input.population_bonus=bonus.parameter;}}
    const auto& capability=state.capabilities->factions.at(owner-1);
    input.maximum_attack=capability.available[faction::SmacCapabilityState::Weapon];
    input.maximum_defense=capability.foreign[faction::SmacCapabilityState::Weapon];
    input.turn=Checked(GetTurnId());input.cloud_cover=state.ecology_world->cloud_cover;
    input.ecology_aggregate=state.ecology_history->research_aggregate.at(owner);
    if(input.base_count<0) {throw std::runtime_error("Research valuation has a negative retained Base count");}
    if(state.victory_history->voice_host!=-1) {throw std::runtime_error("No-project research cannot admit Voice ownership");}
    faction::SmacVictoryContext victory;victory.turn=input.turn;victory.humans=state.human_membership;
    victory.economic_deadlines=state.victory_history->economic_deadlines;
    input.endgame_pressure=faction::SmacVictoryPressure(victory,{{settings::SmacTechnologyReference::Kind::UNKNOWN,{}},-1,-1});
    for(size_t peer=0;peer<8;++peer) {input.relations[peer]=state.hostility.owners.at(owner).directions[peer].primary;}
    for(size_t i=0;i<128;++i) {
        input.regional_known_tiles[i]=topology->Summary()->owners[owner].known[i];
        input.regional_strategy[i]=state.regional_strategy->at(owner)[i];
        input.region_area[i]=static_cast<uint32_t>(topology->Areas()->at(i));
        for(const auto& peer:state.economy) {
            if(!peer.second.regional_base_counts) {throw std::runtime_error("Research valuation lacks retained peer Base counts");}
            input.regional_bases.at(peer.first)[i]=peer.second.regional_base_counts->at(i);
        }
    }
    return input; // Query only: never refresh summaries, grant ownership or draw RNG.
}

bool Game::IsCanonicalBlindResearch() const {
    const auto& rules=m_state->m_settings.global.GetSmacCampaignRules();
    return rules && rules->BlindResearch() && !(m_canonical_continuation && m_canonical_continuation->research_legacy_manual);
}
void Game::ApplyCanonicalBlindResearch(int32_t owner,std::optional<uint8_t> mask) {
    auto input=PrepareCanonicalHumanResearchInputs(owner,true);
    auto priorities=m_simulation->value.strategy->at(owner);priorities.ApplyResearchPriorities(mask);
    input.priorities=priorities.priorities;auto random=m_simulation->value.gameplay_random;
    const faction::SmacAutomaticResearchInputs selection{owner,true,true,input.technologies,input.priorities,input.former_prerequisite};
    const auto selected=faction::SelectSmacAutomaticResearch(selection,random,[&](int32_t id){return faction::CalculateSmacHumanResearchValue(input,id);});
    auto& technology=Owner(m_simulation->value.technology,owner);
    m_simulation->value.strategy->at(owner)=priorities;m_simulation->value.gameplay_random=random;
    technology.target=selected<0 ? std::nullopt:std::optional<int32_t>(selected);
}
void Game::ValidateCanonicalResearchQuestion(const CanonicalResearchRequest& request) const {
    if(request.blind!=IsCanonicalBlindResearch()) {throw std::runtime_error("Research question mode changed while suspended");}
    if(request.blind) {
        const auto input=PrepareCanonicalHumanResearchInputs(request.owner,true);
        if(!request.choices.empty() || input.priorities!=request.priorities) {throw std::runtime_error("Research priorities changed while suspended");}
    } else if(request.choices!=faction::SmacManualResearchPool(*m_simulation->value.technology.Find(request.owner),*m_state->m_settings.global.GetSmacTechnologyRules())) {
        throw std::runtime_error("Research choice context changed while suspended");
    }
}

bool Game::RequestCanonicalResearch(GSE_CALLABLE,int32_t owner,bool bonus) {
	const auto* state=m_simulation->value.technology.Find(owner);
	if(!state) {throw std::runtime_error("Research choice has no current owner");}
    const bool blind=IsCanonicalBlindResearch();
    std::vector<int32_t> choices;std::array<int32_t,4> priorities{};
    if(blind) {
        if(owner!=RequireSimulationParticipant(m_player).simulation_id) {throw std::runtime_error("Blind research requires the focused local human; hotseat focus is not implemented");}
        const auto input=PrepareCanonicalHumanResearchInputs(owner,true);priorities=input.priorities;
        if(!(GetTurnId()<=11 || (state->score/2)%5==1)) {ApplyCanonicalBlindResearch(owner,std::nullopt);return false;}
    } else {
        choices=faction::SmacManualResearchPool(*state,*m_state->m_settings.global.GetSmacTechnologyRules());
        if(choices.empty()) {return false;}
    }
	if(m_next_research_request==INT64_MAX) {throw std::overflow_error("Research request identity exhausted");}
	m_research_request=CanonicalResearchRequest{++m_next_research_request,owner,bonus,std::move(choices),blind,priorities};
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
			if(labs>=threshold) {
                if(IsCanonicalBlindResearch()) {GrantCanonicalResearch(owner,*technology.target,false,true);continuation.stage=Stage::ResearchOrdinarySelect;}
                else {GrantCanonicalResearch(owner,*technology.target,true);}
            }
			// Grant may replace the vector, so reacquire its owned record.
			auto& after=Owner(m_simulation->value.technology,owner);
			if(continuation.stage==Stage::ResearchFinish && after.pending_secrets) {after.pending_secrets=false; continuation.stage=Stage::ResearchBonus;}
		}
	}
    if(continuation.stage==Stage::ResearchOrdinarySelect) {
        if(RequestCanonicalResearch(GSE_CALL,owner,false)) {return false;}
        continuation.stage=Stage::ResearchOrdinaryReset;
    }
    if(continuation.stage==Stage::ResearchOrdinaryReset) {
        auto& after=Owner(m_simulation->value.technology,owner);after.labs=0;after.cached_threshold.reset();
        after.cached_threshold=GetCanonicalResearchCost(owner,m_simulation->value.technology);
        continuation.stage=Stage::ResearchFinish;
        if(after.pending_secrets) {after.pending_secrets=false;continuation.stage=Stage::ResearchBonus;}
    }
    if(continuation.stage==Stage::ResearchBonus) {
        auto selected=Owner(m_simulation->value.technology,owner).target;
        if(!selected) {
            if(RequestCanonicalResearch(GSE_CALL,owner,true)) {return false;}
            selected=Owner(m_simulation->value.technology,owner).target;
        }
        if(selected) {GrantCanonicalResearch(owner,*selected,false);}
        continuation.stage=selected && IsCanonicalBlindResearch() ? Stage::ResearchBonusSelect:Stage::ResearchFinish;
    }
    if(continuation.stage==Stage::ResearchBonusSelect) {
        if(RequestCanonicalResearch(GSE_CALL,owner,false)) {return false;}
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
    gse::value::array_elements_t priorities;for(const auto value:request.priorities) {priorities.push_back(VALUE(gse::value::Int,,value));}
	gse::value::object_properties_t properties{
        {"blind",VALUE(gse::value::Bool,,request.blind)}, {"priorities",VALUE(gse::value::Array,,priorities)},
		{"request_id",VALUE(gse::value::Int,,request.id)}, {"owner_slot",VALUE(gse::value::Int,,owner->GetIndex())},
		{"owner_name",VALUE(gse::value::String,,owner->GetPlayer()->GetPlayerName())},
		{"bonus",VALUE(gse::value::Bool,,request.bonus)}, {"choices",VALUE(gse::value::Array,,choices)}
	};
	return VALUE(gse::value::Object,,GSE_CALL_NOGC,properties);
}

bool Game::HandleSmacResearchEvent(GSE_CALLABLE,const std::string& name,
	const gse::value::object_properties_t& args,size_t caller,bool apply) {
    if(name=="set_research_priorities") {
        const auto request=args.find("request_id"),answer=args.find("mask");
        if(caller!=m_slot_num || args.size()!=2 || request==args.end() || answer==args.end() || !request->second
            || request->second->type!=gse::VT_INT || !answer->second || (answer->second->type!=gse::VT_INT && answer->second->type!=gse::VT_NULL)) {
            throw std::runtime_error("Research priorities require an authenticated request and mask or cancel");
        }
        const auto id=static_cast<gse::value::Int*>(request->second)->value;std::optional<uint8_t> mask;
        if(answer->second->type==gse::VT_INT) {const auto value=static_cast<gse::value::Int*>(answer->second)->value;
            if(value<0 || value>15) {throw std::runtime_error("Research priority mask is outside four axes");}mask=static_cast<uint8_t>(value);}
        if(id<1) {throw std::runtime_error("Invalid priority request identity");}
        if(m_canonical_turn_failed || m_canonical_turn_in_progress) {throw std::runtime_error("Research priorities are not available");}
        if(!m_research_request || m_research_request->id!=id) {return true;}
        if(!IsRunning() || !m_canonical_continuation || !m_research_request->blind) {throw std::runtime_error("No suspended Blind priority question");}
        ValidateCanonicalResearchQuestion(*m_research_request);
        if(!apply) {return true;}
        if(!GetGCSpace()->IsAccumulating()) {throw std::runtime_error("Research priorities require the owning accumulation");}
        m_canonical_turn_in_progress=true;util::FinallyGuard finished([&]{m_canonical_turn_in_progress=false;});
        try {
            const auto pending=*m_research_request;const auto stage=m_canonical_continuation->stage;
            ApplyCanonicalBlindResearch(pending.owner,mask);
            using Stage=CanonicalTurnContinuation::Stage;
            m_canonical_continuation->stage=pending.bonus ? Stage::ResearchBonus:stage==Stage::ResearchOrdinarySelect ? Stage::ResearchOrdinaryReset
                :stage==Stage::ResearchBonusSelect ? Stage::ResearchFinish:Stage::Credit;
            m_research_request.reset();ContinueCanonicalTurn(GSE_CALL);if(!m_research_request) {NotifyCanonicalResearch(GSE_CALL);}
        } catch(const std::exception& error) {FailCanonicalTurn(error.what());throw;}
        catch(...) {FailCanonicalTurn("Unknown Blind continuation failure");throw;}
        return true;
    }
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
    if(pending.blind) {throw std::runtime_error("Blind research accepts priorities, not a chosen technology");}
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
		m_research_request.reset();m_canonical_continuation->research_legacy_manual=false;
		ContinueCanonicalTurn(GSE_CALL);
		if(!m_research_request) {NotifyCanonicalResearch(GSE_CALL);}
	} catch(const std::exception& error) {FailCanonicalTurn(error.what()); throw;}
	catch(...) {FailCanonicalTurn("Unknown research continuation failure"); throw;}
	return true;
}

}
