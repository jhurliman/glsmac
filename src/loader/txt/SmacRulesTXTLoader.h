#pragma once

#include <string>

#include "game/backend/resource/SmacBaseSquareYield.h"

namespace loader::txt {

class SmacRulesTXTLoader {
public:
	// Nine positional SMAC RESOURCEINFO rows, not SMAX. Decode only the
	// N/M/E triples at positions 2, 3 and 5, regardless of their labels. The caller
	// supplies the selected rules file's contents; no I/O, cache or defaults.
	// Missing/extra rows or invalid signed int32 fields throw
	// std::invalid_argument. Other numeric columns and annotations are ignored.
	static game::backend::resource::smac::BaseSquareRules ParseBaseSquareRules( const std::string& source );
};

}
