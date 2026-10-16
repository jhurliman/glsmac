#include "CvrFrameAssembler.h"

#include "loader/cvr/CvrFrameAssembler.h"
#include "task/gsetests/GSETests.h"

#include <cfenv>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace gse::tests {
namespace {

using namespace loader::cvr;

uint32_t Bits( const float value ) { uint32_t bits; std::memcpy( &bits, &value, sizeof( bits ) ); return bits; }
float Float( const uint32_t bits ) { float value; std::memcpy( &value, &bits, sizeof( value ) ); return value; }

CvrScene Scene( const size_t objects, const uint32_t frames ) {
	CvrScene scene{ "same scene name", frames, {} };
	for ( size_t object = 0 ; object < objects ; ++object ) {
		CvrSceneObject record{ "duplicate", std::vector< uint8_t >{ 8, static_cast< uint8_t >( object ) }, {} };
		for ( uint32_t frame = 0 ; frame < frames ; ++frame ) {
			CvrFrame value{ static_cast< uint8_t >( frame ? 255 : 0 ), { static_cast< float >( object ) + 0.25f, -static_cast< float >( frame ) - 0.5f, 2 }, {} };
			for ( size_t i = 0 ; i < 9 ; ++i ) { value.matrix[ i ] = static_cast< float >( object * 100 + frame * 10 + i ) + 0.125f; }
			record.frames.push_back( value );
		}
		scene.objects.push_back( std::move( record ) );
	}
	return scene;
}

template< typename F > bool Rejects( F&& action ) {
	try { action(); } catch ( const std::invalid_argument& error ) { return std::string( error.what() ).find( "CVR assembly:" ) == 0; }
	return false;
}

}

