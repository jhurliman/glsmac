#include "Game.h"
#include "Player.h"
#include "slot/Slot.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/SmacFirstBaseName.h"
#include "gc/Space.h"
#include "gse/value/Null.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Object.h"
#include <limits>
#include <algorithm>
#include <thread>

namespace game::backend {
bool Game::CanBeginFirstBaseIntroduction() const {
    return m_simulation && m_state && !m_state->GetConnection() && m_player && m_slot
        && m_state->m_settings.local.game_mode==settings::LocalSettings::GM_SINGLEPLAYER
        && m_active_admission && !m_active_admission->cancelled
        && m_active_admission->kind==WorldAdmission::INITIALIZATION
        && !m_active_admission->previous && !m_active_admission->map_path
        && !m_active_admission->campaign_world && m_current_turn.GetId()==0
        && m_simulation->value.IsHuman(RequireSimulationParticipant(m_player).simulation_id);
}
bool Game::BeginFirstBaseIntroduction(GSE_CALLABLE) {
    // Original fresh local turn-zero introduction only. Restores, downloaded
    // worlds and nonlocal play do not replay it. No-base starts skip naming.
    if(!CanBeginFirstBaseIntroduction()) {return false;}
    RequireConventionalStartupClaim();
    RequireSmacLocalHumanOwner(m_slot);
    base::Base* first=nullptr;
    for(const auto id:m_bm->GetBaseRecordOrder()) {
        auto* candidate=m_bm->GetBase(id);
        if(candidate && candidate->m_owner==m_slot) {first=candidate;break;}
    }
    if(!first) {return false;}
    if(!first->HasSmacResourceContext() || m_first_base_request || m_next_first_base_request==INT64_MAX) {
        throw std::runtime_error("First Base introduction requires one admitted canonical Base and fresh request identity");
    }
    // The paused initialization does not enter normal manager iteration.
    // Publish its already-created Base and focus it before displaying the
    // prompt; this is presentation only, not first-turn resource processing.
    m_bm->PushUpdates();
    auto center=FrontendRequest(FrontendRequest::FR_MAP_CENTER);
    center.data.map_center={first->GetTile()->coord.x,first->GetTile()->coord.y};
    AddFrontendRequest(center);
    m_first_base_request=FirstBaseRequest{++m_next_first_base_request,m_active_admission->ticket,first->m_id};
    m_active_admission->resume_first_turn=true;
    SetStartupStage(m_active_admission,StartupStage::AwaitFirstBase);
    SetTurnStatus(turn::TS_PLEASE_WAIT);
    NotifyFirstBaseName(GSE_CALL);
    return true;
}

void Game::QueueFirstBaseName(const State::game_session_t& session,int64_t id,const std::string& supplied) {
    if(!m_state || !m_state->IsGameSessionActive(session) || !m_first_base_request || m_first_base_request->id!=id) {return;}
    if(supplied.size()>2*1024*1024) {throw std::runtime_error("First Base name exceeds its command budget");}
    const auto name=supplied.substr(0,std::min(size_t(23),supplied.find('\0')));
    for(unsigned char c:name) {if(c<32 || c>126) {throw std::runtime_error("First Base naming currently supports printable ASCII only");}}
    // Own the copied text across the native/owner handoff; never retain a DOM
    // property or caller wrapper. The admission and session retire stale work.
    std::lock_guard lock(m_admission_mutex);
    if(!m_active_admission || m_active_admission->cancelled || m_active_admission->admitted_binding!=session
        || m_first_base_request->world!=m_active_admission->ticket) {return;}
    // Always defer, even from a prompt listener inside claimed Start: a
    // synchronous nested ingress would still be inside the wrong owner phase.
    m_active_admission->ingress.push_back({this,[id,name](Game* game,GSE_CALLABLE) {
        game->CompleteFirstBaseIntroduction(GSE_CALL,id,name);
    }});
}

void Game::CompleteFirstBaseIntroduction(GSE_CALLABLE,int64_t id,const std::string& supplied) {
    if(!m_first_base_request || m_first_base_request->id!=id) {return;}
    {
        std::lock_guard lock(m_admission_mutex);
        if(!m_active_admission || m_active_admission->cancelled || m_first_base_request->world!=m_active_admission->ticket
            || m_active_admission->stage!=StartupStage::AwaitFirstBase || !m_active_admission->resume_first_turn
            || !m_owner_transition || m_owner_transition->admission!=m_active_admission
            || m_owner_transition->phase!=OwnerTransition::Running || m_owner_transition->stage!=StartupStage::Ingress
            || m_owner_transition->thread!=std::this_thread::get_id()
            || !m_state->IsGameSessionActive(m_active_admission->admitted_binding)) {
            throw std::runtime_error("First Base result requires its claimed introduction continuation");
        }
    }
    if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating() || GetTurnId()!=0 || m_canonical_continuation) {
        throw std::runtime_error("First Base result arrived outside the pre-upkeep boundary");
    }
    auto* base=m_bm->GetBase(m_first_base_request->base);
    if(!base || base->m_owner!=m_slot) {throw std::runtime_error("First Base identity or owner changed while naming");}
    auto name=supplied.substr(0,std::min(size_t(23),supplied.find('\0')));
    for(unsigned char c:name) {if(c<32 || c>126) {throw std::runtime_error("Unsupported First Base display byte");}}
    std::vector<std::string_view> others;
    for(const auto& row:m_bm->GetBases()) {if(row.second!=base) {others.push_back(row.second->m_name);}}
    // Original single-pass, case-sensitive normalization, not an unused-suffix
    // search. Even an empty/whitespace result reaches this copy boundary.
    name=base::NormalizeSmacFirstBaseName(name,others);
    WithRW([&]{m_bm->PublishBaseName(GSE_CALL,*base,std::move(name));});
    m_first_base_request.reset();
    NotifyFirstBaseName(GSE_CALL);
    SetStartupStage(m_active_admission,StartupStage::Start);
}

gse::Value* Game::GetPendingFirstBaseName(GSE_CALLABLE) const {
    if(!m_first_base_request) {return VALUE(gse::value::Null);}
    const auto* base=m_bm->GetBase(m_first_base_request->base);
    if(!base || base->m_owner!=m_slot || GetTurnId()!=0) {throw std::runtime_error("First Base request lost its live Base");}
    return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
        {"request_id",VALUE(gse::value::Int,,m_first_base_request->id)},
        {"name",VALUE(gse::value::String,,base->m_name)}, {"max_bytes",VALUE(gse::value::Int,,23)}
    }));
}
void Game::NotifyFirstBaseName(GSE_CALLABLE) {
    m_state->TriggerObject(this,"first_base_name",ARGS_F(this) {};} );
}
}
