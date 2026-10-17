#pragma once

#include "CvrPaletteReader.h"

#include <string_view>

namespace loader::cvr {

// Owned software-palette/material values for the admitted faction-PCX path.
// Entry flags are not alpha and are not used in the RGB nearest-color search.
struct CvrActorMaterial {
	std::array< CvrPalette::Color, 256 > colors;
	std::array< uint8_t, 24 * 256 > shades;
	uint8_t tint_index;

	// Bounded PCX5/RLE/8-bit/one-plane input, origin0, maximum2048x2048.
	// PALETTE installs its first236 RGB entries at10; reserved entries retain
	// the observed software constructor constants. Faction pixels use offset10
	// modulo256; its own PCX palette never replaces the active palette.
	// Sample435,745 supplies the material tint. No name/blend/UI behavior.
	static std::array< CvrPalette::Color, 256 > ReadSoftwareColors( std::string_view palette_pcx );
	static CvrActorMaterial Read( std::string_view palette_pcx, std::string_view faction_pcx,
		const std::array< uint8_t, 24 * 256 >& body_shades );
};

}
