#include "CvrTransformCalculator.h"

#include "CvrBasisConverter.h"

#include <cstddef>
#include <limits>
#include <stdexcept>

namespace loader::cvr {
namespace {

// Nonzero significands are odd. Exponents describe exact powers of two.
// Values originate in binary32 or signed16; this fixed pipeline keeps exponent
// arithmetic well inside int even before a nonnormal store is rejected.
struct Dyadic {
	uint64_t magnitude;
	int exponent;
	bool negative;
};

struct Wide {
	uint64_t low;
	uint64_t high;
};

[[noreturn]] void Invalid( const char* reason ) {
	throw std::invalid_argument( reason );
}

unsigned Width( uint64_t value ) {
	unsigned result = 0;
	while ( value ) { ++result; value >>= 1; }
	return result;
}

unsigned Zeros( uint64_t value ) {
	// Only called for nonzero words; its result is therefore always 0..63.
	if ( !value ) { Invalid( "CVR transform: internal zero normalization" ); }
	unsigned result = 0;
	while ( !( value & 1 ) ) { ++result; value >>= 1; }
	return result;
}

Dyadic Normalize( const Wide value, int exponent, const bool negative ) {
	if ( !value.low ) {
		if ( !value.high ) { return { 0, 0, false }; }
		const auto shift = Zeros( value.high );
		return { value.high >> shift, exponent + 64 + static_cast< int >( shift ), negative };
	}
	const auto shift = Zeros( value.low );
	if ( value.high >> shift ) { Invalid( "CVR transform: inexact extended intermediate" ); }
	const auto magnitude = ( value.low >> shift ) | ( shift ? value.high << ( 64 - shift ) : uint64_t{ 0 } );
	return { magnitude, exponent + static_cast< int >( shift ), negative };
}

Dyadic Decode( const uint32_t bits ) {
	const auto exponent = ( bits >> 23 ) & 255U;
	if ( exponent == 255 || bits == 0x80000000U ) { Invalid( "CVR transform: nonfinite or negative-zero input" ); }
	const auto magnitude = ( bits & 0x7fffffU ) | ( exponent ? 0x800000U : 0U );
	return Normalize( { magnitude, 0 }, exponent ? static_cast< int >( exponent ) - 150 : -149, ( bits & 0x80000000U ) != 0 );
}

Wide Align( const Dyadic value, const int exponent ) {
	const int shift = value.exponent - exponent;
	// An unequal-exponent sum has an odd low term. If the high term needs
	// >65 bits, the <=64-bit low term cannot cancel it down to 64 bits; reject
	// before shifting. Equal-exponent inputs need at most one carry bit.
	if ( shift < 0 || shift > 64 || Width( value.magnitude ) + static_cast< unsigned >( shift ) > 65 ) {
		Invalid( "CVR transform: inexact extended intermediate" );
	}
	if ( !shift ) { return { value.magnitude, 0 }; }
	if ( shift == 64 ) { return { 0, value.magnitude }; }
	return { value.magnitude << shift, value.magnitude >> ( 64 - shift ) };
}

Dyadic Add( const Dyadic left, const Dyadic right ) {
	if ( !left.magnitude && !right.magnitude ) { return { 0, 0, left.negative && right.negative }; }
	if ( !left.magnitude ) { return right; }
	if ( !right.magnitude ) { return left; }
	const int exponent = left.exponent < right.exponent ? left.exponent : right.exponent;
	const auto a = Align( left, exponent );
	const auto b = Align( right, exponent );
	if ( left.negative == right.negative ) {
		const auto low = a.low + b.low;
		return Normalize( { low, a.high + b.high + ( low < a.low ? 1U : 0U ) }, exponent, left.negative );
	}
	const bool a_larger = a.high > b.high || ( a.high == b.high && a.low >= b.low );
	const auto larger = a_larger ? a : b;
	const auto smaller = a_larger ? b : a;
	return Normalize( { larger.low - smaller.low, larger.high - smaller.high - ( larger.low < smaller.low ? 1U : 0U ) },
		exponent, a_larger ? left.negative : right.negative );
}

Dyadic Multiply( const Dyadic left, const Dyadic right ) {
	if ( !left.magnitude || !right.magnitude ) { return { 0, 0, left.negative != right.negative }; }
	if ( left.magnitude > std::numeric_limits< uint64_t >::max() / right.magnitude ) {
		Invalid( "CVR transform: inexact extended product" );
	}
	return { left.magnitude * right.magnitude, left.exponent + right.exponent, left.negative != right.negative };
}

uint32_t Store( const Dyadic value ) {
	if ( !value.magnitude ) {
		if ( value.negative ) { Invalid( "CVR transform: negative-zero store outside supported domain" ); }
		return 0;
	}
	const auto width = Width( value.magnitude );
	int exponent = value.exponent + static_cast< int >( width ) - 1;
	if ( exponent < -126 || exponent > 127 ) { Invalid( "CVR transform: nonnormal binary32 store" ); }
	uint64_t significand;
	if ( width <= 24 ) {
		significand = value.magnitude << ( 24 - width );
	}
	else {
		const auto shift = width - 24; // 1..40, all shifts below are in range.
		significand = value.magnitude >> shift;
		const auto remainder = value.magnitude & ( ( uint64_t{ 1 } << shift ) - 1 );
		const auto half = uint64_t{ 1 } << ( shift - 1 );
		if ( remainder > half || ( remainder == half && ( significand & 1 ) ) ) { ++significand; }
		if ( significand == ( uint64_t{ 1 } << 24 ) ) { significand >>= 1; ++exponent; }
	}
	if ( exponent > 127 ) { Invalid( "CVR transform: binary32 store overflow" ); }
	return ( value.negative ? 0x80000000U : 0U ) | ( static_cast< uint32_t >( exponent + 127 ) << 23 ) |
		static_cast< uint32_t >( significand - ( uint64_t{ 1 } << 23 ) );
}

template< std::size_t N >
std::array< Dyadic, N > DecodeAll( const std::array< uint32_t, N >& bits ) {
	std::array< Dyadic, N > result;
	for ( std::size_t i = 0; i < N; ++i ) { result[ i ] = Decode( bits[ i ] ); }
	return result;
}

uint32_t Dot( const std::array< Dyadic, 3 >& left, const std::array< Dyadic, 3 >& right ) {
	std::array< Dyadic, 3 > products;
	for ( std::size_t i = 0; i < products.size(); ++i ) { products[ i ] = Multiply( left[ i ], right[ i ] ); }
	// The original helpers schedule different cells differently. Like the
	// evidence oracle, require every possible pair sum and the final sum exact.
	const auto first_pair = Add( products[ 0 ], products[ 1 ] );
	Add( products[ 0 ], products[ 2 ] );
	Add( products[ 1 ], products[ 2 ] );
	return Store( Add( first_pair, products[ 2 ] ) );
}

std::array< uint32_t, 3 > Transform( const std::array< Dyadic, 9 >& matrix, const std::array< Dyadic, 3 >& vector ) {
	std::array< uint32_t, 3 > result;
	for ( std::size_t row = 0; row < 3; ++row ) {
		result[ row ] = Dot( { matrix[ row * 3 ], matrix[ row * 3 + 1 ], matrix[ row * 3 + 2 ] }, vector );
	}
	return result;
}

}

CvrTransformResult CvrTransformCalculator::Calculate( const CvrTransformInput& input ) {
	const auto transform = DecodeAll( input.transform_bits );
	const auto translation = DecodeAll( input.translation_bits );
	const auto frame_matrix = DecodeAll( input.frame_matrix_bits );
	const auto frame_location = DecodeAll( input.frame_location_bits );
	const auto scalar = Decode( input.scalar_bits );
	if ( !scalar.magnitude || scalar.negative ) { Invalid( "CVR transform: scalar must be positive" ); }
	CvrTransformResult result;
	const auto transformed_location = DecodeAll( Transform( transform, frame_location ) );
	for ( std::size_t i = 0; i < 3; ++i ) { result.location_bits[ i ] = Store( Add( transformed_location[ i ], translation[ i ] ) ); }
	for ( std::size_t row = 0; row < 3; ++row ) {
		for ( std::size_t col = 0; col < 3; ++col ) {
			const auto product = Dot( { transform[ row * 3 ], transform[ row * 3 + 1 ], transform[ row * 3 + 2 ] },
				{ frame_matrix[ col ], frame_matrix[ col + 3 ], frame_matrix[ col + 6 ] } );
			result.matrix_bits[ row * 3 + col ] = Store( Multiply( Decode( product ), scalar ) );
		}
	}
	std::array< Dyadic, 3 > header;
	for ( std::size_t i = 0; i < header.size(); ++i ) {
		const int32_t value = input.header_vector[ i ];
		header[ i ] = Normalize( { static_cast< uint64_t >( value < 0 ? -value : value ), 0 }, 0, value < 0 );
	}
	const auto transformed_header = DecodeAll( Transform( DecodeAll( result.matrix_bits ), header ) );
	const auto location = DecodeAll( result.location_bits );
	for ( std::size_t i = 0; i < 3; ++i ) { result.origin_bits[ i ] = Store( Add( location[ i ], transformed_header[ i ] ) ); }
	result.basis = CvrBasisConverter::Convert( result.matrix_bits );
	// The converter's last row rounds to signed32. Its power-of-two scaling
	// is exact within this admitted range, also matching the origin fragment
	// which keeps the scaled value in x87 instead of storing it as binary32.
	std::array< uint32_t, 9 > origin_input{};
	for ( std::size_t i = 0; i < 3; ++i ) {
		origin_input[ i + 6 ] = result.origin_bits[ i ];
		auto scaled = Multiply( Decode( result.origin_bits[ i ] ), { 1, 8, false } );
		scaled.negative = false;
		// Fragment B keeps this half-shift in x87. The evidence subset requires
		// it exact too, even when a tiny inexact input would also round to zero.
		Add( scaled, { 1, -1, false } );
	}
	result.origin = CvrBasisConverter::Convert( origin_input )[ 2 ];
	return result;
}

}
