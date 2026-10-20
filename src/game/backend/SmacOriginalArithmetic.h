#pragma once
#include <cstdint>

namespace game::backend {
// Defined original DWORD storage/arithmetic, without signed C++ overflow.
constexpr int32_t SmacWrap32(int64_t value) noexcept {
    const auto low=uint32_t(value);
    return low<=INT32_MAX?int32_t(low):int32_t(int64_t(low)-4294967296LL);
}
}
