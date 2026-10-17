// Opt-in explicit-input raster caller. Private fixture bytes are never bundled.
#include "loader/cvr/CvrIndexedRasterizer.h"
#include "loader/cvr/CvrContainerReader.h"
#include "loader/cvr/CvrSceneReader.h"

#include <charconv>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
using namespace loader::cvr;

std::string ReadFile( const char* path ) {
	std::ifstream file( path, std::ios::binary | std::ios::ate );
	const auto size = file.tellg();
	if ( !file || size < 0 || size > 64 * 1024 * 1024 ) { throw std::invalid_argument( "missing or excessive fixture file" ); }
	std::string bytes( static_cast< size_t >( size ), '\0' );
	file.seekg( 0 ); file.read( bytes.data(), size );
	if ( !file ) { throw std::invalid_argument( "short fixture read" ); }
	return bytes;
}

class Input {
public:
	explicit Input( std::string bytes ) : m_bytes( std::move( bytes ) ) {}
	uint32_t Word() {
		const auto value = Take( 4 );
		uint32_t result = 0;
		for ( size_t i = 0 ; i < 4 ; ++i ) { result |= uint32_t( static_cast< uint8_t >( value[ i ] ) ) << ( i * 8 ); }
		return result;
	}
	int32_t Signed() {
		const auto value = Word();
		return static_cast< int32_t >( value <= 0x7fffffffu ? int64_t( value ) : int64_t( value ) - 0x100000000ll );
	}
	std::string_view Take( const size_t length ) {
		if ( length > m_bytes.size() - m_offset ) { throw std::invalid_argument( "truncated fixture" ); }
		const auto value = std::string_view( m_bytes ).substr( m_offset, length ); m_offset += length; return value;
	}
	void End() { if ( m_offset != m_bytes.size() ) { throw std::invalid_argument( "trailing fixture bytes" ); } }
private:
	std::string m_bytes;
	size_t m_offset = 0;
};

void WriteWord( std::ostream& output, const uint64_t word, const size_t length ) {
	for ( size_t i = 0 ; i < length ; ++i ) { output.put( static_cast< char >( ( word >> ( 8 * i ) ) & 255 ) ); }
}

}

