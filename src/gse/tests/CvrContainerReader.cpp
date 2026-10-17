#include "CvrContainerReader.h"

#include "loader/cvr/CvrContainerReader.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace gse::tests {
namespace {

using loader::cvr::CvrContainer;
using loader::cvr::CvrContainerReader;

void PutLE32( std::string& bytes, const size_t offset, const uint32_t value ) {
	for ( size_t i = 0 ; i < 4 ; ++i ) { bytes[ offset + i ] = static_cast< char >( value >> ( 8 * i ) ); }
}

std::string Encode( const CvrContainer& source ) {
	std::string bytes( "CVR\0\0\0\0\0", 8 );
	for ( const auto& chunk : source.chunks ) {
		const auto start = bytes.size();
		bytes.resize( start + 8 );
		for ( size_t i = 0 ; i < 4 ; ++i ) { bytes[ start + i ] = static_cast< char >( chunk.tag >> ( 8 * ( 3 - i ) ) ); }
		PutLE32( bytes, start + 4, static_cast< uint32_t >( chunk.payload.size() + 8 ) );
		bytes.append( chunk.payload.begin(), chunk.payload.end() );
	}
	PutLE32( bytes, 4, static_cast< uint32_t >( bytes.size() ) );
	return bytes;
}

CvrContainer Fixture() {
	// Synthetic bytes only. In particular tag 2 need not be a filename, and
	// tags 3/4 deliberately contain NUL and high-bit bytes, not original art.
	return { { { { 1, { 0x9a, 0x99, 0x21, 0x41 } }, { 2, { 'n', 0, 0xff } },
		{ 3, { 0, 1, 0x80, 0xff, 4 } }, { 4, { 0xfe, 0x00, 0x7f } } } } };
}

bool Equal( const CvrContainer& a, const CvrContainer& b ) {
	for ( size_t i = 0 ; i < a.chunks.size() ; ++i ) {
		if ( a.chunks[ i ].tag != b.chunks[ i ].tag || a.chunks[ i ].payload != b.chunks[ i ].payload ) { return false; }
	}
	return true;
}

bool Rejected( const std::string& bytes ) {
	try { (void)CvrContainerReader::Read( bytes ); }
	catch ( const std::invalid_argument& error ) { return std::string( error.what() ).find( "CVR container:" ) == 0; }
	return false;
}

std::array< size_t, 4 > Offsets( const CvrContainer& source ) {
	std::array< size_t, 4 > result;
	size_t offset = 8;
	for ( size_t i = 0 ; i < result.size() ; ++i ) { result[ i ] = offset; offset += source.chunks[ i ].payload.size() + 8; }
	return result;
}

}

