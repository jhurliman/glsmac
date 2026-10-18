#pragma once
#include "SmacDesignCounts.h"
#include "game/backend/SmacGameplayRandomState.h"
#include "types/Buffer.h"
#include <array>

namespace game::backend::faction {
// Minimal retained authority for the measured initialized owner0 factory tail.
// Definition morale is a supplied independent input, not an initialization result.
// This is not the complete native AI/faction state.
struct SmacNativeFactoryState {
    static constexpr size_t MAX_WIRE_BYTES=600;
    int32_t definition_morale;
    std::array<int32_t,2> assignment_reference;
    uint32_t active_plan_tally;
    SmacDesignCounts active_designs;
    static SmacNativeFactoryState Initialize(int32_t definition_morale,SmacGameplayRandomState& random) noexcept;
    void Adjust(int32_t design,uint8_t plan,int32_t delta);
    bool operator==(const SmacNativeFactoryState& other) const noexcept;
    types::Buffer Serialize() const;
    static SmacNativeFactoryState Deserialize(const std::string& wire);
};
}
