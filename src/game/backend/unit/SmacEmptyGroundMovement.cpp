#include "SmacEmptyGroundMovement.h"

#include <algorithm>
#include <stdexcept>

namespace game::backend::unit::smac_move {
namespace {
void ValidateState( const MovementState& unit ) {
	if ( unit.spent < 0 || unit.spent > 255 ) {
		throw std::invalid_argument( "SMAC empty-ground movement requires byte-domain spent movement" );
	}
}
}

MovementState::MovementState( uint64_t identity, int32_t spent, bool prior_failure_marker )
	: identity( identity ), spent( spent ), prior_failure_marker( prior_failure_marker ) {}

Result ResolveEmptyGroundStep( const MovementState& unit, int32_t quantum, int32_t full_allowance, int32_t raw_edge_cost, bool destination_fungus, const SmacGameplayRandomState& random ) {
	ValidateState( unit );
	if ( quantum < 1 || quantum > 100 || full_allowance < 0 || full_allowance > 255 ||
		raw_edge_cost < 0 || raw_edge_cost > 255 || int64_t( unit.spent ) + raw_edge_cost > 255 ) {
		throw std::invalid_argument( "SMAC empty-ground movement requires bounded Q, allowance and nonwrapping byte cost" );
	}
	Result result{ unit, Outcome::Exhausted, Position::Source, { true, false }, random, std::nullopt, 0 };
	const int64_t available = int64_t( full_allowance ) - unit.spent;
	if ( available <= 0 ) { return result; }
	const int64_t remaining = std::clamp< int64_t >( available, 0, 999 );
	bool admitted = raw_edge_cost <= remaining || ( remaining >= quantum && !destination_fungus );
	if ( !admitted ) {
		const uint32_t before = result.random.Shared();
		const uint16_t raw = result.random.NextSharedLow16();
		const int32_t scaled = static_cast< int32_t >( uint64_t( raw ) * static_cast< uint32_t >( raw_edge_cost ) / 65536 );
		result.draw = Draw{ before, result.random.Shared(), raw, scaled };
		const int64_t threshold = remaining + ( unit.prior_failure_marker ? int64_t( quantum ) : 0 );
		admitted = scaled < threshold;
	}
	result.original_wrapper_return = 1;
	if ( admitted ) {
		result.unit.spent += raw_edge_cost;
		result.unit.prior_failure_marker = false;
		result.outcome = Outcome::Moved;
		result.position = Position::Destination;
		result.unit_present = { false, true };
	} else {
		result.unit.spent = full_allowance;
		result.unit.prior_failure_marker = true;
		result.outcome = Outcome::FailedAttempt;
	}
	return result;
}

MovementState ResetSpentForCheckedUnitUpkeep( const MovementState& unit ) {
	ValidateState( unit );
	return { unit.identity, 0, unit.prior_failure_marker };
}

}
