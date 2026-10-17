#include "CvrGeometryReader.h"

#include <stdexcept>
#include <string>
#include <cmath>

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

void Parse( const uint8_t* const data, const size_t local_size, const size_t remaining_size,
	const bool linked, CvrGeometry* const output ) {
	// Fixed header and five-byte trailer must exist before any field access.
	if ( local_size < 47 ) { Invalid( "missing or truncated header/trailer" ); }
	const auto flags = Word( data );
	if ( linked ? flags > 1 : flags != 0 && flags != 2 && flags != 8 && flags != 10 ) { Invalid( "unsupported geometry flags" ); }
	const auto size = Word( data + 4 );
	const auto scalar_bits = Word( data + 8 );
	const auto end_offset = Word( data + 12 );
	const auto end_minus_one = Word( data + 16 );
	const auto count = Word( data + 38 );
	if ( size != remaining_size || end_offset != local_size || end_minus_one != local_size - 1 ) { Invalid( "unsupported payload size or end fields" ); }
	// Classify the original IEEE binary32 bits directly, including subnormals.
	// No floating-point conversion can flush or otherwise normalize the value.
	if ( scalar_bits == 0 || ( scalar_bits & 0x80000000u ) || ( scalar_bits & 0x7f800000u ) == 0x7f800000u ) {
		Invalid( "scalar must be finite and positive" );
	}
	if ( !count ) { Invalid( "geometry record count must be positive" ); }
	const size_t body_end = local_size - 5;
	for ( size_t i = body_end ; i < local_size ; ++i ) {
		if ( data[ i ] ) { Invalid( "unsupported nonzero trailer" ); }
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

void ParseChain( const std::vector< uint8_t >& bytes, std::vector< CvrGeometry >* output ) {
	size_t cursor = 0, levels = 0, records = 0;
	for ( ;; ) {
		const auto remaining = bytes.size() - cursor;
		if ( remaining < 47 || ++levels > CvrSimpleDetailChain::MAX_LEVELS ) { Invalid( "truncated or excessive detail levels" ); }
		const auto* data = bytes.data() + cursor;
		const auto local = Word( data + 12 );
		if ( local < 47 || local > remaining ) { Invalid( "invalid detail link" ); }
		// A terminal level consumes the exact rest; continuation must leave a
		// complete next header. Every link therefore makes forward progress.
		if ( Word( data ) == 0 ? local != remaining : remaining - local < 47 ) { Invalid( "detail continuation disagrees with remaining extent" ); }
		const auto count = Word( data + 38 );
		if ( count > CvrSimpleDetailChain::MAX_RECORDS - records ) { Invalid( "detail record budget exceeded" ); }
		records += count;
		CvrGeometry* level = nullptr;
		if ( output ) { output->emplace_back(); level = &output->back(); }
		Parse( data, local, remaining, true, level );
		cursor += local;
		if ( cursor == bytes.size() ) { return; }
	}
}

double Binary32( const uint32_t bits ) {
	// Decode with integer fields so ambient binary32 denormal modes cannot
	// erase a supplied subnormal. Every result and finite product fits double.
	const auto exponent = ( bits >> 23 ) & 255;
	const auto fraction = bits & 0x7fffff;
	const double magnitude = std::ldexp( static_cast< double >( exponent ? fraction | 0x800000 : fraction ), exponent ? static_cast< int >( exponent ) - 150 : -149 );
	return bits & 0x80000000 ? -magnitude : magnitude;
}

}

CvrGeometry CvrGeometryReader::Read( const std::vector< uint8_t >& bytes ) {
	Parse( bytes.data(), bytes.size(), bytes.size(), false, nullptr );
	CvrGeometry result;
	Parse( bytes.data(), bytes.size(), bytes.size(), false, &result );
	return result;
}

CvrSimpleDetailChain CvrSimpleDetailChain::Read( const std::vector< uint8_t >& bytes ) {
	ParseChain( bytes, nullptr );
	CvrSimpleDetailChain result;
	ParseChain( bytes, &result.m_levels );
	return result;
}

size_t CvrSimpleDetailChain::Select( const uint32_t scale_bits ) const {
	if ( ( scale_bits & 0x7f800000 ) == 0x7f800000 || m_levels.empty() ) { Invalid( "invalid detail scale or empty chain" ); }
	const double scale = Binary32( scale_bits ), threshold = Binary32( 1068827777 );
	size_t current = 0;
	while ( current + 1 < m_levels.size() ) {
		if ( Binary32( m_levels[ current ].scalar_bits ) * scale >= threshold ||
			Binary32( m_levels[ current + 1 ].scalar_bits ) * scale > threshold ) { break; }
		++current;
	}
	return current;
}

}
