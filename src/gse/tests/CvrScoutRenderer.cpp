#include "CvrScoutRenderer.h"

#include "loader/cvr/CvrPaletteReader.h"
#include "loader/cvr/CvrScoutRenderer.h"
#include "loader/cvr/ScoutArithmetic.h"
#include "loader/cvr/ScoutLighting.h"
#include "loader/cvr/ScoutView.h"
#include "task/gsetests/GSETests.h"
#include "types/texture/Texture.h"
#include "fixtures/CvrScoutMatrix.h"

#include <algorithm>
#include <cfenv>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {

using namespace loader::cvr;

void Field( std::vector< uint8_t >& bytes, const uint32_t tag, const std::vector< uint8_t >& payload ) {
	for ( int i = 3; i >= 0; --i ) { bytes.push_back( static_cast< uint8_t >( tag >> ( i * 8 ) ) ); }
	const auto size = static_cast< uint32_t >( payload.size() + 8 );
	for ( unsigned i = 0; i < 4; ++i ) { bytes.push_back( static_cast< uint8_t >( size >> ( i * 8 ) ) ); }
	bytes.insert( bytes.end(), payload.begin(), payload.end() );
}

CvrContainer PaletteSource( const uint32_t version, const bool modern ) {
	CvrContainer source{ { CvrChunk{ 1, {} }, CvrChunk{ 2, { 1 } }, CvrChunk{ 3, {} }, CvrChunk{ 4, { 1 } } } };
	for ( unsigned i = 0; i < 4; ++i ) { source.chunks[ 0 ].payload.push_back( static_cast< uint8_t >( version >> ( 8 * i ) ) ); }
	auto& nested = source.chunks[ 2 ].payload;
	Field( nested, 0x999, { 9, 8, 7 } );
	Field( nested, 0x101, { 'n', 0, 'm' } );
	std::vector< uint8_t > palette( modern ? 6914 : 4866 );
	palette[ 0 ] = 1; palette[ 1 ] = 245;
	for ( size_t i = 0; i < 768; ++i ) { palette[ 2 + i ] = static_cast< uint8_t >( i * 17 + 3 ); }
	for ( size_t i = 770; i < palette.size(); ++i ) { palette[ i ] = static_cast< uint8_t >( ( i - 770 ) / 256 + ( i - 770 ) % 256 ); }
	Field( nested, 0x201, palette );
	Field( nested, 0x301, { 5, 4, 0, 3 } );
	return source;
}

template< typename F > bool Rejects( F&& action ) {
	try { action(); }
	catch ( const std::invalid_argument& ) { return true; }
	return false;
}

using Bytes = std::vector< uint8_t >;
void Word( Bytes& bytes, const uint32_t value ) {
	for ( unsigned i = 0; i < 4; ++i ) { bytes.push_back( static_cast< uint8_t >( value >> ( i * 8 ) ) ); }
}
void PutWord( Bytes& bytes, const size_t offset, const uint32_t value ) {
	for ( unsigned i = 0; i < 4; ++i ) { bytes.at( offset + i ) = static_cast< uint8_t >( value >> ( i * 8 ) ); }
}
void Float( Bytes& bytes, const float value ) {
	uint32_t bits; std::memcpy( &bits, &value, sizeof( bits ) ); Word( bytes, bits );
}

struct AssetInput {
	uint8_t shade = 7;
	float x = 400;
	bool grouped = false;
	int16_t group_first = 100, group_fourth = 100;
	uint8_t frame_flags = 0;
	uint32_t group_flags = 0;
	uint32_t scalar = 0x3f800000;
	uint8_t sample_code = 13;
	bool singular = false;
};

