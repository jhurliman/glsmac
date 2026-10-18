#pragma once
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
namespace game::backend::base {
// Original draw-time label, not a simulation forecast. Food is never normalized
// here. The caller supplies the resource surplus whose lifetime it owns.
inline std::string FormatSmacNutrientGrowthLabel(int64_t food,int32_t capacity,int32_t surplus) {
    if(capacity<0) {throw std::invalid_argument("Negative nutrient display capacity");}
    if(food<capacity && surplus==0) {return "Growth: -stagnant-";}
    if(food<capacity && surplus<0) {return "Hunger!";}
    uint64_t turns=1;
    if(food<capacity) {
        // Unsigned subtraction represents the distance even through INT64_MIN.
        const auto remaining=static_cast<uint64_t>(capacity)-static_cast<uint64_t>(food);
        const auto divisor=static_cast<uint64_t>(surplus);
        turns=std::min<uint64_t>(9999,remaining/divisor+(remaining%divisor!=0)+1);
    }
    return std::string(surplus<0 ? "Hunger: " : "Growth: ")+std::to_string(turns)+(turns==1 ? " turn" : " turns");
}
}
