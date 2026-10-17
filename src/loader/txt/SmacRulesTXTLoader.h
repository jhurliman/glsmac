#pragma once

#include <string>

#include "game/backend/resource/SmacBaseSquareYield.h"
#include "game/backend/settings/SmacProductionRules.h"
#include "game/backend/settings/SmacTechnologyRules.h"

namespace loader::txt {

class SmacRulesTXTLoader {
public:
	// Canonical nine-row SMAC RESOURCEINFO layout, not SMAX. Decode only the
	// Base Square, Bonus Square and Recycling Tanks N/M/E triples. The caller
	// supplies the selected rules file's contents; no I/O, cache or defaults.
	// Missing/ambiguous layout or invalid signed int32 fields throw
	// std::invalid_argument. Other numeric columns and annotations are ignored.
	static game::backend::resource::smac::BaseSquareRules ParseBaseSquareRules( const std::string& source );
	// Canonical SMAC: 70 positional RULES rows and 71 FACILITIES rows.
	// Retains only production rules and facility name/cost/maintenance/prerequisite.
	// Obsolete Free and the effect/AI suffix are not interpreted. No file I/O,
	// technology resolution, port definition creation or buildability decision.
	static game::backend::settings::SmacProductionRules ParseProductionRules( const std::string& source );
	// Exactly 89 canonical positional TECHNOLOGY records, nine columns each.
	// Four signed priorities retain their order; flags retain nine source bits.
	static game::backend::settings::SmacTechnologyRules ParseTechnologyRules( const std::string& source );
};

}
