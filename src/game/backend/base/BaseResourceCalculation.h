#pragma once

#include <array>
#include <cstdint>

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
};

}