void AddCvrContainerReaderTests( task::gsetests::GSETests* task ) {
	task->AddTest( "CVR container preserves mixed-endian envelopes and opaque payload bytes", GT() {
		auto source = Fixture();
		for ( const auto word : { std::vector< uint8_t >{ 0x9a, 0x99, 0x21, 0x41 },
			std::vector< uint8_t >{ 0, 0, 0x20, 0x41 }, std::vector< uint8_t >{ 0xff, 0, 0x80, 0 } } ) {
			source.chunks[ 0 ].payload = word;
			GT_ASSERT( Equal( CvrContainerReader::Read( Encode( source ) ), source ), "opaque bytes or endian interpretation changed" );
		}
		for ( const size_t size : { size_t( 1 ), size_t( 256 ), size_t( 65536 ) } ) {
			source.chunks[ 3 ].payload.assign( size, 0x80 );
			GT_ASSERT( Equal( CvrContainerReader::Read( Encode( source ) ), source ), "multi-byte chunk length changed" );
		}
		GT_OK();
	} );
	task->AddTest( "CVR container owns independent payloads after source destruction copy and move", GT() {
		const auto expected = Fixture();
		CvrContainer parsed;
		{
			auto bytes = Encode( expected );
			const auto before = bytes;
			parsed = CvrContainerReader::Read( bytes );
			GT_ASSERT( bytes == before, "reader modified source bytes" );
			bytes.assign( bytes.size(), 'x' );
		}
		GT_ASSERT( Equal( parsed, expected ), "payload refers to discarded source memory" );
		auto copy = parsed;
		parsed.chunks[ 0 ].payload[ 0 ] ^= 0xff;
		parsed.chunks[ 3 ].payload.clear();
		auto moved = std::move( copy );
		GT_ASSERT( Equal( moved, expected ), "copy or move lost independently owned payloads" );
		GT_OK();
	} );
	task->AddTest( "CVR container rejects invalid signature file size and every truncated prefix", GT() {
		const auto valid = Encode( Fixture() );
		GT_ASSERT( Rejected( {} ), "empty input accepted" );
		for ( size_t end = 0 ; end < valid.size() ; ++end ) {
			auto bytes = valid.substr( 0, end );
			GT_ASSERT( Rejected( bytes ), "truncation accepted with original declared length" );
			if ( end >= 8 ) {
				PutLE32( bytes, 4, static_cast< uint32_t >( end ) );
				GT_ASSERT( Rejected( bytes ), "truncation accepted with matching declared length" );
			}
		}
		for ( size_t i = 0 ; i < 4 ; ++i ) {
			auto bytes = valid; bytes[ i ] ^= 1;
			GT_ASSERT( Rejected( bytes ), "invalid magic accepted" );
		}
		for ( const uint32_t length : { uint32_t( 0 ), uint32_t( valid.size() - 1 ), uint32_t( valid.size() + 1 ), std::numeric_limits< uint32_t >::max() } ) {
			auto bytes = valid; PutLE32( bytes, 4, length );
			GT_ASSERT( Rejected( bytes ), "wrong declared length accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "CVR container rejects chunk length underflow overflow empty and wrong metadata width", GT() {
		const auto source = Fixture();
		const auto valid = Encode( source );
		for ( const auto offset : Offsets( source ) ) {
			for ( const uint32_t length : { uint32_t( 0 ), uint32_t( 7 ), uint32_t( 8 ), uint32_t( valid.size() - offset + 1 ), std::numeric_limits< uint32_t >::max() } ) {
				auto bytes = valid; PutLE32( bytes, offset + 4, length );
				GT_ASSERT( Rejected( bytes ), "invalid chunk length accepted" );
			}
		}
		for ( const size_t size : { size_t( 0 ), size_t( 1 ), size_t( 3 ), size_t( 5 ) } ) {
			auto wrong = source; wrong.chunks[ 0 ].payload.resize( size );
			GT_ASSERT( Rejected( Encode( wrong ) ), "tag 1 width outside supported profile accepted" );
		}
		for ( size_t i = 1 ; i < source.chunks.size() ; ++i ) {
			auto wrong = source; wrong.chunks[ i ].payload.clear();
			GT_ASSERT( Rejected( Encode( wrong ) ), "empty payload outside supported profile accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "CVR container rejects unknown duplicate reordered and extra chunks", GT() {
		const auto source = Fixture();
		for ( size_t i = 0 ; i < source.chunks.size() ; ++i ) {
			for ( const uint32_t tag : { uint32_t( 0 ), uint32_t( 5 ), std::numeric_limits< uint32_t >::max(), uint32_t( ( i + 1 ) << 24 ) } ) {
				auto wrong = source; wrong.chunks[ i ].tag = tag;
				GT_ASSERT( Rejected( Encode( wrong ) ), "unsupported or byte-swapped tag accepted" );
			}
			for ( size_t j = 0 ; j < source.chunks.size() ; ++j ) {
				if ( i == j ) { continue; }
				auto wrong = source; wrong.chunks[ i ].tag = wrong.chunks[ j ].tag;
				GT_ASSERT( Rejected( Encode( wrong ) ), "duplicate tag accepted" );
				wrong = source; std::swap( wrong.chunks[ i ], wrong.chunks[ j ] );
				GT_ASSERT( Rejected( Encode( wrong ) ), "reordered chunks accepted" );
			}
		}
		for ( const auto suffix : { std::string( 1, '\0' ), std::string( "\0\0\0\5\11\0\0\0x", 9 ) } ) {
			auto bytes = Encode( source ) + suffix;
			PutLE32( bytes, 4, static_cast< uint32_t >( bytes.size() ) );
			GT_ASSERT( Rejected( bytes ), "trailing bytes or fifth chunk accepted" );
		}
		GT_OK();
	} );
}

}
