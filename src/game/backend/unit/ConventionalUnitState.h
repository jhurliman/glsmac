#pragma once

#include "SmacConventionalStats.h"
#include "game/backend/settings/SmacMovementRules.h"
#include "types/Buffer.h"
#include <functional>

namespace game::backend::slot { class Slot; }

namespace game::backend::unit {

// Authoritative stored fields. Tile, UnitID and canonical controller remain on
// Unit; simulation identity is explicit and is never inferred from a slot.
struct ConventionalUnitState {
	int32_t design_id, owner_simulation_id;
	uint8_t stored_morale, damage, spent;
	bool failed_move;
	enum class Home : uint8_t { None, Base };
	Home home;
	SmacMoraleMode render_morale_mode;
	std::optional< uint64_t > home_base_id; // stable manager ID; never a retained pointer
	bool operator==( const ConventionalUnitState& other ) const;
	void Validate() const;
	void Serialize( types::Buffer& buffer ) const;
	static ConventionalUnitState Deserialize( types::Buffer& buffer );
};

// An owned resolved copy produced from a checked current or downloaded bound
// simulation. No pointers into profiles, Players, designs or social state.
struct ConventionalUnitContext {
	settings::SmacMovementRules::GroundChassis chassis;
	int32_t quantum, basic_allowance;
	// Absent during checked raw startup admission; never substitutes a rating.
	std::optional< int32_t > retained_morale;
	uint8_t reactor;
	// Checked against the bound participant and owned selection; never inferred
	// from the presentation Def or a transport slot number.
	enum class Presentation { Scout, SelectedSpartanRover, FormerMarker };
	Presentation presentation = Presentation::Scout;
	std::optional< SmacMoraleHome > home;
	enum class Package { Combat, Former };
	Package package = Package::Combat;
};

using ConventionalUnitResolver = std::function< ConventionalUnitContext( const ConventionalUnitState&, const slot::Slot* ) >;

struct ConventionalUnitStats {
	int32_t effective_morale, allowance_morale, quantum, basic_allowance;
	int32_t full_allowance, remaining_allowance, hit_points, maximum_hit_points;
};

ConventionalUnitStats QueryConventionalUnit( const ConventionalUnitState& state,
	const ConventionalUnitContext& context, SmacMoraleMode mode );

}
