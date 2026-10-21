#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace game::backend::unit {struct SmacStartupDesignState;struct SmacPredefinedUnitCatalog;}
namespace game::backend::faction {
// Retained5773c0 outputs, distinct from577140 capability caches.
struct SmacAirSummary {
    int32_t foreign_threats=0,own_plan0=0,own_plan4=0;
    bool operator==(const SmacAirSummary& other) const noexcept {
        return foreign_threats==other.foreign_threats && own_plan0==other.own_plan0 && own_plan4==other.own_plan4;
    }
};
struct SmacAirSummaryUnit {int32_t owner,domain,plan,offense;bool special;};
struct SmacAirSummaryContext {
    int32_t owner,peer_defense;
    std::array<uint32_t,8> relations;
    std::optional<std::array<int32_t,2>> land_psi_ratio;
    int32_t psi_percent;
    bool dream_twister;
};
// Original592d80/592d30; nonnegative power bypasses PSI inputs. No minimum1.
int32_t SmacAirAttackEstimate(int32_t offense,int32_t peer_defense,
    const std::optional<std::array<int32_t,2>>& land_psi_ratio,int32_t psi_percent,bool dream_twister);
// All live instances, including native life. No visibility/technology or
// design-exclusion gate. Does not mutate capability caches or any Unit.
SmacAirSummary CountSmacAirSummary(const std::vector<SmacAirSummaryUnit>& units,const SmacAirSummaryContext& context);
SmacAirSummaryUnit ProjectSmacAirSummaryUnit(const unit::SmacStartupDesignState& designs,
    const unit::SmacPredefinedUnitCatalog& catalog,int32_t design,int32_t owner);
}
