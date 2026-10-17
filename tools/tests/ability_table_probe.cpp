// Opt-in local rules probe. Reports aggregate metadata, never installed text.
#include "loader/txt/UnitRulesTXTLoader.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>

int main( int argc, char** argv ) {
	if ( argc != 3 ) {
		std::cerr << "usage: ability_table_probe smac|smax RULES_FILE\n";
		return 2;
	}
	try {
		std::ifstream file( argv[ 2 ], std::ios::binary );
		if ( !file ) { throw std::runtime_error( "Cannot open rules file" ); }
		const std::string source{ std::istreambuf_iterator< char >( file ), {} };
		if ( file.bad() ) { throw std::runtime_error( "Cannot read rules file" ); }
		const auto table = loader::txt::UnitRulesTXTLoader::ParseAbilities( source, argv[ 1 ] );
		size_t empty_abbreviations = 0, descriptions_with_semicolons = 0;
		size_t min_width = std::numeric_limits< size_t >::max(), max_width = 0;
		int32_t min_cost = std::numeric_limits< int32_t >::max(), max_cost = std::numeric_limits< int32_t >::min();
		for ( const auto& record : table.abilities ) {
			empty_abbreviations += record.abbreviation.empty();
			descriptions_with_semicolons += record.description.find( ';' ) != std::string::npos;
			min_width = std::min( min_width, record.flags.size() );
			max_width = std::max( max_width, record.flags.size() );
			min_cost = std::min( min_cost, record.cost );
			max_cost = std::max( max_cost, record.cost );
		}
		std::cout << "{\"ruleset\":\"" << table.ruleset_id << "\",\"records\":" << table.abilities.size()
			<< ",\"empty_abbreviations\":" << empty_abbreviations
			<< ",\"descriptions_with_semicolons\":" << descriptions_with_semicolons
			<< ",\"minimum_flag_width\":" << min_width << ",\"maximum_flag_width\":" << max_width
			<< ",\"minimum_cost\":" << min_cost << ",\"maximum_cost\":" << max_cost << "}\n";
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}
