#pragma once

#include <string>

#include "game/backend/resource/SmacBaseSquareYield.h"

namespace loader::txt {

class SmacRulesTXTLoader {
public:
	// Canonical nine-row SMAC RESOURCEINFO layout, not SMAX. Decode only the
	// Base Square, Bonus Square and Recycling Tanks N/M/E triples. The caller
	// supplies the selected rules file's contents; no I/O, cache or defaults.
	// Missing/ambiguous layout or invalid signed int32 fields throw
	// std::invalid_argument. Other numeric columns and annotations are ignored.
	static game::backend::resource::smac::BaseSquareRules ParseBaseSquareRules( const std::string& source );
};

}
