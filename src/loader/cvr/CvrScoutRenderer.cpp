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
namespace {template<size_t N> std::array<uint32_t,N> Bits(const std::array<float,N>& values);}

struct CvrScoutRenderer::Assets {
	explicit Assets( const Profile selected ) : profile( selected ) {}
	const Profile profile;
	std::shared_ptr< const CvrFrameSource > frames;
	using Geometry = std::variant< CvrGeometry, CvrSimpleDetailChain >;
	std::vector< std::vector< Geometry > > geometry;
	std::vector< CvrPalette > palettes;
    std::vector<CvrFrameSelection> selections;
    int workshop_weapon_mode=0;
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

CvrScoutRenderer CvrScoutRenderer::LoadWorkshopDesign(const WorkshopReader& read,std::string_view faction_pcx,
    int chassis,int weapon,int armor,int reactor,int kind) {
    if(kind< -1 || kind>3 || !((chassis==0 && weapon>=0 && weapon<=1 && armor>=0 && armor<=3 && reactor>=1 && reactor<=2)
        || (chassis==1 && weapon==2 && armor==1 && reactor==1))) {
        throw std::invalid_argument("Workshop composition requires admitted conventional components");
    }
    return LoadWorkshopDesign(read,faction_pcx,chassis,weapon,armor,reactor,kind,{weapon==2 ? 4:weapon==1 ? 2:1,0,armor==0 ? 1:armor+1});
}

CvrScoutRenderer CvrScoutRenderer::LoadWorkshopDesign(const WorkshopReader& read,std::string_view faction_pcx,
    int chassis,int weapon,int armor,int reactor,int kind,const WorkshopDefinition& definition) {
    return LoadDesign(read,faction_pcx,chassis,weapon,armor,reactor,kind,definition,kind<0 ? Profile::Workshop:Profile::WorkshopPart);
}
CvrScoutRenderer CvrScoutRenderer::LoadStripDesign(const WorkshopReader& read,std::string_view faction_pcx,
    int chassis,int weapon,int armor,int reactor,const WorkshopDefinition& definition) {
    // Original4d8420 selects this before the later Probe body remap and does
    // not consult armor or weapon mode. Preserve the stored component IDs.
    const auto profile=chassis==0 ? ((weapon>=0 && weapon<=2) || weapon==18 ? Profile::StripInfantry:Profile::StripSmallInfantry)
        :chassis==1 && weapon==18 ? Profile::StripInfantry:Profile::StripBody;
    return LoadDesign(read,faction_pcx,chassis,weapon,armor,reactor,-1,definition,profile);
}
CvrScoutRenderer CvrScoutRenderer::LoadDesign(const WorkshopReader& read,std::string_view faction_pcx,
    int chassis,int weapon,int armor,int reactor,int kind,const WorkshopDefinition& definition,Profile profile) {
    if(chassis<0 || chassis>8 || weapon<0 || weapon>20 || armor<0 || armor>9 || reactor<1 || reactor>4 || kind< -1 || kind>3
        || !((definition.weapon_mode>=0 && definition.weapon_mode<=2) || (definition.weapon_mode>=7 && definition.weapon_mode<=12))) {
        throw std::invalid_argument("Workshop composition requires stock component identities and supported modes");
    }
    // The original temporarily remaps this Probe design for all four composers.
    if(weapon==18 && chassis==1 && definition.armor_strength<=1) {chassis=0;}
    // The live weapon side-card caller substitutes chassis-1 for Probe when
    // its ordinary-unit cache flag is zero. Whole-unit composition does not.
    if(kind==1 && weapon==18 && definition.independent_probe) {chassis=-1;}
    const auto palette=read("palette.pcx"),system=read("system.txt");scout::ValidateMapLight(system);
    const auto materials=CvrActorMaterial::ReadWorkshop(palette,read("vbp.cvr"),system);
    const auto shades=CvrPaletteReader::Read(CvrContainerReader::Read(read("vbp.cvr"))).shades;
    auto faction=CvrActorMaterial::Read(palette,faction_pcx,shades);faction.colors=materials.colors;
    faction.shades=CvrActorMaterial::Tint(faction.colors,shades,faction.tint_index);
    std::vector<std::string> bytes;std::vector<WorkshopAttachment> attachments;
    const std::array<float,3> infantry{-400,200,0},zero{};
    const auto append=[&](const std::string& file,const std::array<uint8_t,24*256>* tint=nullptr,
        std::array<float,3> offset={},bool direct=true) {
        bytes.push_back(read(file));attachments.push_back({offset,direct,tint ? std::optional<std::array<uint8_t,24*256>>(*tint):std::nullopt});
    };
    const auto mount=[&](const std::string& name) {
        const auto marker=CvrSceneReader::Read(CvrContainerReader::Read(read(name)));
        if(marker.frame_count!=2 || marker.objects.empty()) {throw std::invalid_argument("Workshop mount requires two source frames");}
        std::array<float,3> result;
        for(size_t i=0;i<3;++i) {
            const auto difference=scout::Store(scout::Subtract(scout::Decode(Bits(marker.objects[0].frames[1].location)[i]),scout::Decode(Bits(marker.objects[0].frames[0].location)[i])));
            std::memcpy(&result[i],&difference,sizeof(difference));
        }return result;
    };
    const auto number=[](const std::string& prefix,int index) {return prefix+(index<10 ? "0":"")+std::to_string(index)+".cvr";};
    const bool infantry_actor=chassis==0 && (definition.weapon_mode<3 || definition.weapon_mode==11);
    const bool body_armored=armor==1 || armor==2 || armor==3 || armor==4 || armor==5 || armor==6 || armor==7 || armor==8;
    const auto base=[&] {append("vb.cvr");append("vbp.cvr",&faction.shades);};
    const auto gunbase=[&] {append("vgmc.cvr");append("vgmcp.cvr",&faction.shades);};
    const auto powerbase=[&] {append(number("vrcp",reactor-1));};
    const auto body=[&] {
        switch(chassis) {
        case 0:
            if(definition.weapon_mode<3) {if(weapon!=14) {append("vi.cvr",nullptr,infantry,false);}}
            else if(weapon==18) {append("vpt.cvr",nullptr,infantry,false);}
            else if(weapon!=19) {base();append("vcl.cvr");append("vclt00.cvr");append("vlights.cvr");}break;
        case 1:powerbase();gunbase();base();append("vsp.cvr");append("vsptb.cvr");append("vsptf.cvr");append("vlights.cvr");break;
        case 2:
            append("vb.cvr");for(const auto* name:{"vht-vbp.cvr","vhtp.cvr","vhttp.cvr"}) {append(name,&faction.shades);}
            if(!body_armored) {append("vhta00.cvr");append("vhtta00.cvr");}
            append("vlights.cvr",nullptr,mount("avht-lightsreposition.cvr"),false);break;
        case 3:powerbase();gunbase();base();append("vfl.cvr");break;
        case 4:base();gunbase();append("vcu.cvr");append("vcup.cvr",&faction.shades);append("vcuw.cvr");if(!body_armored) {append("vcua00.cvr");}break;
        case 5:powerbase();base();gunbase();append("vjtp.cvr",&faction.shades);if(!body_armored) {append("vjt00.cvr");}break;
        case 6:powerbase();base();gunbase();append("vct.cvr");append("vctp.cvr",&faction.shades);append("vctb.cvr");if(!body_armored) {append("vct00.cvr",&faction.shades);}break;
        case 7:powerbase();base();gunbase();append("vgs.cvr");append("vgsp.cvr",&faction.shades);break;
        case 8:append(weapon==13 ? "vw13.cvr":"vm.cvr");base();break;
        }
    };
    const auto gun=[&] {
        if(kind==1 && weapon==0) {append("vw00.cvr");return;}
        if(chassis==8) {return;}
        if(weapon==13 || weapon==20) {append(weapon==13 ? "vw13.cvr":"vm.cvr");return;}
        if(definition.weapon_mode<3) {
            constexpr std::array<int,25> lookup{-1,-1,0,0,1,3,2,2,4,4,5,5,6,7,7,7,8,8,8,8,9,9,9,9,10};
            const auto power=definition.weapon_power;
            const auto n=power==-1 ? 11:power==99 ? 12:lookup[std::clamp(power,0,24)];
            if(n<0) {if(kind==1 && weapon==0) {append("vw00.cvr");}return;}
            const auto name=number("vw",n+1);
            if(chassis==0) {append(name,nullptr,mount("avgmtgun.cvr"),false);append("vgmt.cvr");append("vgmtp.cvr",&faction.shades);}
            else if(chassis==2) {append(name,nullptr,mount("avht-gunreposition.cvr"),false);}else {append(name);}return;
        }
        switch(definition.weapon_mode) {
        case 7:append("vwntt.cvr");break;
        case 8:if(chassis==0) {append("drop.cvr");append("droplet.cvr");}else {append("droplet.cvr",nullptr,mount("adroper.cvr"),false);}break;
        case 9:if(chassis==0) {append("vt.cvr");}append("vwntu.cvr");break;
        case 10:append("vwnst.cvr");break;
        case 11:if(chassis!=0) {append("ptmod.cvr");}break;
        case 12:append("vwnaa.cvr");break;
        }
    };
    const auto shield=[&] {
        const bool ordinary=armor==1 || armor==2 || armor==3 || armor==6 || armor==7;
        const bool structure=armor==4 || armor==8 || armor==9 || armor==5;
        const auto* tint=armor==2 ? &materials.armor[0]:armor==3 ? &materials.armor[1]:armor==6 ? &materials.armor[2]:armor==7 ? &materials.armor[3]:nullptr;
        if(armor==4 || armor==8 || armor==9) {
            if(infantry_actor) {append(armor==4 ? "viptawal.cvr":armor==8 ? "viptasgn.cvr":"viptapsi.cvr",nullptr,infantry,false);}
            else {append("a.cvr");append(armor==4 ? "apwall.cvr":armor==8 ? "asgen.cvr":"apsid.cvr");}
        }
        switch(chassis) {
        case 0:if(ordinary) {append(infantry_actor ? "vipta00.cvr":"va01.cvr",tint,infantry_actor ? infantry:zero,!infantry_actor);}break;
        case 1:if(ordinary) {append("va01.cvr",tint);append("vspa01.cvr",tint);}break;
        case 2:if(structure) {append("vhta00.cvr");append("vhtta00.cvr");}else if(ordinary) {append("vhta01.cvr",tint);append("vhtta01.cvr",tint);}break;
        case 3:case 7:if(ordinary) {append("va01.cvr",tint);}break;
        case 4:if(structure && armor!=5) {append("vcua00.cvr");}else if(ordinary) {append("vcua01.cvr",tint);}break;
        case 5:if(structure && armor!=5) {append("vjt00.cvr");}else if(ordinary) {append("va01.cvr",tint);append("vjt01.cvr",tint);}break;
        case 6:if(structure && armor!=5) {append("vct00.cvr");append("va01.cvr");}else if(ordinary) {append("vct01.cvr",tint);append("va01.cvr",tint);}break;
        }
    };
    const auto power=[&] {append(number(infantry_actor ? "viptr":chassis==8 ? "vpbr":"vr",reactor-1),nullptr,infantry_actor ? infantry:zero,!infantry_actor);};
    if(kind<0) {body();gun();shield();power();}else {
        switch(kind) {case 0:body();break;case 1:gun();break;case 2:shield();break;case 3:power();break;}
    }
    if(bytes.empty() && kind<0) {throw std::invalid_argument("Workshop unit has no model");}
    std::vector<std::string_view> parts;for(const auto& part:bytes) {parts.push_back(part);}
    return LoadParts(parts,profile,&faction,&attachments,definition.weapon_mode);
}

size_t CvrStripImage::Composite(std::vector<uint8_t>& target,uint32_t width,uint32_t height,uint32_t pitch,
    const std::array<uint8_t,256>& lookup,const std::array<int32_t,2>& origin,const std::array<int32_t,4>& clip) const {
    const auto& source=body.indexed;
    if(&target==&source.color || !source.width || !source.height || source.pitch<source.width
        || uint64_t(source.pitch)*source.height!=source.color.size() || source.color.size()>CvrIndexedRasterizer::MAX_SURFACE_SAMPLES
        || source.width!=shadow.width || source.height!=shadow.height) {throw std::invalid_argument("Invalid saved-strip color source");}
    // Blend validates every target/mask/clip bound before its first write. No
    // remaining color-copy operation can allocate or fail after this boundary.
    size_t stores=shadow.Blend(target,width,height,pitch,lookup,origin,clip);
    for(uint32_t y=0;y<source.height;++y) {for(uint32_t x=0;x<source.width;++x) {
        const int64_t px=int64_t(origin[0])+x,py=int64_t(origin[1])+y;const auto color=source.color[size_t(y)*source.pitch+x];
        if(color!=9 && px>=clip[0] && px<clip[2] && py>=clip[1] && py<clip[3]) {target[size_t(py)*pitch+size_t(px)]=color;++stores;}
    }}return stores;
}

CvrStripImage CvrScoutRenderer::RenderStrip() const {
    if(m_assets->profile!=Profile::StripInfantry && m_assets->profile!=Profile::StripBody
        && m_assets->profile!=Profile::StripSmallInfantry) {throw std::invalid_argument("Saved-strip rendering requires its ordinary scene");}
    const CvrScoutViewInput input{2,-10,0,1,{42,207}};
    const auto view=scout::MakeView(input,m_assets->profile==Profile::StripInfantry ? scout::IdleProfile::InitializedInfantry:
        m_assets->profile==Profile::StripSmallInfantry ? scout::IdleProfile::InitializedSmallInfantry:scout::IdleProfile::SpartanSpeeder);
    auto scene=scout::FlatScoutShadowScene(input);scene.auxiliary_scale=view.scales[0];
    return {RenderView(view),RenderShadow(scene)};
}

CvrScoutImage CvrScoutRenderer::RenderWorkshopCard(int kind,std::array<uint32_t,3>& position) const {
    if(kind<0 || kind>3 || m_assets->profile!=Profile::WorkshopPart) {throw std::invalid_argument("Workshop card requires its component scene");}
    const std::array<uint32_t,4> scale{0x3d4ccccd,m_assets->workshop_weapon_mode<3 ? 0x3e19999au:m_assets->workshop_weapon_mode==9 ? 0x3d75c28fu:0x3da3d70au,0x3d8f5c29,0x3e4ccccd};
    const std::array<std::array<int32_t,2>,4> anchor{{{79,47},{79,143},{79,235},{511,45}}};
    auto view=scout::MakeWorkshopView(position,0x3f05f92d,scale[kind],anchor[kind]);
    if(kind!=0) {position=WorkshopPosition(view.orientation,position);view=scout::MakeWorkshopView(position,0x3f05f92d,scale[kind],anchor[kind]);}
    return RenderView(view,9,false,kind==1 ? std::optional<std::array<int32_t,4>>({79,86,178,171}):std::nullopt);
}

CvrScoutImage CvrScoutRenderer::RenderWorkshopPicker(int kind,int ordinal,bool available,std::array<uint32_t,3>& position) const {
    if((kind!=0 && kind!=3) || ordinal<0 || ordinal>=20 || m_assets->profile!=Profile::WorkshopPart) {
        throw std::invalid_argument("Unsupported Workshop chassis/reactor picker scene");
    }
    return RenderWorkshopPart(kind==0 ? 0x3d75c28f:0x3e4ccccd,true,
        {79+108*(ordinal%5),49+94*(ordinal/5)},!available,false,position);
}

CvrScoutRenderer CvrScoutRenderer::LoadParts( const std::vector< std::string_view >& parts, const Profile profile, const CvrActorMaterial* material, const std::vector<WorkshopAttachment>* attachments, int workshop_weapon_mode ) {
	constexpr size_t MAX_ASSET_BYTES = 32 * 1024 * 1024;
	size_t total_bytes = 0;
    if(attachments && attachments->size()!=parts.size()) {throw std::invalid_argument("Workshop attachment extent mismatch");}
	for ( const auto bytes : parts ) {
		if ( bytes.size() > MAX_ASSET_BYTES || bytes.size() > 64 * 1024 * 1024 - total_bytes ) {
			throw std::invalid_argument( "CVR idle composition: asset exceeds adapter budget" );
		}
		total_bytes += bytes.size();
	}
	auto owned = std::make_shared< Assets >( profile );
    owned->workshop_weapon_mode=workshop_weapon_mode;
	owned->geometry.resize( parts.size() ); owned->palettes.resize( parts.size() );
	if ( material ) { owned->software_colors = material->colors; }
	std::vector< CvrScene > scenes;
	size_t object_count = 0, group_count = 0, record_count = 0;
	for ( const auto bytes : parts ) {
		const auto index = scenes.size();
        const auto* attachment=attachments ? &attachments->at(index):nullptr;
        const bool direct=attachment ? attachment->direct:profile==Profile::SpartanSpeeder || profile==Profile::WorkshopWeapon;
        owned->selections.push_back({index,0,attachment ? attachment->offset:direct ? std::array<float,3>{0,0,0}:std::array<float,3>{-400,200,0},attachment && direct});
		const auto container = CvrContainerReader::Read( bytes );
		owned->palettes[ index ] = CvrPaletteReader::Read( container );
        if((profile==Profile::WorkshopPart || profile==Profile::WorkshopWeapon) && material && !attachments) {owned->palettes[index].shades=material->shades;}
		// Fixed Speeder composition: only VGMCP and VBP use the generated actor
		// material. All other parts retain their own decoded shade lookups.
		if ( profile == Profile::SpartanSpeeder && material && ( index == 2 || index == 4 ) ) { owned->palettes[ index ].shades = material->shades; }
		if(attachment && attachment->shades) {owned->palettes[index].shades=*attachment->shades;}
        auto scene = CvrSceneReader::Read( container );
		if ( scene.objects.size() > CvrFrameAssembler::CAPACITY - object_count ) {
			throw std::invalid_argument( "CVR Scout: complete composition exceeds frame budget" );
		}
		object_count += scene.objects.size();
		for ( const auto& object : scene.objects ) {
			if ( !object.voxel_data || object.frames.empty() || (object.frames[0].flags!=0 && !(attachments && object.frames[0].flags==0x80)) ) {
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
			if ( (profile == Profile::SpartanSpeeder && index == 8) || (attachments && (!(object.voxel_data->at(0)&10) || (object.voxel_data->at(0)&1))) ) {
				// VLIGHTS retains every simple level, including continuation flags.
				// Selection happens using each draw's actual global scale.
				auto chain = CvrSimpleDetailChain::Read( *object.voxel_data );
				for ( const auto& level : chain.Levels() ) { account( level ); }
				owned->geometry[ index ].emplace_back( std::move( chain ) );
			} else {
				auto geometry = CvrGeometryReader::Read( *object.voxel_data );
				if ( geometry.flags != 8 && geometry.flags != 10 && !(attachments && geometry.flags==2) ) { throw std::invalid_argument( "CVR idle composition: unsupported geometry flags" ); }
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
	if(m_assets->profile==Profile::Workshop || m_assets->profile==Profile::WorkshopPart || m_assets->profile==Profile::WorkshopWeapon
        || m_assets->profile==Profile::StripInfantry || m_assets->profile==Profile::StripBody
        || m_assets->profile==Profile::StripSmallInfantry) {throw std::invalid_argument("Dialog composition requires its own view");}
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
    const auto frames=CvrFrameAssembler::Assemble(m_assets->frames,m_assets->selections);
    if(frames.first_omitted) {throw std::invalid_argument("Workshop preview has incomplete frames");}
    if(frames.records.empty()) {return previous;}
    std::optional<WorkshopBounds> scene;
    for(const auto& frame:frames.records) {
        const auto& source=m_assets->geometry[frame.scene_index][frame.object_index];
        const auto* chain=std::get_if<CvrSimpleDetailChain>(&source);
        const auto& geometry=chain ? chain->Levels().front():std::get<CvrGeometry>(source);
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

CvrScoutImage CvrScoutRenderer::RenderView(const scout::View& view,const uint8_t clear,const bool unavailable,const std::optional<std::array<int32_t,4>> clip) const {
    using namespace scout;
    const auto frames = CvrFrameAssembler::Assemble( m_assets->frames, m_assets->selections );
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
        if(frame.value.flags&0x80) {continue;}
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
				{ { &group.records, ( geometry.flags & 8 ) != 0, footprint, basis, origin, footprint==CvrSampleFootprint::Single ? clip:std::nullopt } } );
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
    const auto frames=CvrFrameAssembler::Assemble(m_assets->frames,m_assets->selections);
    if(frames.first_omitted) {throw std::invalid_argument("CVR shadow: incomplete frame assembly");}
    std::vector<CvrIndexedDraw> draws;
    for(const auto& frame:frames.records) {
        if(frame.value.flags&0x80) {continue;}
        const auto& source=m_assets->geometry[frame.scene_index][frame.object_index];
        const auto* chain=std::get_if<CvrSimpleDetailChain>(&source);
        const auto scale=Store(Multiply(Decode(scene.auxiliary_scale),Decode(scene.object_scale)));
        const auto& geometry=chain ? chain->Levels()[chain->Select(scale)]:std::get<CvrGeometry>(source);
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
    result.m_backdrop=CvrActorMaterial::ReadWorkshopBackdrop(read("deswin.pcx"));
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
uint8_t CvrWorkshopComponents::CompositeBackdrop(uint8_t source,size_t x,size_t y) const {
    if(x>=623 || y>=368 || m_backdrop.size()!=623*368) {throw std::invalid_argument("Workshop backdrop coordinate exceeds its original image");}
    return Composite(source,m_backdrop[y*623+x]);
}
uint8_t CvrWorkshopComponents::Composite(uint8_t source,uint8_t destination) const {
    if(source==24) {return destination;}if(source>23) {throw std::invalid_argument("Invalid Workshop translucent shade index");}
    return m_translucency[size_t(source)*256+destination];
}

}
