#include "Buffer.h"

#include "task/gsetests/GSETests.h"
#include "types/Buffer.h"

namespace gse::tests {

void AddBufferTests( task::gsetests::GSETests* task ) {
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
