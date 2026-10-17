#include "SmacOrdinaryLandExploration.h"

#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>

namespace game::backend::map::smac_exploration {
namespace {
constexpr std::array< Coordinate, 9 > DIRECT = {{
	{ 0, 0 }, { 1, -1 }, { 2, 0 }, { 1, 1 }, { 0, 2 },
	{ -1, 1 }, { -2, 0 }, { -1, -1 }, { 0, -2 }
}};
constexpr std::array< Coordinate, 16 > OUTER = {{
	{ 2, -2 }, { 2, 2 }, { -2, 2 }, { -2, -2 },
	{ 1, -3 }, { 3, -1 }, { 3, 1 }, { 1, 3 },
	{ -1, 3 }, { -3, 1 }, { -3, -1 }, { -1, -3 },
	{ 4, 0 }, { -4, 0 }, { 0, 4 }, { 0, -4 }
}};

std::size_t Index( const Input& input, const Coordinate p ) {
	return static_cast< std::size_t >( p.y ) * ( input.width / 2 ) + p.x / 2;
}

std::optional< Coordinate > Resolve( const Input& input, Coordinate p ) {
	if ( input.horizontal_wrap ) {
		if ( p.x < 0 ) p.x += input.width;
		else if ( p.x >= input.width ) p.x -= input.width;
	}
	if ( p.x < 0 || p.x >= input.width || p.y < 0 || p.y >= input.height ) return {};
	return p;
}

void Validate( const Input& input ) {
	if ( input.width < 10 || input.width > 512 || input.width % 2 != 0 ||
		input.height < 1 || input.height > 256 || input.faction < 1 || input.faction > 7 ||
		input.center.x < 0 || input.center.x >= input.width ||
		input.center.y < 0 || input.center.y >= input.height ||
		( input.center.x & 1 ) != ( input.center.y & 1 ) ) {
		throw std::invalid_argument( "unsupported ordinary exploration geometry, center or faction" );
	}
	const auto count = static_cast< std::size_t >( input.width / 2 ) * input.height;
	if ( input.sea.size() != count || input.explored.size() != count ) {
		throw std::invalid_argument( "ordinary exploration tile arrays have the wrong extent" );
	}
	for ( const auto value : input.sea ) {
		if ( value > 1 ) throw std::invalid_argument( "ordinary exploration sea value is not Boolean" );
	}
	if ( input.sea[ Index( input, input.center ) ] ) {
		throw std::invalid_argument( "ordinary land exploration requires a land center" );
	}
}
}

Result Explore( const Input& input ) {
	Validate( input );
	Result result{ input.explored, {} };
	result.refresh_improvements.reserve( DIRECT.size() + OUTER.size() );
	const auto mask = static_cast< uint8_t >( 1u << input.faction );
	const auto reveal = [ & ]( const Coordinate p ) {
		result.refresh_improvements.push_back( p );
		result.explored[ Index( input, p ) ] |= mask;
	};
	for ( const auto offset : DIRECT ) {
		const auto p = Resolve( input, { input.center.x + offset.x, input.center.y + offset.y } );
		if ( p ) reveal( *p );
	}
	// A single ordered pass: later candidates see earlier reveals immediately.
	for ( const auto offset : OUTER ) {
		const auto p = Resolve( input, { input.center.x + offset.x, input.center.y + offset.y } );
		if ( !p || ( result.explored[ Index( input, *p ) ] & mask ) ) continue;
		int unknown_weight = 0;
		for ( std::size_t i = 1; i < DIRECT.size(); ++i ) {
			const auto neighbor = Resolve( input, { p->x + DIRECT[ i ].x, p->y + DIRECT[ i ].y } );
			if ( neighbor && !( result.explored[ Index( input, *neighbor ) ] & mask ) ) {
				unknown_weight += ( i % 2 == 0 ) ? 2 : 1;
			}
		}
		const int threshold = input.sea[ Index( input, *p ) ] ? 6 : 4;
		if ( unknown_weight < threshold ) reveal( *p );
	}
	return result;
}

}
