#include "SmacConventionalStats.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace game::backend::unit {
namespace {
int32_t Checked( int64_t value ) {
	if ( value < std::numeric_limits< int32_t >::min() || value > std::numeric_limits< int32_t >::max() ) {
		throw std::overflow_error( "SMAC conventional stat arithmetic exceeds signed32" );
	}
	return static_cast< int32_t >( value );
}
void RequireMorale( int32_t value ) {
	if ( value < 0 || value > 6 ) { throw std::invalid_argument( "SMAC conventional morale requires 0..6" ); }
}
}

int32_t SmacConventionalMorale( const SmacConventionalMoraleInput& in ) {
	if ( in.stored_morale < 0 || in.stored_morale > 255 ) { throw std::invalid_argument( "SMAC stored morale requires byte domain 0..255" ); }
	if ( in.mode != SmacMoraleMode::Mode0 && in.mode != SmacMoraleMode::Mode1 ) {
		throw std::invalid_argument( "Unknown SMAC morale query mode" );
	}
	static constexpr std::array< int32_t, 9 > modifiers{ -3, -2, -1, -1, 0, 1, 1, 2, 3 };
	int32_t modifier = modifiers[ static_cast< size_t >( std::clamp( in.retained_morale, -4, 4 ) + 4 ) ];
	if ( in.faction_morale < 0 ) { modifier = Checked( int64_t( modifier ) + in.faction_morale ); }
	if ( in.home && in.home->childrens_creche && modifier < 0 ) { modifier /= 2; }
	if ( in.faction_morale > 0 ) { modifier = Checked( int64_t( modifier ) + in.faction_morale ); }
	if ( in.negative_morale_immunity && modifier < 0 ) { modifier = 0; }
	if ( in.mode == SmacMoraleMode::Mode1 && in.home && in.home->drone_riots && !in.negative_morale_immunity ) {
		modifier = std::clamp( Checked( int64_t( modifier ) - 1 ), 0, 6 );
	}
	return std::clamp( Checked( int64_t( in.stored_morale ) + modifier ), 0, 6 );
}

int32_t SmacConventionalAllowance( const SmacConventionalAllowanceInput& in ) {
	RequireMorale( in.effective_morale );
	if ( in.basic_allowance < 0 || in.movement_quantum <= 0 ||
		( in.domain != SmacMovementDomain::Land && in.domain != SmacMovementDomain::Sea && in.domain != SmacMovementDomain::Air ) ) {
		throw std::invalid_argument( "Invalid SMAC conventional movement metadata" );
	}
	int32_t allowance = in.basic_allowance;
	if ( in.domain == SmacMovementDomain::Sea && in.maritime_control_center ) {
		allowance = Checked( int64_t( allowance ) + Checked( int64_t( 2 ) * in.movement_quantum ) );
	}
	if ( in.effective_morale == 6 && !in.suppress_elite_bonus ) {
		allowance = Checked( int64_t( allowance ) + in.movement_quantum );
	}
	if ( in.damage == 0 || in.domain == SmacMovementDomain::Air ) { return allowance; }
	const int32_t health = 10 * std::clamp( int32_t( in.reactor ), 1, 100 );
	const int32_t left = std::clamp( health - int32_t( in.damage ), 0, 9999 );
	const int32_t denominator = std::clamp( health, 1, 99 );
	const int32_t whole = allowance / in.movement_quantum;
	const int32_t product = Checked( int64_t( left ) * whole );
	// Nonnegative ceil without an avoidable product + denominator - 1 overflow.
	const int32_t scaled = product / denominator + ( product % denominator != 0 );
	const int32_t minimum = in.domain == SmacMovementDomain::Sea ? 2 : 1;
	return Checked( int64_t( in.movement_quantum ) * std::clamp( scaled, minimum, 999 ) );
}

}
