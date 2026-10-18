#include "FontLoader.h"

#include "types/Font.h"
#include "util/FS.h"
#include <cstdlib>

namespace loader {
namespace font {

FontLoader::~FontLoader() {
	for ( auto& it : m_fonts ) {
		DELETE( it.second );
	}
	for ( const auto& f : m_builtin_fonts ) {
		DELETE( f.second );
	}
}

types::Font* FontLoader::LoadFont( const resource::resource_t res, const unsigned char size ) {
	const auto& name = GetFilename( res );
	const auto key = name + ":" + std::to_string( size );
	auto it = m_fonts.find( key );
	if ( it == m_fonts.end() ) {
		std::vector< unsigned char > data = {};
		util::FS::ReadFile( data, GetPath( res ) );
		it = m_fonts.insert(
			{
				key,
				LoadFontImpl( GetFilename( res ), data, size )
			}
		).first;
	}
	return it->second;
}

types::Font* FontLoader::LoadCustomFont( const std::string& name, const unsigned char size, RasterMode mode ) {
	const auto key = name + ":" + std::to_string( size ) + ( mode == RasterMode::MONOCHROME ? ":mono" : "" );
	auto it = m_fonts.find( key );
	if ( it == m_fonts.end() ) {
		std::vector< unsigned char > data = {};
		auto path = TryGetCustomFilename( name );
		if ( path.empty() && name == "arialbd.ttf" ) {
			std::vector< std::string > candidates;
#ifdef _WIN32
			if ( const auto* windows = std::getenv( "WINDIR" ) ) { candidates.emplace_back( std::string( windows ) + "/Fonts/arialbd.ttf" ); }
#elif defined( __APPLE__ )
			candidates = { "/System/Library/Fonts/Supplemental/Arial Bold.ttf", "/Library/Fonts/Arial Bold.ttf" };
#else
			candidates = { "/usr/share/fonts/truetype/msttcorefonts/Arial_Bold.ttf", "/usr/share/fonts/truetype/msttcorefonts/arialbd.ttf", "/mnt/c/Windows/Fonts/arialbd.ttf" };
#endif
			for ( const auto& candidate : candidates ) { if ( util::FS::FileExists( candidate ) ) { path = candidate; break; } }
			if ( path.empty() ) {
				Log( "Arial Bold is unavailable; title font fidelity requires arialbd.ttf in the game data or system fonts" );
				path = GetCustomFilename( "arialnb.ttf" );
			}
		}
		if ( path.empty() ) { path = GetCustomFilename( name ); }
		util::FS::ReadFile( data, path );
		it = m_fonts.insert(
			{
				key,
				LoadFontImpl( path, data, size, mode )
			}
		).first;
	}
	return it->second;
}

types::Font* FontLoader::GetBuiltinFont( const unsigned char size ) {
	auto it = m_builtin_fonts.find( size );
	if ( it == m_builtin_fonts.end() ) {
		const auto font_key = "<BUILTIN>:" + std::to_string( size );
		std::vector< unsigned char > buffer = {};
		util::FS::ReadFile( buffer, "resource/font/terminus/TerminusTTF-4.49.3-minified.ttf" );
		it = m_builtin_fonts.insert(
			{
				size,
				LoadFontImpl( "<BUILTIN>", buffer, size )
			}
		).first;
	}
	return it->second;
}

}
}
