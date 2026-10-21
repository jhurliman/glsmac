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

CvrScoutRenderer CvrScoutRenderer::LoadWorkshop(std::string_view infantry,std::string_view armor,
    std::string_view reactor,std::string_view palette_pcx,std::string_view system) {
    scout::ValidateMapLight(system);
    CvrActorMaterial palette{};palette.colors=CvrActorMaterial::ReadGamma100Colors(palette_pcx);
    return LoadParts(armor.empty()?std::vector<std::string_view>{infantry,reactor}:
        std::vector<std::string_view>{infantry,armor,reactor},Profile::Workshop,&palette);
}

CvrScoutRenderer CvrScoutRenderer::LoadWorkshopPart(std::string_view part,std::string_view palette_pcx,
    std::string_view system,const std::array<uint8_t,24*256>* shades,bool direct_locations) {
    scout::ValidateMapLight(system);CvrActorMaterial material{};material.colors=CvrActorMaterial::ReadGamma100Colors(palette_pcx);
    if(shades) {material.shades=*shades;} else {material.shades=CvrPaletteReader::Read(CvrContainerReader::Read(part)).shades;}
    return LoadParts({part},direct_locations?Profile::WorkshopWeapon:Profile::WorkshopPart,&material);
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
        if((profile==Profile::WorkshopPart || profile==Profile::WorkshopWeapon) && material) {owned->palettes[index].shades=material->shades;}
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
	if(m_assets->profile==Profile::Workshop || m_assets->profile==Profile::WorkshopPart || m_assets->profile==Profile::WorkshopWeapon) {throw std::invalid_argument("Workshop composition requires its own view");}
	const bool speeder = m_assets->profile == Profile::SpartanSpeeder;
	const auto profile = speeder ? IdleProfile::SpartanSpeeder :
		m_assets->profile == Profile::InitializedInfantry ? IdleProfile::InitializedInfantry : IdleProfile::Infantry;
    return RenderView(MakeView(input,profile));
}

CvrScoutImage CvrScoutRenderer::RenderWorkshop(const uint32_t yaw_bits) const {
    using namespace scout;
    if(m_assets->profile!=Profile::Workshop) {throw std::invalid_argument("Workshop preview requires its assembled assets");}
    const Matrix identity{0x3f800000,0,0,0,0x3f800000,0,0,0,0x3f800000};
    return RenderView(MakeWorkshopView(WorkshopPosition(identity),yaw_bits));
}

std::array<uint32_t,3> CvrScoutRenderer::WorkshopPosition(const std::array<uint32_t,9>& rotation,const std::array<uint32_t,3>& previous) const {
    using namespace scout;
    std::vector<CvrFrameSelection> requests;
    for(size_t i=0;i<m_assets->geometry.size();++i) {requests.push_back({i,0,m_assets->profile==Profile::WorkshopWeapon ? std::array<float,3>{0,0,0}:std::array<float,3>{-400,200,0}});}
    const auto frames=CvrFrameAssembler::Assemble(m_assets->frames,requests);
    if(frames.first_omitted || frames.records.empty()) {throw std::invalid_argument("Workshop preview has incomplete frames");}
    std::optional<WorkshopBounds> scene;
    for(const auto& frame:frames.records) {
        const auto& geometry=std::get<CvrGeometry>(m_assets->geometry[frame.scene_index][frame.object_index]);
        const auto bounds=WorkshopObjectBounds({rotation,Bits(frame.value.matrix),previous,Bits(frame.value.location),
            0x3f800000,geometry.scalar_bits,geometry.vectors[0],geometry.vectors[1]});
        if(!scene) {scene=bounds;} else {
            for(size_t i=0;i<3;++i) {
                if(ToDouble(Decode(bounds.minimum[i]))<ToDouble(Decode(scene->minimum[i]))) {scene->minimum[i]=bounds.minimum[i];}
                if(ToDouble(Decode(bounds.maximum[i]))>ToDouble(Decode(scene->maximum[i]))) {scene->maximum[i]=bounds.maximum[i];}
            }
        }
    }
    Vector position;
    for(size_t i=0;i<3;++i) {
        const auto midpoint=Store(Multiply(Add(Decode(scene->minimum[i]),Decode(scene->maximum[i])),Decode(0x3f000000)));
        position[i]=Store(Subtract(Decode(previous[i]),Decode(midpoint)));
    }
    return position;
}

CvrScoutImage CvrScoutRenderer::RenderWorkshopPart(uint32_t scale,bool centered,std::array<int32_t,2> anchor,
    bool unavailable,bool translucent,std::array<uint32_t,3>& position) const {
    if(m_assets->profile!=Profile::WorkshopPart && m_assets->profile!=Profile::WorkshopWeapon) {throw std::invalid_argument("Component rendering requires a single Workshop part");}
    auto view=scout::MakeWorkshopView(position,0x3f05f92d,scale,anchor);
    if(centered) {position=WorkshopPosition(view.orientation,position);view=scout::MakeWorkshopView(position,0x3f05f92d,scale,anchor);}
    return RenderView(view,translucent && !unavailable ? 24:9,unavailable);
}

