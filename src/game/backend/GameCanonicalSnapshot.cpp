#include "Game.h"
#include "State.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/SmacBaseUpkeepCache.h"
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
    types::Buffer buffer;buffer.WriteInt(1);buffer.WriteBool(m_canonical_continuation.has_value());
    if(!m_canonical_continuation) {
        if(m_colony_request || m_research_request) {throw std::runtime_error("Orphaned canonical choice cannot be saved");}
        return buffer;
    }
    ValidateCanonicalColony();
    const auto& next=*m_canonical_continuation;
    ValidateCanonicalContinuation(next);
    buffer.WriteInt(next.turn);buffer.WriteInt(next.owner_index);buffer.WriteInt(next.base_index);
    buffer.WriteInt(next.owners.size());
    for(const auto& owner:next.owners) {
        buffer.WriteInt(owner.first);buffer.WriteInt(owner.second.size());
        for(const auto id:owner.second) {buffer.WriteInt(id);}
    }
    base::WriteSmacResourceCalculation(buffer,next.colony->first_calculation);
    buffer.WriteInt(static_cast<int64_t>(next.colony->morale_mode));
    return buffer;
}

std::optional<Game::CanonicalTurnContinuation> Game::ReadCanonicalContinuation(types::Buffer buffer,size_t turn) const {
    if(buffer.ReadInt()!=1) {throw std::runtime_error("Unsupported canonical continuation");}
    std::optional<CanonicalTurnContinuation> result;
    if(buffer.ReadBool()) {
        if(!m_state || m_state->GetConnection() || m_state->m_settings.global.GetRulesetId()!="smac") {
            throw std::runtime_error("Saved Colony choice requires a local SMAC campaign");
        }
        result.emplace();auto& next=*result;
        next.stage=CanonicalTurnContinuation::Stage::Colony;
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
        const auto calculation=base::ReadSmacResourceCalculation(buffer);
        const auto mode=buffer.ReadInt();
        if(mode<0 || mode>1) {throw std::runtime_error("Invalid saved Colony morale mode");}
        next.colony=base::SmacColonyUpkeepPause{calculation,static_cast<unit::SmacMoraleMode>(mode)};
    }
    if(buffer.lenr!=buffer.lenw) {throw std::runtime_error("Trailing canonical continuation");}
    return result;
}

void Game::ValidateCanonicalContinuation(const CanonicalTurnContinuation& next) const {
    if(!m_simulation || next.stage!=CanonicalTurnContinuation::Stage::Colony || !next.colony || next.energy || next.screen_base
        || next.turn!=GetTurnId() || next.owner_index>=next.owners.size()) {
        throw std::runtime_error("Only an explicit Colony question is a saved turn boundary");
    }
    std::set<int32_t> roster;
    for(const auto& participant:m_simulation->value.roster) {roster.insert(participant.simulation_id);}
    if(roster.size()!=next.owners.size()) {throw std::runtime_error("Saved Colony owners differ from the current roster");}
    std::set<size_t> ids;auto participant=roster.begin();
    const auto& records=m_bm->GetBaseRecordOrder();
    for(size_t i=0;i<next.owners.size();++i,++participant) {
        const auto& owner=next.owners[i];
        if(owner.first!=*participant) {throw std::runtime_error("Saved Colony owner order differs from the current roster");}
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
    const auto& owner=next.owners[next.owner_index];
    if(next.base_index>=owner.second.size()) {throw std::runtime_error("Saved Colony Base cursor is outside its owner");}
    const auto* base=m_bm->GetBase(owner.second[next.base_index]);
    base->ValidateSmacColonyPause(*next.colony);
    const auto& cache=base->GetSmacUpkeepCache();
    if(!cache) {throw std::runtime_error("Saved Colony question lacks its committed resource cache");}
    types::Buffer first,cached;
    base::WriteSmacResourceCalculation(first,next.colony->first_calculation);
    base::WriteSmacResourceCalculation(cached,cache->calculation);
    if(first.ToString()!=cached.ToString()) {throw std::runtime_error("Saved Colony cache differs from its first pass");}
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
    RequestCanonicalColony(GSE_CALL); // A fresh request; no gameplay stage runs.
}
}
