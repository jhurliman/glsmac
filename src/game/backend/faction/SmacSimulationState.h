#pragma once

#include "SmacTechnologyState.h"
#include "SmacSocialState.h"
#include "game/backend/SmacGameplayRandomState.h"
#include "game/backend/unit/SmacStartupDesignState.h"
#include "game/backend/RosterRecords.h"

namespace game::backend::faction {

// Saved simulation identity and its accepted controller. These are checks
// against a canonical roster, not instructions to create/rebind Players.
struct SmacSimulationParticipant {
	int32_t simulation_id;
	int32_t slot;
	std::string gsid;
	roster::Role role;
	std::string faction_id;
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

struct SmacSimulationState {
	// Supported-campaign admission, not inferred project ownership. Future
	// project consumers must retire this capability before activating effects.
	enum class MovementCapability : int64_t { Unavailable = 0, NoProjectsOrdinaryTerrain = 1 };
	explicit SmacSimulationState( SmacGameplayRandomState random, MovementCapability capability, SmacGlobalResourceModifiers modifiers ) noexcept
		: gameplay_random( random ), movement_capability( capability ), resource_modifiers( modifiers ) {}
	static constexpr size_t MAX_WIRE_BYTES = 64 * 1024;
	static constexpr size_t MAX_TEXT_BYTES = 32 * 1024;
	std::vector< SmacSimulationParticipant > roster;
	SmacTechnologyState technology;
	SmacSocialState social;
	unit::SmacStartupDesignState designs;
	SmacGameplayRandomState gameplay_random;
	MovementCapability movement_capability;
	SmacGlobalResourceModifiers resource_modifiers;
	void Validate( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units ) const;
	bool operator==( const SmacSimulationState& other ) const;
	const SmacSimulationParticipant* FindSlot( size_t slot ) const;
	types::Buffer Serialize( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units ) const;
	static SmacSimulationState Deserialize( const std::string& wire, const settings::SmacTechnologyRules& technologies,
		const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units );
};

}
