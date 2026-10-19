#include "Game.h"
#include "Player.h"
#include "slot/Slot.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "faction/SmacDominance.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "event/Event.h"
#include <stdexcept>

namespace game::backend {
std::array<int32_t,8> Game::NativeDominanceScores(const std::array<int32_t,8>& scores) const {
    const auto& state=m_simulation->value;
    const auto& profiles=*m_state->m_settings.global.GetSmacFactionStartupRules();
    if(!state.dominance_native_keys) {throw std::runtime_error("Dominance history has no retained native identities");}
    std::array<int32_t,8> result{};std::array<bool,8> assigned{};
    for(const auto& owner:state.roster) {
        const auto* profile=profiles.Find(owner.faction_id);
        if(!profile) {throw std::runtime_error("Dominance owner lacks frozen canonical profile");}
        size_t slot=1;while(slot<8 && state.dominance_native_keys->at(slot)!=profile->section) {++slot;}
        if(slot==8 || assigned[slot]) {throw std::runtime_error("Dominance source slots are absent or duplicated");}
        assigned[slot]=true;result[slot]=scores.at(owner.simulation_id);
    }
    return result;
}

bool Game::RequestLocalDominanceGraph(GSE_CALLABLE,int64_t request) {
    if(request<1) {throw std::runtime_error("Dominance graph request must be positive");}
    if(!IsRunning() || !m_state || !m_simulation || !m_slot || !m_player
        || m_state->GetConnection() || m_player->GetRole()!=Player::PR_SINGLE) {return false;}
    const auto& participant=RequireSimulationParticipant(m_player);
    if(participant.slot!=static_cast<int32_t>(m_slot->GetIndex())) {throw std::runtime_error("Dominance local participant mismatch");}
    const auto& state=m_simulation->value;
    // Old payloads retain their original dense columns. Never reinterpret them
    // from today's roster or installed files, nor read beyond native capacity.
    if(!state.dominance_history || !state.dominance_native_keys || !state.ranking_scores
        || GetTurnId()>=faction::SmacDominanceHistory::ROWS
        || state.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {return false;}
    AddEvent(new event::Event(this,event::Event::ES_LOCAL,m_slot_num,GSE_CALL,"_rebuild_smac_dominance",
        {{"request_id",VALUE(gse::value::Int,,request)},{"turn_id",VALUE(gse::value::Int,,GetTurnId())}}));
    return true;
}

bool Game::HandleDominanceGraphEvent(GSE_CALLABLE,const std::string& name,const gse::value::object_properties_t& args,size_t caller,bool apply) {
    if(name!="_rebuild_smac_dominance") {return false;}
    const auto read=[&](const char* key)->int64_t {
        const auto value=args.find(key);
        if(value==args.end() || !value->second || value->second->type!=gse::VT_INT) {throw std::runtime_error("Malformed dominance graph request");}
        return static_cast<const gse::value::Int*>(value->second)->value;
    };
    const auto request=read("request_id"),turn=read("turn_id");
    if(caller!=m_slot_num || args.size()!=2 || request<1 || turn<0 || turn>=1000
        || !IsRunning() || !m_state || !m_simulation || !m_slot || !m_player
        || m_state->GetConnection() || m_player->GetRole()!=Player::PR_SINGLE
        || RequireSimulationParticipant(m_player).slot!=static_cast<int32_t>(m_slot->GetIndex())) {
        throw std::runtime_error("Dominance rebuild requires the authenticated local campaign participant");
    }
    if(!apply) {return true;}
    std::optional<faction::SmacDominanceHistory> next;
    std::optional<std::array<int32_t,8>> current;
    std::array<std::string,8> keys{};
    // A view rebuild may be requested behind a modal/turn change. It does not
    // bypass an unfinished upkeep or reinterpret its partial owner inputs.
    if(!m_canonical_turn_failed && !m_canonical_continuation && !m_canonical_turn_in_progress
        && size_t(turn)==GetTurnId() && m_simulation->value.dominance_history
        && m_simulation->value.dominance_native_keys && m_simulation->value.ranking_scores
        && m_simulation->value.movement_capability==faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {
        if(const auto ranking=PrepareCanonicalRankOrder()) {
            CheckRW(GSE_CALL);
            next=m_simulation->value.dominance_history;current=NativeDominanceScores(ranking->composite);
            keys=*m_simulation->value.dominance_native_keys;next->Publish(GetTurnId(),*current);
            // Real score-only invocation: current native row and working scores
            // change, while the retained rank order is deliberately untouched.
            *m_simulation->value.dominance_history=*next;
            *m_simulation->value.ranking_scores=ranking->composite;
        }
    }
    const auto payload=next ? next->Serialize():std::string{};
    m_state->TriggerObject(this,"smac_dominance_graph",ARGS_F(request,turn,payload,current,keys) {
        {"request_id",VALUE(gse::value::Int,,request)},
        {"frame",current ? [&]()->gse::Value* {
            gse::value::array_elements_t scores,names;
            for(size_t slot=0;slot<8;++slot) {scores.push_back(VALUE(gse::value::Int,,current->at(slot)));names.push_back(VALUE(gse::value::String,,keys[slot]));}
            return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
                {"kind",VALUE(gse::value::String,,"smac-native-dominance-v1")},{"turn",VALUE(gse::value::Int,,turn)},
                {"history",VALUE(gse::value::String,,payload)},{"scores",VALUE(gse::value::Array,,scores)},{"native_keys",VALUE(gse::value::Array,,names)}
            }));
        }():VALUE(gse::value::Undefined)}
    };});
    return true;
}

