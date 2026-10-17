#pragma once

#include <cstdint>

namespace game::backend::resource::smac {

// Resolved scaling categories, not map dimensions or original enum values.
enum class MineralCostWorldSize { Tiny, Small, StandardOrLarger };

struct MineralCostInputs {
	MineralCostInputs( bool human, int32_t global_difficulty, bool leader_human, bool network_session,
		bool leading_human_dominance, MineralCostWorldSize world_size, int32_t mineral_multiplier, int32_t industry )
		: human( human ), global_difficulty( global_difficulty ), leader_human( leader_human ), network_session( network_session ),
		leading_human_dominance( leading_human_dominance ), world_size( world_size ), mineral_multiplier( mineral_multiplier ), industry( industry ) {}
	bool human;
	int32_t global_difficulty; // 0..5; not the requesting faction's stored difficulty
	bool leader_human;
	bool network_session;
	bool leading_human_dominance; // normalized result of the leader's mode-zero query
	MineralCostWorldSize world_size;
	int32_t mineral_multiplier; // decoded mineral rules multiplier, not nutrient or item cost
	int32_t industry;
};

// RE-SMAC-MINERAL-COST-001, arithmetic contract v1. Returns only the mineral
// factor, before multiplication by an item's cost. Every input is explicit;
// this function does not construct world categories or query faction rankings.
// The human branch ignores adaptive inputs, but all metadata is validated:
// difficulty outside 0..5, unknown size and negative multiplier throw
// std::invalid_argument. Every int32 INDUSTRY value is supported, including
// the original discontinuity at -8. Zero factors are preserved.
// Arithmetic is evaluated in int64; a selected signed32 product/numerator
// outside range throws std::overflow_error. Original overflow/wrap behavior,
// negative multipliers, live gameplay integration and SMAX are not established.
int32_t CalculateMineralCostFactor( const MineralCostInputs& inputs );

}
