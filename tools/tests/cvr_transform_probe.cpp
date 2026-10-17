// Opt-in production caller: C9,T3,F9,L3,scalar binary32 hex words, then H3 signed decimals.
#include "loader/cvr/CvrTransformCalculator.h"

#include <charconv>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
bool Word( std::istream& input, uint32_t& value ) {
	std::string token;
	if ( !( input >> token ) ) { return false; }
	const auto* begin = token.data();
	const auto* end = begin + token.size();
	if ( token.size() >= 2 && token[ 0 ] == '0' && token[ 1 ] == 'x' ) { begin += 2; }
	const auto parsed = std::from_chars( begin, end, value, 16 );
	return parsed.ec == std::errc{} && parsed.ptr == end;
}
}

int main() {
	char line[ 2048 ];
	while ( std::cin.getline( line, sizeof( line ) ) ) {
		std::istringstream input( line );
		loader::cvr::CvrTransformInput request;
		bool valid = true;
		for ( auto& v : request.transform_bits ) { valid = Word( input, v ) && valid; }
		for ( auto& v : request.translation_bits ) { valid = Word( input, v ) && valid; }
		for ( auto& v : request.frame_matrix_bits ) { valid = Word( input, v ) && valid; }
		for ( auto& v : request.frame_location_bits ) { valid = Word( input, v ) && valid; }
		valid = Word( input, request.scalar_bits ) && valid;
		for ( auto& v : request.header_vector ) {
			std::string token;
			int32_t parsed_value = 0;
			if ( !( input >> token ) ) { valid = false; continue; }
			const auto parsed = std::from_chars( token.data(), token.data() + token.size(), parsed_value );
			if ( parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size() ||
				parsed_value < std::numeric_limits< int16_t >::min() || parsed_value > std::numeric_limits< int16_t >::max() ) { valid = false; }
			else { v = static_cast< int16_t >( parsed_value ); }
		}
		std::string extra;
		if ( !valid || input >> extra ) { std::cerr << "Expected 25 uint32 hex words and three signed16 decimal values\n"; return 2; }
		try {
			const auto result = loader::cvr::CvrTransformCalculator::Calculate( request );
			std::cout << "ok" << std::hex << std::setfill( '0' );
			for ( const auto v : result.matrix_bits ) { std::cout << ' ' << std::setw( 8 ) << v; }
			for ( const auto v : result.location_bits ) { std::cout << ' ' << std::setw( 8 ) << v; }
			for ( const auto v : result.origin_bits ) { std::cout << ' ' << std::setw( 8 ) << v; }
			std::cout << std::dec;
			for ( const auto& row : result.basis ) { for ( const auto v : row ) { std::cout << ' ' << v; } }
			for ( const auto v : result.origin ) { std::cout << ' ' << v; }
			std::cout << '\n';
		}
		catch ( const std::invalid_argument& ) { std::cout << "invalid\n"; }
	}
	return std::cin.bad() || !std::cin.eof() ? 2 : 0;
}
