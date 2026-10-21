#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <optional>

#include "types/Buffer.h"

namespace game::backend::settings {

struct SmacFacilityRule {
	int32_t source_id; // canonical one-based SMAC slot, not a port Def/production ID
	std::string name;
	int32_t cost, maintenance; // unscaled signed metadata, not a payable cost
	std::string prerequisite; // exact unresolved token, including None/Disable
	bool operator==( const SmacFacilityRule& other ) const;
};

struct SmacProductionRules {
	int32_t mineral_multiplier, retool_loss_percent, retool_strictness, retool_exemption;
	// Slots 1..33 local facilities, 34..37 orbitals, 38 Stockpile, 39..71 projects.
	// Names may repeat/change; identity is positional. No lookup grants eligibility.
	std::array< SmacFacilityRule, 71 > facilities;
	// Land/sea/air RULES 15..17. Absent in V1 saves; never invent stock defaults.
	std::optional<std::array<int32_t,3>> prototype_percentages;

	// Structural ingestion only. All signed32 numeric metadata is preserved;
	// calculation/admission must separately validate its supported domains.
	void Validate() const;
	bool operator==( const SmacProductionRules& other ) const;
	types::Buffer Serialize() const;
	static SmacProductionRules Deserialize( types::Buffer buffer );
};

}
