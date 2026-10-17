#pragma once

#include "SmacSpecialistAllocation.h"

namespace game::backend::base {

struct SmacAutomaticWorkerInput {
	int32_t population, turn, nutrients_per_citizen, mineral_support;
	SmacWorkerProduction production;
	SmacWorkerPriorityContext priorities;
	bool citizen_management, upkeep;
	uint32_t selected, unavailable;
	// Intake already produced by the caller's authenticated orbital/prefix rules.
	std::array< int32_t, 3 > prepared_intake;
	SmacSpecialistSlots specialists;
	SmacSpecialistAllocationCatalog catalog;
	std::optional< uint8_t > initial_overflow_choice;
};

struct SmacAutomaticWorkerResult {
	uint32_t cleaned_selected;
	SmacWorkerStepState workers;
	SmacSpecialistBookkeeping before_fill;
	SmacSpecialistSlots specialists;
};

// Detached composition of the original worker/specialist allocation stages.
// This is not the orbital prefix, post-allocation output/ecology, a manual UI
// command or live Base publication. Availability and rules are resolved inputs.
// See the stage APIs for admitted counts, arithmetic and specialist domains.
// Every collection, scan, addition and rollback queries yields afresh. Scan
// queries cannot set sea participation; missing center is queried before its
// visited bit is set. Callbacks must not publish state or rely on the original
// per-resource-selector side effects. Failure leaves all supplied values intact.
class SmacAutomaticWorkerAllocation {
public:
	static SmacAutomaticWorkerResult Allocate( const SmacAutomaticWorkerInput& input, const SmacWorkerYieldQuery& query );
};

}