void AddCvrFrameAssemblerTests( task::gsetests::GSETests* task ) {
	task->AddTest( "CVR assembly preserves multi-scene order own modulo exact flags matrix and offsets", GT() {
		auto source = CvrFrameSource::Create( { Scene( 2, 2 ), Scene( 1, 3 ) } );
		const auto result = CvrFrameAssembler::Assemble( source, { { 0, 5, { -400, 200, 0 } }, { 1, 5, { 1.25f, -2.5f, 7.75f } }, { 0, 0, { 0, 0, 0 } } } );
		GT_ASSERT( result.records.size() == 5 && !result.first_omitted, "assembly count or completion changed" );
		const auto& first = result.records[ 0 ]; const auto& second = result.records[ 1 ]; const auto& armor = result.records[ 2 ];
		GT_ASSERT( first.selection_index == 0 && first.scene_index == 0 && first.object_index == 0 && first.frame_index == 1 &&
			second.object_index == 1 && armor.selection_index == 1 && armor.scene_index == 1 && armor.object_index == 0 && armor.frame_index == 2,
			"selection/object order or per-scene modulo changed" );
		GT_ASSERT( first.value.location == ( std::array< float, 3 >{ -399.75f, 198.5f, 2 } ) &&
			armor.value.location == ( std::array< float, 3 >{ 1.5f, -5, 9.75f } ), "explicit offsets were lost or axes changed" );
		GT_ASSERT( first.value.flags == 255 && result.records[ 3 ].value.flags == 0, "flags were interpreted as visibility or normalized" );
		for ( const auto& record : result.records ) {
			const auto& input = source->Scenes()[ record.scene_index ].objects[ record.object_index ].frames[ record.frame_index ];
			for ( size_t i = 0 ; i < 9 ; ++i ) { GT_ASSERT( Bits( record.value.matrix[ i ] ) == Bits( input.matrix[ i ] ), "matrix words reordered or transformed" ); }
		}
		const auto large = CvrFrameAssembler::Assemble( source, { { 1, std::numeric_limits< int32_t >::max(), { 0, 0, 0 } } } );
		GT_ASSERT( large.records[ 0 ].frame_index == 1, "maximum positive frame did not use scene modulo" );
		GT_OK();
	} );
	task->AddTest( "CVR assembly shares immutable owned geometry after source lifetime copy and move", GT() {
		CvrFrameAssembly result;
		std::weak_ptr< const CvrFrameSource > lifetime;
		{
			std::vector< CvrScene > scenes{ Scene( 2, 2 ) };
			auto source = CvrFrameSource::Create( scenes );
			lifetime = source;
			scenes[ 0 ].objects[ 0 ].voxel_data->clear(); scenes.clear();
			result = CvrFrameAssembler::Assemble( source, { { 0, 0, { 0, 0, 0 } } } );
			const auto another = CvrFrameAssembler::Assemble( source, { { 0, 1, { 0, 0, 0 } } } );
			GT_ASSERT( another.source == result.source, "per-frame result copied the scene storage" );
		}
		GT_ASSERT( !lifetime.expired(), "assembly failed to retain source storage" );
		auto copy = result; auto moved = std::move( copy ); result = {};
		const auto& record = moved.records[ 1 ];
		GT_ASSERT( *moved.source->Scenes()[ record.scene_index ].objects[ record.object_index ].voxel_data == ( std::vector< uint8_t >{ 8, 1 } ),
			"stable geometry identity dangles or uses duplicate display names" );
		moved = {};
		GT_ASSERT( lifetime.expired(), "unused assembly source storage leaked" );
		GT_OK();
	} );
	task->AddTest( "CVR assembly exposes original 200-record prefix capacity and first omitted object", GT() {
		auto source = CvrFrameSource::Create( { Scene( 199, 1 ), Scene( 2, 1 ), Scene( 200, 1 ) } );
		const auto partial = CvrFrameAssembler::Assemble( source, { { 0, 0, {} }, { 1, 0, {} } } );
		GT_ASSERT( partial.records.size() == 200 && partial.first_omitted && partial.first_omitted->selection_index == 1 && partial.first_omitted->object_index == 1 &&
			partial.records.back().scene_index == 1 && partial.records.back().object_index == 0, "199 plus 2 lost the partial original boundary" );
		const auto full = CvrFrameAssembler::Assemble( source, { { 2, 0, {} } } );
		GT_ASSERT( full.records.size() == 200 && !full.first_omitted, "exact capacity was incorrectly marked partial" );
		const auto stopped = CvrFrameAssembler::Assemble( source, { { 2, 0, {} }, { 1, 0, {} } } );
		GT_ASSERT( stopped.first_omitted && stopped.first_omitted->selection_index == 1 && stopped.first_omitted->object_index == 0, "full-prefix stop position changed" );
		GT_ASSERT( Rejects( [ & ]() { (void)CvrFrameAssembler::Assemble( source, { { 2, 0, {} }, { 99, 0, {} } } ); } ), "invalid late request escaped validation at capacity" );
		GT_OK();
	} );
	task->AddTest( "CVR assembly validates source shape and every frame before immutable publication", GT() {
		for ( int mode = 0 ; mode < 8 ; ++mode ) {
			auto scene = Scene( 2, 2 );
			if ( mode == 0 ) { scene.frame_count = 0; }
			if ( mode == 1 ) { scene.frame_count = std::numeric_limits< uint32_t >::max(); }
			if ( mode == 2 ) { scene.name.clear(); }
			if ( mode == 3 ) { scene.objects.clear(); }
			if ( mode == 4 ) { scene.objects.back().frames.pop_back(); }
			if ( mode == 5 ) { scene.objects.back().voxel_data->clear(); }
			if ( mode == 6 ) { scene.objects.back().frames.back().location[ 2 ] = Float( 0x7f800000 ); }
			if ( mode == 7 ) { scene.objects.back().frames.back().matrix[ 8 ] = Float( 0x7fc00001 ); }
			GT_ASSERT( Rejects( [ & ]() { (void)CvrFrameSource::Create( { scene } ); } ), "invalid source scene accepted" );
		}
		auto scene = Scene( 1, 1 ); scene.objects[ 0 ].name.clear(); scene.objects[ 0 ].voxel_data.reset();
		GT_ASSERT( CvrFrameSource::Create( { scene } )->Scenes()[ 0 ].objects[ 0 ].name.empty(), "valid empty object name or animation-only source rejected" );
		GT_OK();
	} );
	task->AddTest( "CVR assembly rejects unsupported arithmetic and requests including beyond capacity", GT() {
		auto source = CvrFrameSource::Create( { Scene( 1, 1 ) } );
		GT_ASSERT( Rejects( []() { (void)CvrFrameAssembler::Assemble( nullptr, {} ); } ), "null source accepted" );
		for ( const auto selection : { CvrFrameSelection{ 1, 0, {} }, CvrFrameSelection{ 0, -1, {} } } ) {
			GT_ASSERT( Rejects( [ & ]() { (void)CvrFrameAssembler::Assemble( source, { selection } ); } ), "invalid selection accepted" );
		}
		for ( const uint32_t bits : { 1u, 0x80000001u, 0x7f800000u, 0xff800000u, 0x7fc00001u } ) {
			GT_ASSERT( Rejects( [ & ]() { (void)CvrFrameAssembler::Assemble( source, { { 0, 0, { Float( bits ), 0, 0 } } } ); } ), "unsupported offset accepted" );
		}
		for ( int mode = 0 ; mode < 3 ; ++mode ) {
			auto scene = Scene( 1, 1 );
			scene.objects[ 0 ].frames[ 0 ].location[ 0 ] = mode == 0 ? Float( 1 ) : mode == 1 ? Float( 0x7f7fffff ) : Float( 0x00800001 );
			auto input = CvrFrameSource::Create( { Scene( 200, 1 ), scene } );
			const float offset = mode == 1 ? Float( 0x7f7fffff ) : mode == 2 ? -Float( 0x00800000 ) : 0;
			GT_ASSERT( Rejects( [ & ]() { (void)CvrFrameAssembler::Assemble( input, { { 0, 0, {} }, { 1, 0, { offset, 0, 0 } } } ); } ), "unsupported late arithmetic accepted" );
		}
		const auto previous = std::fegetround();
		const auto changed = std::fesetround( FE_DOWNWARD );
		const bool rejected = Rejects( [ & ]() { (void)CvrFrameAssembler::Assemble( source, { { 0, 0, {} } } ); } );
		const auto restored = std::fesetround( previous );
		GT_ASSERT( changed == 0 && restored == 0 && rejected, "unsupported rounding accepted or test environment not restored" );
		GT_OK();
	} );
	task->AddTest( "CVR assembly preserves matrix endpoint bits signed zero and nearest-even additions", GT() {
		auto scene = Scene( 1, 1 ); auto& frame = scene.objects[ 0 ].frames[ 0 ];
		frame.matrix = { Float( 0x80000000 ), Float( 1 ), Float( 0x80000001 ), Float( 0x7f7fffff ), Float( 0xff7fffff ), 1, 2, 3, 4 };
		frame.location = { 1, Float( 0x3f800001 ), -0.0f };
		const auto source = CvrFrameSource::Create( { scene } );
		const auto result = CvrFrameAssembler::Assemble( source, { { 0, 0, { Float( 0x33800000 ), Float( 0x33800000 ), -0.0f } } } );
		const auto& value = result.records[ 0 ].value;
		GT_ASSERT( Bits( value.location[ 0 ] ) == 0x3f800000 && Bits( value.location[ 1 ] ) == 0x3f800002 && Bits( value.location[ 2 ] ) == 0x80000000,
			"binary32 ties-to-even or signed-zero addition changed" );
		for ( size_t i = 0 ; i < 9 ; ++i ) { GT_ASSERT( Bits( value.matrix[ i ] ) == Bits( frame.matrix[ i ] ), "matrix bits were normalized" ); }
		const auto empty = CvrFrameAssembler::Assemble( CvrFrameSource::Create( {} ), {} );
		GT_ASSERT( empty.records.empty() && !empty.first_omitted, "empty explicit request was not empty" );
		GT_OK();
	} );
}

}
