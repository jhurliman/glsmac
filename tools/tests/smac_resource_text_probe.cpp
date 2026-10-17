// Opt-in: parse a supplied rules file and print its three center triples plus
// 16 explicitly neutral resource/facility center queries. No live game setup.
#include "loader/txt/SmacRulesTXTLoader.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

int main( int argc, const char* const* argv ) {
	const bool full = argc == 3 && std::string( argv[ 1 ] ) == "--full-profile";
	if ( argc != 2 && !full ) { std::cerr << "usage: smac_resource_text_probe [--full-profile] rules-file\n"; return 2; }
	try {
		std::ifstream input( argv[ full ? 2 : 1 ], std::ios::binary | std::ios::ate );
		if ( !input || input.tellg() < 0 || input.tellg() > 16 * 1024 * 1024 ) { throw std::runtime_error( "rules input unavailable or exceeds probe limit" ); }
		std::string text( static_cast< size_t >( input.tellg() ), '\0' ); input.seekg( 0 );
		if ( !input.read( text.data(), static_cast< std::streamsize >( text.size() ) ) ) { throw std::runtime_error( "rules input read failed" ); }
		using namespace game::backend::resource::smac;
		if ( full ) {
			const auto technologies = loader::txt::SmacRulesTXTLoader::ParseTechnologyRules( text );
			const auto profile = loader::txt::SmacRulesTXTLoader::ParseResourceRules( text, technologies );
			const auto bytes = profile.Serialize( technologies ).ToString();
			const auto restored = game::backend::settings::SmacResourceRules::Deserialize( types::Buffer( bytes ), technologies );
			if ( !( restored == profile ) || restored.Serialize( technologies ).ToString() != bytes ) { throw std::runtime_error( "resource profile restore differs" ); }
			for ( const auto& row : profile.rows ) { std::cout << row[ 0 ] << ' ' << row[ 1 ] << ' ' << row[ 2 ] << '\n'; }
			std::cout << profile.nutrients_per_citizen << ' ' << profile.unroaded_mine_limit << ' ' << profile.mine_nutrient_effect << '\n';
			for ( const auto& token : profile.technology_tokens ) { std::cout << token << '\n'; }
			if ( !std::cout ) { throw std::runtime_error( "probe output failed" ); }
			return 0;
		}
		const auto rules = loader::txt::SmacRulesTXTLoader::ParseBaseSquareRules( text );
		for ( const auto& triple : { rules.base_square, rules.bonus_square, rules.recycling } ) {
			std::cout << triple[ 0 ] << ' ' << triple[ 1 ] << ' ' << triple[ 2 ] << '\n';
		}
		for ( size_t query = 0; query < 16; ++query ) {
			const auto value = CalculateBaseSquareYield( rules, {
				CenterTerrain( true, static_cast< ResourceBonus >( query / 4 ), { false, false, false }, false ),
				CenterFacilities( false, ( query & 1 ) != 0, ( query & 2 ) != 0 ),
				CenterFaction( 0, false, 2, false, false ),
				CenterEvents( { false, false, false }, { false, false, false }, false, false ) } );
			std::cout << value[ 0 ] << ' ' << value[ 1 ] << ' ' << value[ 2 ] << '\n';
		}
		if ( !std::cout ) { throw std::runtime_error( "probe output failed" ); }
		return 0;
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}
