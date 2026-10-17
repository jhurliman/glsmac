// Opt-in, asset-free probe linked directly to the production calculator.
#include "game/backend/resource/SmacMineralCostFactor.h"

#include <array>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

int main() {
	using namespace game::backend::resource::smac;
	std::string line;
	while ( std::getline( std::cin, line ) ) {
		std::istringstream input( line );
		std::array< int64_t, 8 > values;
		for ( auto& value : values ) {
			if ( !( input >> value ) || value < std::numeric_limits< int32_t >::min() || value > std::numeric_limits< int32_t >::max() ) {
				std::cerr << "Expected eight signed32 inputs\n"; return 2;
			}
		}
		std::string extra;
		if ( input >> extra ) { std::cerr << "Unexpected trailing input\n"; return 2; }
		for ( const size_t index : { 0, 2, 3, 4 } ) {
			if ( values[ index ] < 0 || values[ index ] > 1 ) { std::cerr << "Boolean input must be 0 or 1\n"; return 2; }
		}
		try {
			const auto factor = CalculateMineralCostFactor( {
				values[ 0 ] != 0, static_cast< int32_t >( values[ 1 ] ), values[ 2 ] != 0,
				values[ 3 ] != 0, values[ 4 ] != 0, static_cast< MineralCostWorldSize >( values[ 5 ] ),
				static_cast< int32_t >( values[ 6 ] ), static_cast< int32_t >( values[ 7 ] )
			} );
			std::cout << "ok " << factor << '\n';
		}
		catch ( const std::invalid_argument& ) { std::cout << "invalid\n"; }
		catch ( const std::overflow_error& ) { std::cout << "overflow\n"; }
	}
	return std::cin.bad() ? 2 : 0;
}
