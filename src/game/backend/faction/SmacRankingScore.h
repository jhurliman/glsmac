#pragma once

#include "SmacTechnologyState.h"
#include "game/backend/unit/SmacStartupDesignState.h"
#include "SmacDesignCounts.h"

namespace game::backend::faction {

// RE-SMAC-ORDINARY-RANKING-SCORE-001: ordinary 9999/9999/0 scenario,
// no Secret Projects. Counts and population are retained producer inputs.
int32_t CalculateSmacOrdinaryRankingScore(const SmacFactionTechnologyState& owner,
    int32_t population, const SmacDesignCounts& counts,
    const settings::SmacTechnologyRules& technologies,
    const unit::SmacPredefinedUnitCatalog& catalog, const unit::SmacStartupDesignState& designs);

}
