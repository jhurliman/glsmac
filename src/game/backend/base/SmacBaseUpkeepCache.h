#pragma once

#include "BaseResourceCalculation.h"
#include "SmacUnitSupport.h"
#include <optional>

namespace types { class Buffer; }

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

// Preserve historical values and record order; decoding never queries the
// current map, Unit registry or rules to recompute the saved pass.
void WriteSmacResourceCalculation(types::Buffer&, const SmacBaseResourceCalculation&, bool include_ecology = true, bool include_citizens = true);
SmacBaseResourceCalculation ReadSmacResourceCalculation(types::Buffer&, bool include_ecology = true, bool include_citizens = true);
types::Buffer SerializeSmacBaseUpkeepCache(const std::optional<SmacBaseUpkeepCache>&);
std::optional<SmacBaseUpkeepCache> DeserializeSmacBaseUpkeepCache(types::Buffer);

}
