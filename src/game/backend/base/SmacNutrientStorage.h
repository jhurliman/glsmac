#pragma once
#include <cstdint>

namespace game::backend::base {
// Accounting normalizes only the stock sentinel; it does not collect surplus.
constexpr int64_t NormalizeSmacNutrientStorage(int32_t surplus,int64_t stored) noexcept {
    if(surplus<0 && stored==0) {return -1;}
    if(surplus>=0 && stored<0) {return 0;}
    return stored;
}
}
