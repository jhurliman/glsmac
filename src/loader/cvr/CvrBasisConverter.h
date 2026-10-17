#pragma once

#include <array>
#include <cstdint>

namespace loader::cvr {

class CvrBasisConverter {
public:
	// Explicit nine supplied IEEE binary32 bit patterns, in original order.
	// Matches the supported output of original x87 control-word 0x037f:
	// scale by 256, then round halfway away from zero. First six results must
	// fit signed16, last three signed32. Nonfinite/out-of-domain input throws
	// std::invalid_argument. Integer-only evaluation has no host FP dependency
	// or FP status effects. No camera, frame-matrix or axis default is applied.
	// The resulting basis still needs the rasterizer's separate step/coordinate
	// validation; conversion alone does not guarantee an admissible draw.
	static std::array< std::array< int32_t, 3 >, 3 > Convert( const std::array< uint32_t, 9 >& supplied_matrix_bits );
};

}
