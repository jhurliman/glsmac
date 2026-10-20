#pragma once

#include "SmacEmptyGroundMovement.h"

namespace game::backend::unit::smac_move {

// Controller-owned selector, not faction identity or a saved RNG seed.
// The ordinary command boundary selects SharedLow16. Direct primitive calls
// made from a retained advancement frame may instead select CrtModulo.
// Normalize raw nonzero controller words to CrtModulo; these enum values
// are typed choices, not the raw DWORD or a new persistence format.
enum class LotteryStream { SharedLow16, CrtModulo };
Result ResolveSelectedGroundStep(const MovementState& unit,int32_t quantum,
    int32_t full_allowance,int32_t raw_edge_cost,bool destination_fungus,
    const SmacGameplayRandomState& random,LotteryStream stream);

}
