#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "game/backend/SmacGameplayRandomState.h"

namespace game::backend::unit::smac_move {

struct MovementState {
	MovementState( uint64_t identity, int32_t spent, bool prior_failure_marker );
	uint64_t identity;
	int32_t spent;
	bool prior_failure_marker;
};

enum class Outcome { Moved, FailedAttempt, Exhausted };
enum class Position { Source, Destination };
struct Draw {
	uint32_t state_before, state_after;
	uint16_t raw;
	int32_t scaled;
};
struct Result {
	MovementState unit;
	Outcome outcome;
	Position position;
	std::array< bool, 2 > unit_present;
	SmacGameplayRandomState random;
	std::optional< Draw > draw;
	// Original wrapper returns 1 for both movement and failed-attempt exhaustion.
	// This metadata is not a success predicate or a rollback instruction.
	int32_t original_wrapper_return;
};

// Ordinary conventional ground unit, empty destination, no fungus exemptions.
// Supplied full allowance and raw edge cost are not produced here. Q is 1..100;
// allowance, cost and spent are 0..255; spent+cost must fit a byte even for an
// exhausted input. These are port admission limits, not original legality.
// Inputs remain unchanged; only the result's shared RNG may advance.
Result ResolveEmptyGroundStep( const MovementState& unit, int32_t quantum, int32_t full_allowance, int32_t raw_edge_cost, bool destination_fungus, const SmacGameplayRandomState& random );

// Only the spent/marker effect of the captured healthy-unit upkeep pass.
// No global turn, healing, allowance refresh, RNG or live Unit hook is implied.
MovementState ResetSpentForCheckedUnitUpkeep( const MovementState& unit );

}
