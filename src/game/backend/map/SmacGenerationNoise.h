#pragma once
#include "SmacGenerationBootstrap.h"

namespace game::backend::map::smac_generation {
// Original 0x5d9d70 / 0x5d9e60. Construction advances the caller's CRT
// continuation; evaluation is deterministic and consumes no random numbers.
class Noise {
public:
    explicit Noise(SmacGameplayRandomState& random,const Observer& observer={});
    int32_t Evaluate(int32_t x,int32_t y) const;
    const std::array<std::array<uint8_t,17>,17>& Grid() const {return grid;}
private:
    std::array<std::array<uint8_t,17>,17> grid{};
};
}
