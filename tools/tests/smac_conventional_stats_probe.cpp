// Opt-in supplied-input probe; no original executable or game assets are needed.
#include "game/backend/unit/SmacConventionalStats.h"

#include <array>
#include <charconv>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace game::backend::unit;

int main() {
	char line[ 512 ];
	while ( std::cin.getline( line, sizeof( line ) ) ) {
		const auto length = static_cast< size_t >( std::cin.gcount() ) - ( std::cin.eof() ? 0 : 1 );
		const std::string record( line, length );
		if ( record.find( '\0' ) != std::string::npos ) { return 2; }
		std::istringstream stream( record );
		std::string token;
		std::array< int32_t, 9 > v{};
		size_t count = 0;
		while ( stream >> token ) {
			if ( count == v.size() ) { return 2; }
			const auto parsed = std::from_chars( token.data(), token.data() + token.size(), v[ count ] );
			if ( parsed.ec != std::errc() || parsed.ptr != token.data() + token.size() ) { return 2; }
			++count;
		}
		if ( count != v.size() || v[ 0 ] < 0 || v[ 0 ] > 1 ) { return 2; }
		const auto bit = []( int32_t n ) { return n == 0 || n == 1; };
		try {
			int32_t result;
			if ( v[ 0 ] == 0 ) {
				// kind stored retained faction immunity home creche riots mode
				for ( size_t i = 4; i < 9; ++i ) { if ( !bit( v[ i ] ) ) { return 2; } }
				if ( v[ 5 ] == 0 && ( v[ 6 ] != 0 || v[ 7 ] != 0 ) ) { return 2; }
				const auto home = v[ 5 ] ? std::optional< SmacMoraleHome >( { v[ 6 ] != 0, v[ 7 ] != 0 } ) : std::nullopt;
				result = SmacConventionalMorale( { v[ 1 ], v[ 2 ], v[ 3 ], v[ 4 ] != 0, home, static_cast< SmacMoraleMode >( v[ 8 ] ) } );
			}
			else {
				// kind basic quantum domain reactor damage effective maritime suppress
				if ( v[ 3 ] < 0 || v[ 3 ] > 2 || v[ 4 ] < 0 || v[ 4 ] > 255 || v[ 5 ] < 0 || v[ 5 ] > 255 || !bit( v[ 7 ] ) || !bit( v[ 8 ] ) ) { return 2; }
				result = SmacConventionalAllowance( { v[ 1 ], v[ 2 ], static_cast< SmacMovementDomain >( v[ 3 ] ), static_cast< uint8_t >( v[ 4 ] ), static_cast< uint8_t >( v[ 5 ] ), v[ 6 ], v[ 7 ] != 0, v[ 8 ] != 0 } );
			}
			std::cout << "ok " << result << '\n';
		}
		catch ( const std::invalid_argument& ) { std::cout << "invalid\n"; }
		catch ( const std::overflow_error& ) { std::cout << "overflow\n"; }
	}
	return std::cin.bad() || ( std::cin.fail() && !std::cin.eof() ) ? 2 : 0;
}
