#pragma once

#include "game/backend/settings/Settings.h"

#include <array>
#include <stdexcept>
#include <string>

namespace gse::tests::smac_rules_fixture {

inline game::backend::settings::SmacProductionRules Production() {
	game::backend::settings::SmacProductionRules result{ 123, -50, INT32_MAX, INT32_MIN, {} };
	for ( size_t i = 0 ; i < result.facilities.size() ; ++i ) {
		result.facilities[ i ] = { static_cast< int32_t >( i + 1 ), i < 2 ? "Repeated display name" : "Facility " + std::to_string( i + 1 ),
			i == 0 ? INT32_MIN : i == 70 ? INT32_MAX : static_cast< int32_t >( 100 + i ), -static_cast< int32_t >( i ),
			i == 0 || i == 37 ? "None" : i == 70 ? "Disable" : "UnknownTech" + std::to_string( i ) };
	}
	return result;
}

inline std::string ProductionSource() {
	const auto rules = Production();
	std::string source = "#RULES\n";
	for ( size_t row = 1 ; row <= 70 ; ++row ) {
		const auto value = row == 7 ? rules.mineral_multiplier : row == 43 ? rules.retool_loss_percent
			: row == 44 ? rules.retool_strictness : row == 45 ? rules.retool_exemption : 0;
		source += std::to_string( value ) + ", ; synthetic annotation\n";
	}
	source += "#FACILITIES\n";
	for ( const auto& facility : rules.facilities ) {
		source += facility.name + ", " + std::to_string( facility.cost ) + ", " + std::to_string( facility.maintenance )
			+ ", " + facility.prerequisite + ", ObsoleteToken, unused effect, with commas, 1, 2, 3, 4, 5,\n";
	}
	return source + "#END\n";
}

// Synthetic numeric values intentionally differ from installed defaults.
inline std::string Source( const std::string& filename ) {
	if ( filename != "alpha.txt" ) { throw std::runtime_error( "Unexpected rules filename: " + filename ); }
	return ProductionSource() + "#RESOURCEINFO\nOcean Square,*,*,*\nBase Square,17,-2147483648,2147483647\n"
		"Bonus Square,-2,300,0\nForest Square,*,*,*\nRecycling Tanks,-7,8,9\n"
		"Improved Land,*,*,*\nImproved Sea,*,*,*\nMonolith,*,*,*\nBorehole Square,*,*,*\n#END\n";
}

inline bool Matches( const game::backend::settings::GlobalSettings& settings ) {
	const auto& rules = settings.GetSmacBaseSquareRules();
	using game::backend::resource::smac::RuleTriple;
	return rules && rules->base_square == RuleTriple{ 17, INT32_MIN, INT32_MAX }
		&& rules->bonus_square == RuleTriple{ -2, 300, 0 } && rules->recycling == RuleTriple{ -7, 8, 9 }
		&& settings.GetSmacProductionRules() && *settings.GetSmacProductionRules() == Production();
}

inline types::Buffer Payload( const std::string& id, const bool present,
	const std::array< int64_t, 9 >& values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 },
	const std::optional< std::string >& production = std::nullopt ) {
	types::Buffer result;
	result.WriteInt( 3 );
	result.WriteString( id );
	result.WriteBool( present );
	if ( present ) {
		for ( const auto value : values ) { result.WriteInt( value ); }
		result.WriteString( production ? *production : Production().Serialize().ToString() );
	}
	game::backend::settings::MapSettings map;
	map.size_x = 86;
	result.WriteString( map.Serialize().ToString() );
	result.WriteString( "" ); // Default::Serialize currently has no fields.
	result.WriteInt( 1 );
	result.WriteString( "Host game" );
	return result;
}

}
