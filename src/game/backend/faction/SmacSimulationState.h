#pragma once

#include "SmacTechnologyState.h"
#include "SmacSocialState.h"
#include "SmacEconomyState.h"
#include "SmacRankOrder.h"
#include "SmacHostileState.h"
#include "game/backend/SmacGameplayRandomState.h"
#include "game/backend/SmacMovementHistory.h"
#include "game/backend/unit/SmacStartupDesignState.h"
#include "game/backend/RosterRecords.h"
#include <array>
#include <map>

namespace game::backend::faction {

// Saved simulation identity and its accepted controller. These are checks
// against a canonical roster, not instructions to create/rebind Players.
struct SmacSimulationParticipant {
	int32_t simulation_id;
	int32_t slot;
	std::string gsid;
	roster::Role role;
	std::string faction_id;
	// Independent retained authorities. Successful restart preserves both;
	// restart_prohibited is not a restart-used counter or a zero-Base inference.
	bool active = true;
	bool restart_prohibited = false;
	bool operator==( const SmacSimulationParticipant& other ) const;
};

// Authoritative global inputs, distinct from per-base governor options and
// derived intake. The two solar flags belong to different calculation phases.
struct SmacGlobalResourceModifiers {
	int32_t atmospheric_dust_countdown;
	bool solar_ordinary, solar_upkeep;
	std::optional< int32_t > planetary_governor;
	// RE-SMAC-INITIAL-RESOURCE-MODIFIERS-001. Only new worlds use this reset;
	// restoration supplies every saved field without replaying initialization.
	static SmacGlobalResourceModifiers NewWorld() noexcept;
	void Validate() const;
	bool operator==( const SmacGlobalResourceModifiers& other ) const noexcept;
};

// Current faction assets, ordered nutrients/minerals/energy. Defense satellites
// do not contribute intake and are outside this record. Signed diagnostic
// values restore exactly; a calculation rejects an unmeasured overflow path.
struct SmacSatelliteCounts {
	std::array< int32_t, 3 > resources;
	// Explicit GLSMAC new-campaign asset initialization, not a recovered reset.
	static SmacSatelliteCounts NewWorld() noexcept { return { { 0, 0, 0 } }; }
	std::array< int32_t, 3 > Intake( int32_t population, bool aerospace_complex, bool owns_space_elevator ) const;
	bool operator==( const SmacSatelliteCounts& other ) const noexcept { return resources == other.resources; }
};

struct SmacSimulationState {
	// Supported-campaign admission, not inferred project ownership. Future
	// project consumers must retire this capability before activating effects.
	enum class MovementCapability : int64_t { Unavailable = 0, NoProjectsOrdinaryTerrain = 1 };
	explicit SmacSimulationState( SmacGameplayRandomState random, MovementCapability capability, SmacGlobalResourceModifiers modifiers ) noexcept
		: gameplay_random( random ), movement_capability( capability ), resource_modifiers( modifiers ) {}
	static constexpr size_t MAX_WIRE_BYTES = 256 * 1024;
	static constexpr size_t MAX_TEXT_BYTES = 32 * 1024;
	std::vector< SmacSimulationParticipant > roster;
	SmacTechnologyState technology;
	SmacSocialState social;
	unit::SmacStartupDesignState designs;
	SmacGameplayRandomState gameplay_random;
	MovementCapability movement_capability;
	SmacGlobalResourceModifiers resource_modifiers;
	std::optional< SmacMovementHistory > movement_history;
	// Exactly one record for each roster simulation ID; absence is not zero.
	std::map< int32_t, SmacSatelliteCounts > satellites;
	// Required per-owner financial authority. Absence never means neutral
	// allocation rates or a zero treasury; restoration never posts income.
	std::map< int32_t, SmacEconomyState > economy;
	SmacHostileState hostility;
	// Standard campaign behavior; custom rule selection must retain this value.
	bool do_or_die = false;
	// Original human-loss ending request, distinct from local focus, inactive
	// roster status and a conquest victory. Absence is not inferred from Bases.
	std::optional<int32_t> human_defeat_request;
    // Absent before the first real update and on migrated port campaigns.
    // Retained through capture/elimination; never reconstructed on restore.
    std::optional<SmacRankOrder> rank_order;
	SmacSatelliteCounts GetSatelliteCounts( int32_t simulation_id ) const;
	void Validate( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units ) const;
	bool operator==( const SmacSimulationState& other ) const;
	const SmacSimulationParticipant* FindSlot( size_t slot ) const;
	types::Buffer Serialize( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units ) const;
	static SmacSimulationState Deserialize( const std::string& wire, const settings::SmacTechnologyRules& technologies,
		const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units );
};

}
