// Opt-in actual producer/codec comparison. Proprietary sources stay external.
#include "game/backend/unit/SmacStartupDesignState.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "loader/txt/FactionTXTLoader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace game::backend;
namespace {
std::string Lower( std::string text ) { for ( auto& c : text ) { if ( c >= 'A' && c <= 'Z' ) { c = static_cast< char >( c - 'A' + 'a' ); } } return text; }
std::string Read( const std::filesystem::path& directory, const std::string& filename ) {
	std::filesystem::path found;
	for ( const auto& entry : std::filesystem::directory_iterator( directory ) ) {
		if ( Lower( entry.path().filename().string() ) == Lower( filename ) ) {
			if ( !found.empty() ) { throw std::runtime_error( "Ambiguous installed source" ); } found = entry.path();
		}
	}
	if ( found.empty() || !std::filesystem::is_regular_file( found ) || std::filesystem::file_size( found ) > 2 * 1024 * 1024 ) { throw std::runtime_error( "Missing or oversized installed source" ); }
	std::ifstream input( found, std::ios::binary ); if ( !input ) { throw std::runtime_error( "Cannot read installed source" ); }
	return { std::istreambuf_iterator< char >( input ), {} };
}
void Designs( const std::vector< unit::SmacCustomDesign >& designs ) {
	std::cout << '['; bool first = true;
	for ( const auto& d : designs ) {
		if ( !first ) { std::cout << ','; } first = false;
		std::cout << "{\"id\":" << d.id << ",\"chassis\":" << d.chassis << ",\"weapon\":" << d.weapon << ",\"armor\":" << d.defense
			<< ",\"carry\":" << unsigned( d.carry ) << ",\"reactor\":" << unsigned( d.reactor ) << ",\"abilities\":" << d.abilities
			<< ",\"cost\":" << unsigned( d.cost ) << ",\"plan\":" << unsigned( d.plan ) << ",\"active\":" << ( d.active ? "true" : "false" )
			<< ",\"prototyped\":" << ( d.prototyped ? "true" : "false" ) << ",\"faction_exclusion\":" << d.faction_exclusion << '}';
	}
	std::cout << ']';
}
}
int main( int argc, char** argv ) {
	if ( argc != 2 ) { std::cerr << "usage: probe INSTALLED_DIRECTORY\n"; return 2; }
	try {
		const std::filesystem::path directory( argv[ 1 ] ); const auto alpha = Read( directory, "alpha.txt" );
		const auto technologies = loader::txt::SmacRulesTXTLoader::ParseTechnologyRules( alpha );
		const auto profiles = loader::txt::FactionTXTLoader::ParseStartupRules( alpha, [ & ]( const std::string& filename ) { return Read( directory, filename ); }, technologies );
		const auto units = settings::SmacPredefinedUnitRules::Parse( alpha, technologies );
		std::string profile; size_t count = 0;
		while ( std::cin >> profile ) {
			int32_t owner = 0, difficulty = 0, stored = 0, network = 0;
			if ( ++count > 2048 || !( std::cin >> owner >> difficulty >> stored >> network ) || stored < 0 || stored > 5 || network < 0 || network > 1 ) { throw std::runtime_error( "Invalid bounded human generator request" ); }
			// Stored difficulty/network are supplied context validated by the probe.
			// Neither is persisted or substituted for the consumed global value.
			const auto state = faction::SmacTechnologyState::NewWorld( { { owner, profile } }, technologies, profiles );
			auto generated = unit::SmacStartupDesignState::GenerateEmptyOwner( state.factions[ 0 ], technologies, units.catalog, difficulty );
			std::cout << "{\"before\":"; Designs( generated.designs );
			unit::SmacStartupDesignState::MarkAndSelect( generated, units.catalog );
			std::cout << ",\"after\":"; Designs( generated.designs ); std::cout << ",\"selected\":" << generated.starting_scout;
			const auto combined = unit::SmacStartupDesignState::NewWorld( state, technologies, units, difficulty );
			if ( combined.owners.size() != 1 || !( combined.owners[ 0 ] == generated ) ) { throw std::runtime_error( "Composed new-world producer differs from separate stages" ); }
			const bool roundtrip = unit::SmacStartupDesignState::Deserialize( combined.Serialize( units.catalog ).ToString(), units.catalog ) == combined;
			std::cout << ",\"roundtrip\":" << ( roundtrip ? "true" : "false" ) << "}\n";
		}
		if ( !std::cin.eof() || !count ) { throw std::runtime_error( "Invalid or empty request stream" ); }
	} catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}
