#pragma once

#include "SmacStartupDesignState.h"

namespace game::backend::unit {

struct SmacRestartUnitPackage {
    int32_t defender = 2;
    std::optional<int32_t> balanced, mobile;
    std::vector<int32_t> creation_order;

    // Already admitted ordinary local restart, turns 1..100. These are design
    // IDs for successive factories at the accepted site, not live Units or a
    // placement/disposition decision. Saved designs/technology remain unchanged.
    static SmacRestartUnitPackage Select(int32_t turn, const SmacStartupDesignState& designs,
        const SmacPredefinedUnitCatalog& catalog, const faction::SmacFactionTechnologyState& owner,
        const settings::SmacTechnologyRules& technologies);
};

}
