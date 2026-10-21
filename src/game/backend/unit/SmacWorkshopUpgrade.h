#pragma once
#include "SmacStartupDesignState.h"

namespace game::backend::unit {
// Original human action7 scans retained own custom rows. These queries do not
// construct a new design or reapply technology/component-picker admission.
std::vector<int32_t> SmacWorkshopUpgradeCandidates(const SmacOriginalDesignPool& pool,
    int32_t owner,int32_t source,const SmacPredefinedUnitCatalog& catalog);
int32_t SmacWorkshopUpgradeQuoteRows(const SmacOriginalDesignPool& pool,int32_t target,int32_t source,
    const SmacPredefinedUnitCatalog& catalog,bool owned_nano_factory);
struct SmacWorkshopUpgradePreparation {
    SmacStartupDesignState state;
    int32_t per_unit_rows,total_energy,morale_delta;
    uint8_t cached_source_count;
};
// Detached design portion of504cc0. The live caller stages owned Unit/queue
// conversion and energy/count updates before publishing this result. No history
// write or removal: cached-zero still deactivates a custom source, but does not
// mark the target prototyped. Affordability/confirmation belong to504fe0.
SmacWorkshopUpgradePreparation PrepareSmacWorkshopUpgrade(const SmacStartupDesignState& state,
    int32_t owner,int32_t target,int32_t source,const SmacPredefinedUnitCatalog& catalog,
    uint8_t cached_source_count,bool owned_nano_factory);
}
