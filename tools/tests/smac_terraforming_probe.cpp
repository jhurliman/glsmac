// Opt-in installed source check: parser -> owned codec -> technology resolution.
#include "loader/txt/SmacRulesTXTLoader.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

int main( int argc, char** argv ) {
	if ( argc != 2 ) { std::cerr << "usage: smac_terraforming_probe alpha.txt\n"; return 2; }
	try {
		std::ifstream input( argv[ 1 ], std::ios::binary );
		if ( !input ) { throw std::runtime_error( "Cannot open source" ); }
		input.seekg( 0, std::ios::end );
		if ( input.tellg() < 0 || input.tellg() > 1024 * 1024 ) { throw std::runtime_error( "Source exceeds probe budget" ); }
		input.seekg( 0 ); const std::string source( std::istreambuf_iterator< char >( input ), {} );
		using loader::txt::SmacRulesTXTLoader;
		using namespace game::backend::settings;
		const auto parsed = SmacRulesTXTLoader::ParseTerraformingRules( source );
		const auto decoded = SmacTerraformingRules::Deserialize( parsed.Serialize() );
		if ( !( parsed == decoded ) ) { throw std::runtime_error( "Catalog roundtrip changed fields" ); }
		const auto refs = decoded.TechnologyReferences( SmacRulesTXTLoader::ParseTechnologyRules( source ) );
		const auto id = []( const SmacTechnologyReference& ref ) {
			if ( ref.kind == SmacTechnologyReference::Kind::NONE ) { return -1; }
			if ( ref.kind == SmacTechnologyReference::Kind::DISABLED ) { return -2; }
			if ( ref.kind == SmacTechnologyReference::Kind::TECHNOLOGY_ID ) { return *ref.technology_id; }
			throw std::runtime_error( "Installed prerequisite unresolved" );
		};
		for ( size_t i = 0; i < decoded.actions.size(); ++i ) {
			std::cout << decoded.actions[ i ].source_id << ' ' << id( refs[ i ][ 0 ] ) << ' ' << id( refs[ i ][ 1 ] ) << ' ' << decoded.actions[ i ].work_time << '\n';
		}
	} catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
	return 0;
}
