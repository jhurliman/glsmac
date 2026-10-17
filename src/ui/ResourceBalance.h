#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace ui {

// Presentation only: exact labels, with independently bounded cell counts.
// Neither the original values nor their possibly wider difference is clamped.
struct ResourceBalance {
	std::string intake_consumption;
	std::string surplus;
	int64_t loss_cells;
	int64_t profit_cells;

	static ResourceBalance Format( const int64_t profit, const int64_t loss, const int64_t total_cells ) {
		if ( total_cells < 0 ) { throw std::invalid_argument( "Resource cell count must be nonnegative" ); }
		const bool positive = profit > loss;
		// Unsigned subtraction is defined modulo 2^64. Ordering first makes
		// this the exact nonnegative distance, including INT64_MAX - INT64_MIN.
		const auto magnitude = positive
			? static_cast< uint64_t >( profit ) - static_cast< uint64_t >( loss )
			: static_cast< uint64_t >( loss ) - static_cast< uint64_t >( profit );
		auto difference = Pad( std::to_string( magnitude ) );
		if ( profit != loss ) { difference.insert( 0, positive ? "+" : "-" ); }
		return {
			Amount( profit ) + " - " + Amount( loss ), difference,
			std::clamp< int64_t >( loss, 0, total_cells ),
			positive ? static_cast< int64_t >( std::min( magnitude, static_cast< uint64_t >( total_cells ) ) ) : 0,
		};
	}

private:
	static std::string Pad( std::string text ) {
		if ( text.size() < 3 ) { text.insert( 0, 3 - text.size(), '0' ); }
		return text;
	}
	static std::string Amount( const int64_t value ) {
		return value < 0 ? std::to_string( value ) : Pad( std::to_string( value ) );
	}
};

}
