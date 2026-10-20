#include "Game.h"
#include "CampaignFile.h"
#include "WorldSnapshotRules.h"
#include "WorldSnapshotRandom.h"
#include "WorldSnapshotWorkerKnowledge.h"
#include "WorldSnapshotTerritory.h"
#include "State.h"
#include "slot/Slots.h"
#include "Player.h"
#include "map/Map.h"
#include "map/SmacPlanetMap.h"
#include "util/FinallyGuard.h"
#include "types/Buffer.h"
#include <algorithm>
#include <limits>

namespace game::backend {
void Game::UpdateUiCursor(world_ticket_t world,ui_view::Cursor cursor) {
    std::lock_guard lock(m_admission_mutex);
    if(world && MatchesActiveAdmission(world)) {m_active_admission->ui_cursor=cursor;}
}

uint64_t Game::QueueCampaignSave(const State::game_session_t& session, const std::string& path) {
    if(path.empty()) {THROW("Campaign path is empty");}
    std::lock_guard lock(m_admission_mutex);
    if(!m_active_admission || m_active_admission->cancelled ||
        m_active_admission->stage!=StartupStage::Complete ||
        m_active_admission->admitted_binding!=session || !session || !session->IsActive()) {
        THROW("Save requires an active initialized campaign");
    }
    if(m_campaign_save) {THROW("A campaign save is already pending");}
    // World tickets provide process-wide, nonreused identities across reset.
    const auto id=ReserveWorldTicket();
    if(id>static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {THROW("Campaign request IDs exhausted");}
    m_campaign_save=std::make_shared<CampaignSave>(CampaignSave{
        m_active_admission->ticket,CampaignSave::Queued,{id,path,{}},m_active_admission->ui_cursor});
    return id;
}

bool Game::ProcessCampaignSave() {
    std::shared_ptr<CampaignSave> save;
    std::shared_ptr<OwnerTransition> owner;
    {
        std::lock_guard lock(m_admission_mutex);
        save=m_campaign_save;
        if(!save || save->phase!=CampaignSave::Queued) {return false;}
        if(!MatchesActiveAdmission(save->world)) {m_campaign_save.reset();return false;}
        // Reuse the native/owner handoff: once published, native Iterate cannot
        // touch the world until this session-bound accumulation has finished.
        owner=std::make_shared<OwnerTransition>(m_active_admission,StartupStage::Complete,
            std::vector<WorldAdmission::Ingress>{});
        m_owner_transition_handoffs.push_back(owner);
        m_owner_transition=owner;
        save->phase=CampaignSave::Pending;
    }
    util::FinallyGuard handoff([this,owner]{
        std::lock_guard lock(m_admission_mutex);
        const auto it=std::find(m_owner_transition_handoffs.begin(),m_owner_transition_handoffs.end(),owner);
        if(it!=m_owner_transition_handoffs.end()) {m_owner_transition_handoffs.erase(it);}
    });
    try {
        owner->admission->state->WithGameSession(owner->admission->admitted_binding,owner.get(),
            [this,owner,save](Game* const,GSE_CALLABLE){RunCampaignSave(owner,save);},
            [this,owner]{FinishOwnerTransition(owner);});
    } catch(...) {FinishOwnerTransition(owner);throw;}
    return true;
}

void Game::RunCampaignSave(const std::shared_ptr<OwnerTransition>& owner,const std::shared_ptr<CampaignSave>& save) {
    {
        std::lock_guard lock(m_admission_mutex);
        if(m_owner_transition!=owner || owner->phase!=OwnerTransition::Pending ||
            m_campaign_save!=save || !MatchesActiveAdmission(save->world)) {return;}
        owner->phase=OwnerTransition::Running;
        owner->thread=std::this_thread::get_id();
    }
    util::FinallyGuard finish([this,owner]{FinishOwnerTransition(owner);});
    std::string path,error;
    try {path=SaveCampaign(save->result.path,save->cursor);}
    catch(const std::exception& failure) {error=failure.what();}
    std::lock_guard lock(m_admission_mutex);
    if(m_campaign_save==save && MatchesActiveAdmission(save->world)) {
        save->result.path=std::move(path);
        save->result.error=std::move(error);
        save->phase=CampaignSave::Finished;
    }
}

std::optional<Game::CampaignSaveResult> Game::TakeCampaignSaveResult(const world_ticket_t world) {
    std::lock_guard lock(m_admission_mutex);
    if(!m_campaign_save || m_campaign_save->world!=world ||
        m_campaign_save->phase!=CampaignSave::Finished || !MatchesActiveAdmission(world)) {return {};}
    auto result=std::move(m_campaign_save->result);
    m_campaign_save.reset();
    return result;
}

std::string Game::SaveCampaign(const std::string& path,std::optional<ui_view::Cursor> cursor) {
    {
        std::lock_guard lock(m_admission_mutex);
        if(!m_owner_transition || m_owner_transition->stage!=StartupStage::Complete ||
            m_owner_transition->phase!=OwnerTransition::Running || m_owner_transition->thread!=std::this_thread::get_id()) {
            THROW("Save requires the claimed campaign owner checkpoint");
        }
    }
    if(!m_state || m_state->GetConnection() || m_game_state!=GS_RUNNING || !m_player || m_player->GetRole()!=Player::PR_SINGLE
        || m_state->m_settings.global.GetRulesetId()!="smac") {THROW("Save requires a running local SMAC campaign");}
    {std::lock_guard lock(m_pending_events_mutex); if(!m_pending_events.empty()) {THROW("Finish queued commands before saving");}}
    if(m_energy_rate_session) {THROW("Close the energy allocation window before saving");}
    ValidateCurrentSimulation();
    campaign::Record record{m_state->Serialize().ToString(),m_state->ExportRoster(),m_slot_num,SerializeWorldSnapshot(),cursor};
    return campaign::Write(path,record);
}

void Game::ValidateCampaignWorld(State* state,const std::string& world) {
    if(!state || state->GetGame() || state->GetConnection()) {THROW("Campaign validation requires a detached State");}
    Game validator;
    validator.m_state=state;state->SetGame(&validator);
    util::FinallyGuard unbind([&]{state->UnsetGame();validator.m_state=nullptr;});
    types::Buffer buffer(world);
    world_snapshot::ValidateHeader(buffer,state->m_settings.global);
    world_snapshot::ReadRandomState(buffer);
    const auto turn=world_snapshot::ReadTurnState(buffer,state->m_slots->GetSlots());
    if(!turn.id) {THROW("Cannot load a campaign before its first stable turn");}
    const auto simulation=validator.ReadSimulationState(buffer,turn.id);
    if(!simulation) {THROW("GLSMAC campaign requires saved SMAC simulation state");}
    const auto policy=map::GlsmacResourceMapPolicy::ReadOptional(buffer,true);
    if(bool(policy && policy->OriginalPlanet()) != (state->m_settings.global.map.type==settings::MapSettings::MT_INSTALLED_PLANET)) {
        THROW("World resource provenance differs from the accepted map origin");
    }
    if(policy && policy->OriginalPlanet()) {
        const auto& options=state->m_settings.global.GetSmacCampaignRules();
        if(!options || !state->m_settings.global.IsOrdinarySmacCampaign() || options->AutomaticLandResources()!=policy->AutomaticLandResources()
            || state->m_settings.global.map.GetSmacOceanCoverage()!=policy->OriginalPlanet()->OceanCoverageCategory()) {
            THROW("Installed Planet resource policy differs from active campaign options");
        }
    }
    const auto knowledge=world_snapshot::ReadWorkerKnowledge(buffer,true);
    const auto territory=world_snapshot::ReadTerritoryTopology(buffer,true);
    const auto map=buffer.ReadString(campaign::MAX_BYTES);
    world_snapshot::ValidateMapDimensions(map,state->m_settings.global);
    world_snapshot::ValidateWorkerKnowledgeGeometry(knowledge,map);
    world_snapshot::ValidateTerritoryGeometry(territory,map,knowledge.has_value());
    // Complete manager envelopes are checksum/bounds checked before the menu
    // State changes. Their semantic decode remains the normal restore path.
    for(int i=0;i<4;++i) {buffer.ReadString(campaign::MAX_BYTES);}
    validator.ReadCanonicalContinuation(types::Buffer(buffer.ReadString(campaign::MAX_BYTES)),turn.id);
    if(buffer.lenr!=buffer.lenw) {THROW("Trailing campaign world data");}
}

void Game::BindLocalCampaignState(State* state) {
    if(!state || state->GetConnection() || (state->GetGame() && state->GetGame()!=this)) {THROW("Campaign binding requires an unbound local State");}
    std::lock_guard lock(m_admission_mutex);
    if(m_game_state!=GS_NONE || m_active_admission || m_owner_transition) {THROW("Return to the main menu before loading a campaign");}
    if(m_world_space && m_world_space!=state->m_gc_space) {THROW("Campaign State belongs to another script runtime");}
    if(state!=m_state) {
        if(m_state && m_state->GetGame()==this) {m_state->UnsetGame();}
        m_state=state;m_world_space=state->m_gc_space;
    }
    if(!state->GetGame()) {state->SetGame(this);}
    else if(state->GetGame()!=this) {THROW("Campaign State already belongs to another Game");}
}

pending_initialization_t Game::MT_RestoreCampaign(State* state,std::string world) {
    if(!state || state->GetConnection() || world.empty()) {THROW("Campaign restore requires saved local state");}
    std::lock_guard lock(m_admission_mutex);
    if(m_game_state!=GS_NONE || m_active_admission || m_state!=state) {THROW("Campaign restore requires an empty local controller");}
    return EnqueueInitialization(state,0,{},std::move(world));
}
}
