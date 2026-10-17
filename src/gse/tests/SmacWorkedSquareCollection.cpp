#include "SmacWorkedSquareCollection.h"
#include "game/backend/base/SmacWorkedSquareCollection.h"
#include "fixtures/SmacWorkedSquareCollection.h"
#include "task/gsetests/GSETests.h"

#include <stdexcept>
#include <vector>

namespace gse::tests {
namespace {
using namespace game::backend::base;
using Collection = SmacWorkedSquareCollection;
using Next = SmacWorkedSquareNext;
namespace fixture = smac_worked_fixture;
bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }
}

void AddSmacWorkedSquareCollectionTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC worked collection matches nineteen original masks coordinates and totals", GT() {
		for ( const auto& row : fixture::collection ) {
			const SmacWorkedSquareInput input{ row.population, row.selected, row.unavailable, row.seed, false, 0, 0 };
			std::vector< std::array< int32_t, 2 > > coordinates;
			std::vector< int32_t > visits;
			bool staged_visit = true;
			const auto result = Collection::Collect( input, [ & ]( int32_t index, const SmacWorkerStepState& state ) -> std::optional< SmacWorkerTileYield > {
				visits.push_back( index ); staged_visit &= bool( state.visited & ( uint32_t( 1 ) << index ) );
				const auto coordinate = Collection::Resolve( { row.center, 128, 64, row.wrap }, index );
				if ( !coordinate ) { return std::nullopt; }
				coordinates.push_back( *coordinate );
				return SmacWorkerTileYield{ index ? std::array<int32_t,3>{2,3,4} : std::array<int32_t,3>{2,1,1}, bool( row.sea_indices & ( uint32_t( 1 ) << index ) ) };
			} );
			GT_ASSERT( result.cleaned_selected == row.cleaned && ( result.next == Next::Repair ) == row.repair, "mask/admission: " + std::string( row.id ) );
			GT_ASSERT( result.state.intake == row.intake && result.state.remaining_slots == row.remaining && result.state.visited == row.visited && result.state.sea_participation == row.sea, "collection: " + std::string( row.id ) );
			GT_ASSERT( staged_visit && coordinates.size() == row.coordinate_count, "visit/coordinate count: " + std::string( row.id ) );
			for ( size_t i = 0; i < coordinates.size(); ++i ) { GT_ASSERT( coordinates[ i ] == row.coordinates[ i ], "coordinate order: " + std::string( row.id ) ); }
			uint32_t called = 0; int32_t previous = -1;
			for ( const auto index : visits ) { GT_ASSERT( index > previous, "duplicate/out-of-order visit" ); previous = index; called |= uint32_t( 1 ) << index; }
			GT_ASSERT( called == row.visited, "clipped visit was dropped" );
			GT_ASSERT( input.selected == row.selected && input.intake == row.seed, "caller state changed" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC worked collection matches 168 original signed32 intake boundaries", GT() {
		for ( const auto& row : fixture::wrapping ) {
			int calls = 0;
			const auto result = Collection::Collect( { 2, row.selected ? 1u : 0u, 0, row.seed, false, 0, 0 },
				[ & ]( int32_t index, const SmacWorkerStepState& ) -> std::optional< SmacWorkerTileYield > { ++calls; if ( index ) { throw std::runtime_error( "unexpected noncenter" ); } return SmacWorkerTileYield{ row.yield, row.sea }; } );
			GT_ASSERT( calls == 1 && result.state.intake == row.intake && result.state.remaining_slots == row.remaining && result.state.sea_participation == row.sea, "wrapped original output: " + std::string( row.id ) );
			GT_ASSERT( result.cleaned_selected == ( row.selected ? 1u : 0u ) && result.state.visited == 1, "selected bit confused with collected center" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC worked collection preserves actual complete-allocation early exits", GT() {
		for ( const auto& row : fixture::complete ) {
			uint32_t calls = 0;
			const auto result = Collection::Collect( { row.population, row.selected, 0, {}, false, row.specialists, 0 },
				[ & ]( int32_t index, const SmacWorkerStepState& ) -> std::optional< SmacWorkerTileYield > {
					calls |= uint32_t( 1 ) << index;
					return SmacWorkerTileYield{ index == 0 ? std::array<int32_t,3>{2,1,1} : index == 1 ? row.old_yield : row.new_yield, false };
				} );
			GT_ASSERT( result.next == Next::FinalFill && result.state.remaining_slots == row.specialists && result.state.intake == row.intake && result.state.visited == row.visited && calls == row.visited, "early completion: " + std::string( row.id ) );
		}
		// The early gate is equality, includes provisional count, and does not
		// use nutrient sufficiency or run later cleanup/optimization.
		const auto center = []( int32_t, const SmacWorkerStepState& ) -> std::optional< SmacWorkerTileYield > { return SmacWorkerTileYield{ {2,1,1}, false }; };
		GT_ASSERT( Collection::Collect( {2,1,0,{},false,1,1}, center ).next == Next::FinalFill, "provisional budget ignored" );
		GT_ASSERT( Collection::Collect( {2,1,0,{},false,1,2}, center ).next == Next::LaterAllocation, "equality replaced by less-than" );
		GT_OK();
	} );
	task->AddTest( "SMAC worked collection stages callbacks and leaves caller state on exceptions", GT() {
		const SmacWorkedSquareInput input{ 2, 7, 0, {7,11,13}, true, 0, 0 };
		std::vector< SmacWorkerStepState > observed;
		const auto result = Collection::Collect( input, [ & ]( int32_t index, const SmacWorkerStepState& state ) -> std::optional< SmacWorkerTileYield > {
			observed.push_back( state );
			if ( index == 1 ) { return std::nullopt; }
			return SmacWorkerTileYield{ {2,1,1}, false };
		} );
		GT_ASSERT( observed.size() == 3 && observed[0].visited == 1 && observed[1].visited == 3 && observed[2].visited == 7, "visit publication timing" );
		const std::array<int32_t,3> seed{7,11,13}, after_center{9,12,14}, final{11,13,15};
		GT_ASSERT( observed[0].remaining_slots == 3 && observed[1].remaining_slots == 2 && observed[2].remaining_slots == 1 && observed[0].intake == seed && observed[1].intake == after_center && observed[2].intake == after_center, "callback sees wrong staged slots/intake" );
		GT_ASSERT( result.state.intake == final && result.state.remaining_slots == 0 && result.state.sea_participation, "clipped/seed state lost" );
		int calls = 0;
		GT_ASSERT( Rejects( [ & ] { Collection::Collect( input, [ & ]( int32_t, const SmacWorkerStepState& ) -> std::optional< SmacWorkerTileYield > { if ( ++calls == 2 ) { throw std::runtime_error( "supplied query failed" ); } return SmacWorkerTileYield{{2,1,1},false}; } ); } ), "callback failure swallowed" );
		GT_ASSERT( calls == 2 && input.selected == 7 && input.intake == seed && input.sea_participation, "query failure changed input" );
		const auto repair = Collection::Collect( {0,0xffffffffu,0,seed,true,0,0}, {} );
		GT_ASSERT( repair.next == Next::Repair && repair.cleaned_selected == 0xffffffffu && repair.state.intake == seed && repair.state.visited == 0 && repair.state.remaining_slots == 1 && repair.state.sea_participation, "repair queried or reset untouched fields" );
		GT_OK();
	} );
	task->AddTest( "SMAC worked collection validates domains and preserves high-bit cleanup", GT() {
		const auto empty = []( int32_t, const SmacWorkerStepState& ) -> std::optional< SmacWorkerTileYield > { return SmacWorkerTileYield{ {}, false }; };
		const auto result = Collection::Collect( {127,0xffffffffu,0xfffffffeu,{},false,127,0}, empty );
		GT_ASSERT( result.cleaned_selected == 1 && result.state.visited == 1 && result.state.remaining_slots == 127 && result.next == Next::FinalFill, "upper population or full-mask cleanup" );
		for ( const auto bad : { SmacWorkedSquareInput{-1,0,0,{},false,0,0}, {128,0,0,{},false,0,0}, {1,0,0,{},false,-1,0}, {1,0,0,{},false,0,128}, {1,0,0,{},false,127,1} } ) {
			GT_ASSERT( Rejects( [ & ] { Collection::Collect( bad, empty ); } ), "invalid count admitted" );
		}
		GT_ASSERT( Rejects( [] { Collection::Collect( {0,0,0,{},false,0,0}, {} ); } ), "missing callable admitted" );
		const SmacWorkerRadiusMap map{{0,0},128,64,true};
		GT_ASSERT( !Collection::Resolve( map, 1 ) && !Collection::Resolve( {{0,0},128,64,false}, 6 ), "Y or flat X wrapped" );
		const std::array<int32_t,2> west{126,0}; GT_ASSERT( Collection::Resolve( map, 6 ) == west, "negative X modulo" );
		const std::array<int32_t,2> east{1,1};
		GT_ASSERT( Collection::Resolve( {{2147483644,0},2147483646,2147483647,true}, 15 ) == east, "wide coordinate arithmetic overflowed" );
		// Pinned collection instructions perform one adjustment, not modulo.
		GT_ASSERT( !Collection::Resolve( {{0,4},2,9,true},18 ) && !Collection::Resolve( {{1,3},2,9,true},15 ), "narrow X received a second wrap" );
		const std::array<int32_t,2> narrow_east{1,5}, narrow_west{0,4};
		GT_ASSERT( Collection::Resolve( {{0,4},2,9,true},15 ) == narrow_east && Collection::Resolve( {{1,3},2,9,true},18 ) == narrow_west, "valid single narrow wrap clipped" );
		for ( const auto bad : { SmacWorkerRadiusMap{{0,0},0,64,true}, {{0,0},3,64,true}, {{0,0},128,0,true}, {{-1,1},128,64,true}, {{1,0},128,64,true}, {{128,0},128,64,true}, {{0,64},128,64,true} } ) {
			GT_ASSERT( Rejects( [ & ] { Collection::Resolve( bad, 0 ); } ), "invalid map admitted" );
		}
		GT_ASSERT( Rejects( [ & ] { Collection::Resolve( map, -1 ); } ) && Rejects( [ & ] { Collection::Resolve( map, 21 ); } ), "invalid radius admitted" );
		GT_OK();
	} );
}
}
