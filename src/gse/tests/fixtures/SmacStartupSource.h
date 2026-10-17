#pragma once

#include "SmacRulesSource.h"
#include "SmacFactionStartupSource.h"

#include <sstream>

namespace gse::tests::smac_startup_fixture {

inline std::string PredefinedSource() {
	std::istringstream input( smac_rules_fixture::PredefinedSource() );
	std::string line, section, result;
	size_t index = 0;
	while ( std::getline( input, line ) ) {
		if ( !line.empty() && line[ 0 ] == '#' ) { section = line; index = 0; result += line + "\n"; continue; }
		if ( section == "#UNITS" && line == "14" ) { result += line + "\n"; continue; }
		std::vector< std::string > fields; std::istringstream row( line ); std::string field;
		while ( std::getline( row, field, ',' ) ) { fields.push_back( field ); }
		if ( section == "#CHASSIS" ) { fields[ 14 ] = index == 0 ? "None" : index == 1 ? "Tech5" : "Disable"; }
		if ( section == "#WEAPONS" ) { fields[ 6 ] = index == 0 || index == 14 ? "None" : index == 15 ? "Tech6" : "Disable"; }
		if ( section == "#DEFENSES" ) {
			fields[ 5 ] = index == 0 ? "None" : index == 1 ? "Tech1" : "Disable";
			if ( index == 1 ) { fields[ 2 ] = "2"; fields[ 4 ] = "2"; }
		}
		if ( section == "#REACTORS" ) { fields[ 3 ] = index == 0 ? "None" : "Disable"; }
		if ( section == "#ABILITIES" ) { fields[ 2 ] = "Disable"; }
		if ( section == "#UNITS" ) {
			fields[ 7 ] = index == 0 || index == 2 ? "None" : index == 1 ? "Tech6" : "Disable";
			if ( index == 2 ) { fields[ 4 ] = "3"; }
		}
		for ( size_t i = 0; i < fields.size(); ++i ) { if ( i ) { result += ","; } result += fields[ i ]; }
		result += "\n"; ++index;
	}
	return result;
}

inline std::string FactionSource( const std::string& filename ) {
	const auto& bindings = game::backend::settings::SmacFactionStartupRules::Bindings();
	const int grants[] = { 6, 43, 2, 1, 5, 4, 0 };
	for ( size_t i = 0; i < bindings.size(); ++i ) {
		if ( filename == bindings[ i ].filename ) {
			std::string bonuses = smac_faction_startup_fixture::BonusLines()[ i ];
			const auto end = bonuses.find( ',', 5 );
			bonuses.replace( 5, end - 5, "Tech" + std::to_string( grants[ i ] ) );
			return "#" + std::string( bindings[ i ].section ) + "\nSynthetic,Faction,Plural,F,1,Leader,F,-1,0,0,1,1,\n" + bonuses + "\n#END\n";
		}
	}
	throw std::runtime_error( "Unexpected startup faction source" );
}

inline std::string Source( const std::string& filename ) {
	if ( filename != "alpha.txt" ) { return FactionSource( filename ); }
	auto result = smac_rules_fixture::Source( filename );
	const auto old = smac_rules_fixture::PredefinedSource();
	result.replace( result.find( old ), old.size(), PredefinedSource() );
	return result;
}
inline game::backend::settings::SmacPredefinedUnitRules Units() {
	return game::backend::settings::SmacPredefinedUnitRules::Parse( PredefinedSource(), smac_rules_fixture::Technologies() );
}
inline game::backend::settings::SmacFactionStartupRules Factions() {
	return loader::txt::FactionTXTLoader::ParseStartupRules( smac_rules_fixture::FactionAlpha(), FactionSource, smac_rules_fixture::Technologies() );
}

}
