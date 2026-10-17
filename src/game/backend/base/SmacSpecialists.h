#pragma once

#include "game/backend/settings/SmacSpecialistRules.h"

namespace game::backend::base {

// Detached query values. No worker allocation, governor mutation, replacement
// of obsolete selections, or resource/citizen recalculation occurs here.
struct SmacSpecialistMenu {
	std::optional< uint8_t > current; // absent is Worker/new-specialist context
	std::array< bool, 7 > eligible{}; // positional CITIZENS IDs; Worker is implicit
	bool size_explanation_requested = false;
};

SmacSpecialistMenu QuerySmacSpecialistMenu( const settings::SmacSpecialistRules& rules,
	const settings::SmacTechnologyRules& technologies, const std::array< bool, 89 >& owned,
	uint32_t population, std::optional< uint8_t > current );

// Automatic initial choice, before the separate obsolescence/replacement loop.
// Port admission: population 0..127, evaluated signed32 arithmetic, and at
// least one available positive score. Unproved custom fallback states reject.
uint8_t SelectAutomaticSmacSpecialist( const settings::SmacSpecialistRules& rules,
	const settings::SmacTechnologyRules& technologies, const std::array< bool, 89 >& owned,
	uint32_t population );

}
