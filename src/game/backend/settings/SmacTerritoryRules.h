#pragma once

#include <cstdint>
#include "types/Buffer.h"

namespace game::backend::settings {

// Sole authority for RULES row48. Preserve the source value and derive the
// original 0..100 clamp; no independent mutable border-distance copy.
class SmacTerritoryRules {
public:
	static constexpr uint32_t MAX_WIRE_BYTES = 28;
	explicit SmacTerritoryRules( int32_t raw_border ) : m_raw_border(raw_border) {}
	int32_t RawBorderDistance() const { return m_raw_border; }
	int32_t BorderDistance() const;
	bool operator==( const SmacTerritoryRules& other ) const { return m_raw_border == other.m_raw_border; }
	types::Buffer Serialize() const;
	static SmacTerritoryRules Deserialize( types::Buffer buffer );
private:
	int32_t m_raw_border;
};

}
