#include "SmacResourceBonus.h"

#include <limits>
#include <stdexcept>

namespace game::backend::resource::smac {
namespace {

int64_t Checked( const int64_t value ) {
	if ( value < std::numeric_limits< int32_t >::min() || value > std::numeric_limits< int32_t >::max() ) {
		throw std::overflow_error( "SMAC resource bonus exceeds the verified signed arithmetic domain" );
	}
	return value;
}

int64_t FloorDivide( const int64_t value, const int64_t divisor ) {
	return value / divisor - ( value % divisor < 0 ? 1 : 0 );
}

int64_t Residue( const int64_t value, const int64_t modulus ) {
	const auto remainder = value % modulus;
	return remainder < 0 ? remainder + modulus : remainder;
}

}

ResourceBonus ResolveResourceBonus( const ResourceBonusInputs& inputs ) {
	if ( inputs.x < 0 || inputs.y < 0 || inputs.x % 2 != inputs.y % 2 ) {
		throw std::invalid_argument( "SMAC resource bonus requires nonnegative parity-matched coordinates" );
	}
	if ( static_cast< uint8_t >( inputs.altitude_band ) > static_cast< uint8_t >( AltitudeBand::Land7 ) ) {
		throw std::invalid_argument( "Unknown SMAC altitude band" );
	}
	const bool land = inputs.altitude_band >= AltitudeBand::Land3;
	if ( !inputs.force_presence && ( inputs.pattern_offset == 0 || ( land && !inputs.automatic_land_resources ) ) ) {
		return ResourceBonus::None;
	}
	const auto u = FloorDivide( Checked( int64_t( inputs.x ) + inputs.y ), 2 );
	const auto v = Checked( int64_t( inputs.x ) - u );
	const auto phase = Residue( u, 4 ) + 4 * Residue( v, 4 );
	if ( !inputs.force_presence ) {
		const auto u_term = Checked( -5 * FloorDivide( u, 4 ) );
		const auto v_term = Checked( -3 * FloorDivide( v, 4 ) );
		const auto target = Residue( Checked( Checked( u_term + v_term ) + inputs.pattern_offset ), 16 );
		if ( phase != target ) { return ResourceBonus::None; }
	}
	if ( inputs.altitude_band < AltitudeBand::Shelf ) { return ResourceBonus::None; }
	const auto coordinate_type = inputs.altitude_band == AltitudeBand::Shelf ? phase % 3 + 1 : ( phase % 5 ) % 4;
	if ( coordinate_type != 0 && !inputs.override_type ) { return static_cast< ResourceBonus >( coordinate_type ); }
	if ( inputs.energy_marker ) { return ResourceBonus::Energy; }
	if ( inputs.mineral_marker ) { return ResourceBonus::Minerals; }
	return ResourceBonus::Nutrients;
}

}
