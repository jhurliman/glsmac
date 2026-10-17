#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

#include "types/Buffer.h"

namespace game::backend::settings {

struct SmacTechnologyRule {
	int32_t source_id; // canonical zero-based SMAC slot, not a port runtime ID
	std::string name, key; // key is the loader's first seven bytes
	std::array< int32_t, 4 > priorities; // source positions, no inferred axis names
	std::array< std::string, 2 > prerequisites;
	uint16_t flags; // nine source bits, no gameplay effects applied
	bool operator==( const SmacTechnologyRule& other ) const;
};

struct SmacProductionPrerequisite {
	enum class Status { NO_REQUIREMENT, DISABLED, TECHNOLOGY_ID, UNAVAILABLE };
	Status status;
	std::optional< int32_t > technology_id; // present for a known catalog key
	// TECHNOLOGY_ID passes the production definition sign guard only, not the
	// loader's stricter counted-definition test, ownership, or buildability.
};

struct SmacTechnologyReference {
	enum class Kind { NONE, DISABLED, TECHNOLOGY_ID, UNKNOWN };
	Kind kind;
	std::optional< int32_t > technology_id;
};

struct SmacTechnologyRules {
	std::array< SmacTechnologyRule, 89 > technologies;
	void Validate() const;
	bool operator==( const SmacTechnologyRules& other ) const;
	types::Buffer Serialize() const;
	static SmacTechnologyRules Deserialize( types::Buffer buffer );
	SmacTechnologyReference ResolveToken( const std::string& token ) const;
	SmacProductionPrerequisite ResolveProductionPrerequisite( const std::string& key ) const;
};

}
