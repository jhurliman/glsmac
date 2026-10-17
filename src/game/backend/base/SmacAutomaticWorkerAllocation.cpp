#include "SmacAutomaticWorkerAllocation.h"
#include "SmacWorkedSquareCollection.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend::base {
namespace {
int32_t AddCollectedIntake( int32_t a, int32_t b ) {
	// The missing-center path uses the same signed32 wrapping ADD as collection.
	const auto bits = static_cast< uint32_t >( int64_t( a ) + b );
	return bits <= std::numeric_limits< int32_t >::max() ? static_cast< int32_t >( bits )
		: static_cast< int32_t >( int64_t( bits ) - ( int64_t( 1 ) << 32 ) );
}
}

SmacAutomaticWorkerResult SmacAutomaticWorkerAllocation::Allocate( const SmacAutomaticWorkerInput& input, const SmacWorkerYieldQuery& query ) {
	if ( input.population < 0 || input.population > 127 || input.nutrients_per_citizen <= 0 || !query ) {
		throw std::invalid_argument( "Unsupported SMAC automatic worker population, consumption or query" );
	}
	const bool managed = !input.priorities.human || ( input.priorities.governor_active && input.citizen_management );
	auto specialists = SmacSpecialistAllocation::Split( input.specialists, input.catalog, managed, input.upkeep, input.initial_overflow_choice );
	const auto cleaned_selected = input.selected & ( ~input.unavailable | uint32_t( 1 ) );
	SmacWorkerStepState workers{ 0, input.prepared_intake, input.population + 1, false };
	const auto finish = [ & ]() {
		return SmacAutomaticWorkerResult{ cleaned_selected, workers, specialists,
			SmacSpecialistAllocation::Fill( specialists, workers.remaining_slots, input.catalog, input.population ) };
	};
	// Managed upkeep skips both splitting (inside Split) and old-square collection.
	if ( !( managed && input.upkeep ) ) {
		const auto collection = SmacWorkedSquareCollection::Collect( { input.population, cleaned_selected, input.unavailable,
			input.prepared_intake, false, specialists.retained, specialists.provisional }, query );
		workers = collection.state;
		if ( collection.next == SmacWorkedSquareNext::FinalFill ) { return finish(); }
	}
	specialists = SmacSpecialistAllocation::Compact( specialists, input.catalog, managed, input.initial_overflow_choice );
	if ( !( workers.visited & 1u ) ) {
		const auto center = query( 0, workers );
		if ( center ) {
			for ( size_t resource = 0; resource < 3; ++resource ) {
				workers.intake[ resource ] = AddCollectedIntake( workers.intake[ resource ], center->resources[ resource ] );
			}
			workers.sea_participation |= center->sea;
		}
		workers.visited |= 1u;
		--workers.remaining_slots;
	}
	const auto priorities = SmacWorkerAllocationRules::Priorities( input.priorities );
	for ( ;; ) {
		const bool full = workers.remaining_slots <= specialists.retained + specialists.provisional;
		const auto excluded = input.unavailable | ( full ? 0u : workers.visited );
		SmacWorkerRanking ranking;
		int32_t maximum_nutrients = 0;
		const SmacWorkerScoreContext score_context{ input.population, workers.remaining_slots, input.nutrients_per_citizen,
			workers.intake, input.mineral_support, input.production, priorities };
		for ( int32_t index = 1; index <= 20; ++index ) {
			const auto bit = uint32_t( 1 ) << index;
			if ( excluded & bit ) { continue; }
			const auto tile = query( index, workers );
			if ( !tile ) { continue; }
			maximum_nutrients = std::max( maximum_nutrients, tile->resources[ 0 ] );
			const auto score = SmacWorkerAllocationRules::Score( score_context, tile->resources );
			ranking = SmacWorkerAllocationRules::Rank( ranking, index, bool( workers.visited & bit ), score.score );
		}
		const auto competition = SmacSpecialistAllocation::Compete( input.catalog,
			{ managed, input.population, input.turn, input.nutrients_per_citizen, workers.intake[ 0 ], workers.intake[ 2 ],
				specialists.provisional, maximum_nutrients, priorities.build, priorities.discover, ranking.best_score } );
		if ( competition.type ) {
			const auto insertion = SmacSpecialistAllocation::Insert( specialists, *competition.type, workers, input.population, input.nutrients_per_citizen );
			specialists = insertion.specialists;
			if ( insertion.rescan ) { continue; }
		}
		else {
			const auto step = SmacWorkerAllocationRules::Step( workers,
				{ input.population, input.nutrients_per_citizen, specialists.retained, specialists.provisional, ranking }, query );
			workers = step.state;
			if ( step.rescan ) { continue; }
		}
		return finish();
	}
}

}
