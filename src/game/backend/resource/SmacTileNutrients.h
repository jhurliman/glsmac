#pragma once

#include "SmacResourceBonus.h"

namespace game::backend::resource::smac {

// Decoded nutrient columns from the active rules, never installed defaults.
struct TileNutrientRules {
	TileNutrientRules( int32_t ocean, int32_t bonus, int32_t forest, int32_t improved_land, int32_t improved_sea, int32_t monolith, int32_t borehole, int32_t mine_effect )
		: ocean( ocean ), bonus( bonus ), forest( forest ), improved_land( improved_land ), improved_sea( improved_sea ), monolith( monolith ), borehole( borehole ), mine_effect( mine_effect ) {}
	int32_t ocean, bonus, forest, improved_land, improved_sea, monolith, borehole, mine_effect;
};

enum class Moisture { Arid, Moist, Rainy };
enum class Rockiness { Flat, Rolling, Rocky };

struct NutrientTerrain {
	NutrientTerrain( AltitudeBand altitude, Moisture moisture, Rockiness rockiness, ResourceBonus resource_bonus, bool landmark_bonus, bool forest, bool fungus, bool borehole, bool monolith )
		: altitude( altitude ), moisture( moisture ), rockiness( rockiness ), resource_bonus( resource_bonus ), landmark_bonus( landmark_bonus ), forest( forest ), fungus( fungus ), borehole( borehole ), monolith( monolith ) {}
	AltitudeBand altitude;
	Moisture moisture;
	Rockiness rockiness;
	ResourceBonus resource_bonus; // resolved effective resource, not explicit map markers
	bool landmark_bonus; // one eligible nutrient point, geometry already resolved
	bool forest, fungus, borehole, monolith;
};

struct NutrientImprovements {
	NutrientImprovements( bool farm, bool mine, bool soil_enricher, bool condenser )
		: farm( farm ), mine( mine ), soil_enricher( soil_enricher ), condenser( condenser ) {}
	bool farm; // kelp on shelf
	bool mine; // mining platform on shelf
	bool soil_enricher, condenser;
};

struct NutrientContext {
	NutrientContext( bool tree_farm, bool hybrid_forest, bool nutrient_technology, int32_t fungal_capability, int32_t planet, bool bumper_crop, bool famine )
		: tree_farm( tree_farm ), hybrid_forest( hybrid_forest ), nutrient_technology( nutrient_technology ), fungal_capability( fungal_capability ), planet( planet ), bumper_crop( bumper_crop ), famine( famine ) {}
	bool tree_farm, hybrid_forest; // actual facilities at the evaluating base
	bool nutrient_technology; // ownership of the active rules' limit technology
	int32_t fungal_capability; // already built, nonnegative; RE-SMAC-FUNGAL-001
	int32_t planet; // effective rating, not a faction definition modifier
	bool bumper_crop, famine; // already-active local events, not scheduling inputs
};

struct TileNutrientInputs {
	TileNutrientInputs( NutrientTerrain terrain, NutrientImprovements improvements, NutrientContext context )
		: terrain( terrain ), improvements( improvements ), context( context ) {}
	NutrientTerrain terrain;
	NutrientImprovements improvements;
	NutrientContext context;
};

struct TileNutrientYield {
	int64_t nutrients;
	int64_t technology_capped_excess; // this tile's contribution, no global mutation
};

// RE-SMAC-TILE-N-001: valid non-center tile/base/faction, normal fungus handling.
// All metadata is required; callers must not infer absent systems to be neutral.
// Reject invalid enums, a resolved resource below shelf, or negative capability
// with std::invalid_argument. All rules and PLANET accept the full int32 range.
// Evaluate ordered arithmetic in int64, checking each result fits int32; throw
// std::overflow_error for unverified original overflow, even before a later cap.
// No live-state adapter, preview modes, terraforming legality or SMAX claim.
TileNutrientYield CalculateTileNutrients( const TileNutrientRules& rules, const TileNutrientInputs& inputs );

}
