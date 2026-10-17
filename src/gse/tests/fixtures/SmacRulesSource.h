#pragma once

#include "game/backend/settings/Settings.h"

#include <array>
#include <stdexcept>
#include <string>

namespace gse::tests::smac_rules_fixture {

// Synthetic numeric values intentionally differ from installed defaults.
inline std::string Source( const std::string& filename ) {
	if ( filename != "alpha.txt" ) { throw std::runtime_error( "Unexpected rules filename: " + filename ); }
	return "#RESOURCEINFO\nOcean Square,*,*,*\nBase Square,17,-2147483648,2147483647\n"
		"Bonus Square,-2,300,0\nForest Square,*,*,*\nRecycling Tanks,-7,8,9\n"
		"Improved Land,*,*,*\nImproved Sea,*,*,*\nMonolith,*,*,*\nBorehole Square,*,*,*\n#END\n";
}

inline bool Matches( const game::backend::settings::GlobalSettings& settings ) {
	const auto& rules = settings.GetSmacBaseSquareRules();
	using game::backend::resource::smac::RuleTriple;
	return rules && rules->base_square == RuleTriple{ 17, INT32_MIN, INT32_MAX }
		&& rules->bonus_square == RuleTriple{ -2, 300, 0 } && rules->recycling == RuleTriple{ -7, 8, 9 };
}

inline types::Buffer Payload( const std::string& id, const bool present,
	const std::array< int64_t, 9 >& values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 } ) {
	types::Buffer result;
	result.WriteInt( 2 );
	result.WriteString( id );
	result.WriteBool( present );
	if ( present ) { for ( const auto value : values ) { result.WriteInt( value ); } }
	game::backend::settings::MapSettings map;
	map.size_x = 86;
	result.WriteString( map.Serialize().ToString() );
	result.WriteString( "" ); // Default::Serialize currently has no fields.
	result.WriteInt( 1 );
	result.WriteString( "Host game" );
	return result;
}

}
