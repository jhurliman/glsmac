#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include <SDL.h>

namespace loader::texture {

// Opaque indexed PCX display conversion at Gamma Correction 100. This must
// execute in every build configuration, including without debug assertions.
inline void ApplySmacPalette( SDL_Surface* image, const SDL_Palette* palette ) {
	if ( !image || image->format->BitsPerPixel != 8 || !image->format->palette || image->format->palette->ncolors != 256 ) {
		throw std::invalid_argument( "SMAC display palette requires an indexed source" );
	}
	if ( !palette || palette->ncolors != 256 ) {
		throw std::invalid_argument( "SMAC display palette requires the 256-color common palette" );
	}
	// This PCX profile maps source 0..235 to game entries 10..245.
	// The remaining system-reserved colors are not common-palette colors.
	// Reject them rather than invent a gamma conversion for those indices.
	for ( int y = 0; y < image->h; ++y ) {
		const auto* row = static_cast< const uint8_t* >( image->pixels ) + y * image->pitch;
		for ( int x = 0; x < image->w; ++x ) {
			if ( row[ x ] >= 236 ) {
				throw std::invalid_argument( "SMAC PCX display palette supports source indices 0..235" );
			}
		}
	}
	SDL_Color colors[ 236 ];
	for ( size_t i = 0; i < 236; ++i ) {
		colors[ i ] = palette->colors[ i ];
		colors[ i ].r = std::min< uint8_t >( colors[ i ].r, 235 );
		colors[ i ].g = std::min< uint8_t >( colors[ i ].g, 235 );
		colors[ i ].b = std::min< uint8_t >( colors[ i ].b, 235 );
	}
	if ( SDL_SetPaletteColors( image->format->palette, colors, 0, 236 ) != 0 ) {
		throw std::runtime_error( SDL_GetError() );
	}
}

}
