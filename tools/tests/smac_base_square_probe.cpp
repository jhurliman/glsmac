// Opt-in batch probe: one line of 31 explicit integer inputs per center.
#include "game/backend/resource/SmacBaseSquareYield.h"

#include <array>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
using namespace game::backend::resource::smac;

struct Input {
	std::array< int64_t, 31 > values;
	size_t next = 0;
	int64_t Take( const int64_t minimum, const int64_t maximum ) {
		const auto value = values.at( next++ );
		if ( value < minimum || value > maximum ) { throw std::invalid_argument( "input outside field domain" ); }
		return value;
	}
	int32_t Signed() { return static_cast< int32_t >( Take( INT32_MIN, INT32_MAX ) ); }
	bool Flag() { return Take( 0, 1 ) != 0; }
	RuleTriple Rules() { return { Signed(), Signed(), Signed() }; }
	ResourceFlags Flags() { return { Flag(), Flag(), Flag() }; }
	Yield Calculate() {
		const BaseSquareRules rules{ Rules(), Rules(), Rules() };
		const CenterTerrain terrain{ Flag(), static_cast< ResourceBonus >( Take( 0, 3 ) ), Flags(), Flag() };
		const CenterFacilities facilities{ Flag(), Flag(), Flag() };
		const CenterFaction faction{ Signed(), Flag(), static_cast< uint32_t >( Take( 1, UINT32_MAX ) ), Flag(), Flag() };
		const CenterEvents events{ Flags(), Flags(), Flag(), Flag() };
		return CalculateBaseSquareYield( rules, { terrain, facilities, faction, events } );
	}
};
}

int main( const int argc, const char* const* ) {
	if ( argc != 1 ) { std::cerr << "usage: smac_base_square_probe < cases.txt\n"; return 2; }
	try {
		std::string line;
		while ( std::getline( std::cin, line ) ) {
			Input input{};
			std::istringstream fields( line );
			for ( auto& value : input.values ) {
				if ( !( fields >> value ) ) { throw std::invalid_argument( "expected 31 integer fields" ); }
			}
			fields >> std::ws;
			if ( !fields.eof() ) { throw std::invalid_argument( "unexpected trailing fields" ); }
			const auto result = input.Calculate();
			std::cout << result[ 0 ] << ' ' << result[ 1 ] << ' ' << result[ 2 ] << '\n';
		}
		if ( std::cin.bad() || !std::cout ) { throw std::runtime_error( "probe stream failure" ); }
		return 0;
	}
	catch ( const std::exception& error ) { std::cerr << error.what() << '\n'; return 1; }
}
