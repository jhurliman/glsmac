#pragma once

#include <cstdint>
#include <optional>

namespace game::backend::base {

struct SmacNutrientCostInputs {
	bool human;
	int32_t global_difficulty;
	bool network_session, leader_human, leading_human_dominance;
	int32_t world_size_category, nutrient_multiplier, effective_growth;
};

// Resolved inputs; AI dominance and accepted world category are caller authority.
int32_t CalculateSmacNutrientCostFactor( const SmacNutrientCostInputs& inputs );
bool SmacHousingAllowsGrowth( int32_t population, int32_t population_penalty,
	int32_t hab_complex_limit, int32_t hab_dome_limit, bool hab_complex, bool hab_dome, bool ascetic_virtues );

struct SmacPopulationRules {
	int32_t cost_per_step, effective_growth, nutrients_per_citizen;
	bool housing_allows_growth, cloning_vats;
};

struct SmacPopulationPlan {
	enum class Branch { Collection, Starvation, Removal, Boom, Growth, Stagnation, Housing };
	Branch branch;
	int32_t population;
	// Absence means a Base removal request. For Growth/Boom this is the food
	// value to supply to the actual candidate-population resource recalculation.
	std::optional< int32_t > stored_nutrients;
	int32_t old_threshold;
	bool NeedsRecalculation() const;
	bool CollectAfterRecalculation() const;
	// Complete only Growth/Boom with the actual recomputed surplus. It performs
	// nutrient normalization, then ordinary collection or the boom's old-box cap.
	int32_t Finish( int32_t recomputed_surplus ) const;
};

class SmacPopulationStep {
public:
	// A staged decision, not a repeated turn command. The owner must publish the
	// resulting population/allocation/food together, then run outer upkeep's
	// unconditional second resource pass. Removal needs the real Base teardown.
	static SmacPopulationPlan Prepare( const SmacPopulationRules& rules, int32_t population,
		int64_t stored_nutrients, int32_t surplus, bool objective_protected );
};

}
