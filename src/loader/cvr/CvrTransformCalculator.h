#pragma once

#include <array>
#include <cstdint>

namespace loader::cvr {

struct CvrTransformInput {
	std::array< uint32_t, 9 > transform_bits;
	std::array< uint32_t, 3 > translation_bits;
	std::array< uint32_t, 9 > frame_matrix_bits;
	std::array< uint32_t, 3 > frame_location_bits;
	uint32_t scalar_bits;
	std::array< int16_t, 3 > header_vector;
};

struct CvrTransformResult {
	std::array< uint32_t, 9 > matrix_bits;
	std::array< uint32_t, 3 > location_bits;
	std::array< uint32_t, 3 > origin_bits;
	std::array< std::array< int32_t, 3 >, 3 > basis;
	std::array< int32_t, 3 > origin;
};

class CvrTransformCalculator {
public:
	// Explicit row-major binary32 bit patterns; no camera/axis/default inference.
	// Reproduces the admitted exact-x87-intermediate subset documented in
	// cvr-caller-origin-evidence.md, including each nearest-even binary32 store.
	// All arithmetic uses integers, independent of the host FP environment.
	// Rejects negative zero inputs/stores, nonfinite inputs, nonpositive scalar, intermediates
	// requiring >64 significand bits, rounded nonzero subnormal/overflow stores, and basis/origin
	// width failures with std::invalid_argument. Returns owned values; input is
	// unchanged on success/failure. Later raster bounds remain a separate check.
	static CvrTransformResult Calculate( const CvrTransformInput& input );
};

}
