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
	// Port wire identities, mapped explicitly to positional TERRAIN source rows.
	// Cancellation clears the order but deliberately preserves accumulated work.
	enum class WorkOrder : uint8_t { None, Farm, Road, Forest, Solar, Mine };
	WorkOrder work_order = WorkOrder::None;
	uint8_t work_progress = 0;
	// Native full word remains unknown on migrated Unit4 records. An actual
	// support pass may own these three accounting bits without knowing others.
	std::optional<uint32_t> retained_state_word;
	std::optional<uint8_t> accounting_markers;
	static constexpr uint32_t PAID_SUPPORT=16u, PACIFISM_PENALIZED=0x200000u, PACIFISM_EXEMPT=0x400000u;
	static uint8_t AccountingMarkers(uint32_t word) noexcept;
	void SetAccountingMarkers(uint8_t markers);
	bool HasStateHistory() const noexcept {return retained_state_word.has_value() || accounting_markers.has_value();}
    // General unit orders are distinct from this port's terraforming wire IDs.
    enum class Order : uint8_t { None = 0, Hold = 2 };
    Order order = Order::None;
    int16_t order_countdown = 0;
    bool IsHeld() const noexcept {return order == Order::Hold;}
    bool HasOrderState() const noexcept {return IsHeld() || order_countdown != 0;}
    void AdvanceOrderUpkeep() noexcept;

	bool operator==( const ConventionalUnitState& other ) const;
	void Validate() const;
	void Serialize( types::Buffer& buffer, bool include_state_history = true, bool include_orders = false ) const;
	static ConventionalUnitState Deserialize( types::Buffer& buffer, bool include_state_history = true, bool include_orders = false );
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
	enum class Presentation { Scout, SelectedSpartanRover, FormerMarker, ColonyMarker };
	Presentation presentation = Presentation::Scout;
	std::optional< SmacMoraleHome > home;
	enum class Package { Combat, Former, Colony };
	Package package = Package::Combat;
	// Copied from the admitted owned design's frozen component tables. Absence
	// in diagnostic contexts is not a guessed Scout rating.
	std::optional<int32_t> offense_rating, defense_rating;
};

using ConventionalUnitResolver = std::function< ConventionalUnitContext( const ConventionalUnitState&, const slot::Slot* ) >;

struct ConventionalUnitStats {
	int32_t effective_morale, allowance_morale, quantum, basic_allowance;
	int32_t full_allowance, remaining_allowance, hit_points, maximum_hit_points;
	std::optional<int32_t> offense_rating, defense_rating;
};

ConventionalUnitStats QueryConventionalUnit( const ConventionalUnitState& state,
	const ConventionalUnitContext& context, SmacMoraleMode mode );

}
