#include "CvrFrameAssembler.h"

#include <algorithm>
#include <cfenv>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

#if defined( __SSE__ ) || defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 1 )
#include <xmmintrin.h>
#endif

namespace loader::cvr {
namespace {

static_assert( sizeof( float ) == sizeof( uint32_t ) && std::numeric_limits< float >::is_iec559 &&
	std::numeric_limits< double >::digits >= 53, "CVR assembly requires binary32 and binary64 arithmetic" );

[[noreturn]] void Invalid( const char* const message ) {
	throw std::invalid_argument( std::string( "CVR assembly: " ) + message );
}

uint32_t Bits( const float value ) {
	uint32_t bits;
	std::memcpy( &bits, &value, sizeof( bits ) );
	return bits;
}

bool Finite( const float value ) { return ( Bits( value ) & 0x7f800000u ) != 0x7f800000u; }

void Operand( const float value ) {
	const auto magnitude = Bits( value ) & 0x7fffffffu;
	if ( magnitude >= 0x7f800000u || ( magnitude && magnitude < 0x00800000u ) ) {
		Invalid( "location arithmetic requires finite normal-or-zero operands" );
	}
}

float Add( const float a, const float b ) {
	Operand( a ); Operand( b );
	// The sum of two binary32 values has sufficient precision in binary64 for
	// nearest binary32 rounding. This also avoids host subnormal float arithmetic.
	const double sum = static_cast< double >( a ) + static_cast< double >( b );
	const double magnitude = std::abs( sum );
	if ( magnitude > std::numeric_limits< float >::max() ||
		( magnitude != 0 && magnitude < std::numeric_limits< float >::min() ) ) {
		Invalid( "location sum outside the supported normal-or-zero range" );
	}
	return static_cast< float >( sum );
}

std::array< float, 3 > Location( const CvrFrame& frame, const CvrFrameSelection& selection ) {
	std::array< float, 3 > result;
	for ( size_t i = 0 ; i < result.size() ; ++i ) { result[ i ] = Add( frame.location[ i ], selection.offset[ i ] ); }
	return result;
}

}

CvrFrameSource::CvrFrameSource( std::vector< CvrScene > scenes ) : m_scenes( std::move( scenes ) ) {}

std::shared_ptr< const CvrFrameSource > CvrFrameSource::Create( std::vector< CvrScene > scenes ) {
	for ( const auto& scene : scenes ) {
		if ( scene.name.empty() || !scene.frame_count || scene.frame_count > static_cast< uint32_t >( std::numeric_limits< int32_t >::max() ) || scene.objects.empty() ) {
			Invalid( "source scene requires a name and positive supported object/frame counts" );
		}
		for ( const auto& object : scene.objects ) {
			if ( object.frames.size() != scene.frame_count || ( object.voxel_data && object.voxel_data->empty() ) ) {
				Invalid( "source object has inconsistent frames or empty geometry" );
			}
			for ( const auto& frame : object.frames ) {
				for ( const auto value : frame.location ) { if ( !Finite( value ) ) { Invalid( "nonfinite source location" ); } }
				for ( const auto value : frame.matrix ) { if ( !Finite( value ) ) { Invalid( "nonfinite source matrix" ); } }
			}
		}
	}
	return std::shared_ptr< const CvrFrameSource >( new CvrFrameSource( std::move( scenes ) ) );
}

CvrFrameAssembly CvrFrameAssembler::Assemble( std::shared_ptr< const CvrFrameSource > source,
	const std::vector< CvrFrameSelection >& selections ) {
	if ( !source ) { Invalid( "source is null" ); }
	if ( std::fegetround() != FE_TONEAREST ) { Invalid( "nearest floating-point rounding is required" ); }
#if defined( __SSE__ ) || defined( _M_X64 ) || ( defined( _M_IX86_FP ) && _M_IX86_FP >= 1 )
	// fegetround can report x87 while Add/conversion use SSE. Both controls
	// must admit nearest; the caller must not change them during assembly.
	if ( ( _mm_getcsr() & _MM_ROUND_MASK ) != _MM_ROUND_NEAREST ) { Invalid( "nearest SSE rounding is required" ); }
#endif
	size_t count = 0;
	for ( const auto& selection : selections ) {
		if ( selection.scene_index >= source->Scenes().size() || selection.frame < 0 ) { Invalid( "invalid scene index or negative frame" ); }
		const auto& scene = source->Scenes()[ selection.scene_index ];
		const auto frame = static_cast< uint32_t >( selection.frame ) % scene.frame_count;
		for ( const auto value : selection.offset ) { Operand( value ); }
		for ( const auto& object : scene.objects ) { (void)Location( object.frames[ frame ], selection ); }
		count += std::min( CAPACITY - count, scene.objects.size() );
	}
	CvrFrameAssembly result{ std::move( source ), {}, std::nullopt };
	result.records.reserve( count );
	for ( size_t selection_index = 0 ; selection_index < selections.size() ; ++selection_index ) {
		const auto& selection = selections[ selection_index ];
		const auto& scene = result.source->Scenes()[ selection.scene_index ];
		const auto frame_index = static_cast< uint32_t >( selection.frame ) % scene.frame_count;
		for ( size_t object_index = 0 ; object_index < scene.objects.size() ; ++object_index ) {
			if ( result.records.size() == CAPACITY ) {
				result.first_omitted = CvrAssemblyPosition{ selection_index, object_index };
				return result;
			}
			const auto& frame = scene.objects[ object_index ].frames[ frame_index ];
			CvrAssembledFrame record{ selection_index, selection.scene_index, object_index, frame_index, frame };
			record.value.location = Location( frame, selection );
			result.records.push_back( record );
		}
	}
	return result;
}

}
