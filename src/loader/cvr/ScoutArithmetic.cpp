#include "ScoutArithmetic.h"

#include <cmath>
#include <stdexcept>

namespace loader::cvr::scout {
namespace {

struct Wide { uint64_t high, low; };

[[noreturn]] void Invalid() { throw std::invalid_argument( "CVR Scout: unsupported finite extended arithmetic" ); }

unsigned Width( uint64_t value ) {
	unsigned width = 0;
	while ( value ) { ++width; value >>= 1; }
	return width;
}

Number Checked( const uint64_t magnitude, const int exponent, const bool negative ) {
	if ( magnitude && ( exponent < -16445 || exponent > 16320 ) ) { Invalid(); }
	return { magnitude, magnitude ? exponent : 0, negative };
}

Number Normalize( const uint64_t magnitude, const int exponent, const bool negative ) {
	if ( !magnitude ) { return { 0, 0, negative }; }
	const auto shift = 64 - Width( magnitude );
	return Checked( magnitude << shift, exponent - static_cast< int >( shift ), negative );
}

// Rounded normalized significand; an increment can carry beyond64 bits.
Number Increment( uint64_t magnitude, int exponent, const bool negative, const bool up ) {
	if ( up && ++magnitude == 0 ) { magnitude = uint64_t{ 1 } << 63; ++exponent; }
	return Checked( magnitude, exponent, negative );
}

Number Round( const Wide value, const int exponent, const bool negative ) {
	if ( !value.high ) { return Normalize( value.low, exponent, negative ); }
	const auto shift = Width( value.high ); // 1..64 bits discarded from low.
	const auto magnitude = shift == 64 ? value.high : ( value.high << ( 64 - shift ) ) | ( value.low >> shift );
	const auto remainder = shift == 64 ? value.low : value.low & ( ( uint64_t{ 1 } << shift ) - 1 );
	const auto half = uint64_t{ 1 } << ( shift - 1 );
	return Increment( magnitude, exponent + static_cast< int >( shift ), negative,
		remainder > half || ( remainder == half && ( magnitude & 1 ) ) );
}

Wide ShiftWithSticky( const uint64_t magnitude, const unsigned shift ) {
	if ( !shift ) { return { magnitude, 0 }; }
	if ( shift < 64 ) { return { magnitude >> shift, magnitude << ( 64 - shift ) }; }
	if ( shift == 64 ) { return { 0, magnitude }; }
	if ( shift < 128 ) {
		const auto low_shift = shift - 64;
		return { 0, ( magnitude >> low_shift ) | ( ( magnitude << ( 64 - low_shift ) ) != 0 ? 1U : 0U ) };
	}
	return { 0, magnitude ? 1U : 0U };
}

Wide Product( const uint64_t a, const uint64_t b ) {
	const uint64_t a0 = uint32_t( a ), a1 = a >> 32, b0 = uint32_t( b ), b1 = b >> 32;
	const uint64_t low = a0 * b0;
	const uint64_t middle = a1 * b0 + ( low >> 32 );
	const uint64_t middle_low = ( middle & 0xffffffffU ) + a0 * b1;
	return { a1 * b1 + ( middle >> 32 ) + ( middle_low >> 32 ), ( middle_low << 32 ) | uint32_t( low ) };
}

}

Number Decode( const uint32_t bits ) {
	const auto exponent = ( bits >> 23 ) & 255U;
	if ( exponent == 255 ) { Invalid(); }
	return Normalize( ( bits & 0x7fffffU ) | ( exponent ? 0x800000U : 0U ),
		exponent ? static_cast< int >( exponent ) - 150 : -149, ( bits >> 31 ) != 0 );
}

uint32_t Store( const Number value ) {
	const uint32_t sign = value.negative ? 0x80000000U : 0;
	if ( !value.magnitude ) { return sign; }
	int exponent = value.exponent + 63;
	if ( exponent > 127 ) { Invalid(); }
	if ( exponent < -126 ) {
		const int shift = -149 - value.exponent;
		uint64_t rounded = 0;
		if ( shift < 64 ) {
			rounded = value.magnitude >> shift;
			const auto remainder = value.magnitude & ( ( uint64_t{ 1 } << shift ) - 1 );
			const auto half = uint64_t{ 1 } << ( shift - 1 );
			if ( remainder > half || ( remainder == half && ( rounded & 1 ) ) ) { ++rounded; }
		}
		else if ( shift == 64 && value.magnitude > ( uint64_t{ 1 } << 63 ) ) { rounded = 1; }
		return sign | static_cast< uint32_t >( rounded );
	}
	const auto rounded = Round( { 0, value.magnitude }, value.exponent, value.negative );
	uint64_t significand = rounded.magnitude >> 40;
	const auto remainder = rounded.magnitude & ( ( uint64_t{ 1 } << 40 ) - 1 );
	const auto half = uint64_t{ 1 } << 39;
	if ( remainder > half || ( remainder == half && ( significand & 1 ) ) ) { ++significand; }
	if ( significand == ( uint64_t{ 1 } << 24 ) ) { significand >>= 1; ++exponent; }
	if ( exponent > 127 ) { Invalid(); }
	return sign | ( static_cast< uint32_t >( exponent + 127 ) << 23 ) | ( static_cast< uint32_t >( significand ) & 0x7fffffU );
}

Number Integer( const int32_t value ) {
	const int64_t wide = value;
	return Normalize( static_cast< uint64_t >( wide < 0 ? -wide : wide ), 0, value < 0 );
}

Number Negate( Number value ) { value.negative = !value.negative; return value; }

Number Add( Number left, Number right ) {
	if ( !left.magnitude && !right.magnitude ) { return { 0, 0, left.negative && right.negative }; }
	if ( !left.magnitude ) { return right; }
	if ( !right.magnitude ) { return left; }
	if ( left.exponent < right.exponent || ( left.exponent == right.exponent && left.magnitude < right.magnitude ) ) {
		const auto copy = left; left = right; right = copy;
	}
	const Wide a{ left.magnitude, 0 };
	const auto b = ShiftWithSticky( right.magnitude, static_cast< unsigned >( left.exponent - right.exponent ) );
	if ( left.negative != right.negative ) {
		const Wide difference{ a.high - b.high - ( a.low < b.low ? 1U : 0U ), a.low - b.low };
		return Round( difference, left.exponent - 64, difference.high || difference.low ? left.negative : false );
	}
	const Wide sum{ a.high + b.high, b.low };
	if ( sum.high < a.high ) {
		// Preserve the carry above128 bits plus a sticky discarded low bit.
		return Round( { ( uint64_t{ 1 } << 63 ) | ( sum.high >> 1 ),
			( sum.high << 63 ) | ( sum.low >> 1 ) | ( sum.low & 1 ) }, left.exponent - 63, left.negative );
	}
	return Round( sum, left.exponent - 64, left.negative );
}

Number Subtract( const Number left, const Number right ) { return Add( left, Negate( right ) ); }

Number Multiply( const Number left, const Number right ) {
	return Round( Product( left.magnitude, right.magnitude ), left.exponent + right.exponent, left.negative != right.negative );
}

Number Divide( const Number left, const Number right ) {
	if ( !right.magnitude ) { Invalid(); }
	if ( !left.magnitude ) { return { 0, 0, left.negative != right.negative }; }
	const bool at_least_one = left.magnitude >= right.magnitude;
	uint64_t quotient = at_least_one ? 1 : 0;
	uint64_t remainder = at_least_one ? left.magnitude - right.magnitude : left.magnitude;
	const unsigned count = at_least_one ? 63 : 64;
	for ( unsigned i = 0; i < count; ++i ) {
		const bool carry = ( remainder >> 63 ) != 0;
		remainder <<= 1;
		const bool subtract = carry || remainder >= right.magnitude;
		if ( subtract ) { remainder -= right.magnitude; }
		quotient = ( quotient << 1 ) | ( subtract ? 1U : 0U );
	}
	const auto complement = right.magnitude - remainder;
	return Increment( quotient, left.exponent - right.exponent - static_cast< int >( count ), left.negative != right.negative,
		remainder > complement || ( remainder == complement && ( quotient & 1 ) ) );
}

double ToDouble( const Number value ) {
	const double magnitude = std::ldexp( static_cast< double >( value.magnitude ), value.exponent );
	if ( !std::isfinite( magnitude ) ) { Invalid(); }
	return value.negative ? -magnitude : magnitude;
}

}
