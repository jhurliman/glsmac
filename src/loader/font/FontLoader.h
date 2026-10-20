#pragma once

#include <string>
#include <unordered_map>
#include <optional>

#include "loader/Loader.h"

#include "resource/Types.h"
#include "types/Vec2.h"

namespace types {
class Font;
}

namespace loader {
namespace font {

struct NativeLayout {int logical_height,width_8,width_88,width_H;};

enum class RasterMode { GRAYSCALE, MONOCHROME, MONOCHROME_BOLD };

CLASS( FontLoader, Loader )
	virtual ~FontLoader();

	types::Font* LoadFont( const resource::resource_t res, const unsigned char size );
	types::Font* LoadCustomFont( const std::string& name, const unsigned char size, RasterMode mode = RasterMode::GRAYSCALE );
	types::Font* GetBuiltinFont( const unsigned char size );
    std::optional<NativeLayout> GetSmacLayout(const types::Font* font) const;

protected:
	virtual types::Font* LoadFontImpl( const std::string& name, const std::vector< unsigned char >& data, const unsigned char size, RasterMode mode = RasterMode::GRAYSCALE ) = 0;

private:
	// cache all fonts for future use
    struct CachedFont {types::Font* font;std::optional<NativeLayout> layout;};
	std::unordered_map< std::string, CachedFont > m_fonts;
	std::unordered_map< unsigned char, types::Font* > m_builtin_fonts;
};

}
}
