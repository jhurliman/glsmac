#include "CvrContainerReader.h"

#include <stdexcept>

namespace loader::cvr {
namespace {

uint32_t ReadLE32( const uint8_t* const bytes ) {
	return static_cast< uint32_t >( bytes[ 0 ] ) |
		( static_cast< uint32_t >( bytes[ 1 ] ) << 8 ) |
		( static_cast< uint32_t >( bytes[ 2 ] ) << 16 ) |
		( static_cast< uint32_t >( bytes[ 3 ] ) << 24 );
}

uint32_t ReadBE32( const uint8_t* const bytes ) {
	return ( static_cast< uint32_t >( bytes[ 0 ] ) << 24 ) |
		( static_cast< uint32_t >( bytes[ 1 ] ) << 16 ) |
		( static_cast< uint32_t >( bytes[ 2 ] ) << 8 ) |
		static_cast< uint32_t >( bytes[ 3 ] );
}

}

CvrContainer CvrContainerReader::Read( const std::string_view bytes ) {
	if ( bytes.size() < 8 || bytes.substr( 0, 4 ) != std::string_view( "CVR\0", 4 ) ) {
		throw std::invalid_argument( "CVR container: missing or invalid file header" );
	}
	const auto* const data = reinterpret_cast< const uint8_t* >( bytes.data() );
	if ( ReadLE32( data + 4 ) != bytes.size() ) {
		throw std::invalid_argument( "CVR container: declared file size does not match input" );
	}
	struct Payload { size_t offset, size; };
	std::array< Payload, 4 > payloads;
	size_t offset = 8;
	for ( size_t i = 0 ; i < payloads.size() ; ++i ) {
		const auto remaining = bytes.size() - offset;
		if ( remaining < 8 ) {
			throw std::invalid_argument( "CVR container: missing or truncated chunk header" );
		}
		if ( ReadBE32( data + offset ) != i + 1 ) {
			throw std::invalid_argument( "CVR container: expected chunk tags 1, 2, 3, 4 in order" );
		}
		const auto size = ReadLE32( data + offset + 4 );
		if ( size <= 8 || size > remaining ) {
			throw std::invalid_argument( "CVR container: empty, truncated or invalid chunk length" );
		}
		if ( i == 0 && size != 12 ) {
			throw std::invalid_argument( "CVR container: tag 1 requires a four-byte payload" );
		}
		payloads[ i ] = { offset + 8, size - 8 };
		offset += size;
	}
	if ( offset != bytes.size() ) {
		throw std::invalid_argument( "CVR container: trailing bytes or unsupported extra chunks" );
	}
	// Validate the complete envelope before allocating or copying any payload.
	CvrContainer result;
	for ( size_t i = 0 ; i < payloads.size() ; ++i ) {
		result.chunks[ i ].tag = static_cast< uint32_t >( i + 1 );
		const auto& payload = payloads[ i ];
		result.chunks[ i ].payload.assign( data + payload.offset, data + payload.offset + payload.size );
	}
	return result;
}

}
