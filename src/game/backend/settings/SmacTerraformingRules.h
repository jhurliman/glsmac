#pragma once

#include "SmacTechnologyRules.h"

namespace game::backend::settings {

struct SmacTerraformingRule {
	int32_t source_id; // positional TERRAIN row 0..19, never a display-name identity
	std::string land_name, land_prerequisite, sea_name, sea_prerequisite;
	int32_t work_time; // raw active signed32 value, not a clamped duration
	std::string order_text, order_letter, keystroke, trailing_annotation;
	bool operator==( const SmacTerraformingRule& other ) const;
};

struct SmacTerraformingRules {
	static constexpr uint32_t MAX_TEXT_BYTES = 256; // explicit port ingestion budget
	// Version/count, then twenty rows of two integer and eight string frames.
	static constexpr uint32_t MAX_WIRE_BYTES = 2 * 14 + 20 * ( 2 * 14 + 8 * ( 6 + MAX_TEXT_BYTES ) );
	std::array< SmacTerraformingRule, 20 > actions;
	void Validate() const;
	bool operator==( const SmacTerraformingRules& other ) const;
	// Exact tokens remain owned. Resolution does not assert ownership/eligibility.
	std::array< std::array< SmacTechnologyReference, 2 >, 20 > TechnologyReferences( const SmacTechnologyRules& technologies ) const;
	types::Buffer Serialize() const;
	static SmacTerraformingRules Deserialize( types::Buffer buffer );
};

}
