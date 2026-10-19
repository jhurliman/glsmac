#pragma once

#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace game {

struct SmacHealthbar {
    uint8_t filled;
    enum class Color : uint8_t { Red, Yellow, Green } color;
};

// Original zoom0 backing is 2x21. Presentation health is an integer HP ratio;
// retaining its denominator also preserves floor(H/3) threshold boundaries.
inline SmacHealthbar CalculateSmacHealthbar(float health,int32_t maximum) {
    if(!std::isfinite(health) || health<0 || health>1 ||
        !(maximum==1 || (maximum>=10 && maximum<=1000 && maximum%10==0))) {
        throw std::invalid_argument("SMAC health bar requires canonical hit points");
    }
    const auto remaining=static_cast<int32_t>(std::lround(health*maximum));
    const auto damage=maximum-remaining;
    const auto color=damage==0 || damage<maximum/3 ? SmacHealthbar::Color::Green
        : damage<(2*maximum)/3 ? SmacHealthbar::Color::Yellow:SmacHealthbar::Color::Red;
    return {static_cast<uint8_t>(21*remaining/maximum),color};
}

}
