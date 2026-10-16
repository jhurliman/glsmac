#include "CvrIndexedRasterizer.h"

#include "loader/cvr/CvrIndexedRasterizer.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace gse::tests {
namespace {

using namespace loader::cvr;

CvrIndexedSurface Surface() { return { 8, 8, 10, std::vector< uint8_t >( 80, 165 ), std::vector< uint16_t >( 80, 65535 ) }; }

CvrIndexedLookups Lookups() {
	CvrIndexedLookups tables{};
	for ( size_t i = 0 ; i < tables.first.size() ; ++i ) { tables.first[ i ] = ( i * 13 + 7 ) % 24; }
	for ( size_t s = 0 ; s < 24 ; ++s ) {
		for ( size_t c = 0 ; c < 256 ; ++c ) { tables.second.push_back( ( s * 17 + c * 7 + 3 ) & 255 ); }
	}
	return tables;
}

CvrGeometryRecord Record( const uint8_t code, const uint16_t first = 0, const uint8_t second = 1 ) {
	return { { static_cast< uint8_t >( ( code << 3 ) | ( first >> 8 ) ), static_cast< uint8_t >( first ), second } };
}

CvrIndexedDraw Draw( const std::vector< CvrGeometryRecord >& records ) {
	return { &records, false, CvrSampleFootprint::Single, { { { 256, 0, 0 }, { 0, 256, 0 }, { 0, 0, 256 } } }, { 512, 512, 8192 } };
}

template< typename F > bool Rejects( F&& action ) {
	try { action(); } catch ( const std::invalid_argument& error ) { return std::string( error.what() ).find( "CVR indexed raster:" ) == 0; }
	return false;
}

}

