#include "Flic.h"

#include <functional>
#include <stdexcept>

#include "loader/flic/FlicReader.h"
#include "loader/flic/MenuCompositor.h"
#include "task/gsetests/GSETests.h"

namespace gse::tests {
namespace {

using Bytes = std::vector< uint8_t >;
void Put( Bytes& bytes, size_t at, uint32_t value, size_t count ) {
	for ( size_t i = 0; i < count; ++i ) { bytes.at( at + i ) = value >> ( 8 * i ); }
}
void Word( Bytes& bytes, uint16_t value ) { bytes.push_back( value & 255 ); bytes.push_back( value >> 8 ); }
Bytes Chunk( uint16_t type, const Bytes& payload ) {
	Bytes bytes( 6 ); Put( bytes, 0, 6 + payload.size(), 4 ); Put( bytes, 4, type, 2 );
	bytes.insert( bytes.end(), payload.begin(), payload.end() ); return bytes;
}
Bytes Frame( const std::vector< Bytes >& chunks ) {
	Bytes bytes( 16 ); Put( bytes, 4, 0xf1fa, 2 ); Put( bytes, 6, chunks.size(), 2 );
	for ( const auto& chunk : chunks ) { bytes.insert( bytes.end(), chunk.begin(), chunk.end() ); }
	Put( bytes, 0, bytes.size(), 4 ); return bytes;
}
Bytes File( uint16_t width, uint16_t height, uint16_t declared, const std::vector< Bytes >& frames ) {
	Bytes bytes( 128 ); Put( bytes, 4, 0xaf12, 2 ); Put( bytes, 6, declared, 2 );
	Put( bytes, 8, width, 2 ); Put( bytes, 10, height, 2 ); Put( bytes, 12, 8, 2 );
	Put( bytes, 16, 40, 4 ); Put( bytes, 80, 128, 4 );
	if ( frames.size() > 1 ) { Put( bytes, 84, 128 + frames.front().size(), 4 ); }
	for ( const auto& frame : frames ) { bytes.insert( bytes.end(), frame.begin(), frame.end() ); }
	Put( bytes, 0, bytes.size(), 4 ); return bytes;
}
struct Fixture { Bytes bytes; size_t delta, brun; };
Fixture MakeFixture() {
	Bytes colors; Word( colors, 1 ); colors.push_back( 0 ); colors.push_back( 0 );
	for ( size_t i = 0; i < 256; ++i ) { colors.push_back( i ); colors.push_back( 255 - i ); colors.push_back( i / 2 ); }
	const Bytes runs{ 1, 251, 1, 2, 3, 4, 5, 1, 5, 6, 1, 251, 7, 8, 9, 10, 11 };
	Bytes delta; Word( delta, 2 ); Word( delta, 0xffff ); Word( delta, 0x8063 ); Word( delta, 1 );
	delta.insert( delta.end(), { 1, 1, 20, 21 } ); Word( delta, 1 ); delta.insert( delta.end(), { 0, 254, 30, 31 } );
	Bytes change; Word( change, 1 ); change.insert( change.end(), { 10, 2, 55, 66, 77, 88, 99, 110 } );
	const auto first = Frame( { Chunk( 4, colors ), Chunk( 15, runs ) } );
	const auto second = Frame( { Chunk( 7, delta ) } );
	const auto third = Frame( { Chunk( 4, change ) } );
	return { File( 5, 3, 3, { first, second, third, Frame( { Chunk( 7, { 0, 0 } ) } ) } ),
		128 + first.size() + 16 + 6, 128 + 16 + Chunk( 4, colors ).size() + 6 };
}
bool Rejects( const std::function< void() >& call ) {
	try { call(); } catch ( const std::invalid_argument& ) { return true; }
	return false;
}

std::string CheckFrames() {
	const auto fixture = MakeFixture();
	const auto first = loader::flic::FlicReader::ReadFrame( fixture.bytes, 0 );
	GT_ASSERT( first.width == 5 && first.height == 3 && first.frame_count == 3 && first.speed_ms == 40, "header metadata changed" );
	GT_ASSERT( first.pixels == Bytes( { 1, 2, 3, 4, 5, 6, 6, 6, 6, 6, 7, 8, 9, 10, 11 } ), "BRUN literal/repeated rows differ" );
	GT_ASSERT( first.palette[ 200 ] == ( std::array< uint8_t, 3 >{ 200, 55, 100 } ), "256-entry palette packet changed" );
	const auto second = loader::flic::FlicReader::ReadFrame( fixture.bytes, 1 );
	GT_ASSERT( second.pixels == Bytes( { 1, 2, 3, 4, 5, 6, 20, 21, 6, 99, 30, 31, 30, 31, 11 } ), "SS2 skip/odd last pixel/word runs differ" );
	const auto third = loader::flic::FlicReader::ReadFrame( fixture.bytes, 2 );
	GT_ASSERT( third.pixels == second.pixels && third.palette[ 10 ] == ( std::array< uint8_t, 3 >{ 55, 66, 77 } ) &&
		third.palette[ 11 ] == ( std::array< uint8_t, 3 >{ 88, 99, 110 } ) && third.palette[ 12 ] == first.palette[ 12 ], "palette-only frame lost retained state" );
	GT_ASSERT( Rejects( [ & ] { loader::flic::FlicReader::ReadFrame( fixture.bytes, 3 ); } ), "physical ring frame became a declared frame" );
	GT_OK();
}

std::string CheckMalformed() {
	const auto fixture = MakeFixture();
	GT_ASSERT( Rejects( [ & ] { loader::flic::FlicReader::ReadFrame( fixture.bytes, 0, 14 ); } ), "pixel budget ignored" );
	const auto reject = [ & ]( const std::function< void( Bytes& ) >& edit ) {
		auto bytes = fixture.bytes; edit( bytes );
		return Rejects( [ & ] { loader::flic::FlicReader::ReadFrame( bytes, 1 ); } );
	};
	GT_ASSERT( reject( []( Bytes& b ) { b.resize( 127 ); } ), "short header accepted" );
	GT_ASSERT( reject( []( Bytes& b ) { Put( b, 0, b.size() + 1, 4 ); } ), "wrong file extent accepted" );
	GT_ASSERT( reject( []( Bytes& b ) { Put( b, 84, 129, 4 ); } ), "wrong second frame offset accepted" );
	GT_ASSERT( reject( []( Bytes& b ) { Put( b, 144, 5, 4 ); } ), "short child envelope accepted" );
	GT_ASSERT( reject( []( Bytes& b ) { Put( b, 148, 99, 2 ); } ), "unsupported chunk accepted" );
	GT_ASSERT( reject( [ & ]( Bytes& b ) { b.at( fixture.brun + 1 ) = 0; } ), "zero-length byte run accepted" );
	GT_ASSERT( reject( [ & ]( Bytes& b ) { b.at( fixture.brun + 1 ) = 6; } ), "byte run overflow accepted" );
	GT_ASSERT( reject( [ & ]( Bytes& b ) { Put( b, fixture.delta + 2, 0xfffc, 2 ); } ), "line skip overflow accepted" );
	GT_ASSERT( reject( [ & ]( Bytes& b ) { b.at( fixture.delta + 8 ) = 6; } ), "pixel skip overflow accepted" );
	const auto truncated = File( 5, 1, 1, { Frame( { Chunk( 15, { 1, 251 } ) } ) } );
	GT_ASSERT( Rejects( [ & ] { loader::flic::FlicReader::ReadFrame( truncated, 0 ); } ), "truncated literal bytes accepted" );
	GT_OK();
}


std::string CheckMenu() {
	loader::flic::Frame frame;
	frame.width = 3; frame.height = 1; frame.pixels = { 9, 0, 12 };
	std::array< std::array< uint8_t, 3 >, 256 > palette = {};
	palette[ 1 ] = { 80, 80, 80 }; palette[ 2 ] = { 250, 245, 240 }; palette[ 169 ] = { 100, 100, 100 };
	Bytes background( 5 * 4, 0 );
	const auto result = loader::flic::ComposeMenu( frame, background, 5, 4, palette, 2 );
	GT_ASSERT( result.width == 3 && result.height == 2, "menu rows did not expand vertically" );
	const Bytes expected{ 0,0,0,0, 80,80,80,255, 235,235,235,255, 0,0,0,0, 80,80,80,255, 235,235,235,255 };
	GT_ASSERT( result.rgba == expected, "menu transparency, native blue lookup or game-index palette mapping changed" );
	background[ 2 * 5 + 3 ] = 2;
	const auto tinted = loader::flic::ComposeMenu( frame, background, 5, 4, palette, 2 );
	GT_ASSERT( tinted.rgba[ 4 ] == 100 && tinted.rgba[ 16 ] == 80, "menu tint sampled the wrong background position or reused a row" );
	GT_ASSERT( Rejects( [&] { loader::flic::ComposeMenu( frame, background, 5, 4, palette, 16 * 1024 * 1024 ); } ), "menu output budget ignored" );
	std::fill( background.begin(), background.end(), uint8_t( 0 ) );
	background[ 3 * 5 + 4 ] = 2;
	const auto enlarged = loader::flic::ComposeMenu( frame, background, 5, 4, palette, 2, 10, 8 );
	GT_ASSERT( enlarged.rgba[ 4 ] == 100 && enlarged.rgba[ 16 ] == 100,
		"enlarged viewport sampled the unscaled PCX corner" );
	background[ 3 * 5 + 4 ] = 0; background[ 2 * 5 + 3 ] = 2;
	const auto reduced = loader::flic::ComposeMenu( frame, background, 5, 4, palette, 2, 4, 3 );
	GT_ASSERT( reduced.rgba[ 4 ] == 100, "reduced viewport did not sample displayed pixel centers" );
	GT_ASSERT( Rejects( [&] { loader::flic::ComposeMenu( frame, background, 5, 4, palette, 2, 10, 0 ); } ), "partial viewport dimensions accepted" );
	frame.pixels[ 2 ] = 246;
	GT_ASSERT( Rejects( [&] { loader::flic::ComposeMenu( frame, background, 5, 4, palette, 2 ); } ), "unsupported reserved artwork index accepted" );
	GT_OK();
}

}

void AddFlicTests( task::gsetests::GSETests* task ) {
	task->AddTest( "FLC menu composition preserves native indexed drawing codes", GT() {
		try { return CheckMenu(); } catch ( const std::exception& error ) { GT_FAIL( error.what() ); }
	} );
	task->AddTest( "FLC indexed frames preserve palette and BRUN/SS2 state", GT() {
		try { return CheckFrames(); } catch ( const std::exception& error ) { GT_FAIL( error.what() ); }
	} );
	task->AddTest( "FLC malformed streams reject before publishing decoded state", GT() {
		try { return CheckMalformed(); } catch ( const std::exception& error ) { GT_FAIL( error.what() ); }
	} );
}

}
