#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace game::backend::base {

enum class SmacBaseResourcePhase { Ordinary, Upkeep };

// A fresh owned preparation, not a cache or posting instruction. No fields are
// written to Base. Upkeep must explicitly consume normalized_stored at its
// nutrient-processing boundary, separately from later nutrient collection.
struct SmacBaseResourceCalculation {
    std::array< int64_t, 3 > raw_intake;
    struct Nutrients {
        int32_t intake, demand, surplus;
        int64_t stored_before, normalized_stored;
    } nutrients;
    struct Minerals {
        int32_t multiplier_steps, intake, demand, surplus, pre_orbital_surplus;
    } minerals;
    // Missing retained inputs remain unavailable; queries never invent history.
    std::optional<int32_t> eco_damage;
};

// Finish an already calculated raw intake. Founding retains the allocator's
// actual pass through this same N/M processing without querying terrain again.
SmacBaseResourceCalculation FinishSmacResourceIntake( const std::array<int64_t,3>& raw_intake,
    int64_t nutrient_demand, int64_t stored_nutrients, int32_t mineral_support, int32_t mineral_multiplier_steps );

}
