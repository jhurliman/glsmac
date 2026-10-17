#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace loader::cvr {

using CvrGeometryVector = std::array< int16_t, 3 >;

struct CvrGeometryRecord {
	std::array< uint8_t, 3 > bytes;
	uint8_t Code() const { return bytes[ 0 ] >> 3; }
};

struct CvrGeometryGroupHeader {
	std::array< CvrGeometryVector, 4 > vectors;
	uint32_t record_count;
	uint32_t flags;
};

struct CvrGeometryGroup {
	// Absent for the single group in the simple (flags 0/8) profile.
	std::optional< CvrGeometryGroupHeader > header;
	std::vector< CvrGeometryRecord > records; // Includes the terminal record, when present.
};

struct CvrGeometry {
	uint32_t flags;
	uint32_t payload_size;
	uint32_t scalar_bits; // Exact little-endian IEEE binary32 bits; not normalized or applied.
	uint32_t end_offset;
	uint32_t end_minus_one;
	std::array< CvrGeometryVector, 3 > vectors;
	uint32_t record_count; // Simple: all records. Grouped: excludes terminal records.
	std::vector< CvrGeometryGroup > groups;
};

class CvrGeometryReader {
public:
	// Decode the observed flags 0/2/8/10 geometry payload profile. All fields and
	// records are owned, in source order; vector/scalar/record rendering semantics
	// remain unapplied. The finite positive scalar is retained as exact bits.
	// Unsupported/malformed input throws std::invalid_argument with a "CVR
	// geometry:" prefix. Complete validation precedes result-storage allocation;
	// allocation errors propagate. Input must remain unchanged during the call.
	static CvrGeometry Read( const std::vector< uint8_t >& bytes );
};

// Owned simple linked detail profile. Original flags, remaining-chain size,
// local extent and records remain intact. Grouped/terminal chains are excluded.
class CvrSimpleDetailChain {
public:
	static constexpr size_t MAX_LEVELS = 64; // Port resource policy.
	static constexpr size_t MAX_RECORDS = 1024 * 1024;
	// Validates the complete chain before allocating result storage.
	static CvrSimpleDetailChain Read( const std::vector< uint8_t >& bytes );
	const std::vector< CvrGeometry >& Levels() const { return m_levels; }
	// Exact product of supplied finite binary32 scale and each positive scalar.
	// Zero/negative scales are diagnostic inputs, not renderer zoom admission.
	size_t Select( uint32_t scale_bits ) const;
private:
	CvrSimpleDetailChain() = default;
	std::vector< CvrGeometry > m_levels;
};

}
