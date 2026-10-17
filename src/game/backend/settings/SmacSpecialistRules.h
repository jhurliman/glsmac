#pragma once

#include "SmacTechnologyRules.h"

namespace game::backend::settings {

struct SmacSpecialistRule {
	int32_t source_id; // positional CITIZENS slot 0..6, never a display-name identity
	std::string singular, plural, prerequisite, obsolete;
	int32_t economy, psych, labs;
	std::string trailing_token; // retained opaque source metadata; no inferred effects
	bool operator==( const SmacSpecialistRule& other ) const;
};

struct SmacSpecialistRules {
	static constexpr uint32_t MAX_TEXT_BYTES = 256; // explicit port ingestion budget
	// V1, minimum size, count, then seven (four integers + five strings).
	// Buffer integer frames are 14 bytes; string framing costs 6 bytes.
	static constexpr uint32_t MAX_WIRE_BYTES = 3 * 14 + 7 * ( 4 * 14 + 5 * ( 6 + MAX_TEXT_BYTES ) );
	int32_t minimum_base_size; // canonical RULES row 11; not competition's fixed gate
	std::array< SmacSpecialistRule, 7 > specialists;
	void Validate() const;
	bool operator==( const SmacSpecialistRules& other ) const;
	// Pure catalog resolution, not ownership, eligibility or automatic selection.
	// Both exact source tokens remain owned; UNKNOWN is distinct from sentinels.
	std::array< std::array< SmacTechnologyReference, 2 >, 7 > TechnologyReferences( const SmacTechnologyRules& technologies ) const;
	types::Buffer Serialize() const;
	static SmacSpecialistRules Deserialize( types::Buffer buffer );
};

}
