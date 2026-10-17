#include "SmacSpecialistAllocation.h"

#include <algorithm>
#include <stdexcept>

namespace game::backend::base {
namespace {
int32_t Checked( int64_t value ) {
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::invalid_argument( "SMAC specialist allocation requires signed32 intermediates" ); }
	return static_cast< int32_t >( value );
}
int32_t Add( int32_t a, int32_t b ) { return Checked( int64_t( a ) + b ); }
int32_t Mul( int32_t a, int32_t b ) { return Checked( int64_t( a ) * b ); }
void Count( int32_t value, int32_t maximum = 127 ) {
	if ( value < 0 || value > maximum ) { throw std::invalid_argument( "Unsupported SMAC specialist allocation count" ); }
}
void Type( uint8_t type ) {
	if ( type >= 7 ) { throw std::invalid_argument( "Unsupported SMAC specialist positional type" ); }
}
void Types( const std::array< uint8_t, 16 >& types ) { for ( const auto type : types ) { Type( type ); } }
void Validate( const SmacSpecialistBookkeeping& state ) {
	Types( state.types ); Count( state.retained ); Count( state.provisional ); Count( state.retained + state.provisional );
}
uint8_t Read( const std::array< uint8_t, 16 >& types, int32_t index, std::optional< uint8_t > overflow ) {
	if ( index < 16 ) { return types[ index ]; }
	if ( !overflow ) { throw std::invalid_argument( "SMAC specialist overflow read requires an explicit initial selector result" ); }
	return *overflow;
}
void DemandInputs( int32_t population, int32_t consumption ) {
	Count( population );
	if ( consumption <= 0 ) { throw std::invalid_argument( "SMAC specialist allocation requires positive consumption" ); }
}
}

SmacSpecialistBookkeeping SmacSpecialistAllocation::Split( const SmacSpecialistSlots& slots,
	const SmacSpecialistAllocationCatalog& catalog, bool managed, bool upkeep, std::optional< uint8_t > overflow ) {
	Types( slots.types ); Count( slots.total_count ); if ( overflow ) { Type( *overflow ); }
	SmacSpecialistBookkeeping result{ slots.types, slots.total_count, 0 };
	if ( managed && !upkeep ) {
		for ( int32_t i = 0; i < slots.total_count; ++i ) {
			if ( catalog.rows[ Read( result.types, i, overflow ) ].psych == 0 ) { ++result.provisional; }
		}
		result.retained -= result.provisional;
	}
	return result;
}

SmacSpecialistBookkeeping SmacSpecialistAllocation::Compact( const SmacSpecialistBookkeeping& state,
	const SmacSpecialistAllocationCatalog& catalog, bool managed, std::optional< uint8_t > overflow ) {
	Validate( state ); if ( overflow ) { Type( *overflow ); }
	auto result = state;
	if ( managed && result.retained > 0 ) {
		for ( int32_t i = 0; i < result.retained + result.provisional; ++i ) {
			if ( catalog.rows[ Read( result.types, i, overflow ) ].psych != 0 ) { continue; }
			// Read through the old active end, including its padding/overflow
			// value. Increment i even after removal: adjacent zeros can survive.
			for ( int32_t j = i; j < result.retained + result.provisional; ++j ) {
				const auto type = Read( result.types, j + 1, overflow );
				if ( j < 16 ) { result.types[ j ] = type; }
			}
			if ( result.provisional ) { --result.provisional; }
		}
	}
	return result;
}

SmacSpecialistCompetition SmacSpecialistAllocation::Compete( const SmacSpecialistAllocationCatalog& catalog,
	const SmacSpecialistCompetitionContext& c ) {
	DemandInputs( c.population, c.nutrients_per_citizen ); Count( c.provisional );
	if ( c.maximum_candidate_nutrients < 0 ) { throw std::invalid_argument( "SMAC candidate nutrient maximum starts at zero" ); }
	SmacSpecialistCompetition result{ {}, c.tile_best_score };
	if ( !c.managed || c.population < 5 ) { return result; }
	// Original 0x4fe0b5 evaluates demand before the turn gate at 0x4fe0d0.
	// Early turns skip later scoring, not this checked intermediate.
	if ( c.current_nutrients <= Mul( c.population, c.nutrients_per_citizen ) && c.maximum_candidate_nutrients != 0 ) { return result; }
	if ( c.turn <= 100 ) { return result; }
	for ( uint8_t id = 0; id < 7; ++id ) {
		const auto& row = catalog.rows[ id ];
		if ( !row.prerequisite_available || row.obsolete_available ) { continue; }
		auto score = Mul( row.psych, 64 );
		const auto denominator = std::clamp( Add( c.current_energy, c.provisional ), 1, 99 );
		const auto economy = Mul( Mul( Mul( Add( c.build_priority, 2 ), c.population ), row.economy ), 64 ) / denominator;
		score = Add( score, economy );
		const auto labs = Mul( Mul( Mul( Add( c.discover_priority, 2 ), c.population ), row.labs ), 64 ) / denominator;
		score = Add( score, labs );
		if ( score > result.score ) { result = { id, score }; }
	}
	return result;
}

SmacSpecialistInsertion SmacSpecialistAllocation::Insert( const SmacSpecialistBookkeeping& state, uint8_t winner,
	const SmacWorkerStepState& workers, int32_t population, int32_t consumption ) {
	Validate( state ); Type( winner ); Count( workers.remaining_slots, 128 ); DemandInputs( population, consumption );
	SmacSpecialistInsertion result{ state };
	if ( workers.remaining_slots <= state.retained + state.provisional ) { return result; }
	Count( state.retained + state.provisional + 1 );
	if ( state.provisional < 16 ) { result.specialists.types[ state.provisional ] = winner; }
	++result.specialists.provisional;
	result.inserted = true;
	result.rescan = workers.remaining_slots > result.specialists.retained + result.specialists.provisional
		|| workers.intake[ 0 ] < Mul( population, consumption );
	return result;
}

SmacSpecialistSlots SmacSpecialistAllocation::Fill( const SmacSpecialistBookkeeping& state, int32_t remaining_slots,
	const SmacSpecialistAllocationCatalog& catalog, int32_t population ) {
	Validate( state ); Count( remaining_slots ); Count( population );
	SmacSpecialistSlots result{ state.types, remaining_slots };
	for ( int32_t i = state.retained; i < remaining_slots; ++i ) {
		int32_t best_score = -999;
		std::optional< uint8_t > best;
		for ( uint8_t id = 0; id < 7; ++id ) {
			const auto& row = catalog.rows[ id ];
			if ( !row.prerequisite_available ) { continue; }
			auto score = Mul( row.psych, 3 );
			if ( population >= catalog.minimum_base_size ) { score = Add( score, Add( row.economy, row.labs ) ); }
			if ( score > best_score ) { best_score = score; best = id; }
		}
		if ( !best || best_score <= 0 ) { throw std::invalid_argument( "SMAC final specialist fill requires an available positive winner" ); }
		if ( i < 16 ) { result.types[ i ] = *best; }
	}
	return result;
}

}
