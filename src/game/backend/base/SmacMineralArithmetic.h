#pragma once
#include <cstdint>

namespace game::backend::base {
struct SmacMineralAmounts {int32_t intake,demand,surplus;};
// The owning caller supplies checked narrowing or original DWORD wrapping.
// Half-bonuses combine before one multiply/divide, then demand is subtracted.
template<class Narrow> SmacMineralAmounts CalculateSmacMineralAmounts(int32_t intake,int32_t demand,int32_t steps,const Narrow& narrow) {
    const auto gross=narrow(int64_t(intake)*(int64_t(2)+steps))/2;
    return {gross,demand,narrow(int64_t(gross)-demand)};
}
}
