#include "CvrActorMaterial.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace loader::cvr {
namespace {

struct Pcx {
	size_t width, height;
	std::vector< uint8_t > pixels;
	std::array< CvrPalette::Color, 256 > palette;
};

Pcx ReadPcx( const std::string_view bytes ) {
	constexpr size_t HEADER = 128, PALETTE = 769, MAX_BYTES = 16 * 1024 * 1024;
	if ( bytes.size() < HEADER + PALETTE || bytes.size() > MAX_BYTES ) { throw std::invalid_argument( "CVR actor PCX: invalid file size" ); }
	const auto byte = [ & ]( const size_t i ) { return static_cast< uint8_t >( bytes[ i ] ); };
	const auto word = [ & ]( const size_t i ) { return uint32_t( byte( i ) ) | ( uint32_t( byte( i + 1 ) ) << 8 ); };
	if ( byte( 0 ) != 10 || byte( 1 ) != 5 || byte( 2 ) != 1 || byte( 3 ) != 8 || byte( 65 ) != 1 || word( 4 ) != 0 || word( 6 ) != 0 ) {
		throw std::invalid_argument( "CVR actor PCX: unsupported format" );
	}
	const size_t width = word( 8 ) + 1, height = word( 10 ) + 1, stride = word( 66 );
	if ( width > 2048 || height > 2048 || stride < width || stride > 2048 || ( stride & 1 ) != 0 || byte( bytes.size() - PALETTE ) != 12 ) {
		throw std::invalid_argument( "CVR actor PCX: unsupported dimensions or palette" );
	}
	const size_t end = bytes.size() - PALETTE;
	// Validate every RLE row and exact framing before allocating decoded pixels.
	const auto decode = [ & ]( std::vector< uint8_t >* output ) {
		size_t cursor = HEADER;
		for ( size_t y = 0; y < height; ++y ) {
			size_t x = 0;
			while ( x < stride ) {
				if ( cursor == end ) { throw std::invalid_argument( "CVR actor PCX: truncated pixels" ); }
				uint8_t value = byte( cursor++ ); size_t count = 1;
				if ( ( value & 0xc0 ) == 0xc0 ) {
					count = value & 0x3f;
					if ( count == 0 || cursor == end ) { throw std::invalid_argument( "CVR actor PCX: invalid run" ); }
					value = byte( cursor++ );
				}
				if ( count > stride - x ) { throw std::invalid_argument( "CVR actor PCX: run crosses scanline" ); }
				if ( output && x < width ) {
					std::fill_n( output->begin() + static_cast< ptrdiff_t >( y * width + x ), std::min( count, width - x ), value );
				}
				x += count;
			}
		}
		if ( cursor != end ) { throw std::invalid_argument( "CVR actor PCX: trailing pixel data" ); }
	};
	decode( nullptr );
	Pcx result{ width, height, std::vector< uint8_t >( width * height ), {} };
	decode( &result.pixels );
	for ( size_t i = 0; i < 256; ++i ) {
		for ( size_t channel = 0; channel < 3; ++channel ) { result.palette[ i ][ channel ] = byte( end + 1 + i * 3 + channel ); }
	}
	return result;
}

}

std::array< CvrPalette::Color, 256 > CvrActorMaterial::ReadSoftwareColors( const std::string_view palette_pcx ) {
	const auto palette = ReadPcx( palette_pcx );
	std::array< CvrPalette::Color, 256 > colors{};
	static constexpr std::array< CvrPalette::Color, 20 > RESERVED{{
		{ 0, 0, 0 }, { 128, 0, 0 }, { 0, 128, 0 }, { 128, 128, 0 },
		{ 0, 0, 128 }, { 128, 0, 128 }, { 0, 128, 128 }, { 192, 192, 192 },
		{ 8, 0, 0 }, { 9, 0, 0 }, { 245, 0, 0 }, { 246, 0, 0 },
		{ 128, 128, 128 }, { 255, 0, 0 }, { 0, 255, 0 }, { 255, 255, 0 },
		{ 0, 0, 255 }, { 255, 0, 255 }, { 0, 255, 255 }, { 255, 255, 255 }
	}};
	for ( size_t i = 0; i < 10; ++i ) { colors[ i ] = RESERVED[ i ]; colors[ i + 246 ] = RESERVED[ i + 10 ]; }
	std::copy_n( palette.palette.begin(), 236, colors.begin() + 10 );
	return colors;
}

CvrActorMaterial CvrActorMaterial::Read( const std::string_view palette_pcx, const std::string_view faction_pcx,
	const std::array< uint8_t, 24 * 256 >& body_shades ) {
	CvrActorMaterial result{};
	result.colors = ReadSoftwareColors( palette_pcx );
	const auto faction = ReadPcx( faction_pcx );
	if ( faction.width <= 435 || faction.height <= 745 ) { throw std::invalid_argument( "CVR actor PCX: tint sample unavailable" ); }
	result.tint_index = static_cast< uint8_t >( uint32_t( faction.pixels[ 745 * faction.width + 435 ] ) + 10 );
	const auto& tint = result.colors[ result.tint_index ];
	std::array< uint8_t, 256 > nearest{};
	for ( size_t i = 0; i < 256; ++i ) {
		std::array< int32_t, 3 > mixed;
		for ( size_t channel = 0; channel < 3; ++channel ) {
			// The binary32 1/255 constant is exactly representable as this double;
			// the uint8 products and product with it are exact (at most40 bits).
			mixed[ channel ] = static_cast< int32_t >( int32_t( result.colors[ i ][ channel ] ) * tint[ channel ] * 0.0039215688593685626983642578125 );
		}
		int32_t best = std::numeric_limits< int32_t >::max();
		for ( size_t candidate = 0; candidate < 256; ++candidate ) {
			int32_t distance = 0;
			for ( size_t channel = 0; channel < 3; ++channel ) {
				const int32_t difference = mixed[ channel ] - result.colors[ candidate ][ channel ];
				distance += difference * difference;
			}
			if ( distance < best ) { best = distance; nearest[ i ] = static_cast< uint8_t >( candidate ); }
		}
	}
	for ( size_t i = 0; i < body_shades.size(); ++i ) { result.shades[ i ] = nearest[ body_shades[ i ] ]; }
	return result;
}

}
