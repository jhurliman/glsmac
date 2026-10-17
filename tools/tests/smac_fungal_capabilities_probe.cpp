// Opt-in: installed source files stay local; possession is explicitly supplied.
#include "game/backend/faction/SmacTechnologyState.h"
#include "loader/txt/FactionTXTLoader.h"
#include "loader/txt/SmacRulesTXTLoader.h"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
std::string Read( const std::filesystem::path& path ) {
	std::ifstream input( path, std::ios::binary | std::ios::ate );
	if ( !input || input.tellg() < 0 || input.tellg() > 16 * 1024 * 1024 ) { throw std::runtime_error( "source unavailable or exceeds probe limit" ); }
	std::string result( static_cast< size_t >( input.tellg() ), '\0' ); input.seekg( 0 );
	if ( !input.read( result.data(), static_cast< std::streamsize >( result.size() ) ) ) { throw std::runtime_error( "source read failed" ); }
	return result;
}
}

int main( int argc, const char* const* argv ) {
	if ( argc < 3 || argc > 92 ) { std::cerr << "usage: smac_fungal_capabilities_probe source-directory faction-id [owned-id ...]\n"; return 2; }
	try {
		using namespace game::backend;
		const std::filesystem::path directory( argv[ 1 ] );
		const auto alpha = Read( directory / "alpha.txt" );
		const auto catalog = loader::txt::SmacRulesTXTLoader::ParseTechnologyRules( alpha );
		const auto profiles = loader::txt::FactionTXTLoader::ParseStartupRules( alpha, [ & ]( const std::string& name ) { return Read( directory / name ); }, catalog );
		auto state = faction::SmacTechnologyState::NewWorld( { { 1, argv[ 2 ] } }, catalog, profiles );
		state.ClearOwnershipForNewWorld(); // arguments are a complete observed set, not extra grants
		for ( int i = 3; i < argc; ++i ) {
			const std::string token( argv[ i ] ); int32_t id = -1;
			const auto parsed = std::from_chars( token.data(), token.data() + token.size(), id );
			if ( parsed.ec != std::errc() || parsed.ptr != token.data() + token.size() || id < 0 || id >= 89 || state.factions[ 0 ].owned[ id ] ) { throw std::invalid_argument( "owned ID must be distinct decimal0..88" ); }
			state.factions[ 0 ].owned[ id ] = true;
		}
		const auto before = state.Serialize( catalog, profiles ).ToString();
		const auto actual = state.FungalCapabilities( 1, catalog, profiles );
		const auto restored_catalog = settings::SmacTechnologyRules::Deserialize( catalog.Serialize() );
		const auto restored_profiles = settings::SmacFactionStartupRules::Deserialize( profiles.Serialize( catalog ), restored_catalog );
		const auto restored = faction::SmacTechnologyState::Deserialize( before, restored_catalog, restored_profiles );
		if ( !( restored.FungalCapabilities( 1, restored_catalog, restored_profiles ) == actual ) || restored.Serialize( restored_catalog, restored_profiles ).ToString() != before
			|| state.Serialize( catalog, profiles ).ToString() != before ) { throw std::runtime_error( "owned restore/recomputation differs" ); }
		std::cout << actual.nutrients << ' ' << actual.minerals << ' ' << actual.energy << '\n';
		if ( !std::cout ) { throw std::runtime_error( "probe output failed" ); }
		return 0;
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}
