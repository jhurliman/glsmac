#include "ScoutLighting.h"
#include "ScoutArithmetic.h"

#include <algorithm>
#include <cfenv>
#include <cmath>
#include <stdexcept>

#if defined( __SSE__ ) || defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 1 )
#include <xmmintrin.h>
#endif

namespace loader::cvr::scout {

void RequireNearestRounding() {
	if ( std::fegetround() != FE_TONEAREST ) { throw std::invalid_argument( "CVR Scout lighting: nearest rounding required" ); }
#if defined( __SSE__ ) || defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 1 )
	if ( ( _mm_getcsr() & _MM_ROUND_MASK ) != _MM_ROUND_NEAREST ) {
		throw std::invalid_argument( "CVR Scout lighting: nearest SSE rounding required" );
	}
#endif
}

LightingTable GenerateLightingTable() {
	RequireNearestRounding();
	const double pi = std::acos( -1.0 );
	LightingTable result{};
	for ( size_t i = 0; i < result.size(); ++i ) {
		auto& row = result[ i ];
		row[ 0 ] = static_cast< uint8_t >( 23 - i );
		const double a = static_cast< double >( i ) * pi / 46;
		for ( size_t j = 1; j <= 23; ++j ) {
			const double b = static_cast< double >( j ) * pi / 46;
			for ( size_t k = 0; k < 89; ++k ) {
				const double t = static_cast< double >( k ) * 2 * pi / 89;
				const double d = std::clamp( std::cos( a ) * std::cos( b ) + std::sin( a ) * std::sin( b ) * std::cos( t ), -1.0, 1.0 );
				const double shade = std::floor( std::abs( 23 - std::acos( d ) * 46 / pi ) + 0.5 );
				if ( !std::isfinite( shade ) || shade < 0 || shade > 23 ) {
					throw std::invalid_argument( "CVR Scout lighting: generated shade outside range" );
				}
				row[ 1 + ( j - 1 ) * 89 + k ] = static_cast< uint8_t >( shade );
			}
		}
		std::copy_n( row.begin() + 1 + 22 * 89, 89, row.begin() + 1 + 23 * 89 );
	}
	return result;
}

namespace {

Number Stored( const Number value ) { return Decode( Store( value ) ); }

template< size_t N > std::array< Number, N > Numbers( const std::array< uint32_t, N >& input ) {
	std::array< Number, N > result;
	for ( size_t i = 0; i < N; ++i ) { result[ i ] = Decode( input[ i ] ); }
	return result;
}

std::array< uint32_t, 3 > Vector( const std::array< Number, 9 >& matrix, const std::array< Number, 3 >& vector ) {
	std::array< uint32_t, 3 > result;
	for ( size_t row = 0; row < 3; ++row ) {
		result[ row ] = Store( Add( Add( Multiply( matrix[ row * 3 + 1 ], vector[ 1 ] ),
			Multiply( matrix[ row * 3 + 2 ], vector[ 2 ] ) ), Multiply( matrix[ row * 3 ], vector[ 0 ] ) ) );
	}
	return result;
}

}

LightingSelection SelectLighting( const LightingInput& input ) {
	RequireNearestRounding();
	const auto m = Numbers( input.matrix );
	const auto minor = [ & ]( const size_t a, const size_t b, const size_t c, const size_t d ) {
		return Subtract( Multiply( m[ a ], m[ b ] ), Multiply( m[ c ], m[ d ] ) );
	};
	const auto u = Stored( minor( 4, 8, 5, 7 ) );
	const auto v = minor( 3, 8, 6, 5 );
	const auto w = minor( 3, 7, 6, 4 );
	const auto determinant = Add( Subtract( Multiply( m[ 0 ], u ), Multiply( v, m[ 1 ] ) ), Multiply( w, m[ 2 ] ) );
	const auto reciprocal = Stored( Divide( Integer( 1 ), determinant ) );
	const std::array< Number, 9 > cofactors{
		u, Negate( Stored( minor( 1, 8, 2, 7 ) ) ), Stored( minor( 1, 5, 2, 4 ) ),
		Negate( v ), Stored( minor( 0, 8, 2, 6 ) ), Negate( Stored( minor( 0, 5, 2, 3 ) ) ),
		w, Negate( Stored( minor( 0, 7, 1, 6 ) ) ), Stored( minor( 0, 4, 1, 3 ) )
	};
	LightingSelection result;
	for ( size_t i = 0; i < 9; ++i ) { result.inverse[ i ] = Store( Multiply( reciprocal, cofactors[ i ] ) ); }
	auto light = Numbers( Vector( Numbers( input.auxiliary ), Numbers( input.light ) ) );
	const auto scalar = Decode( input.scalar );
	const auto xy = Stored( Multiply( scalar, Decode( input.scales[ 0 ] ) ) );
	const auto z = Stored( Multiply( scalar, Decode( input.scales[ 1 ] ) ) );
	for ( size_t i = 0; i < 3; ++i ) { light[ i ] = Stored( Multiply( light[ i ], i == 2 ? z : xy ) ); }
	result.transformed_light = Vector( Numbers( result.inverse ), light );
	std::array< double, 3 > direction;
	for ( size_t i = 0; i < 3; ++i ) { direction[ i ] = ToDouble( Decode( result.transformed_light[ i ] ) ); }
	if ( direction[ 2 ] < 0 ) { for ( auto& value : direction ) { value = -value; } }
	const double inclination = direction[ 2 ] >= 1 ? 0 : std::acos( direction[ 2 ] );
	double azimuth = std::atan2( direction[ 1 ], direction[ 0 ] );
	if ( azimuth < 0 ) { azimuth += ToDouble( Decode( 0x40c90fdb ) ); }
	const double row = std::trunc( 0.5 + inclination * ToDouble( Decode( 0x416a46ad ) ) );
	const double phase = std::trunc( 0.5 + azimuth * ToDouble( Decode( 0x4162a2fa ) ) );
	if ( !std::isfinite( row ) || !std::isfinite( phase ) || row < 0 || row >= 24 || phase < 0 || phase > 89 ) {
		throw std::invalid_argument( "CVR Scout lighting: invalid generated window" );
	}
	result.row = static_cast< unsigned >( row );
	result.offset = 89 - ( phase == 89 ? 0 : static_cast< unsigned >( phase ) );
	return result;
}

}
