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
	SDL_Color colors[ 256 ];
	for ( size_t i = 0; i < 256; ++i ) {
		colors[ i ] = palette->colors[ i ];
		colors[ i ].r = std::min< uint8_t >( colors[ i ].r, 235 );
		colors[ i ].g = std::min< uint8_t >( colors[ i ].g, 235 );
		colors[ i ].b = std::min< uint8_t >( colors[ i ].b, 235 );
	}
	if ( SDL_SetPaletteColors( image->format->palette, colors, 0, 256 ) != 0 ) {
		throw std::runtime_error( SDL_GetError() );
	}
}

}
