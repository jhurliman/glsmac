#pragma once

#include <cstdint>

namespace game::backend::resource::smac {

enum class ResourceBonus { None, Nutrients, Minerals, Energy };

// Decoded original bands, not GLSMAC elevation units or rendering flags.
// Land band names deliberately make no claim about their height thresholds.
enum class AltitudeBand : uint8_t { Trench, DeepOcean, Shelf, Land3, Land4, Land5, Land6, Land7 };

struct ResourceBonusInputs {
	ResourceBonusInputs( int32_t x, int32_t y, AltitudeBand altitude_band, int32_t pattern_offset,
		bool automatic_land_resources, bool force_presence, bool override_type, bool energy_marker, bool mineral_marker )
		: x( x ), y( y ), altitude_band( altitude_band ), pattern_offset( pattern_offset ),
		automatic_land_resources( automatic_land_resources ), force_presence( force_presence ),
		override_type( override_type ), energy_marker( energy_marker ), mineral_marker( mineral_marker ) {}
	int32_t x, y; // nonnegative doubled-x map coordinates with matching parity
	AltitudeBand altitude_band;
	int32_t pattern_offset; // a resource-pattern offset, not a general RNG seed
	bool automatic_land_resources;
	bool force_presence;
	bool override_type;
	bool energy_marker;
	bool mineral_marker;
};

// RE-SMAC-RESOURCE-001: pure decoded-input resolver, original-x86 checked over
// the documented synthetic domain. No map/save/editor decoding, coordinate
// wrapping, generation policy, or SMAX equivalence is implied.
// Invalid coordinates/parity/bands throw invalid_argument. Evaluated signed32
// intermediates are checked with int64; unverified overflow throws overflow_error.
ResourceBonus ResolveResourceBonus( const ResourceBonusInputs& inputs );

}