gse::Value* Game::CopyLocalFactionSummary(GSE_CALLABLE) const {
    // No supplied Player/owner selector: this presentation belongs only to the
    // authenticated local campaign participant. Legacy/diagnostic worlds have
    // no simulation authority to substitute from their visible UI roster.
    if(!IsRunning() || !m_state || !m_simulation || !m_slot || !m_player
        || m_state->GetConnection() || m_player->GetRole()!=Player::PR_SINGLE) {return VALUE(gse::value::Undefined);}
    const auto& participant=RequireSimulationParticipant(m_player);
    if(participant.slot!=static_cast<int32_t>(m_slot->GetIndex())) {throw std::runtime_error("Faction summary local participant mismatch");}
    const auto owner=participant.simulation_id;
    const auto& state=m_simulation->value;
    const auto& global=m_state->m_settings.global;
    const auto& labels=global.GetSmacFactionSummaryLabels();
    gse::Value* integrity=VALUE(gse::value::Undefined);
    gse::Value* might=VALUE(gse::value::Undefined);
    gse::Value* votes=VALUE(gse::value::Undefined);
    if(labels) {
        const auto hostile=state.hostility.owners.find(owner);
        if(hostile!=state.hostility.owners.end()) {integrity=VALUE(gse::value::String,,labels->Integrity(hostile->second.integrity));}
        // Native Might reads its saved rank1..7. Our retained inverse table
        // preserves active ranks exactly, but deliberately drops inactive rows.
        if(participant.active && state.rank_order) {
            for(int32_t rank=1;rank<=7;++rank) {if(state.rank_order->faction_at_rank[rank]==owner) {
                might=VALUE(gse::value::String,,labels->Might(rank));break;
            }}
        }
    }
    if(state.movement_capability==faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain
        && global.GetSmacFactionStartupRules()) {
        const auto* profile=global.GetSmacFactionStartupRules()->Find(participant.faction_id);
        if(!profile) {throw std::runtime_error("Council summary lacks frozen faction profile");}
        std::vector<faction::SmacCouncilBase> bases;
        for(const auto id:m_bm->GetBaseRecordOrder()) {
            const auto* base=m_bm->GetBase(id);
            if(!base || !base->HasSmacResourceContext() || base->GetPopulation()>INT32_MAX) {throw std::runtime_error("Council summary Base record is invalid");}
            bases.push_back({base->GetSmacResourceContext().owner_simulation_id,static_cast<int32_t>(base->GetPopulation())});
        }
        std::vector<int32_t> modifiers;
        for(const auto& bonus:profile->Records()) {if(bonus.kind==settings::SmacFactionStartupBonus::Kind::VOTES) {modifiers.push_back(bonus.parameter);}}
        // False project flags are the saved no-project capability, not an
        // inference from absent project/location data in a legacy campaign.
        votes=VALUE(gse::value::Int,,faction::SmacCouncilVotes(owner,bases,false,false,modifiers));
    }
    return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
        {"integrity",integrity},{"might",might},{"council_votes",votes}
    }));
}
}
