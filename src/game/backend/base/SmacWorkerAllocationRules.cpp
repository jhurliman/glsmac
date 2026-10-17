#include "SmacWorkerAllocationRules.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend::base {
namespace {
int32_t Checked( int64_t value ) {
	if ( value < std::numeric_limits< int32_t >::min() || value > std::numeric_limits< int32_t >::max() ) {
		throw std::invalid_argument( "SMAC worker allocation requires representable signed32 intermediates" );
	}
	return static_cast< int32_t >( value );
}
int32_t Add( int32_t a, int32_t b ) { return Checked( int64_t( a ) + b ); }
int32_t Sub( int32_t a, int32_t b ) { return Checked( int64_t( a ) - b ); }
int32_t Mul( int32_t a, int32_t b ) { return Checked( int64_t( a ) * b ); }
int32_t Clamp( int32_t value ) { return std::clamp( value, 1, 99 ); }
void Count( int32_t value ) {
	if ( value < 0 || value > 128 ) { throw std::invalid_argument( "Unsupported SMAC worker count" ); }
}
void Validate( int32_t population, int32_t slots, int32_t consumption ) {
	if ( population < 0 || population > 127 || consumption <= 0 ) {
		throw std::invalid_argument( "Unsupported SMAC worker population or consumption" );
	}
	Count( slots );
}
void Validate( const SmacWorkerRanking& ranking ) {
	if ( ranking.best_index < 0 || ranking.best_index > 20 || ranking.worst_index < -1 || ranking.worst_index > 20 ) {
		throw std::invalid_argument( "SMAC worker stage requires tile candidates, not specialist choices" );
	}
}
}

SmacWorkerPriorities SmacWorkerAllocationRules::Priorities( const SmacWorkerPriorityContext& c ) {
	if ( !c.human ) { return c.faction; }
	if ( !c.governor_active ) { return {}; }
	return { c.conquer ? 2 : c.explore || c.build ? 0 : c.discover ? -2 : c.faction.aggression,
		c.explore ? 1 : 0, c.discover ? 2 : 0, c.build ? 2 : 0, c.conquer ? 2 : 0 };
}

SmacWorkerScore SmacWorkerAllocationRules::Score( const SmacWorkerScoreContext& c, const std::array< int32_t, 3 >& yield ) {
	Validate( c.population, c.remaining_slots, c.nutrients_per_citizen );
	bool later_project;
	switch ( c.production ) {
		case SmacWorkerProduction::Unit: case SmacWorkerProduction::Facility:
		case SmacWorkerProduction::Stockpile: case SmacWorkerProduction::HumanGenomeProject: later_project = false; break;
		case SmacWorkerProduction::LaterProject: later_project = true; break;
		default: throw std::invalid_argument( "Unknown SMAC worker production category" );
	}
	const auto demand = Mul( c.population, c.nutrients_per_citizen );
	const bool sufficient = Add( c.remaining_slots / c.nutrients_per_citizen, c.intake[ 0 ] ) >= demand;
	const auto boost = sufficient ? 0 : Add( c.remaining_slots, c.intake[ 0 ] ) >= demand ? 2 : 8;
	auto n = Mul( Add( yield[ 0 ], yield[ 0 ] >= 0 && boost > 0 ? std::min( yield[ 0 ], boost ) : 0 ), 2 );
	auto m = Mul( yield[ 1 ], 2 );
	auto e = Mul( yield[ 2 ], 2 );
	if ( c.intake[ 0 ] <= Add( c.population, demand ) && n == 0 ) { m /= 2; e /= 2; }
	if ( m == 0 && boost == 0 ) { n /= 2; }
	const auto food_base = Add( Sub( Sub( c.remaining_slots, c.population / 3 ), demand ), c.intake[ 0 ] );
	const auto food_denominator = Clamp( Checked( int64_t( food_base ) * 2 - 2 ) );
	const auto food = Mul( Mul( Add( c.priorities.explore, 4 ), n ), 256 ) / food_denominator;
	const auto adjustment = later_project ? 2 : Sub( 0, c.priorities.discover );
	const auto mineral_sum = Add( Add( adjustment, c.priorities.aggression ), c.priorities.conquer );
	// The final original LEA adds Build+6 in one operation; checking an
	// artificial intermediate sum here would reject representable inputs.
	const auto mineral_weight = Checked( int64_t( mineral_sum ) + c.priorities.build + 6 );
	const auto mineral_denominator = Clamp( Add( Sub( c.intake[ 1 ], c.mineral_support ), 1 ) / 2 );
	const auto minerals = Mul( Mul( Mul( mineral_weight, c.population ), m ), 64 ) / mineral_denominator;
	const auto energy_weight = Checked( int64_t( c.priorities.build ) + int64_t( c.priorities.discover ) * 2 + 1 );
	const auto energy = Mul( Mul( Mul( energy_weight, Clamp( Sub( c.population, c.priorities.conquer ) ) ), e ), 64 ) / Clamp( c.intake[ 2 ] );
	return { Add( Add( food, minerals ), energy ), sufficient };
}

