#include "GlsmacResourceMapPolicy.h"

#include <algorithm>
#include <stdexcept>

#include "tile/Tile.h"

namespace game::backend::map {

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::DisplayedTerrainV1() { return {}; }

resource::smac::AltitudeBand GlsmacResourceMapPolicy::Altitude( bool water, int64_t center_elevation ) const {
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

resource::smac::ResourceBonus GlsmacResourceMapPolicy::Bonus( const tile::Tile& value ) const {
	using resource::smac::ResourceBonus;
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
	types::Buffer result; result.WriteInt( 1 ); result.WriteInt( 1 ); // format, DisplayedTerrainV1
	return result;
}

GlsmacResourceMapPolicy GlsmacResourceMapPolicy::Deserialize( const std::string& wire ) {
	if ( wire.size() > MAX_WIRE_BYTES ) { throw std::runtime_error( "GLSMAC resource map policy exceeds wire budget" ); }
	types::Buffer input( wire );
	if ( input.ReadInt() != 1 || input.ReadInt() != 1 ) { throw std::runtime_error( "Unsupported GLSMAC resource map policy" ); }
	if ( input.lenr != input.lenw ) { throw std::runtime_error( "Unexpected GLSMAC resource map policy fields" ); }
	return DisplayedTerrainV1();
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
