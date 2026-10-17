#include "SmacWorkerAllocation.h"

#include "fixtures/SmacWorkerAllocation.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>
#include <vector>

namespace gse::tests {
namespace {
using namespace game::backend::base;
using Rules = SmacWorkerAllocationRules;
namespace fixture = fixtures::worker_allocation;
bool Equal( const SmacWorkerPriorities& a, const SmacWorkerPriorities& b ) {
	return a.aggression == b.aggression && a.explore == b.explore && a.discover == b.discover && a.build == b.build && a.conquer == b.conquer;
}
bool Equal( const SmacWorkerStepState& a, const SmacWorkerStepState& b ) {
	return a.visited == b.visited && a.intake == b.intake && a.remaining_slots == b.remaining_slots && a.sea_participation == b.sea_participation;
}
template< typename F > bool Rejects( F&& callback ) {
	try { callback(); } catch ( const std::invalid_argument& ) { return true; } return false;
}
}

void AddSmacWorkerAllocationTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC worker allocation original scoring and strict ranking", GT() {
		for ( const auto& f : fixture::scores ) {
			const auto score = Rules::Score( f.context, f.yield );
			GT_ASSERT( score.score == f.score && score.food_sufficient == f.sufficient, "original score mismatch in " + std::string( f.id ) );
			const auto rank = Rules::Rank( { 0, f.best, -1, f.worst }, 1, f.collected, score.score );
			GT_ASSERT( ( rank.best_index == 1 ) == f.accepted_best && ( rank.worst_index == 1 ) == f.accepted_worst, "original ranking mismatch in " + std::string( f.id ) );
		}
		auto rank = Rules::Rank( {}, 1, false, 100 ); rank = Rules::Rank( rank, 2, false, 100 );
		rank = Rules::Rank( rank, 3, true, 100 ); rank = Rules::Rank( rank, 4, true, 100 );
		GT_ASSERT( rank.best_index == 1 && rank.worst_index == 3, "ties replaced earlier radius candidates" );
		GT_ASSERT( Rules::Rank( {}, 1, false, 0 ).best_index == 0 && Rules::Rank( {}, 1, true, 9999 ).worst_index == -1, "initial thresholds became unbounded extrema" );
		GT_OK();
	} );
	task->AddTest( "SMAC worker allocation original governor and faction priorities", GT() {
		for ( const auto& f : fixture::priorities ) {
			const auto priorities = Rules::Priorities( f.context );
			GT_ASSERT( Equal( priorities, f.expected ), "original priorities mismatch in " + std::string( f.id ) );
			auto c = f.scoring; c.priorities = priorities; const auto score = Rules::Score( c, f.yield );
			GT_ASSERT( score.score == f.score && score.food_sufficient == f.sufficient, "weighted score mismatch in " + std::string( f.id ) );
		}
		// The citizen-management flag from the captures is intentionally absent
		// from the priority API; it belongs to the earlier allocation branch.
		SmacWorkerPriorityContext ai; ai.human = false; ai.faction = { -3, 7, -2, 9, 4 };
		GT_ASSERT( Equal( Rules::Priorities( ai ), ai.faction ), "AI signed priorities normalized to governor booleans" );
		for ( const auto production : { SmacWorkerProduction::Unit, SmacWorkerProduction::Facility, SmacWorkerProduction::Stockpile, SmacWorkerProduction::HumanGenomeProject } ) {
			auto c = fixture::scores[ 0 ].context; c.production = production;
			GT_ASSERT( Rules::Score( c, { 2, 1, 1 } ).score == 5376, "ordinary production or first project received later-project adjustment" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC worker allocation original addition swap and rollback", GT() {
		for ( const auto& f : fixture::steps ) {
			std::vector< int32_t > calls;
			const auto result = Rules::Step( f.state, f.context, [ & ]( int32_t index, const SmacWorkerStepState& ) -> std::optional< SmacWorkerTileYield > {
				calls.push_back( index ); return index == f.context.ranking.worst_index ? f.old_tile : f.new_tile;
			} );
			GT_ASSERT( Equal( result.state, f.expected ) && result.action == f.action && result.rescan == f.rescan, "original worker step mismatch in " + std::string( f.id ) );
			std::vector< int32_t > expected;
			if ( f.action == SmacWorkerStepAction::Added ) { expected = { f.context.ranking.best_index }; }
			else if ( f.action == SmacWorkerStepAction::Swapped || f.action == SmacWorkerStepAction::RolledBack ) {
				expected = { f.context.ranking.worst_index, f.context.ranking.best_index };
				if ( f.action == SmacWorkerStepAction::RolledBack ) { expected.push_back( f.context.ranking.best_index ); expected.push_back( f.context.ranking.worst_index ); }
			}
			GT_ASSERT( calls == expected, "fresh resource dispatch order changed in " + std::string( f.id ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC worker allocation composes original priority score and worker choices", GT() {
		const std::array< std::array< int32_t, 3 >, 2 > yields{ { { 1, 4, 0 }, { 1, 0, 4 } } };
		for ( const auto& f : fixture::compositions ) {
			SmacWorkerScoreContext scoring{ 1, 1, 2, { 2, 1, 1 }, 0, SmacWorkerProduction::Unit, Rules::Priorities( f.priorities ) };
			SmacWorkerRanking ranking;
			for ( int32_t index = 1; index <= 2; ++index ) {
				const auto score = Rules::Score( scoring, yields[ index - 1 ] );
				ranking = Rules::Rank( ranking, index, false, score.score );
			}
			const auto result = Rules::Step( { 1, { 2, 1, 1 }, 1, false }, { 1, 2, 0, 0, ranking }, [ & ]( int32_t index, const SmacWorkerStepState& ) {
				return SmacWorkerTileYield{ yields.at( index - 1 ), false };
			} );
			GT_ASSERT( result.state.visited == f.selected && result.state.intake == f.intake && result.state.remaining_slots == 0 && !result.rescan, "composed worker choice mismatch in " + std::string( f.id ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC worker allocation staged queries bounds and exception isolation", GT() {
		const auto f = fixture::steps[ 1 ]; const auto before = f.state;
		std::vector< SmacWorkerStepState > stages;
		const auto result = Rules::Step( f.state, f.context, [ & ]( int32_t index, const SmacWorkerStepState& state ) {
			stages.push_back( state ); return index == 1 ? f.old_tile : f.new_tile;
		} );
		GT_ASSERT( stages.size() == 4 && stages[ 0 ].visited == 5 && stages[ 1 ].visited == 5 && stages[ 2 ].visited == 3 && stages[ 3 ].visited == 3,
			"query did not observe staged visited-mask changes before dispatch" );
		GT_ASSERT( stages[ 0 ].intake[ 0 ] == 5 && stages[ 1 ].intake[ 0 ] == 3 && stages[ 2 ].intake[ 0 ] == 7 && stages[ 3 ].intake[ 0 ] == 3 && Equal( result.state, before ), "rollback cached a resource query or changed order" );
		for ( int fail = 0; fail < 4; ++fail ) {
			int calls = 0;
			GT_ASSERT( Rejects( [ & ] { Rules::Step( f.state, f.context, [ & ]( int32_t index, const SmacWorkerStepState& ) {
				if ( calls++ == fail ) { throw std::invalid_argument( "query failed" ); } return index == 1 ? f.old_tile : f.new_tile;
			} ); } ) && Equal( f.state, before ), "failed query changed original allocation" );
		}
		auto context = f.context; context.retained_specialists = 0;
		const auto clipped = Rules::Step( f.state, context, []( int32_t, const SmacWorkerStepState& ) -> std::optional< SmacWorkerTileYield > { return std::nullopt; } );
		GT_ASSERT( clipped.state.visited == 7 && clipped.state.remaining_slots == 0 && clipped.state.intake == f.state.intake && clipped.rescan, "out-of-map add did not consume slot before food rescan" );
		context.retained_specialists = 0; context.provisional_specialists = 1;
		GT_ASSERT( Rules::Step( f.state, context, [ & ]( int32_t index, const SmacWorkerStepState& ) { return index == 1 ? f.old_tile : f.new_tile; } ).action == SmacWorkerStepAction::RolledBack, "provisional specialists omitted from budget" );
		GT_OK();
	} );
	task->AddTest( "SMAC worker allocation rejects unverified arithmetic and candidate domains", GT() {
		const auto base = fixture::scores[ 0 ].context; const std::array< int32_t, 3 > yield{ 2, 1, 1 };
		SmacWorkerScoreContext order{ 0, 0, 1, { 0, 0, 0 }, 0, SmacWorkerProduction::Unit, { INT32_MAX, 0, 1, -8, 1 } };
		GT_ASSERT( Rules::Score( order, { 0, 0, 0 } ).score == 0, "mineral weight used an artificial overflowing intermediate" );
		order.priorities = { INT32_MAX, 0, -1, -4, -4 };
		GT_ASSERT( Rejects( [ & ] { Rules::Score( order, { 0, 0, 0 } ); } ), "original mineral-weight intermediate overflow ignored" );
		order.priorities = { INT32_MAX, 0, INT32_MAX, INT32_MIN, 0 };
		GT_ASSERT( Rules::Score( order, { 0, 0, 0 } ).score == 0, "energy LEA split into artificial overflowing intermediates" );
		for ( int field = 0; field < 12; ++field ) {
			auto c = base;
			switch ( field ) {
				case 0: c.population = -1; break; case 1: c.population = 128; break;
				case 2: c.remaining_slots = -1; break; case 3: c.remaining_slots = 129; break;
				case 4: c.nutrients_per_citizen = 0; break; case 5: c.nutrients_per_citizen = INT32_MAX; break;
				case 6: c.intake[ 0 ] = INT32_MAX; break; case 7: c.intake[ 1 ] = INT32_MIN; c.mineral_support = 1; break;
				case 8: c.priorities.discover = INT32_MIN; break; case 9: c.priorities.explore = INT32_MAX; break;
				case 10: c.priorities.conquer = INT32_MIN; break; case 11: c.production = static_cast< SmacWorkerProduction >( 99 ); break;
			}
			GT_ASSERT( Rejects( [ & ] { Rules::Score( c, yield ); } ), "unsupported scoring domain admitted" );
		}
		for ( size_t resource = 0; resource < 3; ++resource ) { auto extreme = yield; extreme[ resource ] = INT32_MAX;
			GT_ASSERT( Rejects( [ & ] { Rules::Score( base, extreme ); } ), "doubled yield overflow accepted" );
		}
		for ( const int32_t index : { -1, 0, 21, INT32_MAX } ) {
			GT_ASSERT( Rejects( [ & ] { Rules::Rank( {}, index, false, 1 ); } ), "invalid candidate index accepted" );
		}
		const auto f = fixture::steps[ 0 ];
		for ( int field = 0; field < 6; ++field ) {
			auto state = f.state; auto c = f.context;
			switch ( field ) {
				case 0: c.ranking.best_index = -1; break; case 1: c.ranking.worst_index = 21; break;
				case 2: state.visited |= 1u << 31; break; case 3: state.visited |= 4; break;
				case 4: state.visited &= ~2u; break; case 5: c.provisional_specialists = 128; break;
			}
			GT_ASSERT( Rejects( [ & ] { Rules::Step( state, c, []( int32_t, const SmacWorkerStepState& ) { return SmacWorkerTileYield{}; } ); } ), "unsupported worker step admitted" );
		}
		auto c = f.context; c.retained_specialists = 0; auto state = f.state; state.intake[ 1 ] = INT32_MAX;
		GT_ASSERT( Rejects( [ & ] { Rules::Step( state, c, []( int32_t, const SmacWorkerStepState& ) { return SmacWorkerTileYield{ { 0, 1, 0 }, false }; } ); } ) && state.intake[ 1 ] == INT32_MAX, "overflowing intake was published" );
		GT_OK();
	} );
}
}
