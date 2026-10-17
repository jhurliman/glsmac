// Opt-in actual production adapter. No proprietary payloads are compiled in.
#include "loader/cvr/CvrSpartanRoverRenderer.h"
#include "loader/cvr/CvrSceneReader.h"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {

std::string Lower( std::string text ) {
	for ( auto& c : text ) { if ( c >= 'A' && c <= 'Z' ) { c = static_cast< char >( c + 'a' - 'A' ); } }
	return text;
}
std::string Read( const std::filesystem::path& path ) {
	if ( !std::filesystem::is_regular_file( path ) || std::filesystem::file_size( path ) > 32 * 1024 * 1024 ) { throw std::invalid_argument( "missing/oversized input" ); }
	std::ifstream input( path, std::ios::binary );
	if ( !input ) { throw std::runtime_error( "cannot open input" ); }
	std::string result{ std::istreambuf_iterator< char >( input ), std::istreambuf_iterator< char >() };
	if ( input.bad() ) { throw std::runtime_error( "cannot read input" ); }
	return result;
}
std::string Installed( const std::filesystem::path& directory, const std::string& name ) {
	std::optional< std::filesystem::path > result;
	for ( const auto& entry : std::filesystem::directory_iterator( directory ) ) {
		if ( Lower( entry.path().filename().string() ) == Lower( name ) ) {
			if ( result ) { throw std::invalid_argument( "ambiguous installed asset" ); }
			result = entry.path();
		}
	}
	if ( !result ) { throw std::invalid_argument( "missing installed asset: " + name ); }
	return Read( *result );
}
int32_t Integer( const char* text ) {
	const std::string_view value( text ); int32_t result;
	const auto parsed = std::from_chars( value.data(), value.data() + value.size(), result );
	if ( parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() ) { throw std::invalid_argument( "invalid integer argument" ); }
	return result;
}
uint32_t Bits( const char* text ) {
	const std::string_view value( text ); uint32_t result;
	const auto parsed = std::from_chars( value.data(), value.data() + value.size(), result );
	if ( parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() ) { throw std::invalid_argument( "invalid bits argument" ); }
	return result;
}
void Append( std::string& bytes, const uint8_t* data, const size_t size ) { bytes.append( reinterpret_cast< const char* >( data ), size ); }
void Word( std::string& bytes, const uint32_t value, const unsigned count = 4 ) {
	for ( unsigned i = 0; i < count; ++i ) { bytes.push_back( static_cast< char >( value >> ( i * 8 ) ) ); }
}

}

int main( const int argc, const char* const* argv ) {
	const bool render = argc == 10 && std::string_view( argv[ 1 ] ) == "--render";
	const bool material = argc == 6 && std::string_view( argv[ 1 ] ) == "--material";
	const bool details = argc == 5 && ( std::string_view( argv[ 1 ] ) == "--details" || std::string_view( argv[ 1 ] ) == "--details-raw" );
	if ( !render && !material && !details ) { std::cerr << "usage: --render installed-directory output facing zoom numerator denominator anchor-x anchor-y | --material palette-pcx faction-pcx body-cvr output | --details[-raw] input output scale-bits\n"; return 2; }
	try {
		std::string bytes;
		if ( render ) {
			const auto renderer = loader::cvr::CvrSpartanRoverRenderer::Load( [ & ]( const std::string& name ) { return Installed( argv[ 2 ], name ); } );
			const auto result = renderer.Render( { Integer( argv[ 4 ] ), Integer( argv[ 5 ] ), Integer( argv[ 6 ] ), Integer( argv[ 7 ] ), { Integer( argv[ 8 ] ), Integer( argv[ 9 ] ) } } );
			Append( bytes, result.indexed.color.data(), result.indexed.color.size() );
			for ( const auto value : result.indexed.depth ) { bytes.push_back( static_cast< char >( value ) ); bytes.push_back( static_cast< char >( value >> 8 ) ); }
			Append( bytes, result.rgba.data(), result.rgba.size() );
			std::cout << result.draw_count << ' ' << result.processed_records << ' ' << result.written_samples << ' ' << result.target_anchor[ 0 ] << ' ' << result.target_anchor[ 1 ] << '\n';
		} else if ( details ) {
			const auto input = Read( argv[ 2 ] );
			std::vector< uint8_t > payload;
			if ( std::string_view( argv[ 1 ] ) == "--details-raw" ) { payload.assign( input.begin(), input.end() ); }
			else {
				const auto scene = loader::cvr::CvrSceneReader::Read( loader::cvr::CvrContainerReader::Read( input ) );
				if ( scene.objects.size() != 1 || !scene.objects[ 0 ].voxel_data ) { throw std::invalid_argument( "expected single detail object" ); }
				payload = *scene.objects[ 0 ].voxel_data;
			}
			const auto chain = loader::cvr::CvrSimpleDetailChain::Read( payload );
			for ( const auto& level : chain.Levels() ) {
				for ( const auto value : { level.flags, level.payload_size, level.scalar_bits, level.end_offset, level.end_minus_one } ) { Word( bytes, value ); }
				for ( const auto& vector : level.vectors ) { for ( const auto value : vector ) { Word( bytes, static_cast< uint16_t >( value ), 2 ); } }
				Word( bytes, level.record_count );
				for ( const auto& record : level.groups[ 0 ].records ) { Append( bytes, record.bytes.data(), record.bytes.size() ); }
				bytes.append( 5, '\0' );
			}
			std::cout << chain.Select( Bits( argv[ 4 ] ) ) << ' ' << chain.Levels().size() << '\n';
		} else {
			const auto body = loader::cvr::CvrPaletteReader::Read( loader::cvr::CvrContainerReader::Read( Read( argv[ 4 ] ) ) );
			const auto result = loader::cvr::CvrActorMaterial::Read( Read( argv[ 2 ] ), Read( argv[ 3 ] ), body.shades );
			for ( const auto& color : result.colors ) { Append( bytes, color.data(), color.size() ); }
			Append( bytes, result.shades.data(), result.shades.size() ); bytes.push_back( static_cast< char >( result.tint_index ) );
		}
		std::ofstream output( argv[ render || details ? 3 : 5 ], std::ios::binary | std::ios::trunc );
		output.write( bytes.data(), static_cast< std::streamsize >( bytes.size() ) ); output.close();
		if ( !output ) { throw std::runtime_error( "cannot write output" ); }
		std::cout << bytes.size() << '\n'; return 0;
	} catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}
