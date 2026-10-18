#include "SDL2.h"

#include <algorithm>
#include <memory>

#include "util/FS.h"
#include "types/texture/Texture.h"

namespace loader {
namespace texture {

SDL2::~SDL2() {
	for ( auto& it : m_textures ) {
		DELETE( it.second );
	}
	for ( auto& it : m_subtextures ) {
		DELETE( it.second );
	}
}

void SDL2::Start() {

}

void SDL2::Stop() {

}

void SDL2::Iterate() {

}

SDL_Surface* load_image( const std::vector< unsigned char >& buffer ) {
	SDL_RWops* rw = SDL_RWFromConstMem( buffer.data(), buffer.size() );
	ASSERT( rw, "rw is null" );
	return IMG_Load_RW( rw, 1 );
}

types::texture::Texture* SDL2::LoadTextureImpl( const std::string& filename, const types::texture::texture_flag_t flags ) {

	const auto cache_key = filename + ":flags=" + std::to_string( flags );
	texture_map_t::iterator it = m_textures.find( cache_key );
	if ( it != m_textures.end() ) {
		return it->second;
	}
	else {
		Log( "Loading texture \"" + filename + "\"" );
		std::vector< unsigned char > buffer = {};
		util::FS::ReadFile( buffer, filename );
		using Surface = std::unique_ptr< SDL_Surface, decltype( &SDL_FreeSurface ) >;
		Surface image( load_image( buffer ), SDL_FreeSurface );
		ASSERT( image, IMG_GetError() );
		if ( flags & types::texture::TF_SMAC_PALETTE ) {
			ASSERT( image->format->BitsPerPixel == 8 && image->format->palette && image->format->palette->ncolors == 256,
				"SMAC display palette requires an indexed source" );
			ASSERT( m_transparent_colors.empty() && !m_fix_yellow_shadows,
				"SMAC display palette currently requires an opaque source" );
			std::vector< unsigned char > palette_buffer;
			util::FS::ReadFile( palette_buffer, GetCustomFilename( "palette.pcx" ) );
			Surface palette_image( load_image( palette_buffer ), SDL_FreeSurface );
			ASSERT( palette_image && palette_image->format->palette && palette_image->format->palette->ncolors == 256,
				"SMAC display palette requires the 256-color common palette" );
			SDL_Color colors[ 256 ];
			for ( size_t i = 0; i < 256; ++i ) {
				colors[ i ] = palette_image->format->palette->colors[ i ];
				colors[ i ].r = std::min< uint8_t >( colors[ i ].r, 235 );
				colors[ i ].g = std::min< uint8_t >( colors[ i ].g, 235 );
				colors[ i ].b = std::min< uint8_t >( colors[ i ].b, 235 );
			}
			ASSERT( SDL_SetPaletteColors( image->format->palette, colors, 0, 256 ) == 0, SDL_GetError() );
		}
		if ( image->format->format != SDL_PIXELFORMAT_RGBA32 ) {
			// we must have all images in same format
			auto* converted = SDL_ConvertSurfaceFormat( image.get(), SDL_PIXELFORMAT_RGBA32, 0 );
			ASSERT( converted, SDL_GetError() );
			image.reset( converted );
		}

		NEWV( texture, types::texture::Texture, filename, image->w, image->h, flags );
		texture->SetBitmap( image->pixels );
		ASSERT( texture->m_bpp == image->format->BitsPerPixel / 8, "unsupported texture bpp" );
		memcpy( ptr( texture->m_bitmap, 0, texture->m_bitmap_size ), image->pixels, texture->m_bitmap_size );
		image.reset();

		FixTexture( texture ); // some pcx files have strange artifacts that we need to fix procedurally

		FixTransparency( texture );
		FixYellowShadows( texture );

		m_textures.insert(
			{
				cache_key,
				texture
			}
		);

		DEBUG_STAT_INC( textures_loaded );

		return texture;
	}

}

types::texture::Texture* SDL2::LoadTextureImpl( const std::string& name, const size_t x1, const size_t y1, const size_t x2, const size_t y2, const uint8_t flags, const float value, const types::texture::texture_flag_t texture_flags ) {
	ASSERT( x1 <= x2, "LoadTexture x overflow ( " + std::to_string( x1 ) + " > " + std::to_string( x2 ) + " )" );
	ASSERT( y1 <= y2, "LoadTexture y overflow ( " + std::to_string( y1 ) + " > " + std::to_string( y2 ) + " )" );

	const std::string subtexture_key =
		name + ":" +
			std::to_string( x1 ) + ":" +
			std::to_string( y1 ) + ":" +
			std::to_string( x2 ) + ":" +
			std::to_string( y2 ) + ":" +
			std::to_string( flags ) + ":" +
			std::to_string( value ) + ":" + std::to_string( texture_flags );

	texture_map_t::iterator it = m_subtextures.find( subtexture_key );
	if ( it != m_subtextures.end() ) {
		return it->second;
	}
	else {

		auto* full_texture = LoadTextureImpl( name, texture_flags );

		NEWV( subtexture, types::texture::Texture, subtexture_key, x2 - x1 + 1, y2 - y1 + 1 );

		subtexture->AddFrom( full_texture, types::texture::AM_DEFAULT, x1, y1, x2, y2 );
		if ( flags & types::texture::LT_ROTATE ) {
			subtexture->Rotate();
		}
		if ( flags & types::texture::LT_FLIPV ) {
			subtexture->FlipV();
		}

		if ( flags & types::texture::LT_ALPHA ) {
			subtexture->SetAlpha( value );
		}
		if ( flags & types::texture::LT_CONTRAST ) {
			subtexture->SetContrast( value );
		}

		if ( flags & types::texture::LT_TILED ) {
			subtexture->m_is_tiled = true;
		}
		
		// needed?
		FixTransparency( subtexture );
		//FixYellowShadows( subtexture );

		m_subtextures[ subtexture_key ] = subtexture;

		return subtexture;
	}
}

void SDL2::FixTransparency( types::texture::Texture* texture ) const {
	if ( !m_transparent_colors.empty() ) {
		void* at = nullptr;
		for ( size_t i = 0 ; i < texture->m_bitmap_size ; i += texture->m_bpp ) {
			at = ptr( texture->m_bitmap, i, texture->m_bpp );
			for ( auto& c : m_transparent_colors ) {
				if ( !memcmp( at, &c, texture->m_bpp ) ) {
					memset( at, 0, texture->m_bpp );
					break;
				}
			}
		}
	}
}

static const types::Color::rgba_t s_yellow_shadow_src = types::Color::RGB( 253, 189, 118 );
static const types::Color::rgba_t s_yellow_shadow_dst = types::Color::RGBA( 0, 0, 0, 127 );

void SDL2::FixYellowShadows( types::texture::Texture* texture ) const {
	if ( m_fix_yellow_shadows ) {
		void* at = nullptr;
		for ( size_t i = 0 ; i < texture->m_bitmap_size ; i += texture->m_bpp ) {
			at = ptr( texture->m_bitmap, i, texture->m_bpp );
			if ( !memcmp( at, &s_yellow_shadow_src, texture->m_bpp ) ) {
				memcpy( at, &s_yellow_shadow_dst, texture->m_bpp );
			}
		}
	}
}

void SDL2::FixTexture( types::texture::Texture* texture ) const {
	if ( texture->m_name == "interface.pcx" ) {
		ASSERT( texture->m_width == 750 && texture->m_height == 900, "unexpected texture.pcx dimensions" );
		// rain icons have weird brown lines on them
		const std::vector< types::Vec2< size_t > > pixels_to_fix = {
			{ 725, 309 },
			{ 724, 310 },
			{ 723, 310 },
			{ 722, 310 },
			{ 721, 311 },
			{ 720, 312 },
			{ 719, 312 },
		};
		for ( auto& v : pixels_to_fix ) {
			texture->SetPixel( v.x, v.y, texture->GetPixel( v.x + 1, v.y ) );
		}
	}
}

}
}
