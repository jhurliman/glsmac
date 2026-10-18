#include "FlicReader.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace loader::flic {
namespace {

[[noreturn]] void Invalid( const char* reason ) { throw std::invalid_argument( std::string( "FLC: " ) + reason ); }

class Cursor {
public:
	Cursor( const std::vector< uint8_t >& bytes, size_t begin, size_t end ) : m_bytes( bytes ), m_at( begin ), m_end( end ) {
		if ( begin > end || end > bytes.size() ) { Invalid( "invalid byte range" ); }
	}
	size_t Position() const { return m_at; }
	size_t Remaining() const { return m_end - m_at; }
	uint8_t Byte() { Require( 1 ); return m_bytes[ m_at++ ]; }
	uint16_t Word() { const uint16_t lo = Byte(); return lo | ( uint16_t( Byte() ) << 8 ); }
	uint32_t Dword() { const uint32_t lo = Word(); return lo | ( uint32_t( Word() ) << 16 ); }
	void Skip( size_t count ) { Require( count ); m_at += count; }
	void Copy( uint8_t* destination, size_t count ) {
		Require( count );
		std::copy_n( m_bytes.data() + m_at, count, destination );
		m_at += count;
	}
	void EndPayload() const { if ( Remaining() > 1 ) { Invalid( "unexpected chunk payload" ); } }
private:
	void Require( size_t count ) const { if ( count > Remaining() ) { Invalid( "truncated payload" ); } }
	const std::vector< uint8_t >& m_bytes;
	size_t m_at, m_end;
};

int SignedByte( uint8_t value ) { return value < 128 ? value : int( value ) - 256; }

void Colors( Cursor& input, Frame& frame ) {
	const size_t packets = input.Word();
	size_t index = 0;
	for ( size_t packet = 0; packet < packets; ++packet ) {
		const size_t skip = input.Byte();
		if ( skip > 256 - index ) { Invalid( "palette skip overflow" ); }
		index += skip;
		const auto encoded = input.Byte();
		const size_t count = encoded ? encoded : 256;
		if ( count > 256 - index ) { Invalid( "palette run overflow" ); }
		for ( size_t i = 0; i < count; ++i ) {
			for ( auto& channel : frame.palette[ index++ ] ) { channel = input.Byte(); }
		}
	}
}

void ByteRun( Cursor& input, Frame& frame ) {
	for ( size_t y = 0; y < frame.height; ++y ) {
		input.Byte(); // Historical packet-count byte; the expanded width terminates the line.
		size_t x = 0;
		while ( x < frame.width ) {
			const int run = SignedByte( input.Byte() );
			if ( !run ) { Invalid( "zero-length byte run" ); }
			const size_t count = run < 0 ? -run : run;
			if ( count > frame.width - x ) { Invalid( "byte run crosses row" ); }
			auto* output = frame.pixels.data() + y * frame.width + x;
			if ( run < 0 ) { input.Copy( output, count ); }
			else { std::fill_n( output, count, input.Byte() ); }
			x += count;
		}
	}
}

void WordDelta( Cursor& input, Frame& frame ) {
	const size_t lines = input.Word();
	if ( lines > frame.height ) { Invalid( "too many delta lines" ); }
	size_t y = 0;
	for ( size_t line = 0; line < lines; ++line, ++y ) {
		if ( y >= frame.height ) { Invalid( "delta line overflow" ); }
		auto control = input.Word();
		while ( control & 0x8000 ) {
			if ( control & 0x4000 ) {
				const size_t skip = 65536U - control;
				if ( skip >= frame.height - y ) { Invalid( "delta line skip overflow" ); }
				y += skip;
			}
			else { frame.pixels[ y * frame.width + frame.width - 1 ] = control & 0xff; }
			control = input.Word();
		}
		size_t x = 0;
		for ( size_t packet = 0; packet < control; ++packet ) {
			const size_t skip = input.Byte();
			if ( skip > frame.width - x ) { Invalid( "delta pixel skip overflow" ); }
			x += skip;
			const int run = SignedByte( input.Byte() );
			const size_t words = run < 0 ? -run : run;
			if ( words > ( frame.width - x ) / 2 ) { Invalid( "delta word run crosses row" ); }
			auto* output = frame.pixels.data() + y * frame.width + x;
			if ( run >= 0 ) { input.Copy( output, 2 * words ); }
			else {
				const auto first = input.Byte(), second = input.Byte();
				for ( size_t i = 0; i < words; ++i ) { output[ 2 * i ] = first; output[ 2 * i + 1 ] = second; }
			}
			x += 2 * words;
		}
	}
}

struct Chunk { size_t begin, end; uint16_t type; };
using FrameChunks = std::vector< Chunk >;

}

