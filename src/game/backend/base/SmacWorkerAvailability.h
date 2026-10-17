#pragma once

#include "SmacWorkedSquareCollection.h"
#include <vector>

namespace game::backend::base {

struct SmacWorkerUnitPresence {
	int32_t owner; // 0 is native; playable identities are 1..7
	bool visible_to_evaluator, active_convoy;
	bool operator==( const SmacWorkerUnitPresence& other ) const noexcept;
};

struct SmacWorkerAvailabilitySquare {
	bool explored, base_marker, sea;
	// Result of the directed recognized-territory query, not a raw claim.
	// None/native is nullopt. Positive owners are 1..7.
	std::optional< int32_t > recognized_owner;
	std::vector< SmacWorkerUnitPresence > units;
	bool operator==( const SmacWorkerAvailabilitySquare& other ) const noexcept;
};

struct SmacWorkerAvailabilityContext {
	uint64_t base_id;
	int32_t owner;
	SmacWorkerRadiusMap map;
	bool all_map_revealed;
	// Each relation is directed from this evaluator to the indexed owner.
	// Native identity 0 is not a diplomacy partner; both entries must be false.
	std::array< bool, 8 > pact, treaty;
};

struct SmacWorkerOtherBase {
	uint64_t id;
	int32_t owner;
	std::array< int32_t, 2 > center;
	uint32_t selected; // preserved in full; this stage inspects only bits 1..20
};

class SmacWorkerAvailability {
public:
	using Squares = std::array< std::optional< SmacWorkerAvailabilitySquare >, 21 >;
	// Detached ordinary unavailable-square stage. Caller supplies all exploration,
	// recognition, visibility, convoy and relationship state explicitly. nullopt
	// must exactly match coordinate clipping; aliases must contain equal values.
	// Other-base IDs are unique. The evaluating ID may be present only with its
	// matching owner/center and is skipped. No population or selection cleanup is
	// inferred here. Inputs remain unchanged on success and failure.
	static uint32_t Build( const SmacWorkerAvailabilityContext& context, const Squares& squares,
		const std::vector< SmacWorkerOtherBase >& other_bases );
};

}
