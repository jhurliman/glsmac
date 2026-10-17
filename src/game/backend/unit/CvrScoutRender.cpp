#include "CvrScoutRender.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace game::backend::unit {
namespace {
constexpr int64_t MARKER = 0x43565253;
int32_t Integer( types::Buffer& buffer ) {
	const auto value = buffer.ReadInt();
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::invalid_argument( "Scout descriptor integer out of range" ); }
	return static_cast< int32_t >( value );
}
std::string Bits( const float value ) {
	uint32_t bits; std::memcpy( &bits, &value, sizeof( bits ) ); return std::to_string( bits );
}
}

CvrScoutRender::CvrScoutRender( const int32_t facing, const int32_t zoom, const int32_t numerator, const int32_t denominator,
	const types::Vec2< float >& image_size, const types::Vec2< float >& image_center )
	: Render( RT_CVR_SCOUT ), view{ facing, zoom, numerator, denominator, { 0, 0 } }, size( image_size ), center( image_center ) {
	if ( facing < -1 || facing > 7 || zoom < -8 || zoom > 8 || denominator <= 0 ) {
		throw std::invalid_argument( "Unsupported Scout idle view" );
	}
	// Explicit port admission bounds, independent of source camera arithmetic.
	for ( const float dimension : { size.x, size.y } ) {
		if ( !std::isfinite( dimension ) || dimension <= 0 || dimension > 16 || dimension / 2 == 0 ) {
			throw std::invalid_argument( "Invalid Scout world size" );
		}
	}
	for ( const float coordinate : { center.x, center.y } ) {
		if ( !std::isfinite( coordinate ) || coordinate < 0 || coordinate > 256 ) {
			throw std::invalid_argument( "Invalid Scout image center" );
		}
	}
	const auto offset = Offset();
	if ( !std::isfinite( offset.x ) || !std::isfinite( offset.y ) ||
		!std::isfinite( offset.x + size.x / 2 ) || !std::isfinite( offset.y + size.y / 2 ) ) {
		throw std::invalid_argument( "Invalid Scout derived placement" );
	}
}

types::Vec2< float > CvrScoutRender::Offset() const {
	// Rectangle is centered; Sprite::NormalizePosition adds these offsets.
	return { ( 0.5f - center.x / 256.0f ) * size.x, ( 0.5f - center.y / 256.0f ) * size.y };
}

std::string CvrScoutRender::PixelKey() const {
	return std::to_string( view.facing ) + ":" + std::to_string( view.zoom ) + ":" +
		std::to_string( view.scale_numerator ) + ":" + std::to_string( view.scale_denominator );
}

std::string CvrScoutRender::PlacementKey() const {
	return PixelKey() + ":" + Bits( size.x ) + ":" + Bits( size.y ) + ":" + Bits( center.x ) + ":" + Bits( center.y );
}

std::string CvrScoutRender::Encode() const {
	types::Buffer buffer;
	buffer.WriteInt( MARKER ); buffer.WriteInt( 1 ); buffer.WriteInt( 0 ); // Version1, fixed Scout composition0.
	buffer.WriteInt( view.facing ); buffer.WriteInt( view.zoom );
	buffer.WriteInt( view.scale_numerator ); buffer.WriteInt( view.scale_denominator );
	buffer.WriteFloat( size.x ); buffer.WriteFloat( size.y ); buffer.WriteFloat( center.x ); buffer.WriteFloat( center.y );
	return buffer.ToString();
}

CvrScoutRender CvrScoutRender::Decode( const std::string& bytes ) {
	try {
		if ( bytes.size() > MAX_WIRE_BYTES ) { throw std::invalid_argument( "Scout descriptor exceeds frame budget" ); }
		types::Buffer buffer( bytes );
		if ( buffer.ReadInt() != MARKER || buffer.ReadInt() != 1 || buffer.ReadInt() != 0 ) {
			throw std::invalid_argument( "Unsupported Scout descriptor profile" );
		}
		const auto facing = Integer( buffer ); const auto zoom = Integer( buffer );
		const auto numerator = Integer( buffer ); const auto denominator = Integer( buffer );
		const auto width = buffer.ReadFloat(); const auto height = buffer.ReadFloat();
		const auto x = buffer.ReadFloat(); const auto y = buffer.ReadFloat();
		if ( buffer.lenr != buffer.lenw ) { throw std::invalid_argument( "Trailing Scout descriptor data" ); }
		return { facing, zoom, numerator, denominator, { width, height }, { x, y } };
	}
	catch ( const std::invalid_argument& error ) {
		// Wire failures use the existing snapshot reader's runtime-error path.
		throw std::runtime_error( error.what() );
	}
}

const std::string CvrScoutRender::ToString( const std::string& prefix ) const {
	return prefix + "CvrScoutRender(" + PlacementKey() + ")";
}
}
