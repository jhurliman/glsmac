#include "SmacGenerationNoise.h"

namespace game::backend::map::smac_generation {
namespace {
int64_t Floor(int64_t value,int64_t divisor) {
    return value/divisor-(value%divisor<0?1:0);
}
}
Noise::Noise(SmacGameplayRandomState& random,const Observer& observer) {
    std::array<std::array<uint8_t,18>,18> values{};
    for(auto& row:values) {for(auto& value:row) {
        const auto before=random.Crt();const auto raw=random.NextCrtRaw15();
        value=uint8_t(raw&15);
        if(observer) {observer({Draw::Kind::Crt,before,random.Crt(),raw});}
    }}
    for(size_t x=0;x<17;++x) {for(size_t y=0;y<17;++y) {grid[x][y]=values[x%16][y%16];}}
}
int32_t Noise::Evaluate(int32_t x,int32_t y) const {
    // Widen before subtracting. Explicit floor division preserves the x86
    // arithmetic-shift result without C++ signed-shift/overflow assumptions.
    const int64_t px=int64_t(x)-128,py=int64_t(y)-128;
    const auto ix=uint64_t(Floor(px,256))&15,iy=uint64_t(Floor(py,256))&15;
    const auto fx=int32_t(uint64_t(Floor(px,8))&31),fy=int32_t(uint64_t(Floor(py,8))&31);
    return ((grid[ix+1][iy]*fx+grid[ix][iy]*(32-fx))*(32-fy)
        +(grid[ix][iy+1]*(32-fx)+grid[ix+1][iy+1]*fx)*fy)/32;
}
}
