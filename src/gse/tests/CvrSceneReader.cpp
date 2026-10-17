#include "CvrSceneReader.h"

#include "loader/cvr/CvrSceneReader.h"
#include "task/gsetests/GSETests.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace gse::tests {
namespace {

using namespace loader::cvr;
using Bytes = std::vector< uint8_t >;
struct Node { uint32_t tag; Bytes bytes; std::vector< Node > children; };

Bytes Word( const uint32_t value ) {
	return { static_cast< uint8_t >( value ), static_cast< uint8_t >( value >> 8 ),
		static_cast< uint8_t >( value >> 16 ), static_cast< uint8_t >( value >> 24 ) };
}

uint32_t Bits( const float value ) {
	uint32_t result;
	std::memcpy( &result, &value, sizeof( result ) );
	return result;
}

void Append( Bytes& destination, const Bytes& source ) { destination.insert( destination.end(), source.begin(), source.end() ); }

Bytes Encode( const Node& node ) {
	Bytes payload = node.bytes;
	for ( const auto& child : node.children ) { Append( payload, Encode( child ) ); }
	Bytes result{ static_cast< uint8_t >( node.tag >> 24 ), static_cast< uint8_t >( node.tag >> 16 ),
		static_cast< uint8_t >( node.tag >> 8 ), static_cast< uint8_t >( node.tag ) };
	Append( result, Word( static_cast< uint32_t >( payload.size() + 8 ) ) );
	Append( result, payload );
	return result;
}

Node Leaf( const uint32_t tag, Bytes bytes ) { return { tag, std::move( bytes ), {} }; }
Node Branch( const uint32_t tag, std::vector< Node > children ) { return { tag, {}, std::move( children ) }; }

CvrScene Expected() {
	CvrFrame first{ 0x80, { 1.25f, -2.5f, -0.0f }, { 1, 2, 3, 4, 5, 6, 7, 8, 9 } };
	CvrFrame second{ 0xff, { -100, 0.5f, 7.75f }, { -1, -2, -3, -4, -5, -6, -7, -8, -9 } };
	CvrFrame third{ 0, { -1.25f, 2.5f, 0 }, { 10, 20, 30, 40, 50, 60, 70, 80, 90 } };
	CvrFrame fourth{ 0x7f, { 10, 20, 30 }, { -10, -20, -30, -40, -50, -60, -70, -80, -90 } };
	return { "synthetic scene", 2, {
		{ std::string( "part\0name", 9 ), Bytes{ 0xff, 0, 0x01, 0x02, 0x04, 0x04, 0x80 }, { first, second } },
		{ std::string( "part\0name", 9 ), std::nullopt, { third, fourth } }
	} };
}

Node Fixture() {
	const auto expected = Expected();
	Node scene = Branch( 4, { Leaf( 0x104, Bytes( expected.name.begin(), expected.name.end() ) ),
		Leaf( 0x204, Word( 2 ) ), Leaf( 0x304, Word( 2 ) ) } );
	for ( const auto& object : expected.objects ) {
		Node record = Branch( 0x404, { Leaf( 0x10404, Bytes( object.name.begin(), object.name.end() ) ) } );
		if ( object.voxel_data ) { record.children.push_back( Branch( 0x20404, { Leaf( 0x01020404, *object.voxel_data ) } ) ); }
		Bytes flags, locations, matrices;
		for ( const auto& frame : object.frames ) {
			flags.push_back( frame.flags );
			for ( const auto value : frame.location ) { Append( locations, Word( Bits( value ) ) ); }
			for ( const auto value : frame.matrix ) { Append( matrices, Word( Bits( value ) ) ); }
		}
		record.children.push_back( Branch( 0x30404, { Leaf( 0x01030404, flags ), Leaf( 0x02030404, locations ), Leaf( 0x03030404, matrices ) } ) );
		scene.children.push_back( std::move( record ) );
	}
	return scene;
}

CvrContainer Container( const Node& scene ) {
	const auto encoded = Encode( scene );
	return { { { { 1, { 0, 0, 0x20, 0x41 } }, { 2, { 'd' } }, { 3, { 0xff } },
		{ 4, Bytes( encoded.begin() + 8, encoded.end() ) } } } };
}

Node& At( Node& root, const std::vector< size_t >& path ) {
	auto* node = &root;
	for ( const auto index : path ) { node = &node->children.at( index ); }
	return *node;
}

bool Rejected( const CvrContainer& container ) {
	try { (void)CvrSceneReader::Read( container ); }
	catch ( const std::invalid_argument& error ) { return std::string( error.what() ).find( "CVR scene:" ) == 0; }
	return false;
}

bool Equal( const CvrScene& a, const CvrScene& b ) {
	if ( a.name != b.name || a.frame_count != b.frame_count || a.objects.size() != b.objects.size() ) { return false; }
	for ( size_t i = 0 ; i < a.objects.size() ; ++i ) {
		const auto& x = a.objects[ i ]; const auto& y = b.objects[ i ];
		if ( x.name != y.name || x.voxel_data != y.voxel_data || x.frames.size() != y.frames.size() ) { return false; }
		for ( size_t j = 0 ; j < x.frames.size() ; ++j ) {
			if ( x.frames[ j ].flags != y.frames[ j ].flags ) { return false; }
			for ( size_t k = 0 ; k < 3 ; ++k ) { if ( Bits( x.frames[ j ].location[ k ] ) != Bits( y.frames[ j ].location[ k ] ) ) { return false; } }
			for ( size_t k = 0 ; k < 9 ; ++k ) { if ( Bits( x.frames[ j ].matrix[ k ] ) != Bits( y.frames[ j ].matrix[ k ] ) ) { return false; } }
		}
	}
	return true;
}

void HeaderOffsets( const Node& node, const size_t offset, std::vector< size_t >& result ) {
	result.push_back( offset );
	auto child_offset = offset + 8 + node.bytes.size();
	for ( const auto& child : node.children ) { HeaderOffsets( child, child_offset, result ); child_offset += Encode( child ).size(); }
}

}

