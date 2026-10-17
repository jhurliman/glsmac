#include "SmacAutomaticWorkerControls.h"
#include "game/backend/base/SmacAutomaticWorkerAllocation.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>
#include <vector>

namespace gse::tests {
namespace {
using namespace game::backend::base;
using Allocation = SmacAutomaticWorkerAllocation;
SmacAutomaticWorkerInput Input() {
	SmacAutomaticWorkerInput input{};
	input.population = 1; input.turn = 100; input.nutrients_per_citizen = 2;
	input.production = SmacWorkerProduction::Unit;
	input.priorities.human = true;
	input.selected = 1; input.unavailable = ~uint32_t( 7 );
	input.catalog.minimum_base_size = 5;
	input.catalog.rows[ 1 ] = { 0, 2, 0, true, false }; // Doctor
	input.specialists.types.fill( 1 );
	return input;
}
template< typename F > bool Rejects( F&& call ) {
	try { call(); } catch ( const std::invalid_argument& ) { return true; } return false;
}
}

void AddSmacAutomaticWorkerControlTests( task::gsetests::GSETests* task ) {
	// Callback observations test the adapter contract against original branch
	// sites 4fd7bd/4fd84d/4fd9d2/4fdb29, not additional original runtime fixtures.
	task->AddTest( "SMAC automatic worker collection mode and missing-center callback order", GT() {
		for ( int mode = 0; mode < 8; ++mode ) {
			auto input = Input(); input.selected = 3; input.prepared_intake = { 7, 11, 13 };
			input.priorities.human = mode != 6 && mode != 7;
			input.priorities.governor_active = mode >= 2 && mode <= 5;
			input.citizen_management = mode >= 4;
			input.upkeep = bool( mode & 1 );
			const bool bypass = mode == 5 || mode == 7;
			std::vector< int32_t > calls;
			std::vector< SmacWorkerStepState > states;
			const auto result = Allocation::Allocate( input, [ & ]( int32_t index, const SmacWorkerStepState& state ) {
				calls.push_back( index ); states.push_back( state );
				return SmacWorkerTileYield{ index == 0 ? std::array<int32_t,3>{2,1,1} : index == 1 ? std::array<int32_t,3>{1,1,1} : std::array<int32_t,3>{10,20,20}, index == 1 };
			} );
			const std::vector<int32_t> expected = bypass ? std::vector<int32_t>{0,1,2,2} : std::vector<int32_t>{0,1};
			GT_ASSERT( calls == expected, "managed upkeep or complete-allocation bypass changed" );
			GT_ASSERT( states[0].visited == ( bypass ? 0u : 1u ) && states[0].remaining_slots == 2 && states[0].intake == input.prepared_intake && !states[0].sea_participation, "center query sees wrong prepared state" );
			GT_ASSERT( result.workers.visited == ( bypass ? 5u : 3u ) && result.workers.sea_participation == !bypass, "scan-only sea tile changed participation" );
			const std::array<int32_t,3> expected_intake = bypass ? std::array<int32_t,3>{19,32,34} : std::array<int32_t,3>{10,13,15};
			GT_ASSERT( result.workers.intake == expected_intake && result.specialists.total_count == 0, "seed lost or prior workers retained during managed upkeep" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC automatic worker repair keeps prepared intake and clipped scan candidates", GT() {
		auto input = Input(); input.selected = 7; input.prepared_intake = { 7, 11, 13 };
		std::vector<int32_t> calls;
		bool center_before_mark = false;
		const auto result = Allocation::Allocate( input, [ & ]( int32_t index, const SmacWorkerStepState& state ) -> std::optional<SmacWorkerTileYield> {
			calls.push_back( index );
			if ( index == 0 ) { center_before_mark = state.visited == 0 && state.remaining_slots == 2 && state.intake == input.prepared_intake; return SmacWorkerTileYield{{2,1,1},false}; }
			if ( index == 1 ) { return std::nullopt; }
			return SmacWorkerTileYield{{2,1,1},false};
		} );
		const std::vector<int32_t> expected{0,1,2,2}; const std::array<int32_t,3> intake{11,13,15};
		GT_ASSERT( calls == expected && center_before_mark && result.workers.visited == 5 && result.workers.intake == intake, "repair reset seed, collected old mask, or ranked clipped square" );
		// No scoring arithmetic is evaluated in this zero-population path.
		input.population = 0; input.priorities.human = false; input.upkeep = true;
		input.unavailable = ~uint32_t(1); input.prepared_intake = { INT32_MAX, INT32_MIN, -1 };
		const auto wrapped = Allocation::Allocate( input, []( int32_t, const SmacWorkerStepState& ) { return SmacWorkerTileYield{{1,-1,INT32_MIN},false}; } );
		const std::array<int32_t,3> wrapped_intake{INT32_MIN,INT32_MAX,INT32_MAX};
		GT_ASSERT( wrapped.workers.intake == wrapped_intake && wrapped.workers.visited == 1 && wrapped.workers.remaining_slots == 0, "missing-center ADD does not retain original signed32 wrap" );
		GT_OK();
	} );
	task->AddTest( "SMAC automatic worker uses fresh yields through full-budget swap and rollback", GT() {
		for ( const bool rollback : { false, true } ) {
			const auto input = Input();
			std::vector<int32_t> calls; std::vector<SmacWorkerStepState> states;
			const auto result = Allocation::Allocate( input, [ & ]( int32_t index, const SmacWorkerStepState& state ) {
				calls.push_back( index ); states.push_back( state );
				const auto visit = calls.size();
				if ( visit == 2 ) { return SmacWorkerTileYield{{1,10,0},true}; }
				if ( visit == 3 ) { return SmacWorkerTileYield{{0,1,0},false}; }
				if ( visit == 6 ) { return SmacWorkerTileYield{{10,10,0},false}; }
				if ( visit == 8 || visit == 9 ) { return SmacWorkerTileYield{{rollback ? 1 : 10,10,0},false}; }
				return SmacWorkerTileYield{{0,0,0},visit == 4};
			} );
			const std::vector<int32_t> expected = rollback ? std::vector<int32_t>{0,1,2,1,1,2,1,2,2,1} : std::vector<int32_t>{0,1,2,1,1,2,1,2};
			GT_ASSERT( calls == expected, "scan yields cached, visited tile omitted in full scan, or swap rescanned" );
			GT_ASSERT( states[3].visited == 3 && states[3].remaining_slots == 1 && !states[3].sea_participation && states[4].remaining_slots == 0 && states[4].sea_participation, "addition stage/sea timing changed" );
			GT_ASSERT( states[6].visited == 5 && states[7].visited == 5, "swap queries see old mask" );
			GT_ASSERT( result.workers.visited == ( rollback ? 3u : 5u ) && result.workers.intake[0] == ( rollback ? 0 : 10 ) && result.workers.sea_participation, "fresh swap/rollback totals or sticky sea changed" );
			if ( rollback ) { GT_ASSERT( states[8].visited == 3 && states[9].visited == 3, "rollback queries see replacement mask" ); }
		}
		GT_OK();
	} );
	task->AddTest( "SMAC automatic worker resets scan maximum and stops at full-budget specialist winner", GT() {
		auto input = Input(); input.population = 5; input.turn = 101; input.priorities.human = false; input.upkeep = true;
		std::vector<int32_t> calls;
		const auto result = Allocation::Allocate( input, [ & ]( int32_t index, const SmacWorkerStepState& ) {
			calls.push_back( index );
			if ( index == 0 ) { return SmacWorkerTileYield{{10,1,1},false}; }
			if ( calls.size() == 2 ) { return SmacWorkerTileYield{{1,1,0},false}; }
			return SmacWorkerTileYield{{0,0,0},true};
		} );
		const std::vector<int32_t> expected{0,1,2,1,2,2,2,2};
		GT_ASSERT( calls == expected && result.workers.visited == 3 && result.workers.remaining_slots == 4 && result.before_fill.provisional == 4, "previous scan maximum prevented specialist competition" );
		input.upkeep = false; input.selected = 3; input.specialists.total_count = 5;
		input.catalog.rows[1].psych = 20;
		calls.clear();
		const auto full = Allocation::Allocate( input, [ & ]( int32_t index, const SmacWorkerStepState& ) {
			calls.push_back( index );
			return SmacWorkerTileYield{ index == 0 ? std::array<int32_t,3>{20,1,1} : index == 1 ? std::array<int32_t,3>{0,0,0} : std::array<int32_t,3>{0,0,1},false};
		} );
		const std::vector<int32_t> full_expected{0,1,1,2};
		GT_ASSERT( calls == full_expected && full.workers.visited == 3 && full.specialists.total_count == 4 && full.before_fill.retained == 5 && full.before_fill.provisional == 0, "full specialist winner fell back to tile swap or inserted" );
		GT_OK();
	} );
	task->AddTest( "SMAC automatic worker rejects invalid inputs and propagates callback failure", GT() {
		const auto zero = []( int32_t, const SmacWorkerStepState& ) { return SmacWorkerTileYield{}; };
		for ( int failure = 0; failure < 5; ++failure ) {
			auto input = Input();
			if ( failure == 0 ) { input.population = -1; }
			else if ( failure == 1 ) { input.population = 128; }
			else if ( failure == 2 ) { input.nutrients_per_citizen = 0; }
			else if ( failure == 3 ) { input.specialists.total_count = -1; }
			else { input.specialists.total_count = 128; }
			int calls = 0;
			GT_ASSERT( Rejects( [ & ] { Allocation::Allocate( input, [ & ]( int32_t i, const SmacWorkerStepState& state ) { ++calls; return zero( i, state ); } ); } ) && calls == 0, "invalid input reached callback" );
		}
		const auto input = Input();
		GT_ASSERT( Rejects( [ & ] { Allocation::Allocate( input, {} ); } ), "missing query accepted" );
		int calls = 0; bool failed = false;
		try { Allocation::Allocate( input, [ & ]( int32_t, const SmacWorkerStepState& ) -> std::optional<SmacWorkerTileYield> { if ( ++calls == 2 ) { throw std::runtime_error("query failure"); } return SmacWorkerTileYield{{2,1,1},false}; } ); }
		catch ( const std::runtime_error& ) { failed = true; }
		const std::array<int32_t,3> empty{};
		GT_ASSERT( failed && calls == 2 && input.selected == 1 && input.specialists.total_count == 0 && input.prepared_intake == empty, "failed calculation changed owned input or swallowed callback exception" );
		GT_OK();
	} );
}

}