void AddCvrIndexedRasterizerTests( task::gsetests::GSETests* task ) {
	task->AddTest( "CVR indexed raster walks records before sampling and preserves pitch padding", GT() {
		const std::vector< CvrGeometryRecord > records{ Record( 21, 0, 1 ), Record( 15, 257, 2 ), Record( 4, 2047, 255 ) };
		const auto initial = Surface();
		const auto output = CvrIndexedRasterizer::Draw( initial, Lookups(), { Draw( records ) } );
		GT_ASSERT( output.processed_records == 3 && output.written_samples == 3, "wrong record/write count" );
		for ( size_t i = 0 ; i < 80 ; ++i ) {
			const uint8_t expected = i == 23 ? 129 : i == 33 ? 221 : i == 32 ? 30 : 165;
			GT_ASSERT( output.surface.color[ i ] == expected && output.surface.depth[ i ] == ( i == 23 || i == 33 || i == 32 ? 32 : 65535 ),
				"step order, lookup bits, color/depth or row padding changed" );
		}
		GT_ASSERT( initial.color == Surface().color && initial.depth == Surface().depth, "initial surface was modified" );
		GT_OK();
	} );
	task->AddTest( "CVR indexed raster preserves component order strict depth and four-pixel footprints", GT() {
		const std::vector< CvrGeometryRecord > a{ Record( 13, 0, 1 ) }, b{ Record( 13, 2047, 255 ) };
		auto first = Draw( a ); first.basis = {}; first.footprint = CvrSampleFootprint::Square2;
		auto second = Draw( b ); second.basis = {}; second.footprint = CvrSampleFootprint::Square2;
		auto initial = Surface(); initial.depth[ 23 ] = 31; initial.depth[ 32 ] = 32;
		const auto equal = CvrIndexedRasterizer::Draw( initial, Lookups(), { first, second } );
		GT_ASSERT( equal.processed_records == 2 && equal.written_samples == 2 && equal.surface.color[ 22 ] == 129 && equal.surface.color[ 33 ] == 129 &&
			equal.surface.color[ 23 ] == 165 && equal.surface.color[ 32 ] == 165, "equal/near depth or footprint order changed" );
		second.initial[ 2 ] = 30 * 256;
		const auto near = CvrIndexedRasterizer::Draw( initial, Lookups(), { first, second } );
		GT_ASSERT( near.written_samples == 6 && near.surface.color[ 22 ] == 30 && near.surface.color[ 23 ] == 30 && near.surface.color[ 32 ] == 30 && near.surface.color[ 33 ] == 30,
			"later nearer component did not cover all four samples" );
		GT_OK();
	} );
	task->AddTest( "CVR indexed raster excludes terminal bytes and floors fractional signed depth", GT() {
		const std::vector< CvrGeometryRecord > records{ Record( 13, 1, 2 ), { { 215, 255, 255 } } };
		auto draw = Draw( records ); draw.terminal = true;
		draw.basis = { { { 0, 0, -129 }, { 0, 0, 127 }, { 0, 0, -257 } } };
		const auto result = CvrIndexedRasterizer::Draw( Surface(), Lookups(), { draw } );
		GT_ASSERT( result.processed_records == 1 && result.written_samples == 1 && result.surface.depth[ 21 ] == 30 && result.surface.color[ 21 ] != 165,
			"terminal was sampled or fractional values truncated toward zero" );
		draw.basis = {}; draw.initial[ 2 ] = -1;
		const auto negative = CvrIndexedRasterizer::Draw( Surface(), Lookups(), { draw } );
		GT_ASSERT( negative.processed_records == 1 && !negative.written_samples, "negative depth was compared as signed or rounded toward zero" );
		draw.initial[ 2 ] = -257;
		const auto wrapped = CvrIndexedRasterizer::Draw( Surface(), Lookups(), { draw } );
		GT_ASSERT( wrapped.surface.depth[ 22 ] == 65534 && wrapped.written_samples == 1, "unsigned high-word depth extraction changed" );
		GT_OK();
	} );
	task->AddTest( "CVR indexed raster owns output after all input lifetime ends", GT() {
		CvrIndexedResult result;
		{
			auto source = Surface(); auto lookups = Lookups();
			const std::vector< CvrGeometryRecord > records{ Record( 21 ) };
			result = CvrIndexedRasterizer::Draw( source, lookups, { Draw( records ) } );
			source.color.clear(); source.depth.clear(); lookups.second.clear();
		}
		auto copied = result; auto moved = std::move( copied ); result.surface.color.clear(); result.surface.depth.clear();
		GT_ASSERT( moved.surface.color[ 23 ] == 129 && moved.surface.depth[ 23 ] == 32, "output borrowed source buffers or records" );
		auto initial = Surface(); auto empty = CvrIndexedRasterizer::Draw( initial, Lookups(), {} );
		initial.color[ 0 ] = 0;
		GT_ASSERT( empty.surface.color[ 0 ] == 165 && !empty.processed_records && !empty.written_samples, "empty batch failed to return an owned copy" );
		GT_OK();
	} );
	task->AddTest( "CVR indexed raster enforces shifted signed32 depth boundaries", GT() {
		const std::vector< CvrGeometryRecord > records{ Record( 13 ) };
		auto draw = Draw( records ); draw.basis = {};
		for ( const int32_t boundary : { -8388608, 8388607 } ) {
			draw.initial[ 2 ] = boundary;
			const auto result = CvrIndexedRasterizer::Draw( Surface(), Lookups(), { draw } );
			GT_ASSERT( result.written_samples == 1 && result.surface.depth[ 22 ] == ( boundary < 0 ? 32768 : 32767 ), "valid shifted depth endpoint changed" );
			draw.basis[ 2 ][ 2 ] = boundary < 0 ? -1 : 1;
			GT_ASSERT( Rejects( [ & ]() { CvrIndexedRasterizer::Draw( Surface(), Lookups(), { draw } ); } ), "cumulative shifted depth overflow accepted" );
			draw.basis = {};
		}
		GT_OK();
	} );
	task->AddTest( "CVR indexed raster rejects malformed late draws before altering inputs", GT() {
		std::vector< CvrGeometryRecord > records{ Record( 21 ) }, empty, terminal{ Record( 26 ) }, early{ Record( 26 ), Record( 13 ) }, bad{ Record( 31 ) };
		const auto initial = Surface(); const auto good = Draw( records );
		std::vector< CvrIndexedDraw > invalid{ Draw( empty ), Draw( early ), Draw( bad ) };
		auto value = Draw( terminal ); value.terminal = true; invalid.push_back( value );
		value = good; value.terminal = true; invalid.push_back( value );
		value = good; value.records = nullptr; invalid.push_back( value );
		value = good; value.footprint = static_cast< CvrSampleFootprint >( 99 ); invalid.push_back( value );
		value = good; value.initial[ 0 ] = -1024; invalid.push_back( value );
		value = good; value.initial[ 1 ] = 8 * 256; invalid.push_back( value );
		value = good; value.initial[ 0 ] = 6 * 256; value.footprint = CvrSampleFootprint::Square2; invalid.push_back( value );
		value = good; value.basis[ 0 ][ 0 ] = 32768; invalid.push_back( value );
		value = good; value.basis[ 1 ][ 0 ] = -32769; invalid.push_back( value );
		value = good; value.basis[ 2 ][ 0 ] = std::numeric_limits< int32_t >::max(); invalid.push_back( value );
		value = good; value.initial[ 2 ] = 8388608; invalid.push_back( value );
		value = good; value.initial[ 2 ] = -8388609; invalid.push_back( value );
		value = good; value.initial[ 0 ] = std::numeric_limits< int32_t >::max(); invalid.push_back( value );
		for ( const auto& draw : invalid ) {
			GT_ASSERT( Rejects( [ & ]() { CvrIndexedRasterizer::Draw( initial, Lookups(), { good, draw } ); } ), "invalid later draw accepted" );
			GT_ASSERT( initial.color == Surface().color && initial.depth == Surface().depth, "late validation changed initial surface" );
		}
		const std::vector< CvrGeometryRecord > late{ Record( 21 ), Record( 31 ) };
		GT_ASSERT( Rejects( [ & ]() { CvrIndexedRasterizer::Draw( initial, Lookups(), { Draw( late ) } ); } ), "invalid late record accepted" );
		GT_OK();
	} );
	task->AddTest( "CVR indexed raster checks lookup surface and batch allocation limits", GT() {
		const std::vector< CvrGeometryRecord > records{ Record( 13, 2047, 255 ) };
		auto draw = Draw( records ); draw.basis = {};
		for ( const size_t length : { size_t( 0 ), size_t( 257 ), size_t( 65537 ), size_t( 256 ) } ) {
			auto tables = Lookups(); tables.second.resize( length );
			GT_ASSERT( Rejects( [ & ]() { CvrIndexedRasterizer::Draw( Surface(), tables, { draw } ); } ), "invalid table extent/index accepted" );
		}
		for ( const int mode : { 0, 1, 2, 3, 4, 5 } ) {
			auto surface = Surface();
			if ( mode == 0 ) { surface.width = 0; } else if ( mode == 1 ) { surface.height = 0; }
			else if ( mode == 2 ) { surface.pitch = 7; } else if ( mode == 3 ) { surface.color.pop_back(); }
			else if ( mode == 4 ) { surface.depth.pop_back(); }
			else { surface.pitch = surface.height = std::numeric_limits< uint32_t >::max(); }
			GT_ASSERT( Rejects( [ & ]() { CvrIndexedRasterizer::Draw( surface, Lookups(), { draw } ); } ), "invalid surface extent accepted" );
		}
		GT_ASSERT( Rejects( [ & ]() { CvrIndexedRasterizer::Draw( Surface(), Lookups(), std::vector< CvrIndexedDraw >( CvrIndexedRasterizer::MAX_DRAWS + 1, draw ) ); } ), "excess draws accepted" );
		const std::vector< CvrGeometryRecord > many( CvrIndexedRasterizer::MAX_RECORDS / 2 + 1, Record( 13 ) );
		auto large = Draw( many ); large.basis = {};
		GT_ASSERT( Rejects( [ & ]() { CvrIndexedRasterizer::Draw( Surface(), Lookups(), { large, large } ); } ), "combined record limit bypassed" );
		GT_OK();
	} );
}

}
