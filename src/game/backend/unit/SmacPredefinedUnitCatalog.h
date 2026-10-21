#pragma once

#include "loader/txt/UnitRulesTXTLoader.h"
#include <optional>
#include "game/backend/settings/SmacTechnologyRules.h"

namespace game::backend::unit {

struct SmacPredefinedUnit {
	uint32_t source_id, chassis_id, weapon_id, defense_id;
	uint8_t reactor = 1; // original predefined loader argument, not a source column
	uint32_t abilities;
	uint8_t automatic_plan, calculated_cost, calculated_carry;
	uint8_t plan, cost, carry, icon;
	settings::SmacTechnologyReference prerequisite;
	bool active = true, prototyped = false;
	uint32_t faction_exclusion = 0;
};

// Original LABELS slots: Scout, Recon, Artillery, Battery, Marines, Invaders, Mk.
using SmacWorkshopNameLabels=std::array<std::string,7>;
SmacWorkshopNameLabels ReadSmacWorkshopNameLabels(const std::string& source);

// Owns source metadata as well as final positional records. Component indices
// address these owned tables; no source-buffer or caller-container pointers.
struct SmacPredefinedUnitCatalog {
	loader::txt::UnitTable source;
	loader::txt::ChassisTable chassis;
	loader::txt::WeaponTable weapons;
	loader::txt::DefenseTable defenses;
	loader::txt::ReactorTable reactors;
	loader::txt::AbilityTable abilities;
	std::vector< SmacPredefinedUnit > units;
    std::optional<SmacWorkshopNameLabels> workshop_name_labels; // absent in legacy V1 profiles

	static SmacPredefinedUnitCatalog Build( const loader::txt::UnitTable& units,
		const loader::txt::ChassisTable& chassis, const loader::txt::WeaponTable& weapons,
		const loader::txt::DefenseTable& defenses, const loader::txt::ReactorTable& reactors,
		const loader::txt::AbilityTable& abilities, const settings::SmacTechnologyRules& technologies );

	// These are the same calculation paths Build uses before byte caching and
	// row overrides. Cost is not prototype surcharge or build eligibility.
	// Explicit supported domain: positive component factors, reactor 1..4,
	// 24 positional abilities, cost codes >= -7, and signed32 intermediates.
	static int32_t CalculateCost( const loader::txt::ChassisComponent& chassis,
		const loader::txt::WeaponComponent& weapon, const loader::txt::DefenseComponent& defense,
		uint32_t reactor, uint32_t selected_abilities, const loader::txt::AbilityTable& abilities );
	// The helper has no weapon argument. Build invokes it only for Transport.
	// Capacity 0..255; sea multiplies by reactor, then Slow before Heavy.
	static int32_t CalculateCapacity( const loader::txt::ChassisComponent& chassis,
		uint32_t reactor, uint32_t selected_abilities );
};

}
