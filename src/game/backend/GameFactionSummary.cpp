#include "Game.h"
#include "Player.h"
#include "slot/Slot.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "faction/SmacDominance.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include <stdexcept>

namespace game::backend {
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
