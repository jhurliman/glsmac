#include "SmacTerritoryRules.h"
#include "SmacWorkerKnowledge.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace game::backend::map::smac_territory {
namespace {
void Require( bool condition, const char* message ) {
	if ( !condition ) { throw std::invalid_argument( message ); }
}
size_t Index( const Input& input, const Coordinate& position ) {
	return static_cast< size_t >( position[1] ) * ( input.width / 2 ) + position[0] / 2;
}
void Validate( const Input& input ) {
	Require( input.width >= 2 && input.width <= 512 && !( input.width & 1 ) && input.height >= 1 && input.height <= 256,
		"Territory requires the bounded doubled-coordinate worker grid" );
	Require( input.border_distance >= 0, "Territory requires a nonnegative border distance" );
	const auto count = static_cast< size_t >( input.width ) * input.height / 2;
	Require( input.tiles.size() == count && input.bases.size() <= count, "Territory map or Base count differs from geometry" );
	for ( const auto& tile : input.tiles ) {
		Require( tile.sea ? tile.region >= 65 && tile.region <= 127 : tile.region >= 1 && tile.region <= 63,
			"Territory requires supplied valid land or sea region IDs" );
	}
	std::set< size_t > centers;
	for ( const auto& base : input.bases ) {
		const auto& p = base.coordinate;
		Require( base.owner >= 1 && base.owner <= 7 && p[0] >= 0 && p[0] < input.width && p[1] >= 0 && p[1] < input.height && !( ( p[0] ^ p[1] ) & 1 ),
			"Territory Base has an unsupported owner or coordinate" );
		Require( centers.insert( Index( input, p ) ).second, "Territory has duplicate Base centers" );
	}
}
int64_t Distance( const Input& input, const Coordinate& from, const Coordinate& to ) {
	int64_t dx = int64_t( from[0] ) - to[0], dy = int64_t( from[1] ) - to[1];
	dx = dx < 0 ? -dx : dx; dy = dy < 0 ? -dy : dy;
	if ( input.horizontal_wrap ) { dx = std::min( dx, int64_t( input.width ) - dx ); }
	const auto difference = dx >= dy ? dx - dy : dy - dx;
	return std::max( dx, dy ) - ( difference + 2 ) / 4;
}
}

Claims Rebuild( const Input& input ) {
	Validate( input );
	Claims result( input.tiles.size() );
	for ( int32_t y = 0; y < input.height; ++y ) { for ( int32_t x = y & 1; x < input.width; x += 2 ) {
		const Coordinate position{x,y}; const auto index = Index( input, position );
		const auto& tile = input.tiles[index];
		const Base* selected = nullptr;
		int64_t nearest = 0;
		for ( const auto& base : input.bases ) {
			if ( input.tiles[Index( input, base.coordinate )].region != tile.region ) { continue; }
			const auto distance = Distance( input, position, base.coordinate );
			if ( !selected || distance <= nearest ) { selected = &base; nearest = distance; }
		}
		const auto cutoff = tile.sea ? input.border_distance / 2 : input.border_distance;
		if ( !selected || nearest >= cutoff ) { continue; }
		bool veto = false;
		if ( tile.sea ) {
			for ( const auto& base : input.bases ) {
				if ( base.owner != selected->owner && !input.tiles[Index( input, base.coordinate )].sea
					&& Distance( input, position, base.coordinate ) <= nearest ) { veto = true; break; }
			}
		}
		if ( !veto ) { result[index] = selected->owner; }
	} }
	return result;
}

SmacWorkerKnowledge ReplaceClaims( const SmacWorkerKnowledge& knowledge, const Input& input ) {
	auto data = knowledge.CopyData();
	Require( data.width == input.width && data.height == input.height && data.horizontal_wrap == input.horizontal_wrap,
		"Territory geometry differs from worker knowledge" );
	const auto claims = Rebuild( input );
	for ( size_t i = 0; i < claims.size(); ++i ) { data.tiles[i].claimant = claims[i]; }
	return SmacWorkerKnowledge::Create( data );
}

}
