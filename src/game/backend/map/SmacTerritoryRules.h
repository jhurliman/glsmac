#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace game::backend::map {
class SmacWorkerKnowledge;
namespace smac_territory {

using Coordinate = std::array< int32_t, 2 >;
struct Tile {
	bool sea;
	uint8_t region; // supplied current topology: land1..63, sea65..127
};
struct Base {
	Coordinate coordinate;
	int32_t owner; // playable1..7; native territory is outside this receipt
};
struct Input {
	int32_t width, height;
	bool horizontal_wrap;
	int32_t border_distance; // nonnegative signed32; sea uses floor(value/2)
	// Y ascending, X from row parity in steps of two, including polar rows.
	std::vector< Tile > tiles;
	// Explicit current record order. Later records win equal-distance ties.
	// This is neither inferred creation age nor a sorted owner/port-ID order.
	std::vector< Base > bases;
};
using Claims = std::vector< std::optional< int32_t > >;

// Bounded to the existing worker grid (even width2..512, height1..256), one
// Base per tile. Regions must already exist; this does not flood-fill them.
// Returns every claim, including unclaimed. No old claim is retained.
Claims Rebuild( const Input& input );

// Owned-value composition only: exact geometry must match. Keeps exploration,
// observer relations and Unit observations byte-for-byte. No live publication,
// event scheduling or wire-format change is implied by this operation.
SmacWorkerKnowledge ReplaceClaims( const SmacWorkerKnowledge& knowledge, const Input& input );

}
}
