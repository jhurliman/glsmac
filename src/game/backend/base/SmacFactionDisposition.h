#pragma once
#include "BaseManager.h"
#include "game/backend/faction/SmacSimulationState.h"
#include "game/backend/map/GlsmacTerritoryTopology.h"
#include "game/backend/map/SmacWorkerKnowledge.h"
#include "game/backend/unit/ConventionalUnitState.h"

namespace game::backend::base {
// Shared private plan for starvation/abandonment and capture. Its callers own
// the combined Base/Unit transaction; preparation does not publish authority.
struct BaseManager::SmacDispositionPlan {
    size_t participant,technology;
    faction::SmacEconomyState economy;
    faction::SmacHostileState hostility;
    map::tile::Tile* site=nullptr;
    std::vector<std::pair<unit::Def*,unit::ConventionalUnitState>> units;
    SmacGameplayRandomState random;
    std::optional<map::SmacWorkerKnowledge> territory;
    std::optional<map::GlsmacTerritoryTopology> topology;
	std::optional<int32_t> initialization_history_turn;
};
}
