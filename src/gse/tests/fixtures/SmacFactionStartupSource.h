#pragma once

#include "SmacRulesSource.h"
#include "game/backend/settings/SmacFactionStartupRules.h"

namespace gse::tests::smac_faction_startup_fixture {

inline game::backend::settings::SmacTechnologyRules Technologies() {
	auto result = smac_rules_fixture::Technologies();
	for ( const auto& entry : std::array< std::pair< size_t, const char* >, 7 >{{
		{ 0, "Biogen" }, { 1, "Indust" }, { 2, "InfNet" }, { 4, "Psych" }, { 5, "Mobile" }, { 6, "Ecology" }, { 43, "DocLoy" }
	}} ) { result.technologies[ entry.first ].key = entry.second; }
	return result;
}

inline std::string Alpha() { return smac_rules_fixture::FactionAlpha(); }

inline std::string Faction( const std::string& section, const std::string& bonuses ) {
	return "## synthetic faction source\r\n#" + section + "\r\nSynthetic,Faction,Plural,F,1,Leader,F,-1,0,0,1,1,\r\n; ignored full-line comment\r\n"
		+ bonuses + "\r\nUnused ideology, not bonus data\r\n#BASES\r\nSynthetic Base\r\n#END\r\n";
}

// Reduced semantic bonus inputs from RE-SMAC-START-FACTION-001. No source
// headers/dialogue/assets or downstream startup/ownership expectations.
inline const std::array< const char*, 7 >& BonusLines() {
	static const std::array< const char*, 7 > lines = {
		"TECH,Ecology,SOCIAL,-MORALE,SOCIAL,-POLICE,SOCIAL,++EFFIC,SOCIAL,+PLANET,FUNGNUTRIENT,1",
		"TECH,DocLoy,FACILITY,4,SOCIAL,+GROWTH,SOCIAL,+INDUSTRY,SOCIAL,--ECONOMY,IMMUNITY,EFFIC",
		"TECH,InfNet,TECH,1,SOCIAL,++RESEARCH,SOCIAL,--PROBE,DRONE,4,FACILITY,8",
		"TECH,Indust,ENERGY,100,SOCIAL,+ECONOMY,POPULATION,3,COMMERCE,1,SOCIAL,-SUPPORT",
		"TECH,Mobile,FREEPROTO,0,SOCIAL,++MORALE,SOCIAL,+POLICE,SOCIAL,-INDUSTRY",
		"TECH,Psych,SOCIAL,--RESEARCH,SOCIAL,+PROBE,SOCIAL,++SUPPORT,SOCIAL,-PLANET,FANATIC,0",
		"TECH,Biogen,TALENT,4,SOCIAL,-EFFIC,POPULATION,-2,VOTES,2"
	};
	return lines;
}

inline std::string Source( const std::string& filename ) {
	const auto& bindings = game::backend::settings::SmacFactionStartupRules::Bindings();
	for ( size_t i = 0 ; i < bindings.size() ; ++i ) {
		if ( filename == bindings[ i ].filename ) { return Faction( bindings[ i ].section, BonusLines()[ i ] ); }
	}
	throw std::runtime_error( "Unexpected faction filename" );
}

}