CvrScoutImage CvrScoutRenderer::RenderView(const scout::View& view,const uint8_t clear,const bool unavailable) const {
    using namespace scout;
    const bool speeder=m_assets->profile==Profile::SpartanSpeeder || m_assets->profile==Profile::WorkshopWeapon;
	std::vector< CvrFrameSelection > requests;
	// Speeder assembly uses each part's asset location directly. The Infantry
	// attachment offset belongs only to its distinct VI/reactor composition.
	const std::array< float, 3 > offset = speeder ? std::array< float, 3 >{ 0, 0, 0 } : std::array< float, 3 >{ -400, 200, 0 };
	for ( size_t i = 0; i < m_assets->geometry.size(); ++i ) { requests.push_back( { i, 0, offset } ); }
	const auto frames = CvrFrameAssembler::Assemble( m_assets->frames, requests );
	if ( frames.first_omitted ) { throw std::invalid_argument( "CVR Scout: incomplete frame assembly" ); }
	CvrScoutImage result{ { 256, 256, 256, std::vector< uint8_t >( 256 * 256, clear ),
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
        if(unavailable) {std::fill(lookups.second.begin(),lookups.second.end(),185);}
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
		result.rgba.push_back( index == clear ? 0 : 255 );
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

CvrWorkshopComponents CvrWorkshopComponents::Load(const Reader& read) {
    CvrWorkshopComponents result;const auto palette=read("palette.pcx"),system=read("system.txt");
    const auto materials=CvrActorMaterial::ReadWorkshop(palette,read("vbp.cvr"),system);
    result.m_colors=materials.colors;result.m_translucency=materials.translucency;
    const auto armor=read("vipta00.cvr");
    result.m_side[0]=CvrScoutRenderer::LoadWorkshopPart(read("vi.cvr"),palette,system);
    result.m_side[1]=CvrScoutRenderer::LoadWorkshopPart(read("vw00.cvr"),palette,system,nullptr,true);
    result.m_side[2]=CvrScoutRenderer::LoadWorkshopPart(armor,palette,system);
    result.m_side[3]=CvrScoutRenderer::LoadWorkshopPart(read("viptr00.cvr"),palette,system);
    result.m_armor[1]=result.m_side[2];
    for(size_t i=0;i<4;++i) {result.m_armor[std::array<size_t,4>{2,3,6,7}[i]]=CvrScoutRenderer::LoadWorkshopPart(armor,palette,system,&materials.armor[i]);}
    std::array<uint8_t,24*256> identity;for(size_t i=0;i<identity.size();++i) {identity[i]=uint8_t(i/256);}
    result.m_armor[5]=CvrScoutRenderer::LoadWorkshopPart(armor,palette,system,&identity);
    result.m_armor[4]=CvrScoutRenderer::LoadWorkshopPart(read("viptawal.cvr"),palette,system);
    result.m_armor[8]=CvrScoutRenderer::LoadWorkshopPart(read("viptasgn.cvr"),palette,system);
    result.m_armor[9]=CvrScoutRenderer::LoadWorkshopPart(read("viptapsi.cvr"),palette,system);
    return result;
}
std::optional<CvrScoutImage> CvrWorkshopComponents::Render(int kind,int id,bool picker,bool available,std::array<uint32_t,3>& position,int ordinal) const {
    if(kind<0 || kind>3 || id<0 || (kind==2 ? id>9:id!=0) || (picker && kind!=2)) {throw std::invalid_argument("Unsupported Workshop component card");}
    if(kind==2 && id==0) {return std::nullopt;}
    if(picker) {if(ordinal<0) {ordinal=id;}if(ordinal>=20) {throw std::invalid_argument("Workshop picker ordinal exceeds page");}
        const int x=27+108*(ordinal%5),y=4+94*(ordinal/5);
        return m_armor[id]->RenderWorkshopPart(0x3dcccccd,true,{x+52,y+45},!available,id==5,position);
    }
    const std::array<uint32_t,4> scale{0x3d4ccccd,0x3e19999a,0x3d8f5c29,0x3e4ccccd};
    const std::array<std::array<int32_t,2>,4> anchor{{{79,47},{79,143},{79,235},{511,45}}};
    return (kind==2 ? m_armor[id]:m_side[kind])->RenderWorkshopPart(scale[kind],kind!=0,anchor[kind],!available,kind==2 && id==5,position);
}
uint8_t CvrWorkshopComponents::Composite(uint8_t source,uint8_t destination) const {
    if(source==24) {return destination;}if(source>23) {throw std::invalid_argument("Invalid Workshop translucent shade index");}
    return m_translucency[size_t(source)*256+destination];
}

}
