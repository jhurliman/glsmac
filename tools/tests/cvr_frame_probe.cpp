// Opt-in production-reader/assembler caller. No runtime, rendering or assets bundled.
#include "loader/cvr/CvrContainerReader.h"
#include "loader/cvr/CvrSceneReader.h"
#include "loader/cvr/CvrFrameAssembler.h"

#include <charconv>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace {

template< typename T > T Number( const std::string& text, const int base ) {
	T value;
	const auto parsed = std::from_chars( text.data(), text.data() + text.size(), value, base );
	if ( parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() ) { throw std::invalid_argument( "invalid numeric argument: " + text ); }
	return value;
}

float Component( const std::string& text ) {
	if ( text.size() != 8 ) { throw std::invalid_argument( "offset bits require eight hex digits without 0x" ); }
	const auto bits = Number< uint32_t >( text, 16 );
	float value;
	std::memcpy( &value, &bits, sizeof( value ) );
	return value;
}

template< size_t N > void WriteBits( const std::array< float, N >& values ) {
	std::cout << '[';
	for ( size_t i = 0 ; i < N ; ++i ) {
		uint32_t bits;
		std::memcpy( &bits, &values[ i ], sizeof( bits ) );
		if ( i ) { std::cout << ','; }
		std::cout << bits;
	}
	std::cout << ']';
}

}

int main( const int argc, const char* const* argv ) {
	if ( argc < 6 || ( argc - 1 ) % 5 ) {
		std::cerr << "usage: cvr_frame_probe <file> <nonnegative frame> <offset0 hex bits> <offset1 hex bits> <offset2 hex bits> [more groups...]\n";
		return 2;
	}
	try {
		std::vector< loader::cvr::CvrScene > scenes;
		std::vector< loader::cvr::CvrFrameSelection > selections;
		for ( int i = 1 ; i < argc ; i += 5 ) {
			std::ifstream stream( argv[ i ], std::ios::binary );
			if ( !stream ) { throw std::runtime_error( "could not open CVR file" ); }
			const std::string bytes{ std::istreambuf_iterator< char >( stream ), std::istreambuf_iterator< char >() };
			if ( stream.bad() ) { throw std::runtime_error( "could not read complete CVR file" ); }
			selections.push_back( { scenes.size(), Number< int32_t >( argv[ i + 1 ], 10 ),
				{ Component( argv[ i + 2 ] ), Component( argv[ i + 3 ] ), Component( argv[ i + 4 ] ) } } );
			scenes.push_back( loader::cvr::CvrSceneReader::Read( loader::cvr::CvrContainerReader::Read( bytes ) ) );
		}
		const auto result = loader::cvr::CvrFrameAssembler::Assemble( loader::cvr::CvrFrameSource::Create( std::move( scenes ) ), selections );
		std::cout << "{\"count\":" << result.records.size() << ",\"first_omitted\":";
		if ( result.first_omitted ) {
			std::cout << "{\"selection\":" << result.first_omitted->selection_index << ",\"object\":" << result.first_omitted->object_index << '}';
		}
		else { std::cout << "null"; }
		std::cout << ",\"records\":[";
		bool first = true;
		for ( const auto& record : result.records ) {
			if ( !first ) { std::cout << ','; }
			first = false;
			const auto& object = result.source->Scenes()[ record.scene_index ].objects[ record.object_index ];
			std::cout << "{\"selection\":" << record.selection_index << ",\"scene\":" << record.scene_index << ",\"object\":" << record.object_index <<
				",\"frame\":" << record.frame_index << ",\"flag\":" << static_cast< unsigned >( record.value.flags ) <<
				",\"geometry_bytes\":" << ( object.voxel_data ? object.voxel_data->size() : 0 ) << ",\"location_bits\":";
			WriteBits( record.value.location );
			std::cout << ",\"matrix_bits\":"; WriteBits( record.value.matrix ); std::cout << '}';
		}
		std::cout << "]}\n";
		return result.first_omitted ? 3 : 0; // A partial prefix is explicitly distinguishable from success.
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}
