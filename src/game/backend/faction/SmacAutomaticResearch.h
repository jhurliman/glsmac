#pragma once

#include "SmacResearchValue.h"
#include "game/backend/SmacGameplayRandomState.h"
#include <functional>

namespace game::backend::faction {

struct SmacAutomaticResearchInputs {
    int32_t faction_id;
    bool human, blind;
    std::array<SmacResearchTechnology,89> technologies;
    std::array<int32_t,4> priorities; // Explore/Discover/Build/Conquer
    int32_t former_prerequisite;
};

// Returns -1 when no candidate wins. The callback resolves the complete value
// at each admitted candidate, before the Blind Formers preference. Selection
// advances only the supplied gameplay streams and never grants ownership.
int32_t SelectSmacAutomaticResearch(const SmacAutomaticResearchInputs& input,
    SmacGameplayRandomState& random, const std::function<int32_t(int32_t)>& value);

}
