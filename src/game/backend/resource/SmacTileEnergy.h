#pragma once

#include "SmacResourceBonus.h"

namespace game::backend::resource::smac {

struct TileEnergyRules {
	TileEnergyRules( int32_t ocean, int32_t bonus, int32_t forest, int32_t improved_sea, int32_t monolith, int32_t borehole )
		: ocean( ocean ), bonus( bonus ), forest( forest ), improved_sea( improved_sea ), monolith( monolith ), borehole( borehole ) {}
	int32_t ocean, bonus, forest, improved_sea, monolith, borehole;
};

struct EnergyTerrain {
	EnergyTerrain( AltitudeBand altitude, ResourceBonus resource_bonus, bool landmark_bonus, bool forest, bool fungus, bool monolith, bool borehole, bool river )
		: altitude( altitude ), resource_bonus( resource_bonus ), landmark_bonus( landmark_bonus ), forest( forest ), fungus( fungus ), monolith( monolith ), borehole( borehole ), river( river ) {}
	AltitudeBand altitude;
	ResourceBonus resource_bonus; // effective resolved resource, not an explicit map marker
	bool landmark_bonus; // one eligible energy point; geometry already resolved
	bool forest, fungus, monolith, borehole, river;
};

struct EnergyImprovements {
	EnergyImprovements( bool solar_collector, bool echelon_mirror, int32_t matching_adjacent_mirrors )
		: solar_collector( solar_collector ), echelon_mirror( echelon_mirror ), matching_adjacent_mirrors( matching_adjacent_mirrors ) {}
	bool solar_collector; // Tidal Harness on shelf
	bool echelon_mirror;
	int32_t matching_adjacent_mirrors; // resolved 0..8, matching the current evaluating base's faction
};

struct EnergyContext {
	EnergyContext( bool hybrid_forest, bool energy_technology, int32_t fungal_capability, int32_t planet, int32_t economy, bool golden_age, bool merchant_exchange_here,
		bool heat_wave, bool cloud_cover, bool atmospheric_dust, bool solar_multiplier_active )
		: hybrid_forest( hybrid_forest ), energy_technology( energy_technology ), fungal_capability( fungal_capability ), planet( planet ), economy( economy ),
		golden_age( golden_age ), merchant_exchange_here( merchant_exchange_here ), heat_wave( heat_wave ), cloud_cover( cloud_cover ),
		atmospheric_dust( atmospheric_dust ), solar_multiplier_active( solar_multiplier_active ) {}
	bool hybrid_forest; // actual facility at the evaluating base
	bool energy_technology; // ownership of the active rules' energy-limit technology
	int32_t fungal_capability; // already constructed and nonnegative, RE-SMAC-FUNGAL-001
	int32_t planet, economy; // effective social ratings
	bool golden_age, merchant_exchange_here;
	bool heat_wave, cloud_cover, atmospheric_dust, solar_multiplier_active; // already-active modifiers
};

struct TileEnergyInputs {
	TileEnergyInputs( EnergyTerrain terrain, EnergyImprovements improvements, EnergyContext context )
		: terrain( terrain ), improvements( improvements ), context( context ) {}
	EnergyTerrain terrain;
	EnergyImprovements improvements;
	EnergyContext context;
};

struct TileEnergyYield {
	int64_t energy;
	int64_t technology_capped_excess; // this tile's contribution before events, no global mutation
};

// RE-SMAC-TILE-E-001: valid non-center tile/base/faction, normal evaluation mode.
// Every input is required; absent systems must not be inferred neutral.
// Invalid enums, resources below shelf, negative constructed capability or a
// mirror count outside 0..8 throw std::invalid_argument. Rules/ratings accept int32.
// Ordered arithmetic uses int64 and rejects evaluated int32 overflow with
// std::overflow_error, even before a later cap. Original wrap behavior is unverified.
// No mirror geometry/affiliation resolver, live adapter, preview modes or SMAX claim.
TileEnergyYield CalculateTileEnergy( const TileEnergyRules& rules, const TileEnergyInputs& inputs );

}