// Complete self-authored two-record asset, through the real container, scene,
// geometry and palette readers. No installed bytes are part of this fixture.
std::string Asset( const AssetInput& input ) {
	Bytes geometry;
	for ( const uint32_t v : { input.grouped ? 10U : 8U, 0U, input.scalar, 0U, 0U } ) { Word( geometry, v ); }
	geometry.resize( 38, 0 ); Word( geometry, input.grouped ? 1U : 2U );
	if ( input.grouped ) {
		for ( size_t i = 0; i < 12; ++i ) {
			const auto v = static_cast< uint16_t >( i == 0 ? input.group_first : i == 9 ? input.group_fourth : 0 );
			geometry.push_back( static_cast< uint8_t >( v ) ); geometry.push_back( static_cast< uint8_t >( v >> 8 ) );
		}
		Word( geometry, 2 ); Word( geometry, input.group_flags );
	}
	geometry.insert( geometry.end(), { static_cast< uint8_t >( input.sample_code << 3 ), 0, 42, 208, 255, 255 } );
	geometry.resize( geometry.size() + 5, 0 );
	PutWord( geometry, 4, static_cast< uint32_t >( geometry.size() ) );
	PutWord( geometry, 12, static_cast< uint32_t >( geometry.size() ) );
	PutWord( geometry, 16, static_cast< uint32_t >( geometry.size() - 1 ) );
	Bytes voxel; Field( voxel, 0x01020404, geometry );
	Bytes locations; Float( locations, input.x ); Float( locations, -200 ); Float( locations, 0 );
	Bytes matrix;
	for ( size_t i = 0; i < 9; ++i ) { Float( matrix, !input.singular && i % 4 == 0 ? 1 : 0 ); }
	Bytes frames; Field( frames, 0x01030404, { input.frame_flags } ); Field( frames, 0x02030404, locations ); Field( frames, 0x03030404, matrix );
	Bytes object; Field( object, 0x10404, { 'o' } ); Field( object, 0x20404, voxel ); Field( object, 0x30404, frames );
	Bytes one; Word( one, 1 );
	Bytes scene; Field( scene, 0x104, { 's' } ); Field( scene, 0x204, one ); Field( scene, 0x304, one ); Field( scene, 0x404, object );
	Bytes palette( 6914, input.shade ); palette[ 0 ] = 0; palette[ 1 ] = 255;
	for ( size_t i = 0; i < 256; ++i ) {
		palette[ 2 + i * 3 ] = static_cast< uint8_t >( i ); palette[ 3 + i * 3 ] = 17; palette[ 4 + i * 3 ] = 31;
	}
	Bytes color; Field( color, 0x101, { 'p' } ); Field( color, 0x201, palette );
	Bytes file; Field( file, 1, { 0, 0, 0x20, 0x41 } ); Field( file, 2, { 'f' } ); Field( file, 3, color ); Field( file, 4, scene );
	Bytes result; Field( result, 0x43565200, file );
	return { reinterpret_cast< const char* >( result.data() ), result.size() };
}

constexpr CvrScoutViewInput VIEW{ 0, 0, 1, 1, { 128, 128 } };

}

