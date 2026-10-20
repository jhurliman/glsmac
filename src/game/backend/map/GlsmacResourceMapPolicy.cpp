#include "GlsmacResourceMapPolicy.h"

#include <algorithm>
#include <stdexcept>

#include "tile/Tile.h"
#include "SmacPlanetMap.h"
#include "SmacTerrainAuthority.h"
#include "SmacGlobalSpread.h"
#include "SmacClimate.h"
#include "game/backend/resource/SmacSiteSignal.h"

namespace game::backend::map {

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::DisplayedTerrainV1() { return {}; }
GlsmacResourceMapPolicy GlsmacResourceMapPolicy::GeneratedTerrainV3(SmacTerrainAuthority source) {
    if(source.SourceOrigin()!=SmacTerrainAuthority::Origin::Generated || source.InstalledSource() || !source.Info().generation_primary_rules) {
        throw std::invalid_argument("Generated terrain policy requires generated source provenance");
    }
    GlsmacResourceMapPolicy result;
    result.m_automatic_land_resources=bool(*source.Info().generation_primary_rules&0x2000);
    result.m_native=std::make_shared<const SmacTerrainAuthority>(std::move(source));return result;
}
void GlsmacResourceMapPolicy::SetOriginalSource(SmacPlanetMap source) {
    auto native=std::make_shared<const SmacTerrainAuthority>(SmacTerrainAuthority::Installed(std::move(source)));
    auto planet=native->InstalledSource();
    m_planet=std::move(planet);m_native=std::move(native);
}

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::InstalledPlanetV1( SmacPlanetMap source, bool automatic_land_resources ) {
	if ( source.WorldSizeCategory() != 4 || source.Header()[0x1c] != 0 || source.Header()[0x1d] != 0
		|| source.Header()[0x1e] != 0 || source.Header()[0x1f] != 0 ) { throw std::runtime_error("Installed Planet requires its saved category4 and horizontal-wrap topology"); }
    if(source.OceanCoverageCategory()<0 || source.OceanCoverageCategory()>2) {
        throw std::runtime_error("Installed Planet requires a normal serialized ocean category");
    }
	for (uint32_t y=0;y<SmacPlanetMap::HEIGHT;++y) {for(uint32_t x=y&1;x<SmacPlanetMap::WIDTH;x+=2) {
		const auto& square=source.At(x,y);
		if(((square.Climate()>>3)&3)>2 || (square.Climate()&7)>2 || (square.bytes[5]>>6)>2 || (square.Items()&(3|0x800000)) || square.bytes[4]!=0) {
			throw std::runtime_error("Installed Planet contains unsupported climate, presence or visibility state");
		}
	}}
	GlsmacResourceMapPolicy result;
	result.SetOriginalSource(std::move(source));
	result.m_automatic_land_resources=automatic_land_resources;
	return result;
}

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::RetainedClimatePlanetV2(SmacPlanetMap source,bool automatic_land_resources) {
    if(source.WorldSizeCategory()!=4 || source.Header()[0x1c]!=0 || source.Header()[0x1d]!=0
        || source.Header()[0x1e]!=0 || source.Header()[0x1f]!=0) {throw std::runtime_error("Retained Planet climate requires admitted standard-world headers");}
    if(source.OceanCoverageCategory()<0 || source.OceanCoverageCategory()>2) {throw std::runtime_error("Retained Planet climate requires normal ocean category");}
    (void)source.ExportClimateTerrain(true);
    GlsmacResourceMapPolicy result;result.SetOriginalSource(std::move(source));
    result.m_automatic_land_resources=automatic_land_resources;result.m_retained_climate=true;return result;
}

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::WithClimateTerrain(const smac_climate::World& world,int32_t sea_level_offset,int32_t climate_extra) const {
    if(m_native && !m_planet) {return GeneratedTerrainV3(m_native->WithClimateTerrain(world,sea_level_offset,climate_extra));}
    if(!m_planet || !world.horizontal_wrap) {throw std::runtime_error("Climate publication requires installed wrapped Planet authority");}
    return RetainedClimatePlanetV2(m_planet->WithClimateTerrain(world,sea_level_offset,climate_extra),m_automatic_land_resources);
}

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::WithGlobalSpread(const smac_spread::Input& before,const smac_spread::Result& plan) const {
    if(m_native && !m_planet) {return GeneratedTerrainV3(m_native->WithGlobalSpread(before,plan));}
    if(!m_planet || !before.geometry.wrap) {throw std::invalid_argument("Global spread publication requires wrapped installed terrain authority");}
    auto result=*this;result.SetOriginalSource(m_planet->WithGlobalSpread(before,plan));return result;
}

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::WithLiveGlobalSpread(const smac_spread::Input& before,const smac_spread::Result& plan) const {
    if(m_native && !m_planet) {return GeneratedTerrainV3(m_native->WithGlobalSpread(before,plan,true));}
    if(!m_planet || !before.geometry.wrap) {throw std::runtime_error("Live flora publication requires installed wrapped Planet authority");}
    // Visibility/current terrain are legal in the existing retained-world V2
    // codec; the stricter new-import V1 codec deliberately rejects them.
    return RetainedClimatePlanetV2(m_planet->WithLiveGlobalSpread(before,plan),m_automatic_land_resources);
}

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::WithStartingBalance(const smac_balance::Result& plan) const {
    if(m_native && !m_planet) {return GeneratedTerrainV3(m_native->WithStartingBalance(plan));}
    if(!m_planet) {throw std::runtime_error("Compensation markers require original terrain item authority");}
    auto result=*this;result.SetOriginalSource(m_planet->WithStartingBalance(plan));return result;
}

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::WithBloomTerrain(const base::SmacBloomTerrainPlan& plan) const {
    if(m_native && !m_planet) {return GeneratedTerrainV3(m_native->WithBloomTerrain(plan));}
    if (!m_planet) { throw std::runtime_error("Bloom terrain requires original Planet resource authority"); }
    const auto updated=m_planet->WithBloomTerrain(plan);
    return m_retained_climate ? RetainedClimatePlanetV2(updated,m_automatic_land_resources):InstalledPlanetV1(updated,m_automatic_land_resources);
}

bool GlsmacResourceMapPolicy::operator==( const GlsmacResourceMapPolicy& other ) const {
    if(m_retained_climate!=other.m_retained_climate || m_automatic_land_resources!=other.m_automatic_land_resources) {return false;}
    if(m_planet || other.m_planet) {return m_planet && other.m_planet && m_planet->Encode()==other.m_planet->Encode();}
    return m_native==other.m_native || (m_native && other.m_native && *m_native==*other.m_native);
}

resource::smac::AltitudeBand GlsmacResourceMapPolicy::Altitude( const tile::Tile& value ) const {
	if(m_native) {
        if(value.coord.x>=m_native->Info().width || value.coord.y>=m_native->Info().height) {throw std::runtime_error("Native resource coordinate exceeds source geometry");}
		return static_cast<resource::smac::AltitudeBand>(m_native->At(value.coord.x,value.coord.y).Climate()>>5);
	}
	if(!value.elevation.center) {throw std::runtime_error("Resource tile has no elevation");}
	return Altitude(value.is_water_tile,*value.elevation.center);
}

std::array<bool,3> GlsmacResourceMapPolicy::LandmarkBonuses( const tile::Tile& value ) const {
	if(!m_native) {return {!value.is_water_tile && bool(value.features&tile::FEATURE_JUNGLE),false,false};}
	const auto altitude=Altitude(value);
	const auto word=m_native->At(value.coord.x,value.coord.y).Landmarks();
	const bool land=altitude>=resource::smac::AltitudeBand::Land3;
	const bool unsuppressed=(word&0x80000000u)==0;
	const bool inner=(word>>24)<9;
	return { (land && unsuppressed && (word&4)) || (!land && (word&0x80)),
		unsuppressed && (((word&3) && inner) || (word&0x200)),
		unsuppressed && (((word&2) && inner) || (word&(8|0x400|0x800))) };
}

resource::smac::AltitudeBand GlsmacResourceMapPolicy::Altitude( bool water, int64_t center_elevation ) const {
	if(m_native) {throw std::runtime_error("Native resource altitude requires a tile coordinate");}
	using resource::smac::AltitudeBand;
	if ( center_elevation < tile::ELEVATION_MIN || center_elevation > tile::ELEVATION_MAX ) {
		throw std::runtime_error( "GLSMAC resource elevation is outside DisplayedTerrainV1" );
	}
	if ( water ) {
		if ( center_elevation < tile::ELEVATION_LEVEL_TRENCH ) { return AltitudeBand::Trench; }
		if ( center_elevation < tile::ELEVATION_LEVEL_OCEAN ) { return AltitudeBand::DeepOcean; }
		return AltitudeBand::Shelf;
	}
	// A majority-land coast may have a negative raw center. Its visible land
	// remains the lowest tier. This is a declared port policy, not SMAC height.
	return static_cast< AltitudeBand >( 3 + std::max< int64_t >( 0, center_elevation ) / 1000 );
}

bool GlsmacResourceMapPolicy::AllowsGeneratedBonus( bool water, int64_t center_elevation ) const {
	return Altitude( water, center_elevation ) >= resource::smac::AltitudeBand::Shelf;
}

uint8_t GlsmacResourceMapPolicy::SiteSignal(const tile::Tile& value) const {
    if(!m_native) {return (value.features&tile::FEATURE_UNITY_POD)?1:0;}
    const auto altitude=Altitude(value); // authenticates imported coordinates
    const auto flags=m_native->At(value.coord.x,value.coord.y).Items();
    return resource::smac::ResolveSiteSignal({static_cast<int32_t>(value.coord.x),static_cast<int32_t>(value.coord.y),
        altitude,flags,m_automatic_land_resources,m_native->Info().pattern,11,61});
}

resource::smac::ResourceBonus GlsmacResourceMapPolicy::Bonus( const tile::Tile& value ) const {
	using resource::smac::ResourceBonus;
	if(m_native) {
		const auto altitude=Altitude(value);
		const auto& square=m_native->At(value.coord.x,value.coord.y);
		const auto items=square.Items();
		return resource::smac::ResolveResourceBonus({static_cast<int32_t>(value.coord.x),static_cast<int32_t>(value.coord.y),altitude,
			m_native->Info().pattern,m_automatic_land_resources,bool(items&0x400),bool(items&0x20000000),bool(items&0x10000),bool(items&0x20000)});
	}
	if ( !value.elevation.center ) { throw std::runtime_error( "GLSMAC resource tile has no elevation" ); }
	const auto altitude = Altitude( value.is_water_tile, *value.elevation.center );
	ResourceBonus bonus;
	switch ( value.bonus ) {
		case tile::BONUS_NONE: bonus = ResourceBonus::None; break;
		case tile::BONUS_NUTRIENT: bonus = ResourceBonus::Nutrients; break;
		case tile::BONUS_MINERALS: bonus = ResourceBonus::Minerals; break;
		case tile::BONUS_ENERGY: bonus = ResourceBonus::Energy; break;
		default: throw std::runtime_error( "Unknown GLSMAC displayed resource bonus" );
	}
	if ( bonus != ResourceBonus::None && altitude < resource::smac::AltitudeBand::Shelf ) {
		throw std::runtime_error( "DisplayedTerrainV1 cannot reinterpret a resource bonus below shelf" );
	}
	if ( value.coord.x > INT32_MAX || value.coord.y > INT32_MAX ) { throw std::runtime_error( "GLSMAC resource coordinates exceed signed32" ); }
	return resource::smac::ResolveResourceBonus( { static_cast< int32_t >( value.coord.x ), static_cast< int32_t >( value.coord.y ), altitude,
		0, false, bonus != ResourceBonus::None, true, bonus == ResourceBonus::Energy, bonus == ResourceBonus::Minerals } );
}

types::Buffer GlsmacResourceMapPolicy::Serialize() const {
	if(m_native && !m_planet) {
        const auto& meta=m_native->Info();types::Buffer result;result.WriteInt(3);result.WriteInt(3);result.WriteBool(m_automatic_land_resources);
        result.WriteInt(meta.width);result.WriteInt(meta.height);result.WriteBool(meta.horizontal_wrap);
        result.WriteInt(meta.pattern);result.WriteInt(meta.sea_offset);result.WriteInt(meta.climate_extra);
        for(const auto category:meta.categories) {result.WriteInt(category.value());}
        result.WriteInt(meta.generation_primary_rules.value());result.WriteString(m_native->RecordBytes());
        if(result.lenw>MAX_WIRE_BYTES) {throw std::runtime_error("Generated resource policy exceeds wire budget");}return result;
    }
	types::Buffer result; result.WriteInt( m_retained_climate ? 2 : 1 ); result.WriteInt( m_planet ? 2 : 1 );
	if(m_planet) {result.WriteBool(m_automatic_land_resources);result.WriteString(m_planet->Encode());}
	return result;
}

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::Deserialize( const std::string& wire ) {
	if ( wire.size() > MAX_WIRE_BYTES ) { throw std::runtime_error( "GLSMAC resource map policy exceeds wire budget" ); }
	types::Buffer input( wire );
    const auto version=input.ReadInt();
    if(version!=1 && version!=2 && version!=3) {throw std::runtime_error("Unsupported GLSMAC resource map policy");}
	const auto kind=input.ReadInt();
    if(version==3) {
        if(kind!=3) {throw std::runtime_error("Unsupported GLSMAC resource map policy V3 kind");}
        const auto integer=[&](int64_t low,int64_t high){const auto v=input.ReadInt();if(v<low || v>high) {throw std::runtime_error("Generated terrain metadata integer is outside its domain");}return v;};
        const auto automatic=input.ReadBool();SmacTerrainAuthority::Metadata meta;
        meta.width=uint32_t(integer(16,512));meta.height=uint32_t(integer(8,256));meta.horizontal_wrap=input.ReadBool();
        if((meta.width&1) || (meta.height&1)) {throw std::runtime_error("Generated terrain requires even parity-grid dimensions");}
        meta.pattern=int32_t(integer(1,32767));meta.sea_offset=int32_t(integer(INT32_MIN,INT32_MAX));meta.climate_extra=int32_t(integer(INT32_MIN,INT32_MAX));
        for(size_t i=0;i<7;++i) {meta.categories[i]=int32_t(integer(0,i==0?4:2));}
        meta.generation_primary_rules=uint32_t(integer(0,UINT32_MAX));
        if(automatic!=bool(*meta.generation_primary_rules&0x2000)) {throw std::runtime_error("Generated terrain automatic-resource option differs from source rules");}
        const auto records=input.ReadString(meta.width/2*meta.height*44);
        auto generated=GeneratedTerrainV3(SmacTerrainAuthority::GeneratedRetained(std::move(meta),records));
        if(input.lenr!=input.lenw) {throw std::runtime_error("Unexpected generated resource map policy fields");}return generated;
    }
    if(version==2 && kind!=2) {throw std::runtime_error("Retained climate requires installed Planet policy");}
	auto result=DisplayedTerrainV1();
	if(kind==2) {
		const bool automatic=input.ReadBool();
        auto source=SmacPlanetMap::Decode(input.ReadString(SmacPlanetMap::FILE_BYTES));
        result=version==2 ? RetainedClimatePlanetV2(std::move(source),automatic):InstalledPlanetV1(std::move(source),automatic);
	} else if(kind!=1) {throw std::runtime_error("Unsupported GLSMAC resource map policy");}
	if ( input.lenr != input.lenw ) { throw std::runtime_error( "Unexpected GLSMAC resource map policy fields" ); }
	return result;
}

void GlsmacResourceMapPolicy::WriteOptional( types::Buffer& output, const std::optional< GlsmacResourceMapPolicy >& policy, bool smac ) {
	if ( policy && !smac ) { throw std::runtime_error( "GLSMAC resource map policy is unavailable for this ruleset" ); }
	const auto wire = policy ? std::optional< std::string >( policy->Serialize().ToString() ) : std::nullopt;
	output.WriteBool( policy.has_value() ); if ( wire ) { output.WriteString( *wire ); }
}

std::optional< GlsmacResourceMapPolicy > GlsmacResourceMapPolicy::ReadOptional( types::Buffer& input, bool smac ) {
	if ( !input.ReadBool() ) { return {}; }
	if ( !smac ) { throw std::runtime_error( "Unexpected GLSMAC resource map policy for this ruleset" ); }
	return Deserialize( input.ReadString( MAX_WIRE_BYTES ) );
}

}
