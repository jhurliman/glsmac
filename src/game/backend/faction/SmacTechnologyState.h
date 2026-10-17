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
	std::optional< int32_t > target, cached_threshold;
	bool operator==( const SmacFactionTechnologyState& other ) const;
};

// Owned mutable simulation values, separate from frozen definitions. This
// first codec does not publish into Game or infer a simulation roster.
struct SmacTechnologyState {
	static constexpr size_t MAX_WIRE_BYTES = 8 * 1024;
	std::vector< SmacFactionTechnologyState > factions;
	void Validate( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) const;
	bool operator==( const SmacTechnologyState& other ) const;
	const SmacFactionTechnologyState* Find( int32_t simulation_id ) const;

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
