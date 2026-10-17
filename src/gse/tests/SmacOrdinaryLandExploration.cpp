#include "SmacOrdinaryLandExploration.h"

#include "game/backend/map/SmacOrdinaryLandExploration.h"
#include "fixtures/SmacOrdinaryLandExploration.h"
#include "task/gsetests/GSETests.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {
namespace sight = game::backend::map::smac_exploration;
namespace captures = fixtures::smac_exploration;
std::size_t Index( const sight::Input& q, const sight::Coordinate p ) { return p.y * ( q.width / 2 ) + p.x / 2; }
sight::Input Empty() { return { 64, 32, true, { 32, 16 }, 1, std::vector< uint8_t >( 1024 ), std::vector< uint8_t >( 1024 ) }; }
std::vector< uint8_t > Masks( const sight::Input& q, const std::vector< sight::Coordinate >& positions, uint8_t others = 0 ) {
	std::vector< uint8_t > result( q.explored.size(), others );
	for ( const auto p : positions ) result[ Index( q, p ) ] |= static_cast< uint8_t >( 1u << q.faction );
	return result;
}
std::vector< sight::Coordinate > Sorted( std::vector< sight::Coordinate > values ) {
	std::sort( values.begin(), values.end(), []( const auto a, const auto b ) { return a.x < b.x || ( a.x == b.x && a.y < b.y ); } );
	return values;
}
template< typename F > bool Rejects( F f ) { try { f(); } catch ( const std::invalid_argument& ) { return true; } return false; }
}

void AddSmacOrdinaryLandExplorationTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC ordinary exploration matches all 194 original factory captures", GT() {
		for ( const auto& c : captures::PLACEMENTS ) {
			auto q = Empty(); q.center = c.center; q.horizontal_wrap = c.wrap;
			q.explored = Masks( q, c.initial, c.other_mask );
			if ( c.coast ) {
				for ( int y = 0; y < q.height; ++y ) for ( int x = y % 2; x < q.width; x += 2 ) q.sea[ Index( q, { x, y } ) ] = x > 32;
			}
			const auto before = q.explored;
			const auto result = sight::Explore( q );
			GT_ASSERT( result.explored == Masks( q, c.expected, c.other_mask ), "captured exploration differs: " + std::string( c.name ) );
			GT_ASSERT( Sorted( result.refresh_improvements ) == Sorted( c.refreshed ), "captured memory refresh set differs: " + std::string( c.name ) );
			GT_ASSERT( q.explored == before, "exploration mutated supplied history" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC ordinary exploration retains all 14 continuous movement and factory transitions", GT() {
		for ( const auto& c : captures::TRANSITIONS ) {
			auto q = Empty(); q.explored = Masks( q, c.initial );
			for ( const auto& step : c.steps ) {
				q.center = step.center;
				auto result = sight::Explore( q );
				GT_ASSERT( result.explored == Masks( q, step.expected ), "retained continuous transition differs: " + std::string( c.faction ) );
				q.explored = std::move( result.explored );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC ordinary exploration preserves refresh order and all faction bits", GT() {
		auto q = Empty(); q.faction = 7;
		q.explored.assign( 1024, 0xff );
		const std::vector< sight::Coordinate > direct = { {32,16}, {33,15}, {34,16}, {33,17}, {32,18}, {31,17}, {30,16}, {31,15}, {32,14} };
		const auto complete = sight::Explore( q );
		GT_ASSERT( complete.explored == q.explored && complete.refresh_improvements == direct, "direct visits must refresh old memory; old outer memory must stay untouched" );
		const std::vector< sight::Coordinate > holes = { {34,14}, {34,18}, {30,18}, {30,14} };
		for ( const auto p : holes ) q.explored[ Index( q, p ) ] = 0x7f;
		auto expected_order = direct; expected_order.insert( expected_order.end(), holes.begin(), holes.end() );
		auto result = sight::Explore( q );
		GT_ASSERT( result.refresh_improvements == expected_order, "outer table traversal order changed" );
		GT_ASSERT( result.explored == complete.explored && q.explored != result.explored, "wrong faction bit or borrowed output" );
		result.explored[ 0 ] = 0; result.refresh_improvements[ 0 ] = { 0, 0 };
		GT_ASSERT( q.explored[ 0 ] == 0xff && q.center == ( sight::Coordinate{ 32, 16 } ), "owned result aliases input" );
		GT_OK();
	} );
	task->AddTest( "SMAC ordinary exploration bounds geometry and rejects malformed arrays before effects", GT() {
		for ( int field = 0; field < 17; ++field ) {
			auto q = Empty();
			switch ( field ) {
				case 0: q.width = 8; break;
				case 1: q.width = 11; break;
				case 2: q.width = 514; break;
				case 3: q.height = 0; break;
				case 4: q.height = 257; break;
				case 5: q.faction = 0; break;
				case 6: q.faction = 8; break;
				case 7: q.center.x = -2; break;
				case 8: q.center.x = 64; break;
				case 9: q.center.y = -2; break;
				case 10: q.center.y = 32; break;
				case 11: q.center.x = 31; break;
				case 12: q.sea.pop_back(); break;
				case 13: q.explored.push_back( 0 ); break;
				case 14: q.sea.back() = 2; break;
				case 15: q.sea[ Index( q, q.center ) ] = 1; break;
				case 16: q.center.x = std::numeric_limits< int32_t >::max(); break;
			}
			const auto before = q.explored;
			GT_ASSERT( Rejects( [ & ]() { sight::Explore( q ); } ), "malformed exploration input accepted: " + std::to_string( field ) );
			GT_ASSERT( q.explored == before, "rejected exploration mutated input" );
		}
		sight::Input minimal{ 10, 1, false, {0,0}, 1, std::vector< uint8_t >(5), std::vector< uint8_t >(5) };
		const auto clipped = sight::Explore( minimal );
		const std::vector< sight::Coordinate > expected{ {0,0}, {2,0}, {4,0} };
		GT_ASSERT( clipped.refresh_improvements == expected, "one-row port boundary clipping or immediate hole fill differs" );
		auto maximum = Empty(); maximum.width = 512; maximum.height = 256;
		maximum.center = { 511, 255 }; maximum.sea.assign( 65536, 0 ); maximum.explored.assign( 65536, 1 );
		const auto bounded = sight::Explore( maximum );
		GT_ASSERT( bounded.explored.size() == 65536 && bounded.refresh_improvements.size() <= 25, "maximum port extent differs" );
		GT_OK();
	} );
}

}
