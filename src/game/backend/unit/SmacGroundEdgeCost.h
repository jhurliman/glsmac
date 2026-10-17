#pragma once

#include <cstdint>

namespace game::backend::unit::smac_edge {

// Original positional chassis IDs, restricted to ordinary ground designs.
enum class Chassis : int32_t { Infantry = 0, Speeder = 1 };
enum Feature : uint32_t {
	Base = 0x1, Road = 0x4, Tube = 0x8, Fungus = 0x20,
	River = 0x80, Forest = 0x200000
};

struct Design {
	int32_t id;
	Chassis chassis;
	int32_t domain, weapon_offense, weapon_mode, plan;
	uint32_t abilities;
	// Supplied basic design allowance, not damaged/remaining unit movement.
	int32_t basic_allowance;
};

struct Tile {
	int32_t x, y, altitude_band, rockiness;
	uint32_t features;
};

struct Query {
	Design design;
	int32_t quantum, faction, retained_planet;
	bool owns_xenoempathy, suppress_terrain_charges;
	int32_t width, height;
	bool horizontal_wrap;
	Tile source, destination;
};

// Owned values only. Explicit port domain: Q1..100, basic allowance Q..99Q,
// faction1..7, ordinary Infantry/Speeder, ground/projectile/positive offense,
// plans0..3, no abilities or native design IDs8/9; valid land coordinates and
// only the feature bits above. Unsupported inputs throw invalid_argument.
// This query does not admit commands, check adjacency or mutate unit/RNG state.
int32_t QueryGroundEdgeCost( const Query& query );

}