Frame FlicReader::ReadFrame( const std::vector< uint8_t >& bytes, size_t index, size_t max_pixels ) {
	if ( bytes.size() < 128 ) { Invalid( "truncated file header" ); }
	Cursor header( bytes, 0, 128 );
	if ( header.Dword() != bytes.size() || header.Word() != 0xaf12 ) { Invalid( "invalid AF12 envelope" ); }
	Frame result;
	result.frame_count = header.Word(); result.width = header.Word(); result.height = header.Word();
	if ( header.Word() != 8 || !result.width || !result.height || !result.frame_count || index >= result.frame_count ) {
		Invalid( "unsupported dimensions, depth or requested frame" );
	}
	const size_t width = result.width, height = result.height;
	if ( width > max_pixels / height ) { Invalid( "decoded image exceeds pixel budget" ); }
	header.Skip( 2 ); result.speed_ms = header.Dword(); header.Skip( 60 );
	const size_t first_frame = header.Dword(), second_frame = header.Dword();
	size_t offset = first_frame ? first_frame : 128;
	if ( offset < 128 || offset > bytes.size() ) { Invalid( "invalid first frame offset" ); }
	std::vector< FrameChunks > frames;
	while ( offset < bytes.size() ) {
		Cursor frame_header( bytes, offset, bytes.size() );
		const size_t length = frame_header.Dword();
		if ( length < 16 || length > bytes.size() - offset || frame_header.Word() != 0xf1fa ) { Invalid( "invalid frame envelope" ); }
		const size_t count = frame_header.Word();
		if ( count > ( length - 16 ) / 6 ) { Invalid( "invalid child chunk count" ); }
		if ( frames.size() == 1 && second_frame && second_frame != offset ) { Invalid( "invalid second frame offset" ); }
		size_t position = offset + 16;
		FrameChunks chunks;
		for ( size_t i = 0; i < count; ++i ) {
			Cursor child( bytes, position, offset + length );
			const size_t chunk_length = child.Dword();
			const auto type = child.Word();
			if ( chunk_length < 6 || chunk_length > offset + length - position ) { Invalid( "invalid child chunk envelope" ); }
			if ( type != 4 && type != 15 && type != 7 ) { Invalid( "unsupported child chunk type" ); }
			chunks.push_back( { position + 6, position + chunk_length, type } );
			position += chunk_length;
		}
		if ( offset + length - position > 1 ) { Invalid( "unexpected frame payload" ); }
		frames.push_back( std::move( chunks ) );
		if ( frames.size() > size_t( result.frame_count ) + 1 ) { Invalid( "too many physical frames" ); }
		offset += length;
	}
	if ( frames.size() < result.frame_count ) { Invalid( "missing declared frames" ); }
	result.pixels.resize( width * height );
	for ( size_t frame = 0; frame <= index; ++frame ) {
		for ( const auto& chunk : frames[ frame ] ) {
			Cursor input( bytes, chunk.begin, chunk.end );
			if ( chunk.type == 4 ) { Colors( input, result ); }
			else if ( chunk.type == 15 ) { ByteRun( input, result ); }
			else { WordDelta( input, result ); }
			input.EndPayload();
		}
	}
	return result;
}

}
