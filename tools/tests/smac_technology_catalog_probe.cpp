// Optional read-only local probe; source path and tokens are explicit inputs.
#include "loader/txt/SmacRulesTXTLoader.h"

#include <fstream>
#include <iostream>
#include <iterator>

int main( int argc, char** argv ) {
	if ( argc != 2 && !( argc == 4 && std::string( argv[ 2 ] ) == "--resolve" ) ) {
		std::cerr << "usage: smac_technology_catalog_probe alpha.txt [--resolve token]\n"; return 2;
	}
	std::ifstream input( argv[ 1 ], std::ios::binary );
	if ( !input ) { std::cerr << "cannot open supplied source\n"; return 2; }
	try {
		const auto catalog = loader::txt::SmacRulesTXTLoader::ParseTechnologyRules( std::string( std::istreambuf_iterator< char >( input ), {} ) );
		if ( argc == 4 ) {
			const auto reference = catalog.ResolveToken( argv[ 3 ] );
			const auto production = catalog.ResolveProductionPrerequisite( argv[ 3 ] );
			std::cout << static_cast< int >( reference.kind ) << '\t' << reference.technology_id.value_or( -1 ) << '\t'
				<< static_cast< int >( production.status ) << '\t' << production.technology_id.value_or( -1 ) << '\n';
		}
		else {
			for ( const auto& row : catalog.technologies ) {
				std::cout << row.source_id << '\t' << row.name << '\t' << row.key;
				for ( const auto value : row.priorities ) { std::cout << '\t' << value; }
				for ( const auto& prerequisite : row.prerequisites ) { std::cout << '\t' << prerequisite; }
				std::cout << '\t' << row.flags << '\n';
			}
		}
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
	return 0;
}
