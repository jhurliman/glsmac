#include "Game.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "map/Map.h"
#include "map/SmacGlobalWildlife.h"
#include "map/SmacTerritoryRules.h"
#include "map/tile/Tile.h"
#include "unit/UnitManager.h"
#include <stdexcept>

namespace game::backend {

void Game::RunSmacWildlifeTurn(GSE_CALLABLE) {
    CheckRW(GSE_CALL);RequireSmacGameplayActive();
    if(!m_simulation || !m_map || !m_map->GetSmacWorkerKnowledge()
        || !m_map->GetTerritoryTopology() || !m_map->GetTerritoryTopology()->Areas()) {
        throw std::runtime_error("Global ecology requires admitted terrain, regions and observer knowledge");
    }
    auto& state=m_simulation->value;const auto& global=m_state->m_settings.global;
    const auto& rules=global.GetSmacCampaignRules();
    if(!rules || !state.ecology_world || !state.victory_history) {
        throw std::runtime_error("Global ecology requires saved campaign and ecology authorities");
    }
    map::smac_wildlife::Input input{};
    input.geometry={int32_t(m_map->GetWidth()),int32_t(m_map->GetHeight()),m_map->GetTerritoryTopology()->Wraps()};
    input.turn=int32_t(GetTurnId());input.life=state.ecology_world->native_life;
    input.difficulty=state.designs.global_difficulty;input.voice_marker=state.victory_history->voice_host;
    input.unit_count=int32_t(m_um->GetUnits().size());input.rules=rules->primary;input.secondary=rules->secondary;
    input.region_areas=*m_map->GetTerritoryTopology()->Areas();
    for(size_t owner=0;owner<8;++owner) {
        if(!state.faction_status.values[owner]) {throw std::runtime_error("Global ecology lacks retained faction status");}
        input.observer_status[owner]=*state.faction_status.values[owner];
        const auto native=state.hostility.owners.find(0);
        if(native!=state.hostility.owners.end()) {input.native_diplomacy[owner]=native->second.directions[owner].primary;}
        // Stock SMAC wildlife has no diplomatic negotiation producer: its cold
        // relation row remains zero. Preserve a retained native row if present.
    }
    // Original global byte masks at95a115/95a13b, relative to technology0 at95a0fc.
    for(const auto& faction:state.technology.factions) {for(size_t i=0;i<2;++i) {
        if(faction.owned[i==0 ? 25:63]) {input.locust_technology_masks[i]|=1u<<faction.admission.simulation_id;}
    }}
    for(const auto id:m_bm->GetBaseRecordOrder()) {
        const auto* base=m_bm->GetBase(id);const auto* tile=base->GetTile();
        input.bases.push_back({int32_t(input.bases.size()),{int32_t(tile->coord.x),int32_t(tile->coord.y)}});
    }
    auto prepared=m_um->PrepareGlobalEcology(GSE_CALL,input);
    prepared.Commit();prepared.Notify(GSE_CALL);
}

}
