#pragma once
#include "Base.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/SmacGameplayRandomState.h"
namespace game::backend::base {
// Borrowed plans live only in one owning accumulation. No pointer enters a save.
struct Base::SmacDefectionAttempt {
    const SmacAllocationRoles& roles;
    int32_t recipient;
    SmacBaseResourceContext context;
    ProductionState production;
    SmacBaseAllocationState allocation;
    size_t population;
    int64_t food;
    const FacilityDef* granted;
    std::vector<std::pair<Base*,facilities_t>> facility_changes;
    unit::UnitManager::PreparedConventionalUnit* created;
    std::vector<size_t> removed;
    SmacGameplayRandomState random;
    bool& committed;
    bool* publication;
    const unit::SmacStartupDesignState* prototype_only=nullptr;
};
}