SmacWorkerRanking SmacWorkerAllocationRules::Rank( const SmacWorkerRanking& previous, int32_t index, bool collected, int32_t score ) {
	Validate( previous );
	if ( index < 1 || index > 20 ) { throw std::invalid_argument( "SMAC worker ranking requires radius index 1..20" ); }
	auto result = previous;
	if ( collected ) {
		if ( score < result.worst_score ) { result.worst_score = score; result.worst_index = index; }
	}
	else if ( score > result.best_score ) { result.best_score = score; result.best_index = index; }
	return result;
}

SmacWorkerStepResult SmacWorkerAllocationRules::Step( const SmacWorkerStepState& state, const SmacWorkerStepContext& c, const SmacWorkerYieldQuery& query ) {
	Validate( c.population, state.remaining_slots, c.nutrients_per_citizen );
	Count( c.retained_specialists ); Count( c.provisional_specialists ); Validate( c.ranking );
	if ( state.visited >> 21 ) { throw std::invalid_argument( "Unsupported SMAC worker visited radius mask" ); }
	const auto budget = Add( c.retained_specialists, c.provisional_specialists ); Count( budget );
	SmacWorkerStepResult result{ state };
	const auto best = c.ranking.best_index, old = c.ranking.worst_index;
	if ( best == 0 ) { return result; }
	const bool vacancy = state.remaining_slots > budget;
	if ( !vacancy && ( old <= 0 || c.ranking.best_score <= c.ranking.worst_score ) ) { return result; }
	const auto best_bit = uint32_t( 1 ) << best;
	if ( ( state.visited & best_bit ) || ( !vacancy && !( state.visited & ( uint32_t( 1 ) << old ) ) ) || !query ) {
		throw std::invalid_argument( "Inconsistent SMAC worker candidates or missing yield query" );
	}
	const auto visit = [ & ]( int32_t index, bool subtract ) {
		const auto tile = query( index, result.state );
		if ( tile ) {
			result.state.sea_participation |= tile->sea;
			for ( size_t i = 0; i < 3; ++i ) {
				result.state.intake[ i ] = subtract ? Sub( result.state.intake[ i ], tile->resources[ i ] ) : Add( result.state.intake[ i ], tile->resources[ i ] );
			}
		}
	};
	result.state.visited |= best_bit;
	if ( vacancy ) {
		visit( best, false );
		--result.state.remaining_slots;
		result.action = SmacWorkerStepAction::Added;
		result.rescan = result.state.remaining_slots > budget || result.state.intake[ 0 ] < Mul( c.population, c.nutrients_per_citizen );
		return result;
	}
	const auto old_bit = uint32_t( 1 ) << old;
	result.state.visited &= ~old_bit;
	visit( old, true ); visit( best, false );
	if ( result.state.intake[ 0 ] >= Mul( Add( c.population, 1 ), c.nutrients_per_citizen ) ) {
		result.action = SmacWorkerStepAction::Swapped;
	}
	else {
		result.state.visited = state.visited;
		visit( best, true ); visit( old, false );
		result.action = SmacWorkerStepAction::RolledBack;
	}
	return result;
}

}
