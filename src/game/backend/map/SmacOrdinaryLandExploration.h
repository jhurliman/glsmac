#pragma once

#include <cstdint>
#include <vector>

namespace game::backend::map::smac_exploration {

struct Coordinate {
	int32_t x;
	int32_t y;
	bool operator==( const Coordinate& other ) const { return x == other.x && y == other.y; }
};

// Supplied ordinary, nonextended land-unit sight only. This does not admit a
// command/design or produce foreign-unit visibility or remembered terrain.
struct Input {
	int32_t width;
	int32_t height;
	bool horizontal_wrap;
	Coordinate center;
	int32_t faction;
	// Row-major doubled-X positions: index = y * (width / 2) + x / 2,
	// with x and y of the same parity. Each array has width * height / 2 entries.
	std::vector< uint8_t > sea; // Each entry is exactly 0 or 1.
	std::vector< uint8_t > explored; // Bit f is exploration by faction f.
};

struct Result {
	std::vector< uint8_t > explored;
	// Caller must refresh the owner's remembered improvements from current map
	// data in this order. Direct visits appear even if previously explored.
	std::vector< Coordinate > refresh_improvements;
};

// Explicit port bounds: even width 10..512, height 1..256, faction 1..7,
// valid land center. Narrow alias maps are outside the captured domain.
// All inputs are validated before copying or changing output state.
Result Explore( const Input& input );

}
