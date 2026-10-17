#pragma once

#include <array>
#include <cstdint>

#include "SmacResourceBonus.h"

namespace game::backend::resource::smac {

// All triples are nutrients, minerals, energy. These are decoded rules values,
// not defaults; callers must supply the active RESOURCEINFO rows.
using RuleTriple = std::array< int32_t, 3 >;
using Yield = std::array< int64_t, 3 >;
using ResourceFlags = std::array< bool, 3 >;

struct BaseSquareRules {
	BaseSquareRules( RuleTriple base_square, RuleTriple bonus_square, RuleTriple recycling )
		: base_square( base_square ), bonus_square( bonus_square ), recycling( recycling ) {}
	RuleTriple base_square;
	RuleTriple bonus_square;
	RuleTriple recycling;
};

struct CenterTerrain {
	CenterTerrain( bool land, ResourceBonus resource_bonus, ResourceFlags landmark_bonus, bool river )
		: land( land ), resource_bonus( resource_bonus ), landmark_bonus( landmark_bonus ), river( river ) {}
	bool land;
	ResourceBonus resource_bonus; // already resolved, not an original/port map flag
	ResourceFlags landmark_bonus; // one eligible point per resource, geometry resolved
	bool river;
};

struct CenterFacilities {
	CenterFacilities( bool headquarters, bool recycling_tanks, bool pressure_dome )
		: headquarters( headquarters ), recycling_tanks( recycling_tanks ), pressure_dome( pressure_dome ) {}
	bool headquarters;
	bool recycling_tanks;
	bool pressure_dome;
};

struct CenterFaction {
	CenterFaction( int32_t economy, bool golden_age, uint32_t base_count, bool planetary_governor, bool merchant_exchange_here )
		: economy( economy ), golden_age( golden_age ), base_count( base_count ), planetary_governor( planetary_governor ), merchant_exchange_here( merchant_exchange_here ) {}
	int32_t economy;
	bool golden_age;
	uint32_t base_count;
	bool planetary_governor;
	bool merchant_exchange_here;
};

struct CenterEvents {
	CenterEvents( ResourceFlags increase, ResourceFlags decrease, bool atmospheric_dust, bool solar_multiplier_active )
		: increase( increase ), decrease( decrease ), atmospheric_dust( atmospheric_dust ), solar_multiplier_active( solar_multiplier_active ) {}
	ResourceFlags increase;
	ResourceFlags decrease;
	bool atmospheric_dust;
	bool solar_multiplier_active; // already active; this API does not schedule events
};

struct BaseSquareInputs {
	BaseSquareInputs( CenterTerrain terrain, CenterFacilities facilities, CenterFaction faction, CenterEvents events )
		: terrain( terrain ), facilities( facilities ), faction( faction ), events( events ) {}
	CenterTerrain terrain;
	CenterFacilities facilities;
	CenterFaction faction;
	CenterEvents events;
};

// RE-SMAC-BASE-001: valid founded SMAC centers, before consumption, support,
// commerce or allocation. No SMAX equivalence or live-state adapter is implied.
// Every input is required: absent gameplay systems must not be inferred neutral.
// RE-SMAC-BASE-INTEGER-001: decoded int32 rules/economy and positive base count.
// Ordered arithmetic wraps to signed32 before sign tests and energy clamping.
// Evaluation uses int64 and explicit modulo conversion to avoid C++ overflow;
// the final signed values are widened into the backend resource-value type.
// Invalid resource enums/zero base count throw std::invalid_argument in release.
Yield CalculateBaseSquareYield( const BaseSquareRules& rules, const BaseSquareInputs& inputs );

}
