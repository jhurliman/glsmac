#pragma once

#include "SmacWorkerAllocationRules.h"

namespace game::backend::base {

struct SmacSpecialistAllocationRow {
	int32_t economy, psych, labs;
	bool prerequisite_available, obsolete_available;
};
struct SmacSpecialistAllocationCatalog {
	int32_t minimum_base_size;
	std::array< SmacSpecialistAllocationRow, 7 > rows;
};

struct SmacSpecialistSlots {
	// All sixteen positions, including unused padding, are supplied state.
	std::array< uint8_t, 16 > types;
	int32_t total_count;
};
struct SmacSpecialistBookkeeping {
	std::array< uint8_t, 16 > types;
	int32_t retained, provisional;
};
struct SmacSpecialistCompetitionContext {
	bool managed;
	int32_t population, turn, nutrients_per_citizen;
	int32_t current_nutrients, current_energy, provisional;
	int32_t maximum_candidate_nutrients; // maximum starts at zero
	int32_t build_priority, discover_priority, tile_best_score;
};
struct SmacSpecialistCompetition {
	std::optional< uint8_t > type; // absent preserves the supplied tile choice
	int32_t score;
};
struct SmacSpecialistInsertion {
	SmacSpecialistBookkeeping specialists;
	bool inserted = false, rescan = false;
};

// Detached value stages. No Base mutation, codec, tile queries or defaults.
// Counts/population: 0..127; input slots: 0..128; positive consumption.
// Evaluated signed32 overflow rejects. Reads at positions >=16 require the
// caller's authenticated initial-selector result; unused absent choices are OK.
class SmacSpecialistAllocation {
public:
	static SmacSpecialistBookkeeping Split( const SmacSpecialistSlots& slots,
		const SmacSpecialistAllocationCatalog& catalog, bool managed, bool upkeep,
		std::optional< uint8_t > initial_overflow_choice );
	// Caller invokes only after collection's complete-allocation bypass.
	static SmacSpecialistBookkeeping Compact( const SmacSpecialistBookkeeping& state,
		const SmacSpecialistAllocationCatalog& catalog, bool managed,
		std::optional< uint8_t > initial_overflow_choice );
	static SmacSpecialistCompetition Compete( const SmacSpecialistAllocationCatalog& catalog,
		const SmacSpecialistCompetitionContext& context );
	// winner must be the validated specialist result of Compete. A full budget
	// does not insert or rescan, even when the supplied intake is food-short.
	static SmacSpecialistInsertion Insert( const SmacSpecialistBookkeeping& state, uint8_t winner,
		const SmacWorkerStepState& workers, int32_t population, int32_t nutrients_per_citizen );
	// Separate inline final selector: 3*psych + (economy+labs), prerequisite
	// only, strict ties. Until broader evidence, a positive winner is required
	// when a fill is actually needed. Final count >127 rejects; padding remains.
	static SmacSpecialistSlots Fill( const SmacSpecialistBookkeeping& state, int32_t remaining_slots,
		const SmacSpecialistAllocationCatalog& catalog, int32_t population );
};

}
