// Opt-in production renderer probe. Inputs are user-owned installed assets.
#include "loader/cvr/CvrPaletteReader.h"
#include "loader/cvr/CvrScoutRenderer.h"
#include "loader/cvr/ScoutLighting.h"

#include <filesystem>
#include <charconv>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

std::string Read( const char* path ) {
	if ( std::filesystem::file_size( path ) > 32 * 1024 * 1024 ) { throw std::invalid_argument( "asset exceeds probe budget" ); }
	std::ifstream input( path, std::ios::binary );
	if ( !input ) { throw std::runtime_error( "cannot open input" ); }
	std::string bytes{ std::istreambuf_iterator< char >( input ), std::istreambuf_iterator< char >() };
	if ( input.bad() ) { throw std::runtime_error( "cannot read input" ); }
	return bytes;
}

int32_t Integer( const char* text ) {
	const std::string value( text );
	int32_t result;
	const auto parsed = std::from_chars( value.data(), value.data() + value.size(), result );
	if ( parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() ) { throw std::invalid_argument( "invalid integer argument" ); }
	return result;
}

}

int main( const int argc, const char* const* argv ) {
	const bool lighting = argc == 3 && std::string( argv[ 1 ] ) == "--lighting";
	const bool palette = argc == 4 && std::string( argv[ 1 ] ) == "--palette";
	const bool render = argc == 11 && std::string( argv[ 1 ] ) == "--render";
	if ( !lighting && !palette && !render ) {
		std::cerr << "usage: cvr_scout_probe --lighting output | --palette asset output | --render infantry reactor output facing zoom numerator denominator anchor_x anchor_y\n";
		return 2;
	}
	try {
		std::string bytes;
		if ( lighting ) {
			const auto table = loader::cvr::scout::GenerateLightingTable();
			for ( const auto& row : table ) { bytes.append( reinterpret_cast< const char* >( row.data() ), row.size() ); }
		}
		else if ( palette ) {
			const auto result = loader::cvr::CvrPaletteReader::Read( loader::cvr::CvrContainerReader::Read( Read( argv[ 2 ] ) ) );
			bytes.push_back( static_cast< char >( result.first ) ); bytes.push_back( static_cast< char >( result.end ) );
			for ( const auto& color : result.colors ) { bytes.append( reinterpret_cast< const char* >( color.data() ), color.size() ); }
			bytes.append( reinterpret_cast< const char* >( result.shades.data() ), result.shades.size() );
		}
		else {
			const auto renderer = loader::cvr::CvrScoutRenderer::Load( Read( argv[ 2 ] ), Read( argv[ 3 ] ) );
			const auto result = renderer.Render( { Integer( argv[ 5 ] ), Integer( argv[ 6 ] ), Integer( argv[ 7 ] ), Integer( argv[ 8 ] ), { Integer( argv[ 9 ] ), Integer( argv[ 10 ] ) } } );
			bytes.append( reinterpret_cast< const char* >( result.indexed.color.data() ), result.indexed.color.size() );
			for ( const auto value : result.indexed.depth ) { bytes.push_back( static_cast< char >( value ) ); bytes.push_back( static_cast< char >( value >> 8 ) ); }
			bytes.append( reinterpret_cast< const char* >( result.rgba.data() ), result.rgba.size() );
			std::cout << result.draw_count << ' ' << result.processed_records << ' ' << result.written_samples << ' ' << result.target_anchor[ 0 ] << ' ' << result.target_anchor[ 1 ] << '\n';
		}
		std::ofstream output( argv[ lighting ? 2 : render ? 4 : 3 ], std::ios::binary | std::ios::trunc );
		output.write( bytes.data(), static_cast< std::streamsize >( bytes.size() ) );
		output.close();
		if ( !output ) { throw std::runtime_error( "cannot write output" ); }
		std::cout << bytes.size() << '\n';
		return 0;
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}
