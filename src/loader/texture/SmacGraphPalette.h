#pragma once
#include <array>
#include <cstdint>

namespace loader::texture {
struct SmacGraphPalette {
    std::array<std::array<uint8_t,3>,256> colors;
    std::array<uint8_t,8> normal,alternate,text{};
    bool text_available=false;
};
}
