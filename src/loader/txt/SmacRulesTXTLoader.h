#pragma once

#include <string>

#include "game/backend/resource/SmacBaseSquareYield.h"
#include "game/backend/settings/SmacResourceRules.h"
#include "game/backend/settings/SmacSpecialistRules.h"
#include "game/backend/settings/SmacProductionRules.h"
#include "game/backend/settings/SmacMovementRules.h"
#include "game/backend/settings/SmacTerritoryRules.h"
#include "game/backend/settings/SmacTechnologyRules.h"
#include "game/backend/settings/SmacWorldSizeRules.h"

namespace loader::txt {

class SmacRulesTXTLoader {
public:
	// Strict port ingestion: seven positional eight-column CITIZENS specialist
	// rows, three ordinary-citizen name rows, and RULES row 11 from 70 rows.
	// Preserve signed metadata and opaque trailing token; no selection/effects.
	static game::backend::settings::SmacSpecialistRules ParseSpecialistRules( const std::string& source );
	// Nine positional SMAC RESOURCEINFO rows, not SMAX. Decode only the
	// N/M/E triples at positions 2, 3 and 5, regardless of their labels. The caller
	// supplies the selected rules file's contents; no I/O, cache or defaults.
	// Consume nine physical lines: blanks/comments store zeros. Numeric
	// prefixes use bounded decimal/hex/binary conversion and signed32 wrap.
	// Missing/duplicate section, early EOF/section, quoted fixed fields or
	// NUL/oversized rows, early inline comments,
	// missing field delimiters or magnitude above 2^32 throw invalid_argument.
	// Fourth numeric columns, annotations and later rows are not interpreted.
	static game::backend::resource::smac::BaseSquareRules ParseBaseSquareRules( const std::string& source );
	// All nine RESOURCEINFO triples plus RULES 2/9/10 and 54/55/56/58.
	// RULES uses the existing strict 70-row decimal/token policy, independently
	// of RESOURCEINFO's measured physical-line/prefix/wrap semantics.
	static game::backend::settings::SmacResourceRules ParseResourceRules( const std::string& source,
		const game::backend::settings::SmacTechnologyRules& technologies );
	// Canonical SMAC: 70 positional RULES rows and 71 FACILITIES rows.
	// Retains only production rules and facility name/cost/maintenance/prerequisite.
	// Obsolete Free and the effect/AI suffix are not interpreted. No file I/O,
	// technology resolution, port definition creation or buildability decision.
	static game::backend::settings::SmacProductionRules ParseProductionRules( const std::string& source );
	// Canonical 70-row SMAC RULES section. Retain only signed32 row 1;
	// strict port text ingestion, no I/O, SMAX inference or live settings binding.
	static game::backend::settings::SmacMovementRules ParseMovementRules( const std::string& source );
	// Same strict 70-row policy, raw signed32 RULES row48. The owned profile
	// derives the original 0..100 clamp separately from source identity.
	static game::backend::settings::SmacTerritoryRules ParseTerritoryRules( const std::string& source );
	// Exactly 89 canonical positional TECHNOLOGY records, nine columns each.
	// Four signed priorities retain their order; flags retain nine source bits.
	static game::backend::settings::SmacTechnologyRules ParseTechnologyRules( const std::string& source );
	// Five positional SMAC presets: count, then name and two source dimensions.
	// Strict port ingestion; no missing-section fallback or SMAX inference.
	static game::backend::settings::SmacWorldSizeRules ParseWorldSizeRules( const std::string& source );
};

}
