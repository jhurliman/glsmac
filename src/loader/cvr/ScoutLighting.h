#pragma once

#include <array>
#include <cstdint>

namespace loader::cvr::scout {

using LightingTable = std::array< std::array< uint8_t, 2144 >, 24 >;

// Construct the independently evidenced replacement table, not a copy of
// executable data. No per-object window or original table bytes are inputs.
LightingTable GenerateLightingTable();

struct LightingInput {
	std::array< uint32_t, 9 > matrix, auxiliary;
	std::array< uint32_t, 3 > light;
	uint32_t scalar;
	std::array< uint32_t, 2 > scales;
};

struct LightingSelection {
	std::array< uint32_t, 9 > inverse;
	std::array< uint32_t, 3 > transformed_light; // Before negative-Z reflection.
	unsigned row, offset;
};

void RequireNearestRounding();
LightingSelection SelectLighting( const LightingInput& input );

}
