// Optional local probe: reads a supplied rules file; never searches installations.
#include "loader/txt/SmacRulesTXTLoader.h"

#include <fstream>
#include <iostream>
#include <iterator>

int main( int argc, char** argv ) {
	if ( argc != 2 ) { std::cerr << "usage: smac_production_profile_probe alpha.txt\n"; return 2; }
	std::ifstream input( argv[ 1 ], std::ios::binary );
	if ( !input ) { std::cerr << "cannot open supplied source\n"; return 2; }
	try {
		const auto rules = loader::txt::SmacRulesTXTLoader::ParseProductionRules( std::string( std::istreambuf_iterator< char >( input ), {} ) );
		std::cout << rules.mineral_multiplier << '\t' << rules.retool_loss_percent << '\t' << rules.retool_strictness << '\t' << rules.retool_exemption << '\n';
		for ( const auto& facility : rules.facilities ) {
			std::cout << facility.source_id << '\t' << facility.name << '\t' << facility.cost << '\t' << facility.maintenance << '\t' << facility.prerequisite << '\n';
		}
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
	return 0;
}
