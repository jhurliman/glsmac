#pragma once

#include "SmacSpecialistAllocation.h"
#include "types/Buffer.h"

namespace game::backend::base {

// Persistent allocation authority, distinct from scratch visited/provisional
// state and from the worker-map consumer's 21-square projection. All sixteen
// ordered slots are retained, including inactive padding. No padding defaults,
// population identity mapping or live publication are inferred here.
struct SmacBaseAllocationState {
	uint32_t selected;
	SmacSpecialistSlots specialists;
	// Three Buffer integer frames and one sixteen-byte string frame.
	static constexpr uint32_t WIRE_BYTES = 3 * 14 + 6 + 16;
	void Validate() const;
	bool operator==( const SmacBaseAllocationState& other ) const;
	types::Buffer Serialize() const;
	// Requires a complete isolated frame at cursor zero; checks its fixed bound
	// before copying. Failure never changes the caller's frame or live state.
	static SmacBaseAllocationState Deserialize( const types::Buffer& source );
};

}
