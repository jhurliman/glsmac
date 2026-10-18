#include "GlsmacResourceMapPolicy.h"

#include <algorithm>
#include <stdexcept>

#include "tile/Tile.h"
#include "SmacPlanetMap.h"
#include "game/backend/resource/SmacSiteSignal.h"

namespace game::backend::map {

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::DisplayedTerrainV1() { return {}; }

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
	result.m_planet=std::make_shared<const SmacPlanetMap>(std::move(source));
	result.m_automatic_land_resources=automatic_land_resources;
	return result;
}

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::WithBloomTerrain(const std::vector<base::SmacBloomTerrainDelta>& changes, bool nonzero_turn) const {
    if (!m_planet) { throw std::runtime_error("Bloom terrain requires original Planet resource authority"); }
    return InstalledPlanetV1(m_planet->WithBloomTerrain(changes, nonzero_turn), m_automatic_land_resources);
}

bool GlsmacResourceMapPolicy::operator==( const GlsmacResourceMapPolicy& other ) const {
	return m_automatic_land_resources==other.m_automatic_land_resources && (m_planet==other.m_planet
		|| (m_planet && other.m_planet && m_planet->Encode()==other.m_planet->Encode()));
}

resource::smac::AltitudeBand GlsmacResourceMapPolicy::Altitude( const tile::Tile& value ) const {
	if(m_planet) {
		if(value.coord.x>=SmacPlanetMap::WIDTH || value.coord.y>=SmacPlanetMap::HEIGHT) {throw std::runtime_error("Imported resource coordinate exceeds source geometry");}
		return static_cast<resource::smac::AltitudeBand>(m_planet->At(value.coord.x,value.coord.y).Climate()>>5);
	}
	if(!value.elevation.center) {throw std::runtime_error("Resource tile has no elevation");}
	return Altitude(value.is_water_tile,*value.elevation.center);
}

std::array<bool,3> GlsmacResourceMapPolicy::LandmarkBonuses( const tile::Tile& value ) const {
	if(!m_planet) {return {!value.is_water_tile && bool(value.features&tile::FEATURE_JUNGLE),false,false};}
	const auto altitude=Altitude(value);
	const auto word=m_planet->At(value.coord.x,value.coord.y).Landmarks();
	const bool land=altitude>=resource::smac::AltitudeBand::Land3;
	const bool unsuppressed=(word&0x80000000u)==0;
	const bool inner=(word>>24)<9;
	return { (land && unsuppressed && (word&4)) || (!land && (word&0x80)),
		unsuppressed && (((word&3) && inner) || (word&0x200)),
		unsuppressed && (((word&2) && inner) || (word&(8|0x400|0x800))) };
}

resource::smac::AltitudeBand GlsmacResourceMapPolicy::Altitude( bool water, int64_t center_elevation ) const {
	if(m_planet) {throw std::runtime_error("Imported resource altitude requires a tile coordinate");}
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
    if(!m_planet) {return (value.features&tile::FEATURE_UNITY_POD)?1:0;}
    const auto altitude=Altitude(value); // authenticates imported coordinates
    const auto flags=m_planet->At(value.coord.x,value.coord.y).Items();
    return resource::smac::ResolveSiteSignal({static_cast<int32_t>(value.coord.x),static_cast<int32_t>(value.coord.y),
        altitude,flags,m_automatic_land_resources,m_planet->ResourcePattern(),11,61});
}

resource::smac::ResourceBonus GlsmacResourceMapPolicy::Bonus( const tile::Tile& value ) const {
	using resource::smac::ResourceBonus;
	if(m_planet) {
		const auto altitude=Altitude(value);
		const auto& square=m_planet->At(value.coord.x,value.coord.y);
		const auto items=square.Items();
		return resource::smac::ResolveResourceBonus({static_cast<int32_t>(value.coord.x),static_cast<int32_t>(value.coord.y),altitude,
			m_planet->ResourcePattern(),m_automatic_land_resources,bool(items&0x400),bool(items&0x20000000),bool(items&0x10000),bool(items&0x20000)});
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
	types::Buffer result; result.WriteInt( 1 ); result.WriteInt( m_planet ? 2 : 1 );
	if(m_planet) {result.WriteBool(m_automatic_land_resources);result.WriteString(m_planet->Encode());}
	return result;
}

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::Deserialize( const std::string& wire ) {
	if ( wire.size() > MAX_WIRE_BYTES ) { throw std::runtime_error( "GLSMAC resource map policy exceeds wire budget" ); }
	types::Buffer input( wire );
	if ( input.ReadInt() != 1 ) { throw std::runtime_error( "Unsupported GLSMAC resource map policy" ); }
	const auto kind=input.ReadInt();
	auto result=DisplayedTerrainV1();
	if(kind==2) {
		const bool automatic=input.ReadBool();
		result=InstalledPlanetV1(SmacPlanetMap::Decode(input.ReadString(SmacPlanetMap::FILE_BYTES)),automatic);
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
