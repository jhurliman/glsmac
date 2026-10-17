#include "CvrGeometryReader.h"

#include <stdexcept>
#include <string>

namespace loader::cvr {
namespace {

[[noreturn]] void Invalid( const char* const message ) {
	throw std::invalid_argument( std::string( "CVR geometry: " ) + message );
}

uint32_t Word( const uint8_t* const p ) {
	return static_cast< uint32_t >( p[ 0 ] ) | ( static_cast< uint32_t >( p[ 1 ] ) << 8 ) |
		( static_cast< uint32_t >( p[ 2 ] ) << 16 ) | ( static_cast< uint32_t >( p[ 3 ] ) << 24 );
}

template< size_t N >
std::array< CvrGeometryVector, N > Vectors( const uint8_t* bytes ) {
	std::array< CvrGeometryVector, N > result;
	for ( auto& vector : result ) {
		for ( auto& component : vector ) {
			const int32_t word = static_cast< int32_t >( bytes[ 0 ] ) | ( static_cast< int32_t >( bytes[ 1 ] ) << 8 );
			component = static_cast< int16_t >( word < 0x8000 ? word : word - 0x10000 );
			bytes += 2;
		}
	}
	return result;
}

void Parse( const std::vector< uint8_t >& bytes, CvrGeometry* const output ) {
	// Fixed header and five-byte trailer must exist before any field access.
	if ( bytes.size() < 47 ) { Invalid( "missing or truncated header/trailer" ); }
	const auto* const data = bytes.data();
	const auto flags = Word( data );
	if ( flags != 0 && flags != 2 && flags != 8 && flags != 10 ) { Invalid( "unsupported geometry flags" ); }
	const auto size = Word( data + 4 );
	const auto scalar_bits = Word( data + 8 );
	const auto end_offset = Word( data + 12 );
	const auto end_minus_one = Word( data + 16 );
	const auto count = Word( data + 38 );
	if ( size != bytes.size() || end_offset != size || end_minus_one != size - 1 ) { Invalid( "unsupported payload size or end fields" ); }
	// Classify the original IEEE binary32 bits directly, including subnormals.
	// No floating-point conversion can flush or otherwise normalize the value.
	if ( scalar_bits == 0 || ( scalar_bits & 0x80000000u ) || ( scalar_bits & 0x7f800000u ) == 0x7f800000u ) {
		Invalid( "scalar must be finite and positive" );
	}
	if ( !count ) { Invalid( "geometry record count must be positive" ); }
	const size_t body_end = bytes.size() - 5;
	for ( size_t i = body_end ; i < bytes.size() ; ++i ) {
		if ( bytes[ i ] ) { Invalid( "unsupported nonzero trailer" ); }
	}
	if ( output ) {
		output->flags = flags;
		output->payload_size = size;
		output->scalar_bits = scalar_bits;
		output->end_offset = end_offset;
		output->end_minus_one = end_minus_one;
		output->vectors = Vectors< 3 >( data + 20 );
		output->record_count = count;
	}
	const bool grouped = flags & 2;
	const bool terminal = flags & 8;
	size_t cursor = 42;
	size_t total = 0;
	while ( cursor < body_end ) {
		uint32_t group_count = count;
		std::optional< CvrGeometryGroupHeader > header;
		if ( grouped ) {
			if ( body_end - cursor < 32 ) { Invalid( "truncated group header" ); }
			group_count = Word( data + cursor + 24 );
			const auto group_flags = Word( data + cursor + 28 );
			if ( group_flags != 0 ) { Invalid( "unsupported group flags" ); }
			if ( output ) { header = CvrGeometryGroupHeader{ Vectors< 4 >( data + cursor ), group_count, group_flags }; }
			cursor += 32;
		}
		if ( !group_count || group_count > ( body_end - cursor ) / 3 ) { Invalid( "group record count exceeds available records or is zero" ); }
		CvrGeometryGroup* group = nullptr;
		if ( output ) {
			output->groups.push_back( { header, {} } );
			group = &output->groups.back();
			group->records.resize( group_count );
		}
		for ( size_t i = 0 ; i < group_count ; ++i ) {
			const auto* const record = data + cursor + i * 3;
			const auto code = record[ 0 ] >> 3;
			if ( code > 26 || ( code == 26 ) != ( terminal && i == group_count - 1 ) ) { Invalid( "unsupported code or terminal placement" ); }
			if ( group ) { group->records[ i ].bytes = { record[ 0 ], record[ 1 ], record[ 2 ] }; }
		}
		cursor += static_cast< size_t >( group_count ) * 3;
		// The uint32-sized payload bounds this sum by its number of byte triples.
		total += group_count - ( grouped && terminal ? 1 : 0 );
		if ( !grouped ) { break; }
	}
	if ( cursor != body_end || total != count ) { Invalid( "record counts or final extent do not match" ); }
}

}

CvrGeometry CvrGeometryReader::Read( const std::vector< uint8_t >& bytes ) {
	Parse( bytes, nullptr );
	CvrGeometry result;
	Parse( bytes, &result );
	return result;
}

}
