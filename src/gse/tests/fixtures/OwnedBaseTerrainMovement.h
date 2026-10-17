#pragma once

#include <array>
#include <cstdint>

namespace gse::tests::owned_base_terrain_fixture {
// Ordered literal projections of RE-SMAC-BASE-TERRAIN-MOVE-001.
// Public cases SHA256: 443556e58378baf6cf30e4f515195a2bd6e9163e5b476eb7598c5894715ac53f
// Costs equal added spending here (initial spent0, full point available).
// Original linked-list indices are represented by actual membership assertions.
struct Case { const char* id; bool rover; int sx, sy, dx, dy, bx, by, population; bool hq; int source_flags, destination_flags, rockiness, allowance, spent; uint32_t shared_after; };
inline constexpr std::array< Case, 18 > CASES{{
	{ "gaians-rocky-near", false, 18, 16, 19, 15, 16, 16, 1, true, 0, 0, 2, 3, 6, 12345u },
	{ "gaians-rocky-far", false, 32, 16, 33, 15, 16, 16, 1, true, 0, 0, 2, 3, 6, 12345u },
	{ "gaians-forest-near", false, 18, 16, 19, 15, 16, 16, 1, true, 0, 2097152, 0, 3, 6, 12345u },
	{ "gaians-forest-far", false, 32, 16, 33, 15, 16, 16, 1, true, 0, 2097152, 0, 3, 6, 12345u },
	{ "gaians-diagonal_river-near", false, 18, 16, 19, 15, 16, 16, 1, true, 128, 128, 0, 3, 1, 12345u },
	{ "gaians-diagonal_river-far", false, 32, 16, 33, 15, 16, 16, 1, true, 128, 128, 0, 3, 1, 12345u },
	{ "gaians-road_to_unroaded_forest-near", false, 18, 16, 19, 15, 16, 16, 1, true, 4, 2097152, 0, 3, 6, 12345u },
	{ "gaians-road_to_unroaded_forest-far", false, 32, 16, 33, 15, 16, 16, 1, true, 4, 2097152, 0, 3, 6, 12345u },
	{ "gaians-rocky-source_larger_no_hq", false, 16, 16, 17, 15, 16, 16, 7, false, 0, 0, 2, 3, 6, 12345u },
	{ "spartans-rocky-near", true, 18, 16, 19, 15, 16, 16, 1, true, 0, 0, 2, 6, 6, 12345u },
	{ "spartans-rocky-far", true, 32, 16, 33, 15, 16, 16, 1, true, 0, 0, 2, 6, 6, 12345u },
	{ "spartans-forest-near", true, 18, 16, 19, 15, 16, 16, 1, true, 0, 2097152, 0, 6, 6, 12345u },
	{ "spartans-forest-far", true, 32, 16, 33, 15, 16, 16, 1, true, 0, 2097152, 0, 6, 6, 12345u },
	{ "spartans-diagonal_river-near", true, 18, 16, 19, 15, 16, 16, 1, true, 128, 128, 0, 6, 1, 12345u },
	{ "spartans-diagonal_river-far", true, 32, 16, 33, 15, 16, 16, 1, true, 128, 128, 0, 6, 1, 12345u },
	{ "spartans-road_to_unroaded_forest-near", true, 18, 16, 19, 15, 16, 16, 1, true, 4, 2097152, 0, 6, 6, 12345u },
	{ "spartans-road_to_unroaded_forest-far", true, 32, 16, 33, 15, 16, 16, 1, true, 4, 2097152, 0, 6, 6, 12345u },
	{ "spartans-rocky-source_larger_no_hq", true, 16, 16, 17, 15, 16, 16, 7, false, 0, 0, 2, 6, 6, 12345u },
}};
}
