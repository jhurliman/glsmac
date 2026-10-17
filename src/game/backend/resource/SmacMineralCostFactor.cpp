#include "SmacMineralCostFactor.h"

#include <array>
#include <limits>
#include <stdexcept>

namespace game::backend::resource::smac {
namespace {

int64_t Checked( const int64_t value ) {
	if ( value < 0 || value > std::numeric_limits< int32_t >::max() ) {
		throw std::overflow_error( "SMAC mineral cost factor exceeds the supported signed arithmetic domain" );
	}
	return value;
}

}

int32_t CalculateMineralCostFactor( const MineralCostInputs& inputs ) {
	if ( inputs.global_difficulty < 0 || inputs.global_difficulty > 5 || inputs.mineral_multiplier < 0 ) {
		throw std::invalid_argument( "Invalid SMAC mineral cost difficulty or multiplier" );
	}
	switch ( inputs.world_size ) {
		case MineralCostWorldSize::Tiny:
		case MineralCostWorldSize::Small:
		case MineralCostWorldSize::StandardOrLarger:
			break;
		default:
			throw std::invalid_argument( "Unknown SMAC mineral cost world category" );
	}
	static constexpr std::array< int32_t, 6 > ai_factors = { 13, 12, 11, 10, 8, 7 };
	int64_t value = 10;
	if ( !inputs.human ) {
		value = ai_factors[ inputs.global_difficulty ];
		if ( inputs.leader_human && !inputs.network_session ) { --value; }
		if ( inputs.leading_human_dominance ) { --value; }
	}
	value = Checked( value * inputs.mineral_multiplier ) / 10;
	if ( inputs.world_size == MineralCostWorldSize::Tiny ) { value = Checked( value * 8 ) / 10; }
	else if ( inputs.world_size == MineralCostWorldSize::Small ) { value = Checked( value * 9 ) / 10; }

	// Preserve the out-of-range branch on both sides, without overflowing an
	// INDUSTRY+7 range check. Ratios below retain separate upward rounding and
	// the reduced numerators used by the original's signed arithmetic.
	if ( inputs.industry <= -8 || inputs.industry >= 5 ) { value = Checked( value + 1 ) / 2; }
	else if ( inputs.industry <= -3 ) { value = Checked( value * 13 + 9 ) / 10; }
	else {
		switch ( inputs.industry ) {
			case -2: value = Checked( value * 6 + 4 ) / 5; break;
			case -1: value = Checked( value * 11 + 9 ) / 10; break;
			case 0: break;
			case 1: value = Checked( value * 9 + 9 ) / 10; break;
			case 2: value = Checked( value * 4 + 4 ) / 5; break;
			case 3: value = Checked( Checked( value * 8 ) - value + 9 ) / 10; break;
			case 4: value = Checked( value * 3 + 4 ) / 5; break;
		}
	}
	return static_cast< int32_t >( value );
}

}
