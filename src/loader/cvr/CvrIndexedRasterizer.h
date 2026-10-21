#pragma once

#include "CvrGeometryReader.h"

#include <cstddef>

namespace loader::cvr {

struct CvrIndexedSurface {
	uint32_t width, height, pitch; // Fixed row offset y*pitch, in samples.
	std::vector< uint8_t > color;
	std::vector< uint16_t > depth;
};

struct CvrIndexedLookups {
	std::array< uint8_t, 2048 > first;
	std::vector< uint8_t > second; // Complete 256-byte pages; no RGB interpretation.
};

enum class CvrSampleFootprint { Single, Square2 };

struct CvrIndexedDraw {
	// Borrowed only during Draw(); records/basis/tables must remain unchanged.
	const std::vector< CvrGeometryRecord >* records;
	bool terminal;
	CvrSampleFootprint footprint;
	std::array< std::array< int32_t, 3 >, 3 > basis;
	std::array< int32_t, 3 > initial;
    std::optional<std::array<int32_t,4>> clip; // Half-open kernel clip, before depth lookup.
};

struct CvrIndexedResult {
	CvrIndexedSurface surface;
	size_t processed_records = 0;
	size_t written_samples = 0;
};

struct CvrShadowResult {
    uint32_t width,height,pitch;
    std::vector<uint8_t> mask;
    size_t processed_records=0,written_samples=0,draw_count=0,frame_count=0;
    // Key-zero 1:1 copy, row-major stores. Lookup indexes destination color.
    // All target/clip validation precedes mutation; depth and RGB are absent.
    size_t Blend(std::vector<uint8_t>& target,uint32_t target_width,uint32_t target_height,uint32_t target_pitch,
        const std::array<uint8_t,256>& lookup,const std::array<int32_t,2>& origin,
        const std::array<int32_t,4>& clip) const;
};

class CvrIndexedRasterizer {
public:
	// Resource limits are GLSMAC policy, not original malformed-input behavior.
	static constexpr size_t MAX_SURFACE_SAMPLES = 16 * 1024 * 1024;
	static constexpr size_t MAX_DRAWS = 200;
	static constexpr size_t MAX_RECORDS = 1024 * 1024; // Total, including terminals.

	// Explicit-input non-block/un-clipped indexed sample path. Validates the
	// whole ordered batch before copying/writing output. Returns owned buffers;
	// no source references escape and initial remains unchanged, even on error.
	// First two basis rows fit int16. Derived steps, initial and accumulators
	// must fit their original signed32 domains (depth is shifted by eight).
	// Rejects overflow, out-of-bounds samples, unsupported modes/codes, empty
	// effective draws and malformed terminals with std::invalid_argument.
	// Empty batches return an owned copy. Allocation failures propagate.
	// No camera, scalar, RGB palette, group-origin or matrix defaults.
    // Counted or terminal, contained one/two-pixel shadow path. Ignores depth and record color bytes;
    // each delta step stamps128 even when a previous record hit the same pixel.
    static CvrShadowResult Shadow(uint32_t width,uint32_t height,const std::vector<CvrIndexedDraw>& draws);
	static CvrIndexedResult Draw( const CvrIndexedSurface& initial, const CvrIndexedLookups& lookups,
		const std::vector< CvrIndexedDraw >& draws );
	// The complete Scout caller uses the original modulo32 shifted depth
	// accumulator, including initial values outside Draw's signed shifted
	// subset. Only depth changes policy: x/y bounds, owned output and complete
	// prevalidation remain identical. High16 bits are compared as unsigned.
	static CvrIndexedResult DrawModuloDepth( const CvrIndexedSurface& initial, const CvrIndexedLookups& lookups,
		const std::vector< CvrIndexedDraw >& draws );
};

}
