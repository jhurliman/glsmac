#pragma once

#include <cstdint>

namespace loader::cvr::scout {

// Renderer-internal finite binary arithmetic. A nonzero magnitude has its top
// bit set and represents magnitude*2^exponent. Each operation rounds nearest,
// ties even to64 significant bits, matching the admitted x87 precision. This
// is not a general x87 emulator: exceptional values/status and extended
// subnormals are rejected; finite binary32 inputs/stores include signed zero.
struct Number {
	uint64_t magnitude;
	int exponent;
	bool negative;
};

Number Decode( uint32_t binary32 );
uint32_t Store( Number value );
Number Integer( int32_t value );
Number Add( Number left, Number right );
Number Subtract( Number left, Number right );
Number Multiply( Number left, Number right );
Number Divide( Number left, Number right );
Number Negate( Number value );
double ToDouble( Number value );

}
