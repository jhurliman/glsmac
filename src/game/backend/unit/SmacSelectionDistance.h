#pragma once
#include "game/PrimaryViewState.h"
#include <algorithm>
#include <cstdint>

namespace game::backend::unit::selection_distance {
// Native selector arithmetic wraps each DWORD operation before comparison.
// Keep INT32_MIN negation and arithmetic shifts defined on every C++17 host.
inline int32_t Signed(uint32_t value) {
    return value<=INT32_MAX?int32_t(value):int32_t(int64_t(value)-0x100000000LL);
}
inline int32_t Add(int32_t a,int32_t b) {return Signed(uint32_t(a)+uint32_t(b));}
inline int32_t Sub(int32_t a,int32_t b) {return Signed(uint32_t(a)-uint32_t(b));}
inline int32_t Abs(int32_t value) {return value<0?Sub(0,value):value;}
inline int32_t Half(int32_t value) {return int32_t((int64_t(value)-(value<0?1:0))/2);}
inline int32_t Metric(ui_view::Cursor cursor,ui_view::Cursor tile,int32_t width,bool wrap) {
    auto dx=Abs(Sub(cursor[0],tile[0]));
    if(wrap && dx>width/2) {dx=Sub(width,dx);}
    dx=Abs(dx);const auto dy=Abs(Abs(Sub(cursor[1],tile[1])));
    const auto correction=Half(Add(Sub(Half(Add(dx,dy)),std::min(dx,dy)),1));
    return Sub(std::max(dx,dy),correction);
}
}
