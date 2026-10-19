#include "CvrScoutRenderer.h"

#include "CvrFrameAssembler.h"
#include "CvrBasisConverter.h"
#include "CvrPaletteReader.h"
#include "CvrActorMaterial.h"
#include "ScoutArithmetic.h"
#include "ScoutLighting.h"
#include "ScoutView.h"

#include <algorithm>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>

namespace loader::cvr {

struct CvrScoutRenderer::Assets {
	explicit Assets( const Profile selected ) : profile( selected ) {}
	const Profile profile;
	std::shared_ptr< const CvrFrameSource > frames;
	using Geometry = std::variant< CvrGeometry, CvrSimpleDetailChain >;
	std::vector< std::vector< Geometry > > geometry;
	std::vector< CvrPalette > palettes;
	std::optional< std::array< CvrPalette::Color, 256 > > software_colors;
	scout::LightingTable lighting;
};

CvrScoutRenderer::CvrScoutRenderer( std::shared_ptr< const Assets > assets ) : m_assets( std::move( assets ) ) {}

CvrScoutRenderer CvrScoutRenderer::Load( const std::string_view infantry, const std::string_view reactor ) {
	return LoadParts( { infantry, reactor }, Profile::Infantry, nullptr );
}

CvrScoutRenderer CvrScoutRenderer::LoadInitialized( const std::string_view infantry, const std::string_view reactor,
	const std::string_view palette_pcx, const std::string_view system ) {
	scout::ValidateMapLight( system );
	CvrActorMaterial palette{};
	palette.colors = CvrActorMaterial::ReadSoftwareColors( palette_pcx );
	// Initialized Infantry uses only main RGB. Its two original scene shade
	// tables remain independent of every faction's actor body remap.
	return LoadParts( { infantry, reactor }, Profile::InitializedInfantry, &palette );
}

CvrScoutRenderer CvrScoutRenderer::LoadParts( const std::vector< std::string_view >& parts, const Profile profile, const CvrActorMaterial* material ) {
	constexpr size_t MAX_ASSET_BYTES = 32 * 1024 * 1024;
	size_t total_bytes = 0;
	for ( const auto bytes : parts ) {
		if ( bytes.size() > MAX_ASSET_BYTES || bytes.size() > 64 * 1024 * 1024 - total_bytes ) {
			throw std::invalid_argument( "CVR idle composition: asset exceeds adapter budget" );
		}
		total_bytes += bytes.size();
	}
	auto owned = std::make_shared< Assets >( profile );
	owned->geometry.resize( parts.size() ); owned->palettes.resize( parts.size() );
	if ( material ) { owned->software_colors = material->colors; }
	std::vector< CvrScene > scenes;
	size_t object_count = 0, group_count = 0, record_count = 0;
	for ( const auto bytes : parts ) {
		const auto index = scenes.size();
		const auto container = CvrContainerReader::Read( bytes );
		owned->palettes[ index ] = CvrPaletteReader::Read( container );
		// Fixed Speeder composition: only VGMCP and VBP use the generated actor
		// material. All other parts retain their own decoded shade lookups.
		if ( profile == Profile::SpartanSpeeder && material && ( index == 2 || index == 4 ) ) { owned->palettes[ index ].shades = material->shades; }
		auto scene = CvrSceneReader::Read( container );
		if ( scene.objects.size() > CvrFrameAssembler::CAPACITY - object_count ) {
			throw std::invalid_argument( "CVR Scout: complete composition exceeds frame budget" );
		}
		object_count += scene.objects.size();
		for ( const auto& object : scene.objects ) {
			if ( !object.voxel_data || object.frames.empty() || object.frames[ 0 ].flags != 0 ) {
				throw std::invalid_argument( "CVR Scout: missing geometry or unsupported idle frame" );
			}
			const auto account = [ & ]( const CvrGeometry& geometry ) {
				if ( geometry.groups.size() > CvrIndexedRasterizer::MAX_DRAWS - group_count ) { throw std::invalid_argument( "CVR idle composition: excessive groups" ); }
				group_count += geometry.groups.size();
				for ( const auto& group : geometry.groups ) {
					if ( ( group.header && group.header->flags != 0 ) || group.records.size() > CvrIndexedRasterizer::MAX_RECORDS - record_count ) {
						throw std::invalid_argument( "CVR idle composition: unsupported group flags or excessive records" );
					}
					record_count += group.records.size();
				}
			};
			if ( profile == Profile::SpartanSpeeder && index == 8 ) {
				// VLIGHTS retains every simple level, including continuation flags.
				// Selection happens using each draw's actual global scale.
				auto chain = CvrSimpleDetailChain::Read( *object.voxel_data );
				for ( const auto& level : chain.Levels() ) { account( level ); }
				owned->geometry[ index ].emplace_back( std::move( chain ) );
			} else {
				auto geometry = CvrGeometryReader::Read( *object.voxel_data );
				if ( geometry.flags != 8 && geometry.flags != 10 ) { throw std::invalid_argument( "CVR idle composition: unsupported geometry flags" ); }
				account( geometry );
				owned->geometry[ index ].emplace_back( std::move( geometry ) );
			}
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
	const bool speeder = m_assets->profile == Profile::SpartanSpeeder;
	const auto profile = speeder ? IdleProfile::SpartanSpeeder :
		m_assets->profile == Profile::InitializedInfantry ? IdleProfile::InitializedInfantry : IdleProfile::Infantry;
	const auto view = MakeView( input, profile );
	std::vector< CvrFrameSelection > requests;
	// Speeder assembly uses each part's asset location directly. The Infantry
	// attachment offset belongs only to its distinct VI/reactor composition.
	const std::array< float, 3 > offset = speeder ? std::array< float, 3 >{ 0, 0, 0 } : std::array< float, 3 >{ -400, 200, 0 };
	for ( size_t i = 0; i < m_assets->geometry.size(); ++i ) { requests.push_back( { i, 0, offset } ); }
	const auto frames = CvrFrameAssembler::Assemble( m_assets->frames, requests );
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
	if ( m_assets->software_colors ) { colors = *m_assets->software_colors; defined.fill( true ); }
	for ( const auto& frame : frames.records ) {
		const auto& source = m_assets->geometry[ frame.scene_index ][ frame.object_index ];
		const auto* chain = std::get_if< CvrSimpleDetailChain >( &source );
		const auto& geometry = chain ? chain->Levels()[ chain->Select( view.scales[ 0 ] ) ] : std::get< CvrGeometry >( source );
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
				{ { &group.records, ( geometry.flags & 8 ) != 0, footprint, basis, origin } } );
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


CvrShadowResult CvrScoutRenderer::RenderShadow(const scout::ShadowScene& scene) const {
    using namespace scout;
    if(m_assets->profile==Profile::SpartanSpeeder) {throw std::invalid_argument("CVR shadow: Speeder detail path is not admitted");}
    const auto view=MakeShadowView(scene);
    std::vector<CvrFrameSelection> requests;
    for(size_t i=0;i<m_assets->geometry.size();++i) {requests.push_back({i,0,{-400,200,0}});}
    const auto frames=CvrFrameAssembler::Assemble(m_assets->frames,requests);
    if(frames.first_omitted) {throw std::invalid_argument("CVR shadow: incomplete frame assembly");}
    std::vector<CvrIndexedDraw> draws;
    for(const auto& frame:frames.records) {
        const auto& geometry=std::get<CvrGeometry>(m_assets->geometry[frame.scene_index][frame.object_index]);
        auto matrix=MultiplyMatrix(view.projection,Bits(frame.value.matrix));
        for(size_t i=0;i<6;++i) {matrix[i]=Store(Multiply(Decode(matrix[i]),Decode(geometry.scalar_bits)));}
        const auto location=AddVector(MultiplyVector(view.projection,Bits(frame.value.location)),view.translation);
        auto basis=CvrBasisConverter::Convert(matrix);basis[2]={0,0,0};
        for(const auto& group:geometry.groups) {
            Vector header;
            for(size_t i=0;i<3;++i) {header[i]=Store(Integer(group.header?
                int32_t(group.header->vectors[0][i])+group.header->vectors[3][i]:geometry.vectors[2][i]));}
            const auto start=AddVector(MultiplyVector(matrix,header),location);
            Matrix origin{};std::copy(start.begin(),start.end(),origin.begin()+6);
            draws.push_back({&group.records,(geometry.flags&8)!=0,CvrSampleFootprint::Single,basis,CvrBasisConverter::Convert(origin)[2]});
        }
    }
    auto result=CvrIndexedRasterizer::Shadow(scene.width,scene.height,draws);
    result.frame_count=frames.records.size();return result;
}

}
