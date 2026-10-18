#include "FreeType.h"

#include "types/Font.h"
#include "util/FinallyGuard.h"
#include <cstddef>

namespace loader {
namespace font {

void FreeType::Start() {
	auto res = FT_Init_FreeType( &m_freetype );
	if ( res ) {
		THROW( "Unable to initialize FreeType: " + std::to_string( res ) );
	}
}

void FreeType::Stop() {
	FT_Done_FreeType( m_freetype );
}

types::Font* FreeType::LoadFontImpl( const std::string& name, const std::vector< unsigned char >& data, const unsigned char size, RasterMode mode ) {

	const auto font_key = name + ":" + std::to_string( size ) + ( mode == RasterMode::MONOCHROME ? ":mono" : "" );

	int res = 0;

	Log( "Loading font \"" + font_key + "\"" );

	NEWV( font, types::Font, font_key );
	util::FinallyGuard release_font( [ & ] { if ( font ) { DELETE( font ); } } );

	FT_Face ftface;
	res = FT_New_Memory_Face( m_freetype, data.data(), data.size(), 0, &ftface );
	if ( res ) {
		THROW( "Unable to load font \"" + name + "\": " + std::to_string( res ) );
	}

	util::FinallyGuard release_face( [ & ] { FT_Done_Face( ftface ); } );
	res = FT_Set_Pixel_Sizes( ftface, 0, size );
	if ( res ) { THROW( "Unable to set font pixel size: " + std::to_string( res ) ); }

	font->m_dimensions.width = font->m_dimensions.height = 0;

	FT_GlyphSlot g = ftface->glyph;
	types::Font::bitmap_t* bitmap;
	int sz;

	for ( int i = 32 ; i < 128 ; i++ ) { // only ascii for now
		res = FT_Load_Char( ftface, i, FT_LOAD_RENDER | ( mode == RasterMode::MONOCHROME ? FT_LOAD_TARGET_MONO : FT_LOAD_TARGET_NORMAL ) );
		if ( res ) {
			THROW( "Font \"" + name + "\" bitmap loading failed: " + std::to_string( res ) );
		}

		bitmap = &font->m_symbols[ i ];

		bitmap->ax = g->advance.x >> 6;
		bitmap->ay = g->advance.y >> 6;
		bitmap->width = g->bitmap.width;
		bitmap->height = g->bitmap.rows;
		bitmap->left = g->bitmap_left;
		bitmap->top = g->bitmap_top;
		sz = bitmap->width * bitmap->height;
		if ( sz > 0 ) {
			bitmap->data = (unsigned char*)malloc( sz );
			if ( !bitmap->data ) { throw std::bad_alloc(); }
			if ( g->bitmap.pixel_mode != FT_PIXEL_MODE_MONO && g->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY ) {
				THROW( "Unsupported font bitmap pixel mode" );
			}
			for ( unsigned int y = 0; y < bitmap->height; ++y ) {
				const auto* row = g->bitmap.buffer + std::ptrdiff_t( y ) * g->bitmap.pitch;
				for ( unsigned int x = 0; x < bitmap->width; ++x ) {
					bitmap->data[ size_t( y ) * bitmap->width + x ] = g->bitmap.pixel_mode == FT_PIXEL_MODE_MONO
						? ( row[ x / 8 ] & ( 0x80 >> ( x % 8 ) ) ? 255 : 0 ) : row[ x ];
				}
			}
		}
		else {
			bitmap->data = nullptr;
		}

		font->m_dimensions.width += g->bitmap.width;
		font->m_dimensions.height = std::max( font->m_dimensions.height, (float)g->bitmap.rows );
	}


	DEBUG_STAT_INC( fonts_loaded );

	auto* result = font;
	font = nullptr;
	return result;
}

}
}
