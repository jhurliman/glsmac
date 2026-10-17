#include "SmacWorkerAvailability.h"
#include "game/backend/base/SmacWorkerAvailability.h"
#include "fixtures/SmacWorkerAvailability.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace game::backend::base;
using Builder = SmacWorkerAvailability;
using Collection = SmacWorkedSquareCollection;
using Square = SmacWorkerAvailabilitySquare;
using Context = SmacWorkerAvailabilityContext;
using Base = SmacWorkerOtherBase;
bool Rejects( const std::function< void() >& operation ) { try { operation(); } catch ( const std::invalid_argument& ) { return true; } return false; }
Context ContextAt( std::array< int32_t, 2 > center = {64,32}, bool wrap = true ) {
	return { 42, 1, {center,128,64,wrap}, false, {}, {} };
}
Builder::Squares EmptySquares( const Context& context ) {
	Builder::Squares squares;
	for ( int32_t i = 0; i <= 20; ++i ) {
		if ( Collection::Resolve( context.map, i ) ) { squares[ i ] = Square{ true, false, false, {}, {} }; }
	}
	return squares;
}
}

void AddSmacWorkerAvailabilityTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC worker availability matches 736 actual original masks and cleanup results", GT() {
		GT_ASSERT( std::size( smac_availability_fixture::cases ) == 736, "literal fixture count changed" );
		for ( const auto& row : smac_availability_fixture::cases ) {
			auto context = ContextAt( row.center, row.wrap ); context.all_map_revealed = row.reveal;
			context.pact[ 2 ] = row.relation == 1; context.treaty[ 2 ] = row.relation == 2;
			auto squares = EmptySquares( context ); std::vector< Base > bases;
			for ( const auto& base : row.bases ) { bases.push_back( { 100 + bases.size(), base.owner, base.center, base.selected } ); }
			for ( int32_t i = 0; i <= 20; ++i ) {
				if ( !squares[ i ] ) { continue; }
				auto& square = *squares[ i ]; const auto bit = uint32_t( 1 ) << i;
				square.explored = !( row.unexplored & bit ); square.sea = bool( row.sea & bit );
				if ( row.recognized && ( row.foreign & bit ) ) { square.recognized_owner = 2; }
				const auto coordinate = *Collection::Resolve( context.map, i );
				square.base_marker = coordinate == context.map.center;
				for ( const auto& base : bases ) { square.base_marker |= coordinate == base.center; }
				for ( const auto& unit : row.units ) {
					if ( unit.index == i ) { square.units.push_back( {unit.owner,unit.visible,unit.convoy} ); }
				}
			}
			const auto before = squares;
			const auto unavailable = Builder::Build( context, squares, bases );
			GT_ASSERT( unavailable == row.unavailable, "original unavailable mask: " + std::string( row.id ) );
			const auto collected = Collection::Collect( {127,row.selected,unavailable,{},false,0,0},
				[ & ]( int32_t i, const SmacWorkerStepState& ) -> std::optional< SmacWorkerTileYield > {
					if ( !Collection::Resolve( context.map, i ) ) { return {}; }
					return SmacWorkerTileYield{ {}, squares[ i ]->sea };
				} );
			GT_ASSERT( collected.cleaned_selected == row.cleaned, "original cleanup result: " + std::string( row.id ) );
			GT_ASSERT( squares == before, "detached builder changed caller squares" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC worker availability keeps directed visibility convoy and territory independent", GT() {
		auto context = ContextAt(); auto squares = EmptySquares( context ); auto& square = *squares[ 1 ];
		square.units = {{7,true,false}}; context.pact[ 1 ] = true;
		GT_ASSERT( Builder::Build( context, squares, {} ) == 2, "wrong directed relation exempted foreign unit" );
		context.treaty[ 7 ] = true;
		GT_ASSERT( Builder::Build( context, squares, {} ) == 0, "directed Treaty did not exempt visible foreign unit" );
		square.recognized_owner = 7;
		GT_ASSERT( Builder::Build( context, squares, {} ) == 2, "friendship exempted recognized foreign territory" );
		square.recognized_owner = 1;
		GT_ASSERT( Builder::Build( context, squares, {} ) == 0, "own recognized territory was rejected" );
		square.units.push_back( {7,false,true} );
		GT_ASSERT( Builder::Build( context, squares, {} ) == 2, "invisible later convoy exempted by Treaty" );
		square.units = {{1,true,false},{0,true,false}};
		GT_ASSERT( Builder::Build( context, squares, {} ) == 2, "visible native did not block owned territory" );
		context.pact[ 0 ] = true;
		GT_ASSERT( Rejects( [ & ] { Builder::Build( context, squares, {} ); } ), "native Pact admitted a visible native square" );
		context.pact[ 0 ] = false; context.treaty[ 0 ] = true;
		GT_ASSERT( Rejects( [ & ] { Builder::Build( context, squares, {} ); } ), "native Treaty admitted a visible native square" );
		context.treaty[ 0 ] = false;
		square.units.back().visible_to_evaluator = false;
		GT_ASSERT( Builder::Build( context, squares, {} ) == 0, "unseen ordinary foreign unit blocked" );
		square.units.front().active_convoy = true;
		GT_ASSERT( Builder::Build( context, squares, {} ) == 2, "friendly convoy did not block" );
		square.units.clear(); square.explored = false;
		GT_ASSERT( Builder::Build( context, squares, {} ) == 2, "unexplored square admitted" );
		context.all_map_revealed = true;
		GT_ASSERT( Builder::Build( context, squares, {} ) == 0, "revelation did not bypass exploration" );
		square.recognized_owner = 7;
		GT_ASSERT( Builder::Build( context, squares, {} ) == 2, "revelation bypassed territory" );
		GT_OK();
	} );
	task->AddTest( "SMAC worker availability reserves mandatory centers and only measured selected bits", GT() {
		auto context = ContextAt(); auto squares = EmptySquares( context );
		std::vector< Base > bases{{9,1,{68,32},uint32_t(1)<<6}};
		GT_ASSERT( Builder::Build( context, squares, bases ) == 4, "friendly land worker overlap omitted" );
		bases[0].owner = 2;
		GT_ASSERT( Builder::Build( context, squares, bases ) == 0, "foreign land reservation invented" );
		squares[2]->sea = true;
		GT_ASSERT( Builder::Build( context, squares, bases ) == 4, "foreign sea reservation omitted" );
		bases[0].selected = 0xffe00000u;
		GT_ASSERT( Builder::Build( context, squares, bases ) == 0 && bases[0].selected == 0xffe00000u, "high selected bits rejected or interpreted as radius" );
		bases[0].center = {66,32};
		GT_ASSERT( Builder::Build( context, squares, bases ) == 4, "other center required its selected bit" );
		bases.push_back( {context.base_id,context.owner,context.map.center,0xffffffffu} );
		GT_ASSERT( Builder::Build( context, squares, bases ) == 4, "evaluating Base reserved its own workers" );
		// No population/count contract belongs to this builder.
		for ( uint64_t i = 0; i < 300; ++i ) { bases.push_back( {1000+i,1,{80,32},0xffffffffu} ); }
		GT_ASSERT( Builder::Build( context, squares, bases ) == 4, "unrelated Base count rejected or changed local reservations" );
		GT_OK();
	} );
	task->AddTest( "SMAC worker availability shares exact clipping wrapping and alias resolution", GT() {
		auto context = ContextAt( {0,0}, false ); auto squares = EmptySquares( context );
		uint32_t clipped = 0; for ( int32_t i = 0; i <= 20; ++i ) { if ( !squares[i] ) { clipped |= uint32_t(1)<<i; } }
		GT_ASSERT( Builder::Build( context, squares, {} ) == clipped, "clipped radius was not unavailable" );
		context = ContextAt( {0,32}, true ); squares = EmptySquares( context );
		GT_ASSERT( Builder::Build( context, squares, {{9,1,{126,32},uint32_t(1)<<2}} ) == (1u|64u), "wrapped mandatory center/selected overlap changed" );
		context.map = {{0,2},2,5,true}; squares = EmptySquares( context );
		GT_ASSERT( Builder::Build( context, squares, {} ) == ( (1u<<13) | (1u<<16) | (1u<<17) | (1u<<18) | (1u<<19) | (1u<<20) ),
			"single-adjustment narrow map clipping changed" );
		squares[2]->explored = false;
		GT_ASSERT( Rejects( [ & ] { Builder::Build( context, squares, {} ); } ), "coordinate aliases accepted conflicting exploration" );
		for ( int32_t i = 0; i <= 20; ++i ) { if ( Collection::Resolve( context.map, i ) == std::optional(context.map.center) ) { squares[i]->explored = false; } }
		const auto aliased = Builder::Build( context, squares, {} );
		GT_ASSERT( (aliased & (1u|4u|64u)) == (1u|4u|64u), "equal aliases did not share unavailable state" );
		context.map = {{2147483644,0},2147483646,2147483647,true}; squares = EmptySquares( context );
		GT_ASSERT( Builder::Build( context, squares, {} ) != 0, "wide map clipping failed" );
		GT_OK();
	} );
	task->AddTest( "SMAC worker availability validates complete owned requests without partial outputs", GT() {
		const auto original_context = ContextAt(); const auto original = EmptySquares( original_context );
		for ( int scenario = 0; scenario < 14; ++scenario ) {
			auto context = original_context; auto squares = original; std::vector<Base> bases;
			if ( scenario == 0 ) { context.owner = 0; }
			if ( scenario == 1 ) { context.owner = 8; }
			if ( scenario == 2 ) { context.map.width = 3; }
			if ( scenario == 3 ) { squares[20].reset(); }
			if ( scenario == 4 ) { squares[20]->recognized_owner = 0; }
			if ( scenario == 5 ) { squares[20]->recognized_owner = 8; }
			if ( scenario == 6 ) { squares[20]->units.push_back( {-1,true,false} ); }
			if ( scenario == 7 ) { squares[20]->units.push_back( {8,true,false} ); }
			if ( scenario == 8 ) { bases = {{3,1,{80,32},0},{3,1,{82,32},0}}; }
			if ( scenario == 9 ) { bases = {{3,0,{80,32},0}}; }
			if ( scenario == 10 ) { bases = {{3,1,{128,32},0}}; }
			if ( scenario == 11 ) { bases = {{context.base_id,2,context.map.center,0}}; }
			if ( scenario == 12 ) { bases = {{context.base_id,1,{80,32},0}}; }
			if ( scenario == 13 ) { context = ContextAt({0,0}); squares = EmptySquares(context); squares[1] = Square{true,false,false,{},{}}; }
			const auto before = squares;
			GT_ASSERT( Rejects( [ & ] { Builder::Build( context, squares, bases ); } ), "invalid request admitted: " + std::to_string(scenario) );
			GT_ASSERT( squares == before && original == EmptySquares(original_context), "validation changed caller input" );
		}
		GT_OK();
	} );
}
}
