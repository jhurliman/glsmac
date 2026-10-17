#pragma once

#include "SmacTechnologyState.h"

namespace game::backend::faction {

struct SmacResearchCostInputs {
	int32_t score, discount, deferred;
	std::array<int32_t,7> other_scores{}, other_deferred{};
	bool human;
	int32_t owner_difficulty, global_difficulty, turn, intrinsic_research;
	int32_t discovery_rate, technology_cost_percent, map_width, map_height;
	bool stagnation;
	std::optional<int32_t> cache;
};

// Returns the measured query value without writing the cache.
int32_t CalculateSmacResearchCost(const SmacResearchCostInputs& input);

// Ordinary positive-owner manual picker, option0, no transfer source.
// Category order is Growth/Tech/Wealth/Power; file ratings are P/T/W/G.
std::vector<int32_t> SmacManualResearchPool(const SmacFactionTechnologyState& owner,
	const settings::SmacTechnologyRules& technologies);

}
