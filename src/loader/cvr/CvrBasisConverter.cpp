#include "CvrBasisConverter.h"

#include <cstddef>
#include <stdexcept>

namespace loader::cvr {

std::array< std::array< int32_t, 3 >, 3 > CvrBasisConverter::Convert( const std::array< uint32_t, 9 >& supplied_matrix_bits ) {
	std::array< std::array< int32_t, 3 >, 3 > result;
	for ( std::size_t index = 0; index < supplied_matrix_bits.size(); ++index ) {
		const auto bits = supplied_matrix_bits[ index ];
		const auto exponent = ( bits >> 23 ) & 255U;
		if ( exponent == 255 ) { throw std::invalid_argument( "CVR basis: nonfinite binary32 input" ); }
		// After scaling by 256, even the smallest value with exponent >150
		// exceeds signed32. This also bounds every following left shift to 8.
		if ( exponent > 150 ) { throw std::invalid_argument( "CVR basis: value outside destination width" ); }
		const bool negative = ( bits & 0x80000000U ) != 0;
		const uint64_t significand = ( bits & 0x7fffffU ) | ( exponent ? 0x800000U : 0U );
		const int power = exponent ? static_cast< int >( exponent ) - 142 : -141;
		uint64_t magnitude = 0;
		if ( power >= 0 ) {
			magnitude = significand << power;
		}
		else if ( power >= -24 ) {
			const auto shift = -power;
			magnitude = ( significand + ( uint64_t{ 1 } << ( shift - 1 ) ) ) >> shift;
		}
		// Smaller values, including all subnormals, scale to less than 0.5.
		const uint64_t limit = uint64_t{ 1 } << ( index < 6 ? 15 : 31 );
		if ( magnitude > ( negative ? limit : limit - 1 ) ) {
			throw std::invalid_argument( "CVR basis: rounded result outside destination width" );
		}
		const auto signed_magnitude = static_cast< int64_t >( magnitude );
		result[ index / 3 ][ index % 3 ] = static_cast< int32_t >( negative ? -signed_magnitude : signed_magnitude );
	}
	return result;
}

}
