#include "CvrPaletteReader.h"

#include <cstring>
#include <limits>
#include <stdexcept>

namespace loader::cvr {
namespace {

[[noreturn]] void Invalid( const char* reason ) {
	throw std::invalid_argument( std::string( "CVR palette: " ) + reason );
}

uint32_t Little( const uint8_t* bytes ) {
	return uint32_t( bytes[ 0 ] ) | ( uint32_t( bytes[ 1 ] ) << 8 ) |
		( uint32_t( bytes[ 2 ] ) << 16 ) | ( uint32_t( bytes[ 3 ] ) << 24 );
}

uint32_t Big( const uint8_t* bytes ) {
	return ( uint32_t( bytes[ 0 ] ) << 24 ) | ( uint32_t( bytes[ 1 ] ) << 16 ) |
		( uint32_t( bytes[ 2 ] ) << 8 ) | uint32_t( bytes[ 3 ] );
}

struct Field {
	size_t offset = 0;
	size_t size = 0;
	bool present = false;
};

}

CvrPalette CvrPaletteReader::Read( const CvrContainer& container ) {
	static_assert( sizeof( float ) == sizeof( uint32_t ) && std::numeric_limits< float >::is_iec559,
		"CVR version requires IEEE binary32" );
	uint64_t total = 8;
	for ( size_t i = 0; i < container.chunks.size(); ++i ) {
		const auto& chunk = container.chunks[ i ];
		if ( chunk.tag != i + 1 || chunk.payload.empty() || chunk.payload.size() > UINT32_MAX - 8U ) {
			Invalid( "invalid container envelope" );
		}
		total += 8 + chunk.payload.size();
		if ( total > UINT32_MAX ) { Invalid( "container size exceeds wire width" ); }
	}
	if ( container.chunks[ 0 ].payload.size() != 4 ) { Invalid( "version must have four bytes" ); }
	const auto bits = Little( container.chunks[ 0 ].payload.data() );
	if ( ( bits & 0x7f800000U ) == 0x7f800000U ) { Invalid( "nonfinite version" ); }
	float version;
	std::memcpy( &version, &bits, sizeof( version ) );
	const bool modern = version > 7.0f;
	const auto& bytes = container.chunks[ 2 ].payload;
	Field name, palette, auxiliary;
	size_t position = 0;
	while ( position < bytes.size() ) {
		if ( bytes.size() - position < 8 ) { Invalid( "truncated nested header" ); }
		const auto tag = Big( bytes.data() + position );
		const auto size = Little( bytes.data() + position + 4 );
		if ( size < 8 || size > bytes.size() - position ) { Invalid( "invalid nested size" ); }
		Field* field = tag == 0x101 ? &name : tag == 0x201 ? &palette : tag == 0x301 ? &auxiliary : nullptr;
		if ( field ) {
			if ( field->present ) { Invalid( "duplicate known field" ); }
			*field = { position + 8, size - 8, true };
		}
		position += size;
	}
	if ( !name.present || !name.size || !palette.present || palette.size != ( modern ? 6914U : 4866U ) ) {
		Invalid( "missing name/palette or wrong palette size" );
	}
	// Complete framing and fixed-size validation precede variable-sized copies.
	CvrPalette result;
	result.name.assign( reinterpret_cast< const char* >( bytes.data() + name.offset ), name.size );
	const auto* source = bytes.data() + palette.offset;
	result.first = source[ 0 ]; result.end = source[ 1 ];
	for ( size_t i = 0; i < result.colors.size(); ++i ) {
		for ( size_t component = 0; component < 3; ++component ) { result.colors[ i ][ component ] = source[ 2 + 3 * i + component ]; }
	}
	for ( size_t row = 0; row < 24; ++row ) {
		const auto stored_row = modern ? row : 2 * row / 3;
		for ( size_t i = 0; i < 256; ++i ) { result.shades[ row * 256 + i ] = source[ 770 + stored_row * 256 + i ]; }
	}
	if ( auxiliary.present ) {
		result.auxiliary.assign( bytes.begin() + auxiliary.offset, bytes.begin() + auxiliary.offset + auxiliary.size );
	}
	return result;
}

}
