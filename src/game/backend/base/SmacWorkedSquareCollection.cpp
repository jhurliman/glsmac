#include "SmacWorkedSquareCollection.h"

#include <limits>
#include <stdexcept>

namespace game::backend::base {
namespace {
int32_t AddIntake( int32_t a, int32_t b ) {
	// The measured collection stores wrap at each signed32 addition.
	const auto bits = static_cast< uint32_t >( int64_t( a ) + b );
	return bits <= std::numeric_limits< int32_t >::max() ? static_cast< int32_t >( bits )
		: static_cast< int32_t >( int64_t( bits ) - ( int64_t( 1 ) << 32 ) );
}

int32_t Popcount( uint32_t bits ) {
	int32_t count = 0;
	while ( bits ) { bits &= bits - 1; ++count; }
	return count;
}
}

SmacWorkedSquareResult SmacWorkedSquareCollection::Collect( const SmacWorkedSquareInput& input, const SmacWorkerYieldQuery& query ) {
	if ( input.population < 0 || input.population > 127 || input.retained_specialists < 0 || input.retained_specialists > 127
		|| input.provisional_specialists < 0 || input.provisional_specialists > 127
		|| input.retained_specialists + input.provisional_specialists > 127 ) {
		throw std::invalid_argument( "Unsupported SMAC worked-square population or specialist budget" );
	}
	SmacWorkedSquareResult result{ input.selected & ( ~input.unavailable | uint32_t( 1 ) ),
		{ 0, input.intake, input.population + 1, input.sea_participation }, SmacWorkedSquareNext::Repair };
	if ( Popcount( result.cleaned_selected ) > result.state.remaining_slots ) { return result; }
	if ( !query ) { throw std::invalid_argument( "SMAC worked-square collection requires a yield query" ); }
	for ( int32_t index = 0; index <= 20; ++index ) {
		const auto bit = uint32_t( 1 ) << index;
		if ( index && !( result.cleaned_selected & bit ) ) { continue; }
		result.state.visited |= bit;
		const auto tile = query( index, result.state );
		if ( tile ) {
			for ( size_t resource = 0; resource < 3; ++resource ) {
				result.state.intake[ resource ] = AddIntake( result.state.intake[ resource ], tile->resources[ resource ] );
			}
			result.state.sea_participation |= tile->sea;
		}
		if ( --result.state.remaining_slots == 0 ) { break; }
	}
	result.next = result.state.remaining_slots == input.retained_specialists + input.provisional_specialists
		? SmacWorkedSquareNext::FinalFill : SmacWorkedSquareNext::LaterAllocation;
	return result;
}

std::optional< std::array< int32_t, 2 > > SmacWorkedSquareCollection::Resolve( const SmacWorkerRadiusMap& map, int32_t index ) {
	if ( index < 0 || index > 20 || map.width < 2 || ( map.width & 1 ) || map.height <= 0
		|| map.center[ 0 ] < 0 || map.center[ 0 ] >= map.width || map.center[ 1 ] < 0 || map.center[ 1 ] >= map.height
		|| ( ( map.center[ 0 ] ^ map.center[ 1 ] ) & 1 ) ) {
		throw std::invalid_argument( "Unsupported SMAC worker radius map or index" );
	}
	static constexpr int32_t offsets[ 21 ][ 2 ] = {
		{0,0}, {1,-1}, {2,0}, {1,1}, {0,2}, {-1,1}, {-2,0}, {-1,-1}, {0,-2}, {2,-2}, {2,2},
		{-2,2}, {-2,-2}, {1,-3}, {3,-1}, {3,1}, {1,3}, {-1,3}, {-3,1}, {-3,-1}, {-1,-3}
	};
	auto x = int64_t( map.center[ 0 ] ) + offsets[ index ][ 0 ];
	const auto y = int64_t( map.center[ 1 ] ) + offsets[ index ][ 1 ];
	if ( map.horizontal_wrap ) { x = ( x % map.width + map.width ) % map.width; }
	if ( x < 0 || x >= map.width || y < 0 || y >= map.height ) { return std::nullopt; }
	return std::array< int32_t, 2 >{ static_cast< int32_t >( x ), static_cast< int32_t >( y ) };
}

}
