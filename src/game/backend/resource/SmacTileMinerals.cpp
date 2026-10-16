#include "SmacTileMinerals.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend::resource::smac {
namespace {

int64_t Checked( const int64_t value ) {
	if ( value < std::numeric_limits< int32_t >::min() || value > std::numeric_limits< int32_t >::max() ) {
		throw std::overflow_error( "SMAC tile minerals exceed the verified signed arithmetic domain" );
	}
	return value;
}

void Validate( const TileMineralInputs& inputs ) {
	const auto& terrain = inputs.terrain;
	if ( static_cast< uint8_t >( terrain.altitude ) > static_cast< uint8_t >( AltitudeBand::Land7 ) ||
		terrain.rockiness < 0 || terrain.rockiness > 3 ||
		terrain.resource_bonus < ResourceBonus::None || terrain.resource_bonus > ResourceBonus::Energy ) {
		throw std::invalid_argument( "Unknown SMAC tile mineral terrain value" );
	}
	if ( terrain.altitude < AltitudeBand::Shelf && terrain.resource_bonus != ResourceBonus::None ) {
		throw std::invalid_argument( "SMAC resource resolution cannot yield a resource below shelf" );
	}
	if ( inputs.context.fungal_capability < 0 ) {
		throw std::invalid_argument( "SMAC fungal capability must be the nonnegative constructed value" );
	}
}

}

TileMineralYield CalculateTileMinerals( const TileMineralRules& rules, const TileMineralInputs& inputs ) {
	Validate( inputs );
	const auto& terrain = inputs.terrain;
	const auto& improvements = inputs.improvements;
	const auto& context = inputs.context;
	const bool mineral_resource = terrain.resource_bonus == ResourceBonus::Minerals;
	const int64_t bonus = mineral_resource ? rules.bonus : 0;
	// This sum is evaluated in the original even when a special branch replaces it.
	int64_t value = Checked( bonus + ( terrain.landmark_bonus ? 1 : 0 ) );
	int64_t removed = 0;
	bool apply_cap = true;
	if ( terrain.monolith ) {
		value = Checked( int64_t( rules.monolith ) + bonus );
		apply_cap = false;
	}
	else if ( terrain.borehole ) {
		value = Checked( int64_t( rules.borehole ) + bonus );
	}
	else if ( terrain.fungus && terrain.altitude >= AltitudeBand::Shelf ) {
		value = Checked( int64_t( context.fungal_capability ) + std::clamp< int64_t >( context.planet, -3, 0 ) );
		value = std::clamp< int64_t >( value, 0, 99 );
		apply_cap = false;
	}
	else if ( terrain.altitude <= AltitudeBand::Shelf ) {
		value = Checked( value + rules.ocean );
		if ( terrain.altitude == AltitudeBand::Shelf && improvements.mine ) {
			value = Checked( value + rules.improved_sea );
			if ( context.advanced_platform_technology ) { value = Checked( value + 1 ); }
		}
	}
	else if ( terrain.forest ) {
		value = Checked( value + rules.forest );
	}
	else if ( improvements.mine ) {
		value = Checked( value + terrain.rockiness );
		int64_t contribution = std::max< int64_t >( 1, terrain.rockiness );
		if ( mineral_resource || terrain.landmark_bonus ) { ++contribution; }
		if ( !improvements.road && contribution > rules.unroaded_mine_limit ) {
			removed = Checked( contribution - rules.unroaded_mine_limit );
			contribution = rules.unroaded_mine_limit;
		}
		value = Checked( value + contribution );
	}
	else if ( terrain.rockiness != 0 ) {
		value = Checked( value + 1 );
	}
	if ( apply_cap && value > 2 && !context.mineral_technology && !mineral_resource ) {
		removed = Checked( removed + ( value - 2 ) );
		value = 2;
	}
	if ( context.industrial_boom ) { value = Checked( value + 1 ); }
	if ( context.industrial_bust && value != 0 ) { value = Checked( value - 1 ); }
	return { value, removed };
}

}
