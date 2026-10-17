#include "SmacScoutKeyboardAdmission.h"

#include <stdexcept>

namespace game::backend::unit::smac_scout_key {

Result AdmitDirectionalKey( const Query& q ) {
	if ( ( q.key_code != 99 && q.key_code != 105 ) || !q.normal_unit_control ||
		q.queued_direction_modifier || q.network_busy || q.scenario_editor ||
		q.focused_faction < 1 || q.focused_faction > 7 || q.width < 2 || ( q.width & 1 ) != 0 || q.height < 1 ) {
		throw std::invalid_argument( "SMAC Scout key gate requires the supported ordinary local keyboard context" );
	}
	const auto direction = q.key_code == 99 ? Direction::Original2 : Direction::Original0;
	Result result{ Outcome::NoSelection, direction, std::nullopt, std::nullopt, 0, q.pending_action_latch };
	if ( !q.selected ) { return result; }
	const auto& unit = *q.selected;
	if ( unit.status != Status::Ordinary || unit.owner < 1 || unit.owner > 7 ||
		unit.spent < 0 || unit.spent > 255 || unit.current_full_allowance < 0 ) {
		throw std::invalid_argument( "SMAC Scout key gate requires ordinary status and supplied valid owner/movement fields" );
	}
	const bool in_bounds = unit.x >= 0 && unit.x < q.width && unit.y >= 0 && unit.y < q.height;
	if ( in_bounds && ( unit.x & 1 ) != ( unit.y & 1 ) ) {
		throw std::invalid_argument( "SMAC Scout key gate does not admit unproved in-map coordinate parity" );
	}
	result.queried_identity = unit.identity;
	if ( !in_bounds || unit.owner != q.focused_faction || unit.spent >= unit.current_full_allowance ) {
		result.outcome = Outcome::CannotAct;
		result.notification_calls = 1;
		result.pending_action_latch = 0;
	} else {
		result.outcome = Outcome::EmitRequest;
		result.request = Request{ unit.identity, direction, 0 };
	}
	return result;
}

}
