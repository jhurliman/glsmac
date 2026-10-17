// Opt-in production probe; original inputs and receipts remain outside Git.
#include "game/backend/faction/SmacSocialState.h"
#include "loader/txt/FactionTXTLoader.h"
#include "loader/txt/SmacRulesTXTLoader.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace game::backend::faction;
using namespace game::backend::settings;

template< size_t N > void Write( const std::array< int32_t, N >& values ) {
	std::cout << '[';
	for ( size_t i = 0; i < N; ++i ) { if ( i ) { std::cout << ','; } std::cout << values[ i ]; }
	std::cout << ']';
}

int main( int argc, char** argv ) {
	if ( argc != 2 ) { std::cerr << "usage: probe ALPHA\n"; return 2; }
	try {
		std::ifstream source( argv[ 1 ], std::ios::binary ); if ( !source ) { throw std::runtime_error( "Cannot read explicit alpha source" ); }
		const std::string alpha( std::istreambuf_iterator< char >( source ), {} );
		const auto tech = loader::txt::SmacRulesTXTLoader::ParseTechnologyRules( alpha );
		auto profiles = loader::txt::FactionTXTLoader::ParseStartupRules( alpha, []( const std::string& filename ) {
			for ( const auto& binding : SmacFactionStartupRules::Bindings() ) {
				if ( filename == binding.filename ) { return "#" + std::string( binding.section ) + "\nSynthetic header\n\n#END\n"; }
			}
			throw std::runtime_error( "Unexpected source binding" );
		}, tech );
		int32_t simulation_id = 0, override_axis = -1, override_value = 0; size_t count = 0;
		if ( !( std::cin >> simulation_id >> override_axis >> override_value >> count ) || count > 128 || override_axis < -1 || override_axis > 10 ) {
			throw std::runtime_error( "Invalid bounded probe request" );
		}
		std::string bonuses;
		for ( size_t i = 0; i < count; ++i ) {
			std::string keyword, argument;
			if ( !( std::cin >> std::quoted( keyword ) >> std::quoted( argument ) ) || keyword.size() > 128 || argument.size() > 128
				|| keyword.find_first_of( ",;\r\n" ) != std::string::npos || argument.find_first_of( ",;\r\n" ) != std::string::npos ) {
				throw std::runtime_error( "Invalid probe bonus pair" );
			}
			if ( i ) { bonuses += ','; } bonuses += keyword + ',' + argument;
		}
		std::string extra; if ( std::cin >> extra ) { throw std::runtime_error( "Unexpected probe fields" ); }
		profiles.factions[ 0 ].bonuses = loader::txt::FactionTXTLoader::ParseStartupBonuses(
			"#GAIANS\nSynthetic header\n" + bonuses + "\n#END\n", "GAIANS", tech );
		auto state = SmacSocialState::NewWorld( { { simulation_id, "GAIANS" } }, tech, profiles );
		const auto early = state.factions.front();
		state.RefreshForFirstCommand( tech, profiles );
		auto& owner = state.factions.front();
		if ( override_axis >= 0 ) {
			// The original boundary cases explicitly replace this resolved input
			// after intrinsic construction. This is not a file-producer claim.
			owner.intrinsic->at( override_axis ) = override_value;
			owner.current = SmacSocialState::RefreshNeutral( *owner.intrinsic, "GAIANS", tech, profiles );
			owner.retained = owner.current;
		}
		const auto wire = state.Serialize( tech, profiles ).ToString();
		const bool roundtrip = SmacSocialState::Deserialize( wire, tech, profiles ) == state;
		std::cout << "{\"records\":" << profiles.factions[ 0 ].Records().size() << ",\"early_models\":"; Write( early.selected );
		std::cout << ",\"early_current\":"; Write( early.current );
		std::cout << ",\"early_unknown\":" << ( !early.intrinsic && !early.retained ? "true" : "false" );
		std::cout << ",\"intrinsic\":"; Write( *owner.intrinsic ); std::cout << ",\"current\":"; Write( owner.current );
		std::cout << ",\"retained\":"; Write( *owner.retained ); std::cout << ",\"roundtrip\":" << ( roundtrip ? "true" : "false" ) << "}\n";
	} catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}
