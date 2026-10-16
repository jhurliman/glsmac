#include "SmacTileEnergy.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend::resource::smac {
namespace {

int64_t Checked( const int64_t value ) {
	if ( value < std::numeric_limits< int32_t >::min() || value > std::numeric_limits< int32_t >::max() ) {
		throw std::overflow_error( "SMAC tile energy exceeds the verified signed arithmetic domain" );
	}
	return value;
}

void Validate( const TileEnergyInputs& inputs ) {
	if ( static_cast< uint8_t >( inputs.terrain.altitude ) > static_cast< uint8_t >( AltitudeBand::Land7 ) ||
		inputs.terrain.resource_bonus < ResourceBonus::None || inputs.terrain.resource_bonus > ResourceBonus::Energy ) {
		throw std::invalid_argument( "Unknown SMAC tile energy terrain value" );
	}
	if ( inputs.terrain.altitude < AltitudeBand::Shelf && inputs.terrain.resource_bonus != ResourceBonus::None ) {
		throw std::invalid_argument( "SMAC resource resolution cannot yield a resource below shelf" );
	}
	if ( inputs.context.fungal_capability < 0 ) {
		throw std::invalid_argument( "SMAC fungal capability must be the nonnegative constructed value" );
	}
	if ( inputs.improvements.matching_adjacent_mirrors < 0 || inputs.improvements.matching_adjacent_mirrors > 8 ) {
		throw std::invalid_argument( "SMAC resolved adjacent mirror count must be between 0 and 8" );
	}
}

}

TileEnergyYield CalculateTileEnergy( const TileEnergyRules& rules, const TileEnergyInputs& inputs ) {
	Validate( inputs );
	const auto& terrain = inputs.terrain;
	const auto& improvements = inputs.improvements;
	const auto& context = inputs.context;
	const bool land = terrain.altitude > AltitudeBand::Shelf;
	const bool energy_resource = terrain.resource_bonus == ResourceBonus::Energy;
	const bool qualifying_fungus = terrain.fungus && terrain.altitude >= AltitudeBand::Shelf && !terrain.monolith && !terrain.borehole;
	int64_t value;
	int64_t excess = 0;
	if ( qualifying_fungus ) {
		value = Checked( int64_t( context.fungal_capability ) + std::clamp< int64_t >( context.planet, -3, 0 ) );
		value = std::clamp< int64_t >( value, 0, 99 );
	}
	else {
		if ( terrain.monolith ) { value = rules.monolith; }
		else if ( terrain.borehole ) { value = rules.borehole; }
		else if ( !land ) {
			value = terrain.altitude == AltitudeBand::Shelf ? rules.ocean : 0;
			if ( terrain.altitude == AltitudeBand::Shelf && improvements.solar_collector ) { value = Checked( value + rules.improved_sea ); }
			if ( terrain.forest ) { value = Checked( value - 1 ); }
		}
		else if ( terrain.forest ) {
			value = rules.forest;
			if ( context.hybrid_forest ) { value = Checked( value + 1 ); }
		}
		else {
			value = improvements.solar_collector || improvements.echelon_mirror ? static_cast< int64_t >( terrain.altitude ) - 2 : 0;
			if ( improvements.solar_collector ) { value = Checked( value + improvements.matching_adjacent_mirrors ); }
		}
		if ( land && terrain.river ) { value = Checked( value + 1 ); }
		if ( energy_resource ) { value = Checked( value + rules.bonus ); }
		if ( terrain.landmark_bonus ) { value = Checked( value + 1 ); }
		if ( context.merchant_exchange_here ) { value = Checked( value + 1 ); }
		const auto economy = Checked( int64_t( context.economy ) + ( context.golden_age ? 1 : 0 ) );
		if ( economy >= 2 ) { value = Checked( value + 1 ); }
		if ( value > 2 && !context.energy_technology && !energy_resource ) {
			excess = value - 2;
			value = 2;
		}
	}
	if ( context.heat_wave ) { value = Checked( value + 1 ); }
	if ( context.cloud_cover && value != 0 ) { value = Checked( value - 1 ); }
	if ( context.atmospheric_dust && value != 0 ) { value = Checked( value - 1 ); }
	if ( context.solar_multiplier_active ) { value = Checked( value * 3 ); }
	return { value, excess };
}

}
