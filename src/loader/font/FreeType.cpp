#include "FreeType.h"

#include "types/Font.h"
#include "SmacFontLayout.h"
#include "util/SHA256.h"
#include "util/FinallyGuard.h"
#include <cstddef>
#include FT_SYNTHESIS_H

namespace loader {
namespace font {
namespace {
// Thirteen scan-conversion deltas for the identified Arial10 face. A complete
// input-mask digest prevents altering unknown/new FreeType raster results.
void CorrectArial10Raster(types::Font& font) {
    struct Delta {int x,y;bool ink;};
    struct Correction {unsigned code;const char* input;std::vector<Delta> deltas;};
    static const std::array<Correction,4> corrections{{
        {64,"f61fc80e000b29c4787d699cab71c74ab3a3e09cff4a01560a89131c9010bbc2",{{1,-5,true},{2,-5,false}}},
        {75,"daa31a99f44caff91620e912e8077cf8f6a7406c41160f779096c345c1e0e079",{{5,-7,true},{6,-7,false},{4,-6,true},{5,-6,false},{3,-5,true},{4,-5,false}}},
        {86,"25a138580145d96167679e6791f18ca6f66788549ee9d3fdc3a7a9101b416aff",{{4,-1,false}}},
        {90,"86961ec2e2a3943e848eb96bfbf84fc390a770f07bac0f4e12ae7cfdffdf379b",{{5,-6,true},{4,-5,true},{3,-4,true},{2,-3,true}}},
    }};
    for(const auto& fix:corrections) {
        auto& glyph=font.m_symbols[fix.code];std::array<char,4096> cell{};bool bounded=true;
        for(unsigned y=0;y<glyph.height;++y) {for(unsigned x=0;x<glyph.width;++x) {
            const int dx=16+glyph.left+int(x),dy=48-glyph.top+int(y);
            if(dx<0 || dx>=64 || dy<0 || dy>=64) {bounded=false;continue;}
            cell[size_t(dy)*64+size_t(dx)]=glyph.data[size_t(y)*glyph.width+x]!=0;
        }}
        if(!bounded || util::SHA256(std::string_view(cell.data(),cell.size()))!=fix.input) {continue;}
        for(const auto& delta:fix.deltas) {
            const int x=delta.x-glyph.left,y=delta.y+glyph.top;
            if(x<0 || x>=int(glyph.width) || y<0 || y>=int(glyph.height)) {bounded=false;}
        }
        if(!bounded) {continue;}
        for(const auto& delta:fix.deltas) {glyph.data[size_t(delta.y+glyph.top)*glyph.width+size_t(delta.x-glyph.left)]=delta.ink ? 255:0;}
    }
}
}

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

	const auto font_key = name + ":" + std::to_string( size ) + ( mode == RasterMode::MONOCHROME_BOLD ? ":mono-bold" : mode == RasterMode::MONOCHROME ? ":mono" : "" );

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
		res = FT_Load_Char( ftface, i, FT_LOAD_RENDER | ( mode != RasterMode::GRAYSCALE ? FT_LOAD_TARGET_MONO : FT_LOAD_TARGET_NORMAL ) );
		if ( res ) {
			THROW( "Font \"" + name + "\" bitmap loading failed: " + std::to_string( res ) );
		}

        // The original registers regular Arial Narrow and requests bold from
        // GDI. Embolden the hinted mono bitmap, not the outline or a different
        // bundled face; the distinction changes both ink and text advances.
        if ( mode == RasterMode::MONOCHROME_BOLD ) { FT_GlyphSlot_Embolden( g ); }

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


    if(mode==RasterMode::MONOCHROME) {
        const auto sha=util::SHA256(std::string_view(reinterpret_cast<const char*>(data.data()),data.size()));
        if(const auto* advances=smac::AsciiAdvances(sha,size)) {
            for(size_t i=0;i<advances->size();++i) {font->m_symbols[i+32].ax=advances->at(i);}
            CorrectArial10Raster(*font);
        }
    }
	DEBUG_STAT_INC( fonts_loaded );

	auto* result = font;
	font = nullptr;
	return result;
}

}
}
