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
#include "util/FinallyGuard.h"
#include "types/Buffer.h"

namespace game::backend {
std::string Game::SaveCampaign(const std::string& path) {
    if(!m_state || m_state->GetConnection() || m_game_state!=GS_RUNNING || !m_player || m_player->GetRole()!=Player::PR_SINGLE
        || m_state->m_settings.global.GetRulesetId()!="smac") {THROW("Save requires a running local SMAC campaign");}
    {std::lock_guard lock(m_pending_events_mutex); if(!m_pending_events.empty()) {THROW("Finish queued commands before saving");}}
    ValidateCurrentSimulation();
    campaign::Record record{m_state->Serialize().ToString(),m_state->ExportRoster(),m_slot_num,SerializeWorldSnapshot()};
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
    map::GlsmacResourceMapPolicy::ReadOptional(buffer,true);
    const auto knowledge=world_snapshot::ReadWorkerKnowledge(buffer,true);
    const auto territory=world_snapshot::ReadTerritoryTopology(buffer,true);
    const auto map=buffer.ReadString(campaign::MAX_BYTES);
    world_snapshot::ValidateMapDimensions(map,state->m_settings.global);
    world_snapshot::ValidateWorkerKnowledgeGeometry(knowledge,map);
    world_snapshot::ValidateTerritoryGeometry(territory,map,knowledge.has_value());
    // Complete manager envelopes are checksum/bounds checked before the menu
    // State changes. Their semantic decode remains the normal restore path.
    for(int i=0;i<4;++i) {buffer.ReadString(campaign::MAX_BYTES);}
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
