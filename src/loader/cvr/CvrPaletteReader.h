#pragma once

#include "CvrContainerReader.h"

#include <string>

namespace loader::cvr {

struct CvrPalette {
	using Color = std::array< uint8_t, 3 >;
	std::string name;
	uint8_t first, end; // Inclusive first, exclusive end; end==0 never means 256.
	std::array< Color, 256 > colors;
	std::array< uint8_t, 24 * 256 > shades;
	std::vector< uint8_t > auxiliary; // Owned opaque 0x301 field, not interpreted.
};

class CvrPaletteReader {
public:
	// Decode tag3's length-delimited fields. Modern version>7 stores24 shade
	// rows; finite version<=7 stores16, expanded with source row floor(2*r/3).
	// Unknown nested tags are skipped. Exactly one nonempty name and palette
	// are required; duplicate known fields, malformed framing/nonfinite version
	// and nonexact palette sizes reject before owned output is materialized.
	// No RGB installation, faction override or alpha inference.
	static CvrPalette Read( const CvrContainer& container );
};

}
