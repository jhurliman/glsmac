#pragma once

#include <string>

namespace gse::tests::smac_rules_fixture {

// Synthetic complete supported catalogs, not copied installed game data.
inline std::string PredefinedSource() {
	std::string s = "#CHASSIS\n";
	const int speeds[] = { 1, 2, 3, 4, 6, 8, 8, 8, 12 };
	for ( int i = 0; i < 9; ++i ) {
		const auto n = "Body" + std::to_string( i ) + ",N1,";
		s += n + n + n + n + std::to_string( speeds[ i ] ) + "," + std::to_string( i < 3 ? 0 : i < 5 ? 1 : 2 )
			+ ",0," + std::to_string( i == 8 ? 1 : 0 ) + "," + std::to_string( i == 3 ? 2 : i == 4 ? 4 : i == 8 ? 0 : 1 )
			+ "," + std::to_string( speeds[ i ] ) + ",None," + n + n + "\n";
	}
	s += "#WEAPONS\n";
	const int costs[] = { 10, 6, 4, 8, 4, 36 }, modes[] = { 8, 9, 7, 10, 11, 12 };
	for ( int i = 0; i < 21; ++i ) {
		const bool special = i >= 14 && i < 20;
		s += "Tool" + std::to_string( i ) + ",W" + std::to_string( i ) + "," + std::to_string( special ? 0 : i == 12 ? -1 : 1 )
			+ "," + std::to_string( special ? modes[ i - 14 ] : i == 12 ? 2 : 0 ) + "," + std::to_string( special ? costs[ i - 14 ] : i == 12 ? 10 : 1 ) + ",-1,None\n";
	}
	s += "#DEFENSES\n";
	for ( int i = 0; i < 10; ++i ) { s += "Shield" + std::to_string( i ) + ",D" + std::to_string( i ) + ( i == 9 ? ",-1,2,6,None\n" : ",1,0,1,None\n" ); }
	s += "#REACTORS\n";
	for ( int i = 0; i < 4; ++i ) { s += "Core" + std::to_string( i ) + ",R" + std::to_string( i ) + "," + std::to_string( i + 1 ) + ",None\n"; }
	s += "#ABILITIES\n";
	for ( int i = 0; i < 24; ++i ) { s += "Ability" + std::to_string( i ) + "," + std::to_string( i == 5 ? 1 : 0 ) + ",None,,00000000000,Synthetic; description\n"; }
	s += "#UNITS\n14\n";
	const int chassis[] = { 0, 0, 0, 3, 3, 0, 1, 0, 0, 3, 7, 1, 6, 3 };
	const int weapons[] = { 14, 15, 0, 16, 15, 17, 18, 19, 12, 12, 12, 0, 0, 16 };
	const int plans[] = { -1, -1, -1, 7, 9, 10, 11, 12, 1, 7, 4, 3, 4, 7 };
	for ( int i = 0; i < 14; ++i ) {
		s += "Synthetic design,Body" + std::to_string( chassis[ i ] ) + ",W" + std::to_string( weapons[ i ] ) + ",D"
			+ std::to_string( i >= 8 && i <= 10 ? 9 : 0 ) + "," + std::to_string( plans[ i ] ) + ",0,0," + ( i == 1 ? "Tech2" : "None" ) + ",-1,"
			+ ( i == 10 || i == 12 ? "000000000000000000100000" : i == 13 ? "100000000000000000000000" : "000000000000000000000000" ) + "\n";
	}
	return s + "#END\n";
}

}
