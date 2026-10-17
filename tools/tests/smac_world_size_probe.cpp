// Opt-in read-only production parser probe; no original data is distributed.
#include "loader/txt/SmacRulesTXTLoader.h"

#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>

int main( int argc, char** argv ) {
	if ( argc != 2 ) { std::cerr << "usage: smac_world_size_probe alpha.txt\n"; return 2; }
	std::ifstream input( argv[ 1 ], std::ios::binary );
	if ( !input ) { std::cerr << "cannot open supplied source\n"; return 2; }
	try {
		const auto rows = loader::txt::SmacRulesTXTLoader::ParseWorldSizeRules( std::string( std::istreambuf_iterator< char >( input ), {} ) );
		std::ostringstream output;
		for ( size_t i = 0; i < rows.presets.size(); ++i ) {
			const auto selected = rows.SelectPreset( i );
			output << selected.category << '\t' << rows.presets[ i ].name << '\t' << rows.presets[ i ].source_x << '\t'
				<< rows.presets[ i ].source_y << '\t' << selected.width << '\t' << selected.height << '\n';
		}
		std::cout << output.str();
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
	return 0;
}
