#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "game/backend/settings/SmacFactionStartupRules.h"

namespace game::backend::faction {

// Explicit simulation identity, never a lobby slot, template index, or GSID.
struct SmacSimulationAdmission {
	int32_t simulation_id;
	std::string faction_id;
	bool operator==( const SmacSimulationAdmission& other ) const;
};

struct SmacFactionTechnologyState {
	SmacSimulationAdmission admission;
	std::array< bool, 89 > owned{};
	int32_t score = 0, future_count = 0, labs = 0;
	int32_t discount = 0, deferred = 0;
	bool pending_secrets = false;
	std::optional< int32_t > target, cached_threshold;
	bool operator==( const SmacFactionTechnologyState& other ) const;
};

// Derived from the owned technology set and frozen faction definition. These
// are capabilities before the tile's PLANET/clamp/event calculation, not a cache.
struct SmacFungalCapabilities {
	int32_t nutrients, minerals, energy;
	bool operator==( const SmacFungalCapabilities& other ) const;
};

// Owned mutable simulation values, separate from frozen definitions. This
// first codec does not publish into Game or infer a simulation roster.
struct SmacTechnologyState {
	static constexpr size_t MAX_WIRE_BYTES = 8 * 1024;
	std::vector< SmacFactionTechnologyState > factions;
	void Validate( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) const;
	bool operator==( const SmacTechnologyState& other ) const;
	const SmacFactionTechnologyState* Find( int32_t simulation_id ) const;

	// RE-SMAC-FUNGAL-001: admitted player simulation IDs only. Resolve owned
	// ordinary flagged records, then the profile's shared eight-record list.
	// Unknown relevant prerequisite tokens and evaluated signed32 overflow reject.
	// All three fungal modifiers share the source-profile record budget;
	// native-faction state remains outside this owned player domain.
	SmacFungalCapabilities FungalCapabilities( int32_t simulation_id, const settings::SmacTechnologyRules& technologies,
		const settings::SmacFactionStartupRules& profiles ) const;

	// These explicit new-world stages are not research-acquisition APIs. The
	// caller supplies turn-zero admission; restoration never calls them.
	void ClearOwnershipForNewWorld() noexcept;
	void InitializeFactionAtTurnZero( int32_t simulation_id, const settings::SmacTechnologyRules& technologies,
		const settings::SmacFactionStartupRules& profiles );
	static SmacTechnologyState NewWorld( const std::vector< SmacSimulationAdmission >& admissions,
		const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles );

	types::Buffer Serialize( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) const;
	static SmacTechnologyState Deserialize( const std::string& wire, const settings::SmacTechnologyRules& technologies,
		const settings::SmacFactionStartupRules& profiles );
};

}
