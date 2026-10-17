#include "SmacBaseSquareYield.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend::resource::smac {
namespace {

int64_t Checked( const int64_t value ) {
	if ( value < std::numeric_limits< int32_t >::min() || value > std::numeric_limits< int32_t >::max() ) {
		throw std::overflow_error( "SMAC base-square yield exceeds the verified signed arithmetic domain" );
	}
	return value;
}

int64_t ApplyLocalEvents( int64_t value, const bool increase, const bool decrease ) {
	if ( increase ) { value = Checked( value + 1 ); }
	if ( decrease && value != 0 ) { value = Checked( value - 1 ); }
	return value;
}

}

Yield CalculateBaseSquareYield( const BaseSquareRules& rules, const BaseSquareInputs& inputs ) {
	if ( inputs.faction.base_count == 0 ) {
		throw std::invalid_argument( "SMAC base-square yield requires a founded faction base" );
	}
	switch ( inputs.terrain.resource_bonus ) {
		case ResourceBonus::None:
		case ResourceBonus::Nutrients:
		case ResourceBonus::Minerals:
		case ResourceBonus::Energy:
			break;
		default:
			throw std::invalid_argument( "Unknown SMAC resource bonus" );
	}
	const bool recycling = inputs.facilities.recycling_tanks || inputs.facilities.pressure_dome;
	const auto& terrain = inputs.terrain;
	const auto& events = inputs.events;

	int64_t nutrients = rules.base_square[ 0 ];
	if ( terrain.resource_bonus == ResourceBonus::Nutrients ) { nutrients = Checked( nutrients * 2 ); }
	if ( terrain.landmark_bonus[ 0 ] ) { nutrients = Checked( nutrients + 1 ); }
	if ( recycling ) { nutrients = Checked( nutrients + rules.recycling[ 0 ] ); }
	nutrients = ApplyLocalEvents( nutrients, events.increase[ 0 ], events.decrease[ 0 ] );

	int64_t minerals = rules.base_square[ 1 ];
	if ( terrain.resource_bonus == ResourceBonus::Minerals ) { minerals = Checked( minerals + rules.bonus_square[ 1 ] ); }
	if ( terrain.landmark_bonus[ 1 ] ) { minerals = Checked( minerals + 1 ); }
	if ( recycling ) { minerals = Checked( minerals + rules.recycling[ 1 ] ); }
	minerals = ApplyLocalEvents( minerals, events.increase[ 1 ], events.decrease[ 1 ] );

	int64_t energy = rules.base_square[ 2 ];
	if ( inputs.facilities.headquarters ) { energy = Checked( energy + 1 ); }
	if ( recycling ) { energy = Checked( energy + rules.recycling[ 2 ] ); }
	const auto economy = Checked( int64_t( inputs.faction.economy ) + ( inputs.faction.golden_age ? 1 : 0 ) );
	int64_t base_economy;
	if ( economy < 0 ) {
		base_economy = economy;
		if ( !inputs.facilities.headquarters || economy < -1 || inputs.faction.base_count == 1 ) {
			base_economy = Checked( base_economy + 1 );
		}
	}
	else if ( economy < 2 ) {
		base_economy = economy;
	}
	else {
		// The checked original applies +2/+4 at effective economy +3/+4,
		// despite different thresholds in the installed economy descriptions.
		base_economy = 2 * ( std::min< int64_t >( economy, 4 ) - 2 );
	}
	energy = Checked( energy + base_economy );
	if ( inputs.faction.planetary_governor ) { energy = Checked( energy + 1 ); }
	if ( terrain.land && terrain.river ) { energy = Checked( energy + 1 ); }
	if ( terrain.resource_bonus == ResourceBonus::Energy ) { energy = Checked( energy + rules.bonus_square[ 2 ] ); }
	if ( terrain.landmark_bonus[ 2 ] ) { energy = Checked( energy + 1 ); }
	if ( inputs.faction.merchant_exchange_here ) { energy = Checked( energy + 1 ); }
	if ( economy >= 2 ) { energy = Checked( energy + 1 ); }
	energy = std::clamp< int64_t >( energy, 0, 99 );
	energy = ApplyLocalEvents( energy, events.increase[ 2 ], events.decrease[ 2 ] );
	if ( events.atmospheric_dust && energy != 0 ) { energy = Checked( energy - 1 ); }
	if ( events.solar_multiplier_active ) { energy = Checked( energy * 3 ); }
	return { nutrients, minerals, energy };
}

}
