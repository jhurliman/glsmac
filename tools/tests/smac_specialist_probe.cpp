// Opt-in installed-source parser/codec probe; no specialist selection or allocation.
#include "loader/txt/SmacRulesTXTLoader.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

int main( int argc, char** argv ) {
	if ( argc != 2 ) { std::cerr << "usage: smac_specialist_probe alpha.txt\n"; return 2; }
	try {
		std::ifstream input( argv[ 1 ], std::ios::binary );
		if ( !input ) { throw std::runtime_error( "Cannot open supplied source" ); }
		input.seekg( 0, std::ios::end );
		if ( input.tellg() < 0 || input.tellg() > 1024 * 1024 ) { throw std::runtime_error( "Source exceeds probe budget" ); }
		input.seekg( 0 );
		const std::string source( std::istreambuf_iterator< char >( input ), {} );
		using loader::txt::SmacRulesTXTLoader;
		using game::backend::settings::SmacSpecialistRules;
		using game::backend::settings::SmacTechnologyReference;
		const auto parsed = SmacRulesTXTLoader::ParseSpecialistRules( source );
		const auto profile = SmacSpecialistRules::Deserialize( parsed.Serialize() );
		if ( !( profile == parsed ) ) { throw std::runtime_error( "Owned profile roundtrip changed fields" ); }
		const auto references = profile.TechnologyReferences( SmacRulesTXTLoader::ParseTechnologyRules( source ) );
		const auto id = []( const SmacTechnologyReference& ref ) -> int32_t {
			switch ( ref.kind ) {
				case SmacTechnologyReference::Kind::TECHNOLOGY_ID: return *ref.technology_id;
				case SmacTechnologyReference::Kind::NONE: return -1;
				case SmacTechnologyReference::Kind::DISABLED: return -2;
				default: throw std::runtime_error( "Installed comparison requires resolved technology references" );
			}
		};
		std::ostringstream output;
		output << "minimum " << profile.minimum_base_size << '\n';
		for ( size_t i = 0; i < profile.specialists.size(); ++i ) {
			const auto& row = profile.specialists[ i ];
			output << row.source_id << ' ' << std::quoted( row.singular ) << ' ' << std::quoted( row.plural )
				<< ' ' << id( references[ i ][ 0 ] ) << ' ' << id( references[ i ][ 1 ] )
				<< ' ' << row.economy << ' ' << row.psych << ' ' << row.labs
				<< ' ' << std::quoted( row.prerequisite ) << ' ' << std::quoted( row.obsolete )
				<< ' ' << std::quoted( row.trailing_token ) << '\n';
		}
		std::cout << output.str();
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
	return 0;
}
