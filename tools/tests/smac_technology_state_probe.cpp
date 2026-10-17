// Opt-in production probe. Original data/reports remain outside the repository.
#include "game/backend/faction/SmacTechnologyState.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "loader/txt/FactionTXTLoader.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace game::backend::faction;
using namespace game::backend::settings;

int main( int argc, char** argv ) {
	if ( argc != 2 ) { std::cerr << "usage: probe ALPHA\n"; return 2; }
	try {
		std::ifstream source( argv[ 1 ], std::ios::binary ); if ( !source ) { throw std::runtime_error( "Cannot read explicit alpha source" ); }
		const std::string alpha( std::istreambuf_iterator< char >( source ), {} );
		const auto tech = loader::txt::SmacRulesTXTLoader::ParseTechnologyRules( alpha );
		int32_t recipient = 0, additional = 0, seeded = 0, score = 0, future = 0, labs = 0, target = 0, cache = 0;
		size_t count = 0;
		if ( !( std::cin >> recipient >> additional >> seeded >> score >> future >> labs >> target >> cache >> count ) || count > 128 ) { throw std::runtime_error( "Invalid bounded probe request" ); }
		auto profiles = loader::txt::FactionTXTLoader::ParseStartupRules( alpha, []( const std::string& filename ) {
			for ( const auto& binding : SmacFactionStartupRules::Bindings() ) {
				if ( filename == binding.filename ) { return "#" + std::string( binding.section ) + "\nSynthetic header\n\n#END\n"; }
			}
			throw std::runtime_error( "Unexpected fixture source" );
		}, tech );
		for ( size_t i = 0 ; i < count ; ++i ) {
			std::string key, value;
			if ( !( std::cin >> std::quoted( key ) >> std::quoted( value ) ) || key.size() > 128 || value.size() > 128 ) { throw std::runtime_error( "Invalid probe bonus" ); }
			profiles.factions[ 0 ].bonuses.push_back( SmacFactionStartupBonus::Parse( key, value, tech ) );
		}
		std::string extra; if ( std::cin >> extra ) { throw std::runtime_error( "Unexpected probe fields" ); }
		// Each test explicitly admits the seven playable IDs. Sharing one source
		// profile is supplied input, not a template-to-simulation mapping policy.
		std::vector< SmacSimulationAdmission > admissions;
		for ( int32_t id = 1 ; id <= 7 ; ++id ) { admissions.push_back( { id, "GAIANS" } ); }
		auto state = SmacTechnologyState::NewWorld( admissions, tech, profiles );
		for ( auto& faction : state.factions ) {
			faction.owned.fill( true ); faction.score = score; faction.future_count = future; faction.labs = labs;
			faction.target = target == -1 ? std::nullopt : std::optional< int32_t >( target );
			faction.cached_threshold = cache == -1 ? std::nullopt : std::optional< int32_t >( cache );
		}
		state.ClearOwnershipForNewWorld(); bool cleared = true;
		for ( const auto& faction : state.factions ) { for ( const auto owned : faction.owned ) { cleared = cleared && !owned; } }
		if ( seeded ) {
			if ( seeded < 1 || seeded > 7 ) { throw std::runtime_error( "Invalid seeded owner" ); }
			state.factions[ seeded - 1 ].owned[ 10 ] = true;
		}
		state.InitializeFactionAtTurnZero( recipient, tech, profiles );
		if ( additional ) { state.InitializeFactionAtTurnZero( additional, tech, profiles ); }
		const auto* faction = state.Find( recipient );
		const auto wire = state.Serialize( tech, profiles ).ToString();
		const bool roundtrip = SmacTechnologyState::Deserialize( wire, tech, profiles ) == state;
		std::cout << "{\"cleared\":" << ( cleared ? "true" : "false" ) << ",\"roundtrip\":" << ( roundtrip ? "true" : "false" )
			<< ",\"research\":[" << faction->score << ',' << faction->future_count << ',' << faction->labs << ','
			<< faction->target.value_or( -1 ) << ',' << faction->cached_threshold.value_or( -1 ) << "],\"owners\":{";
		bool first_tech = true;
		for ( size_t id = 0 ; id < 89 ; ++id ) {
			bool any = false; for ( const auto& owner : state.factions ) { any = any || owner.owned[ id ]; } if ( !any ) { continue; }
			if ( !first_tech ) { std::cout << ','; } first_tech = false;
			std::cout << '\"' << id << "\":["; bool first_owner = true;
			for ( const auto& owner : state.factions ) {
				if ( !owner.owned[ id ] ) { continue; } if ( !first_owner ) { std::cout << ','; } first_owner = false;
				std::cout << owner.admission.simulation_id;
			}
			std::cout << ']';
		}
		std::cout << "}}\n";
	} catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}