void AddCvrScoutRendererTests( task::gsetests::GSETests* task ) {
	task->AddTest( "CVR Scout original synthetic matrix schedule and store cases", GT() {
		for ( const auto& fixture : fixtures::SCOUT_MATRIX_CASES ) {
			if ( fixture.reject_overflow ) {
				GT_ASSERT( Rejects( [ & ] { scout::MultiplyMatrix( fixture.left, fixture.right ); } ), "nonfinite result accepted" );
			}
			else {
				GT_ASSERT( scout::MultiplyMatrix( fixture.left, fixture.right ) == fixture.expected, "matrix mismatch: " + std::string( fixture.name ) );
			}
		}
		GT_OK();
	} );
	task->AddTest( "CVR Scout complete owned assets compose simple and grouped shaded images", GT() {
		AssetInput body, reactor; reactor.x = 500; reactor.shade = 77; body.grouped = true;
		auto first = Asset( body ), second = Asset( reactor );
		auto renderer = CvrScoutRenderer::Load( first, second ); first.clear(); second.clear();
		const auto image = renderer.Render( VIEW );
		GT_ASSERT( image.draw_count == 2 && image.processed_records == 2 && image.written_samples == 2, "complete composition or terminal exclusion changed" );
		GT_ASSERT( std::count( image.indexed.color.begin(), image.indexed.color.end(), 7 ) == 1 && std::count( image.indexed.color.begin(), image.indexed.color.end(), 77 ) == 1, "scene-specific shade table lost" );
		const auto before = image.indexed.color;
		const auto moved = std::move( renderer );
		GT_ASSERT( moved.Render( VIEW ).indexed.color == before, "owned source changed after move/repeated render" );
		body.group_fourth = 0;
		const auto alternate = CvrScoutRenderer::Load( Asset( body ), Asset( reactor ) ).Render( VIEW );
		GT_ASSERT( alternate.indexed.color != before, "fourth group vector did not affect actual group origin" );
		GT_OK();
	} );
	task->AddTest( "CVR Scout key nine alpha ignores depth and produces independent tracked texture", GT() {
		AssetInput body, reactor; body.shade = 9; reactor.shade = 0; reactor.x = 500;
		auto image = CvrScoutRenderer::Load( Asset( body ), Asset( reactor ) ).Render( VIEW );
		size_t transparent_written = 0, opaque_zero = 0;
		for ( size_t i = 0; i < image.indexed.color.size(); ++i ) {
			const auto index = image.indexed.color[ i ];
			GT_ASSERT( image.rgba[ i * 4 ] == index && image.rgba[ i * 4 + 1 ] == 17 && image.rgba[ i * 4 + 2 ] == 31, "RGB changed" );
			GT_ASSERT( image.rgba[ i * 4 + 3 ] == ( index == 9 ? 0 : 255 ), "transparency used RGB/depth instead of index9" );
			if ( index == 9 && image.indexed.depth[ i ] != 65535 ) { ++transparent_written; }
			if ( index == 0 ) { ++opaque_zero; }
		}
		GT_ASSERT( transparent_written == 1 && opaque_zero == 1, "key/depth controls did not execute" );
		const auto expected = image.rgba;
		auto texture = image.CreateTexture();
		image.rgba.clear(); image.indexed.color.clear();
		GT_ASSERT( texture->GetWidth() == 256 && texture->GetHeight() == 256 && texture->GetBitmapSize() == expected.size(), "texture extent changed" );
		GT_ASSERT( std::memcmp( texture->GetBitmap(), expected.data(), expected.size() ) == 0, "texture borrows expired image pixels" );
		auto transferred = std::move( texture ); GT_ASSERT( !texture && transferred, "texture ownership was not transferred" );
		transferred.reset(); // tracked destructor/free is exercised by memorydebug/LSan.
		GT_ASSERT( Rejects( [ & ] { image.CreateTexture(); } ), "incomplete RGBA texture accepted" );
		GT_OK();
	} );
	task->AddTest( "CVR Scout rejects unsupported late asset or draw without damaging reusable source", GT() {
		const auto body = Asset( {} );
		AssetInput input; input.frame_flags = 1;
		GT_ASSERT( Rejects( [ & ] { CvrScoutRenderer::Load( body, Asset( input ) ); } ), "unsupported idle flags accepted" );
		input = {}; input.grouped = true; input.group_flags = 1;
		GT_ASSERT( Rejects( [ & ] { CvrScoutRenderer::Load( body, Asset( input ) ); } ), "unsupported group flags accepted" );
		input = {}; input.singular = true;
		GT_ASSERT( Rejects( [ & ] { CvrScoutRenderer::Load( body, Asset( input ) ).Render( VIEW ); } ), "singular lighting matrix accepted" );
		input = {}; input.shade = 255;
		GT_ASSERT( Rejects( [ & ] { CvrScoutRenderer::Load( Asset( input ), body ).Render( VIEW ); } ), "undefined output RGB accepted" );
		input = {}; input.x = 100000;
		GT_ASSERT( Rejects( [ & ] { CvrScoutRenderer::Load( body, Asset( input ) ).Render( VIEW ); } ), "out-of-surface draw accepted" );
		const auto renderer = CvrScoutRenderer::Load( body, body ); const auto expected = renderer.Render( VIEW );
		auto invalid = VIEW; invalid.facing = 8;
		GT_ASSERT( Rejects( [ & ] { renderer.Render( invalid ); } ), "invalid view accepted" );
		invalid = VIEW; invalid.scale_denominator = 0;
		GT_ASSERT( Rejects( [ & ] { renderer.Render( invalid ); } ), "invalid override denominator accepted" );
		GT_ASSERT( renderer.Render( VIEW ).rgba == expected.rgba, "failed render damaged owned source" );
		GT_OK();
	} );
	task->AddTest( "CVR Scout modulo depth keeps unsigned wrap while strict raster rejects it", GT() {
		CvrIndexedSurface surface{ 1, 1, 1, { 9 }, { 65535 } };
		CvrIndexedLookups lookups{}; lookups.second.resize( 256, 77 );
		const std::vector< CvrGeometryRecord > records{ { { 104, 0, 0 } } }; // +Z
		CvrIndexedDraw draw{ &records, false, CvrSampleFootprint::Single, {}, { 0, 0, 0x00ffff00 } };
		draw.basis[ 2 ][ 2 ] = 0x100;
		GT_ASSERT( Rejects( [ & ] { CvrIndexedRasterizer::Draw( surface, lookups, { draw } ); } ), "strict signed-depth domain changed" );
		const auto result = CvrIndexedRasterizer::DrawModuloDepth( surface, lookups, { draw } );
		GT_ASSERT( result.surface.depth[ 0 ] == 0 && result.surface.color[ 0 ] == 77 && surface.depth[ 0 ] == 65535, "modulo32 depth or source ownership changed" );
		draw.initial[ 0 ] = 256;
		GT_ASSERT( Rejects( [ & ] { CvrIndexedRasterizer::DrawModuloDepth( surface, lookups, { draw } ); } ), "modulo depth relaxed x/y bounds" );
		GT_OK();
	} );
	task->AddTest( "CVR Scout rounded extended matrix preserves scheduled ties stores and zero signs", GT() {
		using namespace scout;
		const Matrix right{ 0, 0x3f800000, 0, 0, 0x3f800000, 0, 0, 0x3f800000, 0 };
		Matrix left{}; left[ 0 ] = 0x5f800000; left[ 1 ] = 0x3f800000; left[ 2 ] = 0xdf800000;
		GT_ASSERT( MultiplyMatrix( left, right )[ 1 ] == 0, "first extended tie was not rounded" );
		left[ 1 ] = 0x40400000;
		GT_ASSERT( MultiplyMatrix( left, right )[ 1 ] == 0x40800000, "extended odd tie failed to carry" );
		left[ 0 ] = 0x5d800000; left[ 1 ] = 0x3f800000; left[ 2 ] = 0xdd800000;
		GT_ASSERT( MultiplyMatrix( left, right )[ 1 ] == 0x3f800000, "binary64 arithmetic lost an extended unit" );
		left.fill( 0x80000000 ); Matrix positive; positive.fill( 0x3f800000 );
		for ( const auto bits : MultiplyMatrix( left, positive ) ) { GT_ASSERT( bits == 0x80000000, "negative zero product/sum normalized" ); }
		GT_ASSERT( Store( Multiply( Decode( 1 ), Decode( 0x3f000000 ) ) ) == 0, "positive tiny store tie changed" );
		GT_ASSERT( Store( Multiply( Decode( 0x80000001 ), Decode( 0x3f000000 ) ) ) == 0x80000000, "underflow zero sign lost" );
		GT_ASSERT( Store( Add( Decode( 1 ), Decode( 2 ) ) ) == 3, "finite subnormal store rejected" );
		GT_ASSERT( Rejects( [] { Store( Multiply( Decode( 0x7f7fffff ), Integer( 2 ) ) ); } ), "nonfinite renderer matrix result accepted" );
		GT_OK();
	} );
	task->AddTest( "CVR Scout generated view scale and signed pole lighting preserve caller controls", GT() {
		using namespace scout;
		const auto ordinary = MakeView( VIEW ); auto half = VIEW; half.zoom = 5; half.scale_numerator = 1; half.scale_denominator = 2;
		const auto smaller = MakeView( half );
		GT_ASSERT( ordinary.scales[ 0 ] == 0x3db33333 && smaller.scales[ 0 ] == 0x3d333333 && smaller.target_anchor[ 0 ] == 153, "stored Scout scale or integer anchor changed" );
		auto minus = VIEW; minus.facing = -1;
		GT_ASSERT( MakeView( minus ).projection == ordinary.projection, "facing sentinel changed" );
		const Matrix identity{ 0x3f800000, 0, 0, 0, 0x3f800000, 0, 0, 0, 0x3f800000 };
		LightingInput light{ identity, identity, { 0, 0, 0x3f800000 }, 0x3f800000, { 0x3f800000, 0x3f800000 } };
		const auto north = SelectLighting( light ); light.light[ 2 ] = 0xbf800000; const auto south = SelectLighting( light );
		GT_ASSERT( north.row == 0 && north.offset == 89 && south.row == 0 && south.offset == 44, "reflection discarded zero signs before atan2" );
		const int saved = std::fegetround();
		const bool changed = std::fesetround( FE_DOWNWARD ) == 0;
		const bool rejected = changed && Rejects( [ & ] { MakeView( VIEW ); } );
		std::fesetround( saved );
		GT_ASSERT( changed && rejected, "non-nearest arithmetic was silently admitted" );
		GT_OK();
	} );
	task->AddTest( "CVR Scout palette owns modern RGB shades and opaque metadata", GT() {
		auto source = PaletteSource( 0x41200000, true );
		auto palette = CvrPaletteReader::Read( source );
		source.chunks[ 2 ].payload.clear();
		GT_ASSERT( palette.name == std::string( "n\0m", 3 ) && palette.auxiliary == std::vector< uint8_t >( { 5, 4, 0, 3 } ), "name/auxiliary ownership changed" );
		GT_ASSERT( palette.first == 1 && palette.end == 245, "palette range changed" );
		for ( size_t i = 0; i < 256; ++i ) {
			for ( size_t c = 0; c < 3; ++c ) { GT_ASSERT( palette.colors[ i ][ c ] == static_cast< uint8_t >( ( i * 3 + c ) * 17 + 3 ), "RGB order or byte scaling changed" ); }
			for ( size_t row = 0; row < 24; ++row ) { GT_ASSERT( palette.shades[ row * 256 + i ] == static_cast< uint8_t >( row + i ), "modern shade bytes changed" ); }
		}
		GT_OK();
	} );
	task->AddTest( "CVR Scout palette uses strict version seven boundary and legacy row selection", GT() {
		for ( const uint32_t version : { 0x40dfffffU, 0x40e00000U, 0x40e00001U } ) {
			const bool modern = version > 0x40e00000U;
			const auto result = CvrPaletteReader::Read( PaletteSource( version, modern ) );
			for ( size_t row = 0; row < 24; ++row ) {
				for ( size_t column = 0; column < 256; ++column ) {
					GT_ASSERT( result.shades[ row * 256 + column ] == static_cast< uint8_t >( ( modern ? row : 2 * row / 3 ) + column ), "legacy shade row mapping changed" );
				}
			}
			GT_ASSERT( Rejects( [ & ]() { CvrPaletteReader::Read( PaletteSource( version, !modern ) ); } ), "version/payload mismatch accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "CVR Scout palette rejects malformed framing and duplicate late fields", GT() {
		for ( const uint32_t version : { 0x7f800000U, 0xff800000U, 0x7fc00000U } ) {
			GT_ASSERT( Rejects( [ & ]() { CvrPaletteReader::Read( PaletteSource( version, true ) ); } ), "nonfinite version accepted" );
		}
		for ( const uint32_t tag : { 0x101U, 0x201U, 0x301U } ) {
			auto source = PaletteSource( 0x41200000, true ); Field( source.chunks[ 2 ].payload, tag, { 1 } );
			GT_ASSERT( Rejects( [ & ]() { CvrPaletteReader::Read( source ); } ), "duplicate known field accepted" );
		}
		for ( size_t i = 1; i < 8; ++i ) {
			auto source = PaletteSource( 0x41200000, true ); source.chunks[ 2 ].payload.insert( source.chunks[ 2 ].payload.end(), i, 0 );
			GT_ASSERT( Rejects( [ & ]() { CvrPaletteReader::Read( source ); } ), "trailing partial header accepted" );
		}
		auto source = PaletteSource( 0x41200000, true ); source.chunks[ 2 ].payload[ 4 ] = 7;
		GT_ASSERT( Rejects( [ & ]() { CvrPaletteReader::Read( source ); } ), "undersized inclusive field accepted" );
		source = PaletteSource( 0x41200000, true ); source.chunks[ 2 ].payload.pop_back();
		GT_ASSERT( Rejects( [ & ]() { CvrPaletteReader::Read( source ); } ), "truncated late auxiliary accepted" );
		GT_OK();
	} );
	task->AddTest( "CVR Scout lighting generator retains pole repeated tail and zero padding", GT() {
		const auto table = scout::GenerateLightingTable();
		uint64_t checksum = 14695981039346656037ULL;
		for ( size_t row = 0; row < table.size(); ++row ) {
			GT_ASSERT( table[ row ][ 0 ] == 23 - row, "wrong pole value" );
			for ( size_t column = 0; column < 89; ++column ) { GT_ASSERT( table[ row ][ 1 + 23 * 89 + column ] == table[ row ][ 1 + 22 * 89 + column ], "last ring was not repeated" ); }
			for ( size_t column = 2137; column < 2144; ++column ) { GT_ASSERT( table[ row ][ column ] == 0, "nonzero padding" ); }
			for ( const auto shade : table[ row ] ) {
				GT_ASSERT( shade <= 23, "shade outside palette row range" );
				checksum = ( checksum ^ shade ) * 1099511628211ULL;
			}
		}
		// Fingerprint of independently generated bytes, also checked against
		// original51,456-byte SHA256 by the opt-in installed evidence probe.
		GT_ASSERT( checksum == 0xc797b2543854d7f7ULL, "complete generated lighting table changed" );
		GT_OK();
	} );
}

}
