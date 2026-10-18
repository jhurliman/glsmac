#pragma once

#include "game/backend/base/SmacBaseEnergy.h"
#include "types/Buffer.h"
#include <optional>

namespace game::backend::faction {

struct SmacEconomyState {
	base::SmacEnergyRates rates;
	int32_t credits, pending_debit;
	int32_t research_fraction;
	uint8_t treaty_partners;
	// Owner-wide refresh after Base upkeep, not a live sum during capture.
	// Absent only when migrating a port V1 record that did not own this field.
	std::optional<int32_t> population_aggregate = 0;
	static constexpr size_t MAX_WIRE_BYTES = 9 * 14 + 7;
	// All startup inputs are supplied explicitly. Restore never calls this.
	static SmacEconomyState NewWorld( base::SmacEnergyRates rates, int32_t credits, int32_t research_fraction );
	void Validate() const;
	bool operator==( const SmacEconomyState& other ) const;
	types::Buffer Serialize() const;
	static SmacEconomyState Deserialize( const std::string& wire );

	// Candidate arithmetic only. The native turn controller owns once-only
	// ordering; this value has no duplicate scheduling state or callbacks.
	SmacEconomyState CreditBase( int32_t economy, int32_t maintenance ) const;
	SmacEconomyState SettleDebit() const;
};

struct SmacResearchSubmission { int32_t gated_labs, increment, fraction; };
SmacResearchSubmission ScaleSmacResearchSubmission( int32_t labs, int32_t current_research,
	int32_t turn, int32_t difficulty, bool research_halving_facility, int32_t fraction );

}
