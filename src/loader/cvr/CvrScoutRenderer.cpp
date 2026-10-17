#include "CvrScoutRenderer.h"

#include "CvrFrameAssembler.h"
#include "CvrBasisConverter.h"
#include "CvrPaletteReader.h"
#include "ScoutArithmetic.h"
#include "ScoutLighting.h"
#include "ScoutView.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace loader::cvr {

struct CvrScoutRenderer::Assets {
	std::shared_ptr< const CvrFrameSource > frames;
	std::array< std::vector< CvrGeometry >, 2 > geometry;
	std::array< CvrPalette, 2 > palettes;
	scout::LightingTable lighting;
};

CvrScoutRenderer::CvrScoutRenderer( std::shared_ptr< const Assets > assets ) : m_assets( std::move( assets ) ) {}

CvrScoutRenderer CvrScoutRenderer::Load( const std::string_view infantry, const std::string_view reactor ) {
	constexpr size_t MAX_ASSET_BYTES = 32 * 1024 * 1024;
	if ( infantry.size() > MAX_ASSET_BYTES || reactor.size() > MAX_ASSET_BYTES ) {
		throw std::invalid_argument( "CVR Scout: asset exceeds adapter budget" );
	}
	auto owned = std::make_shared< Assets >();
	std::vector< CvrScene > scenes;
	size_t object_count = 0, group_count = 0, record_count = 0;
	for ( const auto bytes : { infantry, reactor } ) {
		const auto index = scenes.size();
		const auto container = CvrContainerReader::Read( bytes );
		owned->palettes[ index ] = CvrPaletteReader::Read( container );
		auto scene = CvrSceneReader::Read( container );
		if ( scene.objects.size() > CvrFrameAssembler::CAPACITY - object_count ) {
			throw std::invalid_argument( "CVR Scout: complete composition exceeds frame budget" );
		}
		object_count += scene.objects.size();
		for ( const auto& object : scene.objects ) {
			if ( !object.voxel_data || object.frames.empty() || object.frames[ 0 ].flags != 0 ) {
				throw std::invalid_argument( "CVR Scout: missing geometry or unsupported idle frame" );
			}
			auto geometry = CvrGeometryReader::Read( *object.voxel_data );
			if ( ( geometry.flags != 8 && geometry.flags != 10 ) || geometry.groups.size() > CvrIndexedRasterizer::MAX_DRAWS - group_count ) {
				throw std::invalid_argument( "CVR Scout: unsupported geometry flags or excessive groups" );
			}
			group_count += geometry.groups.size();
			for ( const auto& group : geometry.groups ) {
				if ( ( group.header && group.header->flags != 0 ) || group.records.size() > CvrIndexedRasterizer::MAX_RECORDS - record_count ) {
					throw std::invalid_argument( "CVR Scout: unsupported group flags or excessive records" );
				}
				record_count += group.records.size();
			}
			owned->geometry[ index ].push_back( std::move( geometry ) );
		}
		scenes.push_back( std::move( scene ) );
	}
	owned->frames = CvrFrameSource::Create( std::move( scenes ) );
	owned->lighting = scout::GenerateLightingTable();
	return CvrScoutRenderer( std::move( owned ) );
}

namespace {

template< size_t N > std::array< uint32_t, N > Bits( const std::array< float, N >& values ) {
	std::array< uint32_t, N > bits;
	static_assert( sizeof( float ) == sizeof( uint32_t ) );
	std::memcpy( bits.data(), values.data(), sizeof( bits ) );
	return bits;
}

}

CvrScoutImage CvrScoutRenderer::Render( const CvrScoutViewInput& input ) const {
	using namespace scout;
	const auto view = MakeView( input );
	const auto frames = CvrFrameAssembler::Assemble( m_assets->frames,
		{ { 0, 0, { -400, 200, 0 } }, { 1, 0, { -400, 200, 0 } } } );
	if ( frames.first_omitted ) { throw std::invalid_argument( "CVR Scout: incomplete frame assembly" ); }
	CvrScoutImage result{ { 256, 256, 256, std::vector< uint8_t >( 256 * 256, 9 ),
		std::vector< uint16_t >( 256 * 256, 65535 ) }, {}, view.target_anchor, 0, 0, 0 };
	std::array< CvrPalette::Color, 256 > colors{};
	std::array< bool, 256 > defined{};
	// Scene palette installation is ordered like the composition. No RGB is
	// invented for indices outside the supplied scene ranges.
	for ( const auto& palette : m_assets->palettes ) {
		for ( size_t i = palette.first; i < palette.end; ++i ) { colors[ i ] = palette.colors[ i ]; defined[ i ] = true; }
	}
	for ( const auto& frame : frames.records ) {
		const auto& geometry = m_assets->geometry[ frame.scene_index ][ frame.object_index ];
		auto matrix = MultiplyMatrix( view.projection, Bits( frame.value.matrix ) );
		for ( auto& component : matrix ) { component = Store( Multiply( Decode( component ), Decode( geometry.scalar_bits ) ) ); }
		const auto location = AddVector( MultiplyVector( view.projection, Bits( frame.value.location ) ), view.translation );
		const auto basis = CvrBasisConverter::Convert( matrix );
		// Caller footprint uses geometry scalar * stored map scale, before any
		// binary32 spill. The product of these binary32 values is exact here.
		const auto footprint = ToDouble( Multiply( Decode( geometry.scalar_bits ), Decode( view.scales[ 0 ] ) ) ) <= 0.5 ? CvrSampleFootprint::Single : CvrSampleFootprint::Square2;
		const auto light = SelectLighting( { matrix, view.auxiliary, view.light, geometry.scalar_bits, view.scales } );
		CvrIndexedLookups lookups;
		std::copy_n( m_assets->lighting[ light.row ].begin() + light.offset, lookups.first.size(), lookups.first.begin() );
		const auto& shade = m_assets->palettes[ frame.scene_index ].shades;
		lookups.second.assign( shade.begin(), shade.end() );
		for ( const auto& group : geometry.groups ) {
			Vector header;
			for ( size_t i = 0; i < 3; ++i ) {
				// Grouped origins use first+fourth vectors, not the simple header.
				const int32_t component = group.header ? int32_t( group.header->vectors[ 0 ][ i ] ) + group.header->vectors[ 3 ][ i ] : geometry.vectors[ 2 ][ i ];
				header[ i ] = Store( Integer( component ) );
			}
			const auto origin_bits = AddVector( MultiplyVector( matrix, header ), location );
			// The converter's signed32 row applies the same *256, absolute
			// half-away rounding as the original origin fragment.
			Matrix origin_input{};
			std::copy( origin_bits.begin(), origin_bits.end(), origin_input.begin() + 6 );
			const auto origin = CvrBasisConverter::Convert( origin_input )[ 2 ];
			auto drawn = CvrIndexedRasterizer::DrawModuloDepth( result.indexed, lookups,
				{ { &group.records, true, footprint, basis, origin } } );
			result.indexed = std::move( drawn.surface );
			++result.draw_count;
			result.processed_records += drawn.processed_records;
			result.written_samples += drawn.written_samples;
		}
	}
	result.rgba.reserve( result.indexed.color.size() * 4 );
	for ( const auto index : result.indexed.color ) {
		if ( !defined[ index ] ) { throw std::invalid_argument( "CVR Scout: output index has no supplied RGB entry" ); }
		result.rgba.insert( result.rgba.end(), colors[ index ].begin(), colors[ index ].end() );
		result.rgba.push_back( index == 9 ? 0 : 255 );
	}
	return result;
}

}