void AddCvrSceneReaderTests( task::gsetests::GSETests* task ) {
	task->AddTest( "CVR scene preserves object order duplicate names opaque geometry and distinct frames", GT() {
		auto tree = Fixture();
		// An explicit little-endian bit pattern independently pins the first value.
		auto& locations = At( tree, { 3, 2, 1 } ).bytes;
		locations[ 0 ] = 0; locations[ 1 ] = 0; locations[ 2 ] = 0xa0; locations[ 3 ] = 0x3f;
		const auto container = Container( tree );
		GT_ASSERT( Equal( CvrSceneReader::Read( container ), Expected() ), "scene metadata, source order or frame component bits changed" );
		Node file{ 0x43565200, {}, {} };
		for ( const auto& chunk : container.chunks ) { file.children.push_back( Leaf( chunk.tag, chunk.payload ) ); }
		const auto bytes = Encode( file );
		const auto decoded = CvrContainerReader::Read( std::string_view( reinterpret_cast< const char* >( bytes.data() ), bytes.size() ) );
		GT_ASSERT( Equal( CvrSceneReader::Read( decoded ), Expected() ), "container-to-scene decoding changed values" );
		// A shipped record has an empty object name; names are not identity keys.
		At( tree, { 3, 0 } ).bytes.clear(); At( tree, { 4, 0 } ).bytes.clear();
		auto expected = Expected(); expected.objects[ 0 ].name.clear(); expected.objects[ 1 ].name.clear();
		GT_ASSERT( !Rejected( Container( tree ) ), "valid empty duplicate object names rejected" );
		GT_ASSERT( Equal( CvrSceneReader::Read( Container( tree ) ), expected ), "empty duplicate object names changed scene records" );
		tree.children[ 0 ].bytes.clear();
		GT_ASSERT( Rejected( Container( tree ) ), "unsupported empty scene name accepted" );
		GT_OK();
	} );
	task->AddTest( "CVR scene owns data after input destruction copy and move", GT() {
		CvrScene result;
		{
			auto input = Container( Fixture() );
			const auto before = input.chunks[ 3 ].payload;
			result = CvrSceneReader::Read( input );
			GT_ASSERT( input.chunks[ 3 ].payload == before, "decoder mutated input" );
			input.chunks[ 3 ].payload.assign( 100, 0xff );
		}
		auto copy = result;
		result.name.clear(); result.objects[ 0 ].name.clear(); result.objects[ 0 ].voxel_data->clear();
		result.objects[ 0 ].frames[ 1 ].matrix[ 8 ] = 0;
		auto moved = std::move( copy );
		GT_ASSERT( Equal( moved, Expected() ), "owned names, geometry or frames alias discarded or copied source" );
		GT_OK();
	} );
	task->AddTest( "CVR scene validates directly constructed public containers", GT() {
		const auto valid = Container( Fixture() );
		for ( size_t i = 0 ; i < 4 ; ++i ) {
			auto input = valid; input.chunks[ i ].tag = 0;
			GT_ASSERT( Rejected( input ), "invalid public container tag accepted" );
			input = valid; input.chunks[ i ].payload.clear();
			GT_ASSERT( Rejected( input ), "empty public container payload accepted" );
		}
		for ( const size_t width : { size_t( 1 ), size_t( 3 ), size_t( 5 ) } ) {
			auto input = valid; input.chunks[ 0 ].payload.resize( width );
			GT_ASSERT( Rejected( input ), "invalid public container metadata width accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "CVR scene rejects inconsistent hostile and malformed counts", GT() {
		for ( const size_t index : { size_t( 1 ), size_t( 2 ) } ) {
			for ( const uint32_t count : { uint32_t( 0 ), uint32_t( 1 ), uint32_t( 3 ), uint32_t( 0x40000000 ), std::numeric_limits< uint32_t >::max() } ) {
				auto tree = Fixture(); tree.children[ index ].bytes = Word( count );
				GT_ASSERT( Rejected( Container( tree ) ), "inconsistent or hostile count accepted" );
			}
			for ( const size_t width : { size_t( 0 ), size_t( 1 ), size_t( 3 ), size_t( 5 ), size_t( 8 ) } ) {
				auto tree = Fixture(); tree.children[ index ].bytes.resize( width );
				GT_ASSERT( Rejected( Container( tree ) ), "malformed count width accepted" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "CVR scene rejects missing duplicate reordered unknown and extra nested fields", GT() {
		const std::vector< std::vector< size_t > > branches{ {}, { 3 }, { 3, 1 }, { 3, 2 }, { 4 }, { 4, 1 } };
		for ( const auto& path : branches ) {
			auto baseline = Fixture(); const auto children = At( baseline, path ).children.size();
			for ( size_t i = 0 ; i < children ; ++i ) {
				auto tree = baseline; auto& branch = At( tree, path ); branch.children[ i ].tag = 0xdeadbeef;
				GT_ASSERT( Rejected( Container( tree ) ), "unknown nested field accepted" );
				tree = baseline; At( tree, path ).children.erase( At( tree, path ).children.begin() + i );
				// Removing the optional geometry field is valid.
				if ( !( path == std::vector< size_t >{ 3 } && i == 1 ) ) {
					GT_ASSERT( Rejected( Container( tree ) ), "missing required field accepted" );
				}
				tree = baseline; At( tree, path ).children.push_back( At( baseline, path ).children[ i ] );
				GT_ASSERT( Rejected( Container( tree ) ), "duplicate or extra nested field accepted" );
			}
			if ( children > 1 ) {
				auto tree = baseline; auto& values = At( tree, path ).children; std::swap( values[ 0 ], values[ 1 ] );
				GT_ASSERT( Rejected( Container( tree ) ), "reordered required fields accepted" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "CVR scene rejects every truncated prefix and hostile nested lengths", GT() {
		const auto tree = Fixture(); const auto valid = Container( tree );
		for ( size_t size = 0 ; size < valid.chunks[ 3 ].payload.size() ; ++size ) {
			auto input = valid; input.chunks[ 3 ].payload.resize( size );
			GT_ASSERT( Rejected( input ), "truncated scene prefix accepted" );
		}
		std::vector< size_t > offsets; HeaderOffsets( tree, 0, offsets );
		for ( const auto offset : offsets ) {
			if ( !offset ) { continue; } // The public container already separated the root header.
			for ( const uint32_t size : { uint32_t( 0 ), uint32_t( 7 ), uint32_t( 8 ), std::numeric_limits< uint32_t >::max() } ) {
				auto input = valid; const auto word = Word( size );
				std::copy( word.begin(), word.end(), input.chunks[ 3 ].payload.begin() + offset - 8 + 4 );
				GT_ASSERT( Rejected( input ), "hostile nested chunk length accepted" );
			}
		}
		auto input = valid; input.chunks[ 3 ].payload.push_back( 0 );
		GT_ASSERT( Rejected( input ), "trailing scene byte accepted" );
		GT_OK();
	} );
	task->AddTest( "CVR scene rejects invalid frame array widths and nonfinite components", GT() {
		for ( const size_t object : { size_t( 3 ), size_t( 4 ) } ) {
			const size_t animation = object == 3 ? 2 : 1;
			for ( size_t field = 0 ; field < 3 ; ++field ) {
				const std::vector< size_t > path{ object, animation, field };
				for ( const int delta : { -1, 1, 4 } ) {
					auto tree = Fixture(); auto& bytes = At( tree, path ).bytes;
					bytes.resize( delta < 0 ? bytes.size() - 1 : bytes.size() + delta );
					GT_ASSERT( Rejected( Container( tree ) ), "frame array length mismatch accepted" );
				}
				if ( !field ) { continue; }
				for ( const uint32_t bits : { uint32_t( 0x7f800000 ), uint32_t( 0xff800000 ), uint32_t( 0x7fc00001 ), uint32_t( 0xffc00001 ) } ) {
					auto tree = Fixture(); auto& bytes = At( tree, path ).bytes; const auto word = Word( bits );
					std::copy( word.begin(), word.end(), bytes.end() - 4 );
					GT_ASSERT( Rejected( Container( tree ) ), "nonfinite late transform component accepted" );
				}
			}
		}
		GT_OK();
	} );
	task->AddTest( "CVR scene retains finite endpoint bits and animation-only records", GT() {
		for ( const uint32_t bits : { uint32_t( 0 ), uint32_t( 0x80000000 ), uint32_t( 1 ), uint32_t( 0x80000001 ),
			uint32_t( 0x7f7fffff ), uint32_t( 0xff7fffff ) } ) {
			auto tree = Fixture(); const auto word = Word( bits );
			std::copy( word.begin(), word.end(), At( tree, { 4, 1, 1 } ).bytes.end() - 4 );
			std::copy( word.begin(), word.end(), At( tree, { 3, 2, 2 } ).bytes.begin() );
			const auto scene = CvrSceneReader::Read( Container( tree ) );
			GT_ASSERT( Bits( scene.objects[ 1 ].frames[ 1 ].location[ 2 ] ) == bits && Bits( scene.objects[ 0 ].frames[ 0 ].matrix[ 0 ] ) == bits,
				"finite transform component was normalized or reordered" );
		}
		auto tree = Fixture(); tree.children.erase( tree.children.begin() + 3 ); tree.children[ 1 ].bytes = Word( 1 );
		const auto scene = CvrSceneReader::Read( Container( tree ) );
		GT_ASSERT( scene.objects.size() == 1 && !scene.objects[ 0 ].voxel_data && scene.objects[ 0 ].frames.size() == 2,
			"animation-only scene lost its object or gained geometry" );
		GT_OK();
	} );
}

}
