#pragma once
#include <cstdint>
namespace game::backend::base {
// Shared source-order rounding. The caller owns arithmetic-domain admission.
template<class Narrow> int32_t MultiplySmacEnergyQuarters(int32_t value,int32_t quarters,const Narrow& narrow) {
    return narrow(int64_t(narrow(int64_t(value)*quarters))+3)/4;
}
template<class Narrow> int32_t ReduceSmacEnergyPercent(int32_t value,int32_t percent,const Narrow& narrow) {
    return narrow(int64_t(narrow(int64_t(value)*(int64_t(100)-percent)))+50)/100;
}
}
