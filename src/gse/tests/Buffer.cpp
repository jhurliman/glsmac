#include "Buffer.h"

#include "task/gsetests/GSETests.h"
#include "types/Buffer.h"

namespace gse::tests {

void AddBufferTests( task::gsetests::GSETests* task ) {
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
