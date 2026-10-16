#pragma once

#include "SmacResourceBonus.h"

namespace game::backend::resource::smac {

struct TileMineralRules {
	TileMineralRules( int32_t ocean, int32_t bonus, int32_t forest, int32_t improved_sea, int32_t monolith, int32_t borehole, int32_t unroaded_mine_limit )
		: ocean( ocean ), bonus( bonus ), forest( forest ), improved_sea( improved_sea ), monolith( monolith ), borehole( borehole ), unroaded_mine_limit( unroaded_mine_limit ) {}
	int32_t ocean, bonus, forest, improved_sea, monolith, borehole, unroaded_mine_limit;
};

struct MineralTerrain {
	MineralTerrain( AltitudeBand altitude, int32_t rockiness, ResourceBonus resource_bonus, bool landmark_bonus, bool forest, bool fungus, bool borehole, bool monolith )
		: altitude( altitude ), rockiness( rockiness ), resource_bonus( resource_bonus ), landmark_bonus( landmark_bonus ), forest( forest ), fungus( fungus ), borehole( borehole ), monolith( monolith ) {}
	AltitudeBand altitude;
	int32_t rockiness; // decoded 0..3; 3 is an encoded boundary, not a named terrain class
	ResourceBonus resource_bonus; // already resolved, not explicit map markers
	bool landmark_bonus; // one eligible mineral point, geometry already resolved
	bool forest, fungus, borehole, monolith;
};

struct MineralImprovements {
	MineralImprovements( bool mine, bool road ) : mine( mine ), road( road ) {}
	bool mine; // mining platform on shelf
	bool road; // this API does not infer road presence from mag-tube flags
};

struct MineralContext {
	MineralContext( bool mineral_technology, bool advanced_platform_technology, int32_t fungal_capability, int32_t planet, bool industrial_boom, bool industrial_bust )
		: mineral_technology( mineral_technology ), advanced_platform_technology( advanced_platform_technology ), fungal_capability( fungal_capability ), planet( planet ), industrial_boom( industrial_boom ), industrial_bust( industrial_bust ) {}
	bool mineral_technology, advanced_platform_technology; // actual configured technology ownership
	int32_t fungal_capability; // already built and nonnegative, RE-SMAC-FUNGAL-001
	int32_t planet; // effective rating
	bool industrial_boom, industrial_bust; // already-active local events
};

struct TileMineralInputs {
	TileMineralInputs( MineralTerrain terrain, MineralImprovements improvements, MineralContext context )
		: terrain( terrain ), improvements( improvements ), context( context ) {}
	MineralTerrain terrain;
	MineralImprovements improvements;
	MineralContext context;
};

struct TileMineralYield {
	int64_t minerals;
	int64_t removed_minerals; // this tile's road + technology restrictions, no global mutation
};

// RE-SMAC-TILE-M-001: valid non-center tile/base/faction, normal evaluation mode.
// Every input is required; absent systems must not be inferred neutral.
// All rules and PLANET accept int32. Invalid enums, rockiness outside 0..3,
// resources below shelf or negative built capability throw std::invalid_argument.
// Ordered calculations use int64 and reject evaluated int32 overflow, including
// removed amounts and the initial resource+landmark sum before special branches
// replace it. std::overflow_error excludes unverified original overflow.
// No live adapter, preview-mode behavior, terraforming legality or SMAX claim.
TileMineralYield CalculateTileMinerals( const TileMineralRules& rules, const TileMineralInputs& inputs );

}
