#pragma once

#include "SmacWorkerAllocationRules.h"

namespace game::backend::base {

struct SmacWorkedSquareInput {
	int32_t population;
	uint32_t selected, unavailable;
	std::array< int32_t, 3 > intake;
	bool sea_participation;
	// Earlier specialist bookkeeping is supplied, never inferred here.
	int32_t retained_specialists, provisional_specialists;
};

enum class SmacWorkedSquareNext { Repair, FinalFill, LaterAllocation };
struct SmacWorkedSquareResult {
	uint32_t cleaned_selected;
	SmacWorkerStepState state;
	SmacWorkedSquareNext next;
};

struct SmacWorkerRadiusMap {
	std::array< int32_t, 2 > center;
	int32_t width, height;
	bool horizontal_wrap;
};

class SmacWorkedSquareCollection {
public:
	// Detached existing-allocation stage only. P/counts are 0..127 and the
	// combined specialist budget is at most 127. All 32 selected bits count.
	// The query returns one pre-resolved whole-tile result; it is not a model
	// of the original per-resource callbacks or their side effects. It must
	// not publish live state. nullopt means clipped coordinates and still
	// consumes a slot. Input is unchanged, including when the query throws.
	static SmacWorkedSquareResult Collect( const SmacWorkedSquareInput& input, const SmacWorkerYieldQuery& query );
	// Optional resolver for the measured 21-radius order in doubled X units.
	// Positive dimensions, even width, an in-map parity-valid center and
	// index 0..20 are required. Only X wraps; Y always clips.
	static std::optional< std::array< int32_t, 2 > > Resolve( const SmacWorkerRadiusMap& map, int32_t index );
};

}
