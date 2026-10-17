#include "SmacMovementHistory.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace game::backend {
namespace {
size_t TileCount( int32_t width, int32_t height ) {
	if ( width < 4 || width > 512 || height < 4 || height > 256 || ( width & 1 ) || ( height & 1 ) ) {
		throw std::runtime_error( "Movement history requires bounded even canonical map dimensions" );
	}
	return static_cast< size_t >( width ) * static_cast< size_t >( height ) / 2;
}
void Distance( std::optional< int32_t > distance ) {
	if ( distance && ( *distance < 0 || *distance > 9999 ) ) {
		throw std::runtime_error( "Movement history distance is outside the supported query result domain" );
	}
}
uint32_t ReadWord( const std::string& wire, size_t offset ) {
	uint32_t value = 0;
	for ( unsigned i = 0; i < 4; ++i ) { value |= uint32_t( static_cast< unsigned char >( wire[ offset + i ] ) ) << ( i * 8 ); }
	return value;
}
void WriteWord( std::string& wire, size_t offset, uint32_t value ) {
	for ( unsigned i = 0; i < 4; ++i ) { wire[ offset + i ] = static_cast< char >( ( value >> ( i * 8 ) ) & 255 ); }
}
}

SmacMovementHistory::SmacMovementHistory( int32_t width, int32_t height )
	: m_width( width ), m_height( height ), m_placed( ( TileCount( width, height ) + 7 ) / 8, 0 ) {}

SmacMovementHistory SmacMovementHistory::NewGeneratedWorld( int32_t width, int32_t height ) { return { width, height }; }

void SmacMovementHistory::ValidateDimensions( size_t width, size_t height ) const {
	if ( width != static_cast< size_t >( m_width ) || height != static_cast< size_t >( m_height ) ) {
		throw std::runtime_error( "Movement history belongs to different map dimensions" );
	}
}
size_t SmacMovementHistory::PlacementIndex( int32_t x, int32_t y ) const {
	if ( x < 0 || x >= m_width || y < 0 || y >= m_height || ( x & 1 ) != ( y & 1 ) ) {
		throw std::runtime_error( "Movement placement history requires a canonical coordinate" );
	}
	return static_cast< size_t >( y ) * static_cast< size_t >( m_width / 2 ) + static_cast< size_t >( x / 2 );
}
bool SmacMovementHistory::WasPlaced( int32_t x, int32_t y ) const {
	const auto index = PlacementIndex( x, y );
	return ( m_placed[ index / 8 ] & ( uint8_t( 1 ) << ( index % 8 ) ) ) != 0;
}
void SmacMovementHistory::MarkPlaced( size_t index ) noexcept { m_placed[ index / 8 ] |= uint8_t( 1 ) << ( index % 8 ); }
const std::optional< int32_t >& SmacMovementHistory::NearestDistance() const noexcept { return m_nearest_distance; }
void SmacMovementHistory::RetainNearest( std::optional< int32_t > distance ) { Distance( distance ); m_nearest_distance = distance; }

std::string SmacMovementHistory::Serialize() const {
	std::string wire( "SMCH" ); wire.resize( 28 + m_placed.size() );
	WriteWord( wire, 4, 1 ); WriteWord( wire, 8, 1 ); // version1, GeneratedCanonicalPlacementV1
	WriteWord( wire, 12, m_width ); WriteWord( wire, 16, m_height );
	WriteWord( wire, 20, m_nearest_distance.has_value() ? 1 : 0 ); WriteWord( wire, 24, m_nearest_distance.value_or( 0 ) );
	for ( size_t i = 0; i < m_placed.size(); ++i ) { wire[ 28 + i ] = static_cast< char >( m_placed[ i ] ); }
	return wire;
}

SmacMovementHistory SmacMovementHistory::Deserialize( const std::string& wire ) {
	if ( wire.size() < 28 || wire.size() > MAX_WIRE_BYTES || wire.compare( 0, 4, "SMCH" ) != 0
		|| ReadWord( wire, 4 ) != 1 || ReadWord( wire, 8 ) != 1 ) { throw std::runtime_error( "Unsupported movement history record" ); }
	const auto width = ReadWord( wire, 12 ), height = ReadWord( wire, 16 );
	if ( width > 512 || height > 256 ) { throw std::runtime_error( "Movement history dimensions exceed budget" ); }
	const auto count = TileCount( static_cast< int32_t >( width ), static_cast< int32_t >( height ) );
	const auto present = ReadWord( wire, 20 ), distance = ReadWord( wire, 24 );
	if ( wire.size() != 28 + ( count + 7 ) / 8 || present > 1 || distance > 9999 || ( !present && distance != 0 ) ) {
		throw std::runtime_error( "Invalid movement history framing or nearest distance" );
	}
	if ( count % 8 && ( static_cast< unsigned char >( wire.back() ) >> ( count % 8 ) ) != 0 ) {
		throw std::runtime_error( "Noncanonical movement history padding bits" );
	}
	// All framing, dimensions and scalar fields checked before bitmap allocation.
	SmacMovementHistory result( static_cast< int32_t >( width ), static_cast< int32_t >( height ) );
	for ( size_t i = 0; i < result.m_placed.size(); ++i ) { result.m_placed[ i ] = static_cast< uint8_t >( wire[ 28 + i ] ); }
	if ( present ) { result.m_nearest_distance = static_cast< int32_t >( distance ); }
	return result;
}

bool SmacMovementHistory::operator==( const SmacMovementHistory& other ) const noexcept {
	return m_width == other.m_width && m_height == other.m_height && m_placed == other.m_placed && m_nearest_distance == other.m_nearest_distance;
}

SmacMovementHistory::NearestResult SmacMovementHistory::NearestAnyBase( int32_t x, int32_t y, int32_t width, bool wrap,
	const std::vector< BasePosition >& bases, std::optional< int32_t > prior ) {
	Distance( prior );
	if ( width <= 0 || x < 0 || y < 0 ) { throw std::runtime_error( "Invalid nearest-base query geometry" ); }
	NearestResult result{ {}, prior }; int64_t best = 9999;
	std::optional< uint64_t > last;
	for ( const auto& base : bases ) {
		if ( base.x < 0 || base.y < 0 || ( last && base.id <= *last ) ) { throw std::runtime_error( "Invalid canonical nearest-base records" ); }
		last = base.id;
		int64_t dx = std::abs( int64_t( x ) - base.x ), dy = std::abs( int64_t( y ) - base.y );
		if ( wrap && dx > width / 2 ) { dx = std::abs( int64_t( width ) - dx ); }
		const auto distance = std::max( dx, dy ) - ( ( ( dx + dy ) / 2 - std::min( dx, dy ) + 1 ) / 2 );
		if ( distance <= best ) { best = distance; result = { base.id, static_cast< int32_t >( distance ) }; }
	}
	return result;
}

}
