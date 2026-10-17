#pragma once

#include <cstdint>
#include <optional>

namespace game::backend::unit::smac_scout_key {

using UnitId = uint64_t;
enum class Direction : int32_t { Original0 = 0, Original2 = 2 };
// These are admission categories, not a mapping of original status-word bits.
// Only Ordinary is proved here; all other categories reject explicitly.
enum class Status { Ordinary, Transported, Blocked, OtherOrder };
struct SelectedUnit {
	UnitId identity;
	int32_t owner, x, y, spent, current_full_allowance;
	Status status;
};
struct Query {
	int32_t key_code, focused_faction, width, height;
	bool normal_unit_control, queued_direction_modifier, network_busy, scenario_editor;
	uint32_t pending_action_latch;
	std::optional< SelectedUnit > selected;
};
struct Request {
	UnitId selected_identity;
	Direction direction;
	int32_t original_third_argument;
};
enum class Outcome { NoSelection, CannotAct, EmitRequest };
struct Result {
	Outcome outcome;
	Direction decoded_direction;
	std::optional< UnitId > queried_identity;
	std::optional< Request > request;
	uint32_t notification_calls, pending_action_latch;
};

// Pure KEY-001 ordinary Scout gate for supplied keys99/105. Full allowance
// must be freshly resolved by the caller; spent is a byte-domain value.
// No selection skips actionability/notification. A present foreign, out-of-map
// or exhausted selection produces CannotAct, one notification and latch0.
// Unproved context/status/input domains throw invalid_argument, not CannotAct.
// No RNG, cost, movement body, window input, or general command legality.
// Shared selected-unit body used by the original key wrapper and GLSMAC's
// explicit direction event. It does not map a physical key or emit a request.
bool CanAct( const SelectedUnit& unit, int32_t focused_faction, int32_t width, int32_t height );
Result AdmitDirectionalKey( const Query& query );

}
