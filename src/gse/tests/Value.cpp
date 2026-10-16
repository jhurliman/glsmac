#include "Value.h"

#include "task/gsetests/GSETests.h"
#include "gse/GSE.h"
#include "gse/ExecutionPointer.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Float.h"
#include "gse/value/Int.h"
#include "gse/value/Null.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include "gc/Space.h"
#include "types/Buffer.h"

#include <limits>

namespace gse::tests {

static std::string Reject( GSE_CALLABLE, types::Buffer& buffer, const std::string& reason ) {
	try {
		Value::Deserialize( GSE_CALL, &buffer, nullptr );
	}
	catch ( const std::runtime_error& error ) {
		GT_ASSERT( std::string( error.what() ).find( reason ) != std::string::npos, "unexpected value rejection: " + std::string( error.what() ) );
		GT_OK();
	}
	catch ( const std::exception& error ) {
		GT_FAIL( std::string( "value decoder reached a container/allocation failure: " ) + error.what() );
	}
	GT_FAIL( "invalid encoded value was accepted" );
}

void AddValueTests( task::gsetests::GSETests* task ) {
	task->AddTest( "test encoded value tags cannot narrow into supported types", GT() {
		auto* gc_space = gse->GetGCSpace();
		const si_t si;
		ExecutionPointer ep;
		std::string error;
		gc_space->Accumulate( nullptr, [ & ]() {
			for ( const int64_t tag : { int64_t{ -256 }, int64_t{ -1 }, int64_t{ VT_NOTHING }, int64_t{ VT_CALLABLE }, int64_t{ 256 }, int64_t{ 256 + VT_INT }, std::numeric_limits< int64_t >::max() } ) {
				types::Buffer buffer;
				buffer.WriteInt( tag );
				buffer.WriteInt( 42 ); // makes the narrowed Int tag otherwise valid
				error = Reject( GSE_CALL, buffer, "invalid/unsupported type for unserialization" );
				if ( !error.empty() ) { return; }
			}
		} );
		GT_ASSERT( error.empty(), "value type validation failed: " + error );
		GT_OK();
	} );
	task->AddTest( "test encoded collection counts are checked before allocation or children", GT() {
		auto* gc_space = gse->GetGCSpace();
		const si_t si;
		ExecutionPointer ep;
		std::string error;
		gc_space->Accumulate( nullptr, [ & ]() {
			for ( const auto type : { VT_ARRAY, VT_OBJECT } ) {
				for ( const int64_t count : { int64_t{ -1 }, std::numeric_limits< int64_t >::min(), int64_t{ 1 }, std::numeric_limits< int64_t >::max() } ) {
					types::Buffer buffer;
					buffer.WriteInt( type );
					if ( type == VT_OBJECT ) { buffer.WriteString( "" ); }
					buffer.WriteInt( count );
					error = Reject( GSE_CALL, buffer, "invalid collection count" );
					if ( !error.empty() ) { return; }
				}
			}
		} );
		GT_ASSERT( error.empty(), "value count validation failed: " + error );
		GT_OK();
	} );
	task->AddTest( "test custom value decoding requires a known class and a game", GT() {
		auto* gc_space = gse->GetGCSpace();
		const si_t si;
		ExecutionPointer ep;
		std::string error;
		gc_space->Accumulate( nullptr, [ & ]() {
			for ( const std::string name : { "UnknownClass", "Player", "Tile", "Base", "Pop", "Unit" } ) {
				types::Buffer buffer;
				buffer.WriteInt( VT_OBJECT );
				buffer.WriteString( name );
				error = Reject( GSE_CALL, buffer, name == "UnknownClass" ? "custom object deserializer not found" : "game not available" );
				if ( !error.empty() ) { return; }
			}
		} );
		GT_ASSERT( error.empty(), "custom value validation failed: " + error );
		GT_OK();
	} );
	task->AddTest( "test valid nested values preserve their wire format and following record", GT() {
		auto* gc_space = gse->GetGCSpace();
		const si_t si;
		ExecutionPointer ep;
		std::string error;
		gc_space->Accumulate( nullptr, [ & ]() {
			error = [ & ]() -> std::string {
				auto* empty_array = VALUE( value::Array );
				auto* empty_object = VALUEEXT( value::Object, GSE_CALL );
				value::array_elements_t elements = {
					VALUE( value::Undefined ), VALUE( value::Null ), VALUE( value::Bool,, true ),
					VALUE( value::Int,, std::numeric_limits< int64_t >::min() ), VALUE( value::Float,, 1.25f ),
					VALUE( value::String,, std::string( "a\0b", 3 ) ), empty_array, empty_object
				};
				value::object_properties_t properties = { { "items", VALUE( value::Array,, elements ) } };
				auto* original = VALUEEXT( value::Object, GSE_CALL, properties );
				types::Buffer encoded;
				Value::Serialize( &encoded, original );
				const auto bytes = encoded.ToString();
				encoded.WriteInt( 71 );
				auto* decoded = Value::Deserialize( GSE_CALL, &encoded, nullptr );
				types::Buffer repeated;
				Value::Serialize( &repeated, decoded );
				GT_ASSERT( repeated.ToString() == bytes, "valid nested value wire format changed" );
				GT_ASSERT( encoded.ReadInt() == 71 && encoded.lenr == encoded.lenw, "value decoder consumed the following record" );
				types::Buffer null_pointer;
				Value::Serialize( &null_pointer, nullptr );
				GT_ASSERT( Value::Deserialize( GSE_CALL, &null_pointer, nullptr ) == nullptr, "null pointer encoding changed" );
				GT_OK();
			}();
		} );
		GT_ASSERT( error.empty(), "value roundtrip failed: " + error );
		GT_OK();
	} );
	task->AddTest( "test partially decoded children remain GC owned after failure", GT() {
		auto* gc_space = gse->GetGCSpace();
		const si_t si;
		ExecutionPointer ep;
		for ( const auto type : { VT_ARRAY, VT_OBJECT } ) {
			types::Buffer buffer;
			buffer.WriteInt( type );
			if ( type == VT_OBJECT ) { buffer.WriteString( "" ); }
			buffer.WriteInt( 2 );
			if ( type == VT_OBJECT ) { buffer.WriteString( "first" ); }
			buffer.WriteInt( VT_STRING );
			buffer.WriteString( std::string( 256, 'x' ) );
			if ( type == VT_OBJECT ) { buffer.WriteString( "second" ); }
			buffer.WriteInt( VT_INT ); // payload is missing, after allocating the first child
			bool rejected = false;
			try {
				gc_space->Accumulate( nullptr, [ & ]() { Value::Deserialize( GSE_CALL, &buffer, nullptr ); } );
			}
			catch ( const std::runtime_error& ) { rejected = true; }
			GT_ASSERT( rejected && !gc_space->IsAccumulating(), "partial value failure escaped its accumulation scope" );
		}
		// Subsequent allocations must remain valid; the harness destroys this GSE
		// after the test, including all completed but unpublished decoded children.
		gc_space->Accumulate( nullptr, [ & ]() { VALUE( value::String,, "after failure" ); } );
		GT_OK();
	} );
}

}