int main( const int argc, const char* const* argv ) {
	if ( argc != 3 && argc != 5 ) {
		std::cerr << "usage: cvr_indexed_probe REQUEST OUTPUT [CVR OBJECT_INDEX]\n"; return 2;
	}
	try {
		Input input( ReadFile( argv[ 1 ] ) );
		if ( input.Take( 8 ) != std::string_view( "CVRIDX1\0", 8 ) ) { throw std::invalid_argument( "unknown probe fixture" ); }
		CvrIndexedSurface surface{ input.Word(), input.Word(), input.Word(), {}, {} };
		const auto cells = uint64_t( surface.pitch ) * surface.height;
		if ( cells > CvrIndexedRasterizer::MAX_SURFACE_SAMPLES ) { throw std::invalid_argument( "excessive surface" ); }
		CvrIndexedLookups lookups{};
		const auto table_size = input.Word();
		if ( table_size > 65536 ) { throw std::invalid_argument( "excessive table" ); }
		const auto first = input.Take( lookups.first.size() );
		for ( size_t i = 0 ; i < first.size() ; ++i ) { lookups.first[ i ] = static_cast< uint8_t >( first[ i ] ); }
		const auto second = input.Take( table_size ); lookups.second.assign( second.begin(), second.end() );
		const auto colors = input.Take( cells ); surface.color.assign( colors.begin(), colors.end() );
		const auto depths = input.Take( cells * 2 ); surface.depth.reserve( cells );
		for ( size_t i = 0 ; i < cells ; ++i ) {
			surface.depth.push_back( uint16_t( static_cast< uint8_t >( depths[ i * 2 ] ) ) | ( uint16_t( static_cast< uint8_t >( depths[ i * 2 + 1 ] ) ) << 8 ) );
		}
		const auto count = input.Word();
		if ( count > CvrIndexedRasterizer::MAX_DRAWS ) { throw std::invalid_argument( "excessive draw count" ); }
		std::vector< std::vector< CvrGeometryRecord > > records( count );
		std::vector< CvrIndexedDraw > draws;
		size_t total = 0;
		for ( size_t i = 0 ; i < count ; ++i ) {
			const auto terminal = input.Word(), footprint = input.Word();
			if ( terminal > 1 || ( footprint != 1 && footprint != 2 ) ) { throw std::invalid_argument( "unsupported draw mode" ); }
			CvrIndexedDraw draw{ &records[ i ], terminal != 0, footprint == 1 ? CvrSampleFootprint::Single : CvrSampleFootprint::Square2, {}, {} };
			for ( auto& row : draw.basis ) { for ( auto& value : row ) { value = input.Signed(); } }
			for ( auto& value : draw.initial ) { value = input.Signed(); }
			const auto length = input.Word();
			if ( length > CvrIndexedRasterizer::MAX_RECORDS - total ) { throw std::invalid_argument( "excessive records" ); }
			total += length;
			const auto bytes = input.Take( size_t( length ) * 3 ); records[ i ].resize( length );
			for ( size_t r = 0 ; r < length ; ++r ) {
				for ( size_t c = 0 ; c < 3 ; ++c ) { records[ i ][ r ].bytes[ c ] = static_cast< uint8_t >( bytes[ r * 3 + c ] ); }
			}
			draws.push_back( draw );
		}
		input.End();
		if ( argc == 5 ) {
			if ( draws.size() != 1 ) { throw std::invalid_argument( "asset mode requires one draw" ); }
			size_t index = 0; const std::string text( argv[ 4 ] );
			const auto parsed = std::from_chars( text.data(), text.data() + text.size(), index );
			if ( parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() ) { throw std::invalid_argument( "invalid object index" ); }
			const auto scene = CvrSceneReader::Read( CvrContainerReader::Read( ReadFile( argv[ 3 ] ) ) );
			if ( index >= scene.objects.size() || !scene.objects[ index ].voxel_data ) { throw std::invalid_argument( "missing object geometry" ); }
			auto geometry = CvrGeometryReader::Read( *scene.objects[ index ].voxel_data );
			if ( ( geometry.flags != 0 && geometry.flags != 8 ) || geometry.groups.size() != 1 || draws[ 0 ].terminal != bool( geometry.flags & 8 ) ||
				geometry.groups[ 0 ].records.size() != records[ 0 ].size() ) { throw std::invalid_argument( "asset profile differs from fixture" ); }
			for ( size_t i = 0 ; i < records[ 0 ].size() ; ++i ) {
				if ( records[ 0 ][ i ].bytes != geometry.groups[ 0 ].records[ i ].bytes ) { throw std::invalid_argument( "asset records differ from fixture" ); }
			}
			records[ 0 ] = std::move( geometry.groups[ 0 ].records ); // Actual production decoded input.
		}
		const auto result = CvrIndexedRasterizer::Draw( surface, lookups, draws );
		std::ofstream output( argv[ 2 ], std::ios::binary );
		output.write( "CVROUT1\0", 8 );
		WriteWord( output, result.surface.width, 4 ); WriteWord( output, result.surface.height, 4 ); WriteWord( output, result.surface.pitch, 4 );
		WriteWord( output, result.processed_records, 8 ); WriteWord( output, result.written_samples, 8 );
		output.write( reinterpret_cast< const char* >( result.surface.color.data() ), result.surface.color.size() );
		for ( const auto value : result.surface.depth ) { WriteWord( output, value, 2 ); }
		output.close(); if ( !output ) { throw std::runtime_error( "output write failed" ); }
		std::cout << "{\"records\":" << result.processed_records << ",\"writes\":" << result.written_samples << "}\n";
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}
