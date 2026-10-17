#pragma once

#include "BaseResourceCalculation.h"
#include "SmacUnitSupport.h"

namespace game::backend::base {

// The last committed resource pass, retained separately from fresh read-only
// preparation. In particular, Colony record compaction reaches a successor's
// population/shortage tail before its next resource pass. Recomputing here would
// replace the evidenced cached nutrient surplus and paid-support markings.
// Absence on Base is unavailable history, never an implicitly fresh or zero pass.
struct SmacBaseUpkeepCache {
    SmacBaseResourceCalculation calculation;
    SmacUnitSupportResult support;
};

}
