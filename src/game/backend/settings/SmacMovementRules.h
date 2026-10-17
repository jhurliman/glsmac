#pragma once

#include <cstdint>
#include "types/Buffer.h"

namespace game::backend::settings {

class SmacMovementRules {
public:
	// Supplied canonical SMAC RULES row 1. There is no default or separately
	// writable Q: the raw signed32 value is the sole retained authority.
	explicit SmacMovementRules( int32_t road_rate ) : m_road_rate( road_rate ) {}
	int32_t RawRoadRate() const { return m_road_rate; }
	int32_t Quantum() const;
	bool operator==( const SmacMovementRules& other ) const { return m_road_rate == other.m_road_rate; }

	enum class GroundChassis : uint8_t { Infantry, Speeder };
	// Explicit ordinary ground/Gun/no movement-changing abilities or package
	// modifiers only. parsed_moves is ChassisComponent::speed, not cost. This
	// applies the original byte store, then unsigned Moves clamp. The caller
	// must admit the design; this is not an eligibility or full-unit allowance API.
	int32_t OrdinaryGroundAllowance( GroundChassis chassis, int32_t parsed_moves ) const;

	types::Buffer Serialize() const;
	static SmacMovementRules Deserialize( types::Buffer buffer );

private:
	int32_t m_road_rate;
};

}
