#include "SmacTileNutrients.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend::resource::smac {
namespace {

int64_t Checked( const int64_t value ) {
	if ( value < std::numeric_limits< int32_t >::min() || value > std::numeric_limits< int32_t >::max() ) {
		throw std::overflow_error( "SMAC tile nutrients exceed the verified signed arithmetic domain" );
	}
	return value;
}

void Validate( const TileNutrientInputs& inputs ) {
	const auto& terrain = inputs.terrain;
	if ( static_cast< uint8_t >( terrain.altitude ) > static_cast< uint8_t >( AltitudeBand::Land7 ) ||
		terrain.moisture < Moisture::Arid || terrain.moisture > Moisture::Rainy ||
		terrain.rockiness < Rockiness::Flat || terrain.rockiness > Rockiness::Rocky ||
		terrain.resource_bonus < ResourceBonus::None || terrain.resource_bonus > ResourceBonus::Energy ) {
		throw std::invalid_argument( "Unknown SMAC tile nutrient terrain value" );
	}
	if ( terrain.altitude < AltitudeBand::Shelf && terrain.resource_bonus != ResourceBonus::None ) {
		throw std::invalid_argument( "SMAC resource resolution cannot yield a resource below shelf" );
	}
	if ( inputs.context.fungal_capability < 0 ) {
		throw std::invalid_argument( "SMAC fungal capability must be the nonnegative constructed value" );
	}
}

}

TileNutrientYield CalculateTileNutrients( const TileNutrientRules& rules, const TileNutrientInputs& inputs ) {
	Validate( inputs );
	const auto& terrain = inputs.terrain;
	const auto& improvements = inputs.improvements;
	const auto& context = inputs.context;
	const bool nutrient_resource = terrain.resource_bonus == ResourceBonus::Nutrients;
	const int64_t bonus = nutrient_resource ? rules.bonus : 0;
	int64_t value;
	int64_t excess = 0;
	if ( terrain.borehole ) {
		value = rules.borehole;
		// Original custom-rule behavior depends on the borehole rule, not resource presence.
		if ( value != 0 ) { value = Checked( value + rules.bonus ); }
	}
	else if ( terrain.monolith ) {
		value = Checked( int64_t( rules.monolith ) + bonus );
	}
	else if ( terrain.fungus && terrain.altitude >= AltitudeBand::Shelf ) {
		value = Checked( int64_t( context.fungal_capability ) + std::clamp< int64_t >( context.planet, -3, 0 ) );
		value = std::clamp< int64_t >( value, 0, 99 );
	}
	else {
		if ( terrain.altitude <= AltitudeBand::Shelf ) {
			value = Checked( int64_t( rules.ocean ) + bonus );
			if ( terrain.landmark_bonus ) { value = Checked( value + 1 ); }
			if ( terrain.altitude == AltitudeBand::Shelf ) {
				if ( improvements.farm ) { value = Checked( value + rules.improved_sea ); }
				if ( improvements.mine && value > 1 ) { value = Checked( value + rules.mine_effect ); }
			}
		}
		else if ( terrain.forest ) {
			value = Checked( int64_t( rules.forest ) + bonus );
			if ( terrain.landmark_bonus ) { value = Checked( value + 1 ); }
			if ( context.tree_farm ) { value = Checked( value + 1 ); }
			if ( context.hybrid_forest ) { value = Checked( value + 1 ); }
		}
		else {
			value = terrain.rockiness == Rockiness::Rocky ? 0 : static_cast< int64_t >( terrain.moisture );
			value = Checked( value + bonus );
			if ( terrain.landmark_bonus ) { value = Checked( value + 1 ); }
			value = std::max< int64_t >( 0, value );
			if ( improvements.farm && terrain.rockiness != Rockiness::Rocky ) { value = Checked( value + rules.improved_land ); }
			if ( improvements.mine && value > 1 ) { value = Checked( value + rules.mine_effect ); }
		}
		// Each half is signed division truncated toward zero, rounded independently.
		if ( improvements.soil_enricher ) { value = Checked( value + value / 2 ); }
		if ( improvements.condenser ) { value = Checked( value + value / 2 ); }
		if ( value > 2 && !context.nutrient_technology && !nutrient_resource && !improvements.condenser ) {
			excess = value - 2;
			value = 2;
		}
	}
	if ( context.bumper_crop ) { value = Checked( value + 1 ); }
	if ( context.famine && value != 0 ) { value = Checked( value - 1 ); }
	return { value, excess };
}

TileNutrientYield CalculateBaselessTileNutrients(const TileNutrientRules& rules,int32_t center_rule,
    bool base_present,uint8_t cached_affiliation,const TileNutrientInputs& live) {
    Validate(live);
    if(cached_affiliation>15) {throw std::invalid_argument("Invalid native nutrient center affiliation");}
    if(base_present && cached_affiliation<8) {
        const auto wrap=[](int64_t value)->int64_t {
            const auto bits=static_cast<uint32_t>(value);
            return bits<=INT32_MAX?int64_t(bits):int64_t(bits)-0x100000000LL;
        };
        int64_t nutrients=center_rule;
        if(live.terrain.resource_bonus==ResourceBonus::Nutrients) {nutrients=wrap(nutrients*2);}
        if(live.terrain.landmark_bonus) {nutrients=wrap(nutrients+1);}
        return {nutrients,0};
    }
    auto ordinary=live;
    ordinary.context.tree_farm=false;ordinary.context.hybrid_forest=false;
    ordinary.context.bumper_crop=false;ordinary.context.famine=false;
    return CalculateTileNutrients(rules,ordinary);
}

}
