#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace game::backend::base {

struct SmacWorkerPriorities {
	int32_t aggression = 0, explore = 0, discover = 0, build = 0, conquer = 0;
};

struct SmacWorkerPriorityContext {
	bool human = true, governor_active = false;
	bool explore = false, discover = false, build = false, conquer = false;
	SmacWorkerPriorities faction;
};

// Semantic categories supplied by a caller with an authenticated production row.
// Human Genome Project is deliberately distinct from the later project rows.
enum class SmacWorkerProduction { Unit, Facility, Stockpile, HumanGenomeProject, LaterProject };

struct SmacWorkerScoreContext {
	int32_t population = 0, remaining_slots = 0, nutrients_per_citizen = 2;
	std::array< int32_t, 3 > intake{};
	int32_t mineral_support = 0;
	SmacWorkerProduction production = SmacWorkerProduction::Unit;
	SmacWorkerPriorities priorities;
};

struct SmacWorkerScore {
	int32_t score;
	// This candidate's food test, not the scan's persistent sufficiency marker.
	bool food_sufficient;
};

struct SmacWorkerRanking {
	int32_t best_index = 0, best_score = 0;
	int32_t worst_index = -1, worst_score = 9999;
};

struct SmacWorkerStepState {
	uint32_t visited = 0;
	std::array< int32_t, 3 > intake{};
	int32_t remaining_slots = 0;
	bool sea_participation = false;
};

struct SmacWorkerStepContext {
	int32_t population = 0, nutrients_per_citizen = 2;
	int32_t retained_specialists = 0, provisional_specialists = 0;
	SmacWorkerRanking ranking;
};

struct SmacWorkerTileYield {
	std::array< int32_t, 3 > resources{};
	bool sea = false;
};

enum class SmacWorkerStepAction { None, Added, Swapped, RolledBack };
struct SmacWorkerStepResult {
	SmacWorkerStepState state;
	SmacWorkerStepAction action = SmacWorkerStepAction::None;
	bool rescan = false;
};

// Whole-tile query called afresh on every visit, including rollback; this does
// not model the original's four separate resource-selector dispatches. nullopt
// denotes clipped coordinates; zero yield denotes a valid tile. Visited/intake
// are staged before the query, but sea participation is updated from its return
// (the callback sees the preceding sea marker). Callbacks must not publish live
// state or depend on per-selector side effects.
using SmacWorkerYieldQuery = std::function< std::optional< SmacWorkerTileYield >( int32_t, const SmacWorkerStepState& ) >;

// Automatic allocation stages only. Legality, traversal, existing collection,
// specialist competition/final fill and live publication are separate callers.
// P is 0..127, slots/counts 0..128 and consumption positive. Evaluated signed32
// overflow and unsupported domains throw invalid_argument; no input is mutated.
class SmacWorkerAllocationRules {
public:
	// Citizen management does not affect this stage and is not an input here.
	static SmacWorkerPriorities Priorities( const SmacWorkerPriorityContext& context );
	static SmacWorkerScore Score( const SmacWorkerScoreContext& context, const std::array< int32_t, 3 >& yield );
	// Caller traverses legal radius indices 1..20 in ascending order. Collected
	// means the current visited set, not the original selected-worker mask.
	static SmacWorkerRanking Rank( const SmacWorkerRanking& previous, int32_t index, bool collected, int32_t score );
	static SmacWorkerStepResult Step( const SmacWorkerStepState& state, const SmacWorkerStepContext& context, const SmacWorkerYieldQuery& query );
};

}
