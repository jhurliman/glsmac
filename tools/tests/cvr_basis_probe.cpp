// Opt-in production caller: one line of nine hexadecimal binary32 words.
#include "loader/cvr/CvrBasisConverter.h"

#include <charconv>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

int main() {
	char line[ 1024 ];
	while ( std::cin.getline( line, sizeof( line ) ) ) {
		std::istringstream input( line );
		std::array< uint32_t, 9 > bits;
		for ( auto& value : bits ) {
			std::string token;
			if ( !( input >> token ) ) { std::cerr << "Expected nine binary32 words\n"; return 2; }
			const auto* begin = token.data();
			const auto* end = begin + token.size();
			if ( token.size() >= 2 && token[ 0 ] == '0' && token[ 1 ] == 'x' ) { begin += 2; }
			const auto parsed = std::from_chars( begin, end, value, 16 );
			if ( parsed.ec != std::errc{} || parsed.ptr != end ) { std::cerr << "Invalid uint32 hex word\n"; return 2; }
		}
		std::string extra;
		if ( input >> extra ) { std::cerr << "Unexpected trailing token\n"; return 2; }
		try {
			const auto basis = loader::cvr::CvrBasisConverter::Convert( bits );
			std::cout << "ok";
			for ( const auto& row : basis ) { for ( const auto value : row ) { std::cout << ' ' << value; } }
			std::cout << '\n';
		}
		catch ( const std::invalid_argument& ) { std::cout << "invalid\n"; }
	}
	return std::cin.bad() || !std::cin.eof() ? 2 : 0;
}
