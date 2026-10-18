#pragma once
#include <array>
#include <cstdint>

namespace game::backend::map::smac_site {
struct Geometry { int32_t width, height; bool wrap; };
using Coordinate = std::array<int32_t,2>;
}
