#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace loader::cvr {

struct CvrChunk {
	uint32_t tag;
	std::vector< uint8_t > payload;
};

struct CvrContainer {
	std::array< CvrChunk, 4 > chunks;
};

class CvrContainerReader {
public:
	// Pure binary decoder for the observed four-chunk CVR container profile:
	// CVR\0, LE32 total size, then tags 1..4 in order (BE32), each followed
	// by its LE32 size INCLUDING the eight-byte chunk header. All payloads
	// must be nonempty; tag 1 has exactly four bytes. No trailing bytes.
	// Payload contents are opaque, including tag 1: no version, filename,
	// palette or geometry semantics are inferred. The result owns every byte.
	// Invalid/unsupported envelopes throw std::invalid_argument before payload
	// copies are allocated. Allocation failures propagate normally.
	static CvrContainer Read( std::string_view bytes );
};

}
