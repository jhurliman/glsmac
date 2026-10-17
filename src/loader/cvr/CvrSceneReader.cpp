#include "CvrSceneReader.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace loader::cvr {
namespace {

static_assert( sizeof( float ) == sizeof( uint32_t ) && std::numeric_limits< float >::is_iec559,
	"CVR scene transforms require IEEE binary32 float" );

[[noreturn]] void Invalid( const char* const message ) {
	throw std::invalid_argument( std::string( "CVR scene: " ) + message );
}

uint32_t ReadLE32( const char* const bytes ) {
	const auto* const p = reinterpret_cast< const uint8_t* >( bytes );
	return static_cast< uint32_t >( p[ 0 ] ) | ( static_cast< uint32_t >( p[ 1 ] ) << 8 ) |
		( static_cast< uint32_t >( p[ 2 ] ) << 16 ) | ( static_cast< uint32_t >( p[ 3 ] ) << 24 );
}

uint32_t ReadBE32( const char* const bytes ) {
	const auto* const p = reinterpret_cast< const uint8_t* >( bytes );
	return ( static_cast< uint32_t >( p[ 0 ] ) << 24 ) | ( static_cast< uint32_t >( p[ 1 ] ) << 16 ) |
		( static_cast< uint32_t >( p[ 2 ] ) << 8 ) | static_cast< uint32_t >( p[ 3 ] );
}

float ReadFloat( const char* const bytes ) {
	const auto bits = ReadLE32( bytes );
	float value;
	std::memcpy( &value, &bits, sizeof( value ) );
	if ( !std::isfinite( value ) ) { Invalid( "nonfinite transform component" ); }
	return value;
}

struct ChunkView {
	uint32_t tag;
	std::string_view payload;
};

class Cursor {
public:
	explicit Cursor( const std::string_view bytes ) : m_bytes( bytes ) {}

	ChunkView Next( const bool allow_empty = false ) {
		if ( m_bytes.size() < 8 ) { Invalid( "missing or truncated nested chunk header" ); }
		const auto tag = ReadBE32( m_bytes.data() );
		const auto size = ReadLE32( m_bytes.data() + 4 );
		if ( size < 8 || ( size == 8 && !allow_empty ) || size > m_bytes.size() ) { Invalid( "empty, truncated or invalid nested chunk length" ); }
		const auto payload = m_bytes.substr( 8, size - 8 );
		m_bytes.remove_prefix( size );
		return { tag, payload };
	}

	std::string_view Read( const uint32_t expected_tag, const bool allow_empty = false ) {
		const auto chunk = Next( allow_empty );
		if ( chunk.tag != expected_tag ) { Invalid( "unexpected nested chunk tag or order" ); }
		return chunk.payload;
	}

	void RequireEnd() const {
		if ( !m_bytes.empty() ) { Invalid( "extra nested chunks or trailing bytes" ); }
	}

	size_t Remaining() const { return m_bytes.size(); }

private:
	std::string_view m_bytes;
};

uint32_t Count( const std::string_view payload ) {
	if ( payload.size() != 4 ) { Invalid( "count must contain exactly four bytes" ); }
	const auto value = ReadLE32( payload.data() );
	if ( !value ) { Invalid( "object and frame counts must be positive" ); }
	return value;
}

void FrameSize( const std::string_view payload, const uint32_t count, const size_t stride ) {
	// Divide the available bytes, rather than multiplying an untrusted count.
	if ( payload.size() % stride || payload.size() / stride != count ) {
		Invalid( "frame array length does not match the scene frame count" );
	}
}

void ReadObject( const std::string_view bytes, const uint32_t frame_count, CvrSceneObject* const output ) {
	Cursor object( bytes );
	const auto name = object.Read( 0x00010404, true ); // Empty object names occur in installed ACP03.cvr.
	std::optional< std::string_view > voxel_data;
	auto next = object.Next();
	if ( next.tag == 0x00020404 ) {
		Cursor geometry( next.payload );
		voxel_data = geometry.Read( 0x01020404 );
		geometry.RequireEnd();
		next = object.Next();
	}
	if ( next.tag != 0x00030404 ) { Invalid( "expected object animation container" ); }
	object.RequireEnd();
	Cursor animation( next.payload );
	const auto flags = animation.Read( 0x01030404 );
	const auto locations = animation.Read( 0x02030404 );
	const auto matrices = animation.Read( 0x03030404 );
	animation.RequireEnd();
	FrameSize( flags, frame_count, 1 );
	FrameSize( locations, frame_count, 12 );
	FrameSize( matrices, frame_count, 36 );
	if ( output ) {
		output->name = std::string( name );
		if ( voxel_data ) { output->voxel_data.emplace( voxel_data->begin(), voxel_data->end() ); }
		output->frames.resize( frame_count );
	}
	for ( size_t frame = 0 ; frame < frame_count ; ++frame ) {
		if ( output ) { output->frames[ frame ].flags = static_cast< uint8_t >( flags[ frame ] ); }
		for ( size_t i = 0 ; i < 3 ; ++i ) {
			const auto value = ReadFloat( locations.data() + frame * 12 + i * 4 );
			if ( output ) { output->frames[ frame ].location[ i ] = value; }
		}
		for ( size_t i = 0 ; i < 9 ; ++i ) {
			const auto value = ReadFloat( matrices.data() + frame * 36 + i * 4 );
			if ( output ) { output->frames[ frame ].matrix[ i ] = value; }
		}
	}
}

void ReadScene( const std::string_view bytes, CvrScene* const output ) {
	Cursor scene( bytes );
	const auto name = scene.Read( 0x00000104 );
	const auto object_count = Count( scene.Read( 0x00000204 ) );
	const auto frame_count = Count( scene.Read( 0x00000304 ) );
	if ( object_count > scene.Remaining() / 8 ) { Invalid( "object count exceeds remaining chunk headers" ); }
	if ( output ) {
		output->name = std::string( name );
		output->frame_count = frame_count;
		output->objects.resize( object_count );
	}
	for ( size_t i = 0 ; i < object_count ; ++i ) {
		ReadObject( scene.Read( 0x00000404 ), frame_count, output ? &output->objects[ i ] : nullptr );
	}
	scene.RequireEnd();
}

}

CvrScene CvrSceneReader::Read( const CvrContainer& container ) {
	size_t budget = std::numeric_limits< uint32_t >::max() - 8;
	for ( size_t i = 0 ; i < container.chunks.size() ; ++i ) {
		const auto& chunk = container.chunks[ i ];
		if ( chunk.tag != i + 1 || chunk.payload.empty() || ( i == 0 && chunk.payload.size() != 4 ) ) {
			Invalid( "container does not match the supported four-chunk profile" );
		}
		if ( budget < 8 || chunk.payload.size() > budget - 8 ) { Invalid( "container exceeds the uint32 file-size budget" ); }
		budget -= 8 + chunk.payload.size();
	}
	const auto& payload = container.chunks[ 3 ].payload;
	const std::string_view bytes( reinterpret_cast< const char* >( payload.data() ), payload.size() );
	ReadScene( bytes, nullptr ); // Complete bounds, schema, counts and finite-value validation before allocations.
	CvrScene result;
	ReadScene( bytes, &result );
	return result;
}

}
