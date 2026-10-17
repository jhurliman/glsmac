#pragma once

#include "UnitRulesTXTLoader.h"

#include <cstddef>

namespace loader::txt {

struct ResolvedUnitDesign {
	size_t source_index; // zero-based #UNITS record index, independent of display name
	UnitDesignRecord record;
	ChassisComponent chassis;
	WeaponComponent weapon;
	DefenseComponent defense;
};

struct ResolvedUnitTable {
	std::string ruleset_id;
	std::vector< ResolvedUnitDesign > units;
};

class UnitDesignResolver {
public:
	// Resolve decoded tables with the same explicit smac/smax identity. Keys
	// must be nonempty, NUL-free and unique throughout each component catalog.
	// Missing references or invalid identities/keys throw std::invalid_argument.
	// The complete result owns copies of all metadata; inputs are unchanged.
	// Numeric fields retain their decoder contract and are not revalidated here.
	// This does not select a reactor, decode abilities, calculate cost/capacity,
	// establish playable-design eligibility, or create runtime definitions.
	static ResolvedUnitTable Resolve( const UnitTable& units, const ChassisTable& chassis,
		const WeaponTable& weapons, const DefenseTable& defenses );
};

}
