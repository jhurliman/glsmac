#include "Buffer.h"

#include "task/gsetests/GSETests.h"
#include "types/Buffer.h"

#include <cstring>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

namespace gse::tests {

static std::string ExpectReadFailure( types::Buffer& buffer, const std::function< void() >& read, const std::string& reason ) {
	const auto* cursor = buffer.dr;
	const auto offset = buffer.lenr;
	const auto bytes = buffer.ToString();
	bool rejected = false;
	try {
		read();
	}
	catch ( const std::runtime_error& error ) {
		rejected = true;
		GT_ASSERT( std::string( error.what() ).find( reason ) != std::string::npos, "unexpected buffer rejection: " + std::string( error.what() ) );
	}
	GT_ASSERT( rejected, "malformed buffer read was accepted" );
	GT_ASSERT( buffer.dr == cursor && buffer.lenr == offset, "rejected read changed the cursor" );
	GT_ASSERT( buffer.ToString() == bytes, "rejected read modified the source bytes" );
	GT_OK();
}

static void AddFramedReadTests( task::gsetests::GSETests* task ) {
	task->AddTest(
		"test truncated and corrupt frames preserve the cursor and caller output",
		GT() {
			types::Buffer prefix;
			prefix.WriteInt( 29 );
			types::Buffer record;
			record.WriteColor( types::Color( 0.1f, 0.2f, 0.3f, 0.4f ) );
			const auto bytes = record.ToString();
			std::vector< std::pair< std::string, std::string > > cases;
			// Every incomplete prefix covers the header, payload, and checksum.
			for ( size_t size = 0 ; size < bytes.size() ; ++size ) {
				cases.emplace_back( bytes.substr( 0, size ), "prematurely" );
			}
			auto wrong_type = bytes;
			wrong_type.front() = static_cast< char >( 255 );
			cases.emplace_back( wrong_type, "unexpected type" );
			auto wrong_checksum = bytes;
			wrong_checksum.back() ^= 1;
			cases.emplace_back( wrong_checksum, "checksum mismatch" );
			auto wrong_size = bytes;
			const uint32_t short_size = sizeof( types::Color::color_t ) - 1;
			std::memcpy( wrong_size.data() + 1, &short_size, sizeof( short_size ) );
			cases.emplace_back( wrong_size, "size mismatch" );
			for ( const auto& test : cases ) {
				types::Buffer input( prefix.ToString() + test.first );
				GT_ASSERT( input.ReadInt() == 29, "prefix changed" );
				types::Color output( 0.9f, 0.8f, 0.7f, 0.6f );
				const auto before = output.value;
				const auto error = ExpectReadFailure( input, [ & ]() { input.ReadColor( output ); }, test.second );
				GT_ASSERT( error.empty(), "frame rejection failed: " + error );
				GT_ASSERT( std::memcmp( &output.value, &before, sizeof( before ) ) == 0, "rejected frame changed caller output" );
			}
			GT_OK();
		}
	);
	task->AddTest(
		"test oversized serialized lengths are rejected before allocation",
		GT() {
			for ( const bool is_data : { false, true } ) {
				for ( const uint32_t size : { uint32_t{ 2 }, std::numeric_limits< uint32_t >::max() - 1, std::numeric_limits< uint32_t >::max() } ) {
					types::Buffer original;
					if ( is_data ) { original.WriteData( "x", 1 ); }
					else { original.WriteString( "x" ); }
					auto bytes = original.ToString();
					std::memcpy( bytes.data() + 1, &size, sizeof( size ) );
					types::Buffer input( bytes );
					const auto error = ExpectReadFailure( input, [ & ]() {
						if ( is_data ) {
							const auto* data = input.ReadData( size );
							free( const_cast< void* >( data ) );
						}
						else { input.ReadString(); }
					}, "prematurely" );
					GT_ASSERT( error.empty(), "oversized frame rejection failed: " + error );
				}
			}
			GT_OK();
		}
	);
	task->AddTest(
		"test data reads enforce the requested size including zero",
		GT() {
			for ( const uint32_t size : { 0, 1, 3 } ) {
				types::Buffer input;
				input.WriteData( "ab", 2 );
				const auto error = ExpectReadFailure( input, [ & ]() {
					const auto* data = input.ReadData( size );
					free( const_cast< void* >( data ) );
				}, "size mismatch" );
				GT_ASSERT( error.empty(), "data size rejection failed: " + error );
			}
			GT_OK();
		}
	);
	task->AddTest(
		"test empty and nonempty framed values preserve data ownership and following records",
		GT() {
			types::Buffer input;
			input.WriteString( "" );
			input.WriteData( nullptr, 0 );
			const std::string text( "a\0b", 3 );
			input.WriteString( text );
			input.WriteData( text.data(), text.size() );
			input.WriteInt( 91 );
			const auto bytes = input.ToString();
			GT_ASSERT( input.ReadString().empty(), "empty string changed" );
			GT_ASSERT( input.ReadData( 0 ) == nullptr, "empty data should not allocate storage" );
			GT_ASSERT( input.ReadString() == text, "embedded null in string changed" );
			const auto* data = input.ReadData( text.size() );
			const bool equal = std::memcmp( data, text.data(), text.size() ) == 0;
			free( const_cast< void* >( data ) );
			GT_ASSERT( equal, "data payload changed" );
			GT_ASSERT( input.ReadInt() == 91, "following record changed" );
			GT_ASSERT( input.lenr == input.lenw && input.dr == input.dw, "successful reads did not consume exactly the records" );
			GT_ASSERT( input.ToString() == bytes, "successful reads modified the source bytes" );
			GT_OK();
		}
	);
}

void AddBufferTests( task::gsetests::GSETests* task ) {
	AddFramedReadTests( task );
	task->AddTest(
		"test buffer booleans preserve the canonical one-byte format",
		GT() {
			for ( const bool expected : { false, true } ) {
				types::Buffer buffer;
				buffer.WriteBool( expected );
				const auto bytes = buffer.ToString();
				GT_ASSERT( bytes.size() == 7, "boolean wire record size changed" );
				const auto payload = static_cast< uint8_t >( expected ? 1 : 0 );
				GT_ASSERT( static_cast< uint8_t >( bytes.at( 5 ) ) == payload, "boolean payload is not canonical" );
				GT_ASSERT( static_cast< uint8_t >( bytes.at( 6 ) ) == payload, "boolean checksum changed" );
				buffer.WriteInt( 42 );
				GT_ASSERT( buffer.ReadBool() == expected, "canonical boolean value changed" );
				GT_ASSERT( buffer.ReadInt() == 42, "boolean read consumed the following record" );
			}
			GT_OK();
		}
	);
	for ( const uint8_t payload : { 2, 255 } ) {
		task->AddTest(
			"test buffer rejects boolean payload " + std::to_string( payload ) + " with a valid checksum",
			GT( payload ) {
				types::Buffer valid;
				valid.WriteBool( false );
				auto bytes = valid.ToString();
				// A boolean record ends with its one-byte payload and XOR checksum.
				// Update both so the malformed value passes transport validation.
				bytes.at( bytes.size() - 2 ) = static_cast< char >( payload );
				bytes.back() = static_cast< char >( payload );
				types::Buffer invalid( bytes );
				try {
					invalid.ReadBool();
				}
				catch ( const std::runtime_error& error ) {
					GT_ASSERT( std::string( error.what() ).find( "invalid boolean value" ) != std::string::npos, "boolean was rejected for the wrong reason" );
					GT_OK();
				}
				GT_FAIL( "noncanonical boolean payload was accepted" );
			}
		);
	}
	task->AddTest(
		"test copied buffers preserve cursors and append into owned capacity",
		GT() {
			types::Buffer source;
			source.WriteString( "prefix" );
			source.WriteInt( 41 );
			const auto original_bytes = source.ToString();
			GT_ASSERT( source.ReadString() == "prefix", "source prefix mismatch" );
			types::Buffer copy( source );
			const std::string suffix( 300, 'x' );
			// This fits the source's spare chunk capacity but exceeds its used bytes.
			// A copy must own every byte its allocator says is available.
			copy.WriteString( suffix );
			GT_ASSERT( copy.ReadInt() == 41, "copy lost its read cursor" );
			GT_ASSERT( copy.ReadString() == suffix, "copy append changed" );
			GT_ASSERT( source.ToString() == original_bytes, "copy append modified the source" );
			GT_ASSERT( source.ReadInt() == 41, "copy read advanced the source cursor" );
			types::Buffer empty;
			types::Buffer empty_copy( empty );
			empty_copy.WriteInt( 7 );
			GT_ASSERT( empty_copy.ReadInt() == 7 && empty.ToString().empty(), "empty buffer copy is not independent" );
			GT_OK();
		}
	);
	task->AddTest(
		"test buffer assignment owns storage and preserves independent cursors",
		GT() {
			types::Buffer assigned;
			assigned.WriteString( "replaced storage" );
			{
				types::Buffer source;
				source.WriteInt( 12 );
				source.WriteString( "remaining" );
				GT_ASSERT( source.ReadInt() == 12, "source read failed" );
				assigned = source;
				assigned.WriteInt( 99 );
				GT_ASSERT( source.ReadString() == "remaining", "assignment changed source cursor" );
			}
			// Source destruction must not release the destination's storage.
			assigned = assigned;
			GT_ASSERT( assigned.ReadString() == "remaining", "assigned buffer lost read state" );
			GT_ASSERT( assigned.ReadInt() == 99, "assigned append was lost" );
			assigned = types::Buffer();
			GT_ASSERT( assigned.ToString().empty(), "empty assignment retained old data" );
			assigned.WriteString( "reused" );
			GT_ASSERT( assigned.ReadString() == "reused", "assigned empty buffer cannot be reused" );
			GT_OK();
		}
	);
}

}
