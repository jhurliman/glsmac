#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace loader::flic {

struct Frame {
	uint16_t width = 0, height = 0, frame_count = 0;
	uint32_t speed_ms = 0;
	std::array< std::array< uint8_t, 3 >, 256 > palette = {};
	std::vector< uint8_t > pixels;
};

class FlicReader {
public:
	// AF12, 8-bit FLC with COLOR256, BRUN and SS2 chunks. Decode through the
	// requested declared frame; a physical ring frame is not a declared frame.
	// Indexed pixels remain separate from palette/SMAC drawing-code semantics.
	static Frame ReadFrame( const std::vector< uint8_t >& bytes, size_t index,
		size_t max_pixels = 16 * 1024 * 1024 );
};

}
