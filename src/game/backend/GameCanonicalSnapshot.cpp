#include "Game.h"
#include "State.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/FacilityDef.h"
#include "base/SmacBaseUpkeepCache.h"
#include "base/SmacBaseEnergy.h"
#include "faction/SmacResearch.h"
#include "types/Buffer.h"
#include "util/FinallyGuard.h"
#include <limits>
#include <set>
#include <stdexcept>

namespace game::backend {
namespace {
size_t ReadIdentity(types::Buffer& buffer) {
    const auto value=buffer.ReadInt();
    if(value<0 || static_cast<uint64_t>(value)>std::numeric_limits<size_t>::max()) {
        throw std::runtime_error("Invalid saved Colony identity or cursor");
    }
    return static_cast<size_t>(value);
}
}

types::Buffer Game::SerializeCanonicalContinuation() const {
    if(m_canonical_turn_in_progress || m_canonical_turn_failed || m_pending_canonical_continuation) {
        throw std::runtime_error("Canonical transition is not at a stable snapshot boundary");
    }
    types::Buffer buffer;buffer.WriteInt(6);buffer.WriteBool(m_canonical_continuation.has_value());
    if(!m_canonical_continuation) {
        if(m_colony_request || m_research_request) {throw std::runtime_error("Orphaned canonical choice cannot be saved");}
        return buffer;
    }
    const auto& next=*m_canonical_continuation;
    ValidateCanonicalContinuation(next);
    const bool colony=next.stage==CanonicalTurnContinuation::Stage::Colony;
    const bool former=next.stage==CanonicalTurnContinuation::Stage::FormerOwner;
    if(former) {ValidateCanonicalFormerOwner();}
    else if(colony) {ValidateCanonicalColony();}
    else {
        const auto owner=next.owners.at(next.owner_index).first;
        if(m_colony_request || !m_research_request || m_research_request->owner!=owner ||
            bool(next.screen_base)!=bool(next.screen_request_id) ||
            m_research_request->bonus!=(next.stage==CanonicalTurnContinuation::Stage::ResearchBonus)) {
            throw std::runtime_error("Saved research question differs from its suspended turn");
        }
        ValidateCanonicalResearchQuestion(*m_research_request);
    }
    buffer.WriteInt(former?3:colony?0:next.stage==CanonicalTurnContinuation::Stage::ResearchOrdinarySelect?4
        :next.stage==CanonicalTurnContinuation::Stage::ResearchBonusSelect?5:next.stage==CanonicalTurnContinuation::Stage::ResearchBonus?2:1);
    if(!former && !colony) {buffer.WriteBool(next.research_legacy_manual);}
    buffer.WriteInt(next.turn);buffer.WriteInt(next.owner_index);buffer.WriteInt(next.base_index);
    buffer.WriteInt(next.owners.size());
    for(const auto& owner:next.owners) {
        buffer.WriteInt(owner.first);buffer.WriteInt(owner.second.size());
        for(const auto id:owner.second) {buffer.WriteInt(id);}
    }
    if(former) {buffer.WriteInt(next.colony ? 1:next.former_abandonment_tail ? 2:0);}
    if(colony || (former && next.colony)) {
        base::WriteSmacResourceCalculation(buffer,next.colony->first_calculation);
        buffer.WriteInt(static_cast<int64_t>(next.colony->morale_mode));
        if(former) {buffer.WriteInt(static_cast<int64_t>(*next.former_colony_choice));}
    }
    else if(former && next.former_abandonment_tail) {buffer.WriteInt(static_cast<int64_t>(*next.former_abandonment_tail));}
    else if(!former) {
        base::WriteSmacBaseEnergy(buffer,*next.energy);
        buffer.WriteBool(next.screen_base.has_value());if(next.screen_base) {buffer.WriteInt(*next.screen_base);}
    }
    return buffer;
}

std::optional<Game::CanonicalTurnContinuation> Game::ReadCanonicalContinuation(types::Buffer buffer,size_t turn) const {
    const auto version=buffer.ReadInt();
    if(version!=1 && version!=2 && version!=3 && version!=4 && version!=5 && version!=6) {throw std::runtime_error("Unsupported canonical continuation");}
    std::optional<CanonicalTurnContinuation> result;
    if(buffer.ReadBool()) {
        if(!m_state || m_state->GetConnection() || m_state->m_settings.global.GetRulesetId()!="smac") {
            throw std::runtime_error("Saved Colony choice requires a local SMAC campaign");
        }
        result=CanonicalTurnContinuation{turn};auto& next=*result;
        const auto kind=version==1?0:buffer.ReadInt();
        if(kind<0 || kind>(version>=6?5:version>=5?3:2)) {throw std::runtime_error("Invalid saved canonical choice kind");}
        next.stage=kind==3?CanonicalTurnContinuation::Stage::FormerOwner:kind==0?CanonicalTurnContinuation::Stage::Colony
            :kind==4?CanonicalTurnContinuation::Stage::ResearchOrdinarySelect:kind==5?CanonicalTurnContinuation::Stage::ResearchBonusSelect
            :kind==1?CanonicalTurnContinuation::Stage::ResearchFinish:CanonicalTurnContinuation::Stage::ResearchBonus;
        if(kind!=0 && kind!=3) {
            next.research_legacy_manual=version>=6 ? buffer.ReadBool():true;
            if(kind>=4 && next.research_legacy_manual) {throw std::runtime_error("Blind research tail cannot be a legacy manual question");}
        }
        next.turn=ReadIdentity(buffer);next.owner_index=ReadIdentity(buffer);next.base_index=ReadIdentity(buffer);
        const auto owners=ReadIdentity(buffer);
        if(next.turn!=turn || !turn || turn>INT32_MAX || !owners || owners>7 || next.owner_index>=owners) {
            throw std::runtime_error("Invalid saved Colony turn or owner cursor");
        }
        std::set<size_t> ids;int32_t previous=0;
        for(size_t i=0;i<owners;++i) {
            const auto owner=buffer.ReadInt();const auto count=ReadIdentity(buffer);
            if(owner<=previous || owner>7 || buffer.lenr>buffer.lenw || count>(buffer.lenw-buffer.lenr)/14u) {
                throw std::runtime_error("Invalid saved Colony owner traversal");
            }
            previous=static_cast<int32_t>(owner);
            next.owners.push_back({previous,{}});auto& bases=next.owners.back().second;bases.reserve(count);
            for(size_t j=0;j<count;++j) {
                const auto id=ReadIdentity(buffer);
                if(!ids.insert(id).second) {throw std::runtime_error("Repeated Base in saved Colony traversal");}
                bases.push_back(id);
            }
        }
        if(next.base_index>=next.owners[next.owner_index].second.size()) {throw std::runtime_error("Saved Colony Base cursor is outside its owner");}
        const auto resume=kind==3 ? buffer.ReadInt():0;
        if(resume<0 || resume>2) {throw std::runtime_error("Invalid saved former faction resume tag");}
        const bool has_colony=kind==0 || (kind==3 && resume==1);
        if(has_colony) {
            const auto calculation=base::ReadSmacResourceCalculation(buffer,version>=3,version>=4);
            const auto mode=buffer.ReadInt();
            if(mode<0 || mode>1) {throw std::runtime_error("Invalid saved Colony morale mode");}
            next.colony=base::SmacColonyUpkeepPause{calculation,static_cast<unit::SmacMoraleMode>(mode)};
            if(kind==3) {
                const auto choice=buffer.ReadInt();
                if(choice!=static_cast<int64_t>(base::SmacColonyChoice::Delay) && choice!=static_cast<int64_t>(base::SmacColonyChoice::Screen)) {
                    throw std::runtime_error("Invalid saved former faction Colony answer");
                }
                next.former_colony_choice=static_cast<base::SmacColonyChoice>(choice);
            }
        }
        else if(kind==3 && resume==2) {
            const auto mode=buffer.ReadInt();if(mode<0 || mode>1) {throw std::runtime_error("Invalid saved successor-tail morale mode");}
            next.former_abandonment_tail=static_cast<unit::SmacMoraleMode>(mode);
        }
        else if(kind!=3) {
            next.energy=base::ReadSmacBaseEnergy(buffer);
            if(buffer.ReadBool()) {next.screen_base=ReadIdentity(buffer);}
        }
    }
    if(buffer.lenr!=buffer.lenw) {throw std::runtime_error("Trailing canonical continuation");}
    return result;
}

void Game::ValidateCanonicalContinuation(const CanonicalTurnContinuation& next) const {
    const bool colony=next.stage==CanonicalTurnContinuation::Stage::Colony;
    const bool former=next.stage==CanonicalTurnContinuation::Stage::FormerOwner;
    const bool research=next.stage==CanonicalTurnContinuation::Stage::ResearchFinish || next.stage==CanonicalTurnContinuation::Stage::ResearchBonus
        || next.stage==CanonicalTurnContinuation::Stage::ResearchOrdinarySelect || next.stage==CanonicalTurnContinuation::Stage::ResearchBonusSelect;
    if(!m_simulation || m_simulation->value.human_defeat_request || (!colony && !research && !former) ||
        (next.screen_after_transfer || (former ? (bool(next.energy) || bool(next.screen_base) || bool(next.screen_request_id) || bool(next.colony)!=bool(next.former_colony_choice) || (next.colony && next.former_abandonment_tail))
            : (next.former_colony_choice || next.former_abandonment_tail || (colony?(!next.colony || bool(next.energy) || bool(next.screen_base) || bool(next.screen_request_id)):(bool(next.colony) || !next.energy)))))
        || next.turn!=GetTurnId() || next.owner_index>=next.owners.size()) {
        throw std::runtime_error("Only an explicit Colony or research question is a saved turn boundary");
    }
    std::map<int32_t,bool> roster;std::set<int32_t> required;
    for(const auto& participant:m_simulation->value.roster) {
        roster.emplace(participant.simulation_id,participant.active);
        if(participant.active) {required.insert(participant.simulation_id);}
    }
    std::set<size_t> ids;int32_t previous=0;
    const auto& records=m_bm->GetBaseRecordOrder();
    for(size_t i=0;i<next.owners.size();++i) {
        const auto& owner=next.owners[i];
        const auto participant=roster.find(owner.first);
        // Inactive factions can own entities after riot transfer. Retain their
        // membership without admitting their controller for upkeep.
        if(owner.first<=previous || participant==roster.end()) {
            throw std::runtime_error("Saved Colony owner order or activity differs from the current roster");
        }
        previous=owner.first;required.erase(owner.first);
        std::vector<size_t> live;
        for(size_t j=0;j<owner.second.size();++j) {
            const auto id=owner.second[j];
            if(id>static_cast<uint64_t>(INT64_MAX) || !ids.insert(id).second) {throw std::runtime_error("Invalid saved Colony Base identity");}
            const auto* base=m_bm->GetBase(id);
            const bool visited=i<next.owner_index || (i==next.owner_index && j<next.base_index);
            if(!base) {
                if(!visited) {throw std::runtime_error("Unvisited Base disappeared from saved Colony traversal");}
                continue; // Starvation may have removed an already visited record.
            }
            if(base->GetSmacResourceContext().owner_simulation_id!=owner.first) {throw std::runtime_error("Saved Colony Base owner differs");}
            live.push_back(id);
        }
        std::vector<size_t> expected;
        for(const auto id:records) {if(m_bm->GetBase(id)->GetSmacResourceContext().owner_simulation_id==owner.first) {expected.push_back(id);}}
        if(live!=expected) {throw std::runtime_error("Saved Colony traversal differs from live Base record order");}
    }
    if(!required.empty()) {throw std::runtime_error("Saved Colony traversal omits an active faction");}
    const auto& owner=next.owners[next.owner_index];
    if(next.base_index>=owner.second.size()) {throw std::runtime_error("Saved Colony Base cursor is outside its owner");}
    const auto* base=m_bm->GetBase(owner.second[next.base_index]);
    RequireSmacLocalHumanOwner(base->m_owner);
    if(former && base->GetSmacResourceContext().FormerOwner()) {throw std::runtime_error("Saved former faction question already has history");}
    if(former && !next.colony) {
        if(next.former_abandonment_tail && (!base->GetSmacUpkeepCache() || (*next.former_abandonment_tail!=unit::SmacMoraleMode::Mode0 && *next.former_abandonment_tail!=unit::SmacMoraleMode::Mode1))) {
            throw std::runtime_error("Saved former faction question lacks its successor cache");
        }
        return; // The whole current Base attempt or successor tail is still unpublished.
    }
    if(colony || former) {base->ValidateSmacColonyPause(*next.colony);}
    const auto& cache=base->GetSmacUpkeepCache();
    if(!cache) {throw std::runtime_error("Saved Colony question lacks its committed resource cache");}
    if(colony || former) {
        types::Buffer first,cached;
        // Legacy suspended prompts have no retained optional history. Compare
        // every field they actually own, without inventing the missing pass.
        const bool ecology=next.colony->first_calculation.eco_damage.has_value();
        const bool citizens=next.colony->first_calculation.citizens.has_value();
        base::WriteSmacResourceCalculation(first,next.colony->first_calculation,ecology,citizens);
        base::WriteSmacResourceCalculation(cached,cache->calculation,ecology,citizens);
        if(first.ToString()!=cached.ToString()) {throw std::runtime_error("Saved Colony cache differs from its first pass");}
    }
    else {
        const auto* technology=m_simulation->value.technology.Find(owner.first);
        const auto& rules=m_state->m_settings.global.GetSmacCampaignRules();
        const bool blind=rules && rules->BlindResearch() && !next.research_legacy_manual;
        const bool blind_tail=next.stage==CanonicalTurnContinuation::Stage::ResearchOrdinarySelect
            || next.stage==CanonicalTurnContinuation::Stage::ResearchBonusSelect;
        if((blind_tail && !blind) || (blind && (!technology ||
            owner.first!=RequireSimulationParticipant(m_player).simulation_id ||
            !(GetTurnId()<=11 || (technology->score/2)%5==1)))) {
            // Restoring a question must never execute an automatic selection.
            throw std::runtime_error("Saved Blind research question is outside its original prompt gate");
        }
        if(!technology || technology->target || (next.screen_base && *next.screen_base!=base->m_id) ||
            (next.screen_request_id && (!next.screen_base || *next.screen_request_id<1)) || next.energy->raw_intake!=cache->calculation.raw_intake[2] ||
            (!blind
                && faction::SmacManualResearchPool(*technology,*m_state->m_settings.global.GetSmacTechnologyRules()).empty())) {
            throw std::runtime_error("Saved research question has no current target selection boundary");
        }
        types::Buffer energy;base::WriteSmacBaseEnergy(energy,*next.energy);
        for(const auto& charge:next.energy->maintenance_fees) {
            const auto* definition=m_bm->GetSmacFacilityDef(charge.first);
            if(!definition || !base->m_facilities.count(definition->m_id)) {
                throw std::runtime_error("Saved research maintenance facility is absent");
            }
        }
    }
}

void Game::ApplySnapshotContinuation(GSE_CALLABLE) {
    if(!m_pending_canonical_continuation) {return;}
    if(m_canonical_continuation || m_colony_request || m_research_request || m_canonical_turn_in_progress || m_canonical_turn_failed) {
        throw std::runtime_error("Cannot restore over an active canonical continuation");
    }
    ValidateCanonicalContinuation(*m_pending_canonical_continuation);
    m_canonical_continuation=std::move(m_pending_canonical_continuation);
    m_pending_canonical_continuation.reset();
    m_canonical_turn_in_progress=true;
    util::FinallyGuard finished([&]{m_canonical_turn_in_progress=false;});
    m_is_turn_complete=false;
    if(m_canonical_continuation->stage==CanonicalTurnContinuation::Stage::FormerOwner) {RequestCanonicalFormerOwner(GSE_CALL);}
    else if(m_canonical_continuation->stage==CanonicalTurnContinuation::Stage::Colony) {RequestCanonicalColony(GSE_CALL);}
    else {
        auto& next=*m_canonical_continuation;
        if(next.screen_base) {
            if(m_next_colony_request==INT64_MAX) {throw std::overflow_error("Restored Colony screen identity exhausted");}
            next.screen_request_id=++m_next_colony_request;
        }
        if(!RequestCanonicalResearch(GSE_CALL,next.owners.at(next.owner_index).first,next.stage==CanonicalTurnContinuation::Stage::ResearchBonus)) {
            throw std::runtime_error("Restored research choice has no candidates");
        }
    } // A fresh request; no gameplay stage runs.
}
}
