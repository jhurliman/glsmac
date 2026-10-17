#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace ui {

// Display-only arithmetic. Resource operands remain signed64, while their
// difference may need a sign plus all64 magnitude bits. Only visible cell
// counts are clipped; exact turn labels and authoritative values are not.
struct ResourceProgress {
	int64_t direction;
	int64_t filled_cells;
	int64_t end_cells;
	std::string turns;

	static ResourceProgress Format( const int64_t profit, const int64_t loss, const int64_t filled, const int64_t total_cells ) {
		if ( total_cells < 0 ) { throw std::invalid_argument( "Resource cell count must be nonnegative" ); }
		const auto direction = profit > loss ? 1 : profit < loss ? -1 : 0;
		const uint64_t magnitude = direction > 0
			? static_cast< uint64_t >( profit ) - static_cast< uint64_t >( loss )
			: static_cast< uint64_t >( loss ) - static_cast< uint64_t >( profit );
		const auto cap = static_cast< uint64_t >( total_cells );
		uint64_t end = 0;
		if ( direction > 0 ) {
			if ( filled < 0 ) {
				const auto deficit = uint64_t( 0 ) - static_cast< uint64_t >( filled );
				end = magnitude > deficit ? std::min( magnitude - deficit, cap ) : 0;
			}
			else {
				const auto visible = std::min( static_cast< uint64_t >( filled ), cap );
				end = visible + std::min( magnitude, cap - visible );
			}
		}
		else if ( filled > 0 && static_cast< uint64_t >( filled ) > magnitude ) {
			end = std::min( static_cast< uint64_t >( filled ) - magnitude, cap );
		}
		std::string turns = "0";
		if ( direction > 0 ) {
			// ceil((capacity - stored) / positive pending), including negative
			// numerators and distances through UINT64_MAX. Quotient/remainder
			// avoids both floating conversion and the usual n+d-1 overflow.
			const bool negative = filled > total_cells;
			const uint64_t remaining = negative
				? static_cast< uint64_t >( filled ) - cap
				: cap - static_cast< uint64_t >( filled );
			const auto quotient = remaining / magnitude;
			turns = negative
				? ( quotient ? "-" + std::to_string( quotient ) : "0" )
				: std::to_string( quotient + ( remaining % magnitude != 0 ) );
		}
		return { direction, std::clamp< int64_t >( filled, 0, total_cells ), static_cast< int64_t >( end ), turns };
	}
};

// The base screen's existing placeholder split: nearest40% labs, nearest20%
// psych, exact remainder economy. This does not calculate gameplay allocation.
struct PlaceholderEnergyAllocation {
	int64_t labs;
	int64_t psych;
	int64_t economy;

	static PlaceholderEnergyAllocation Format( const int64_t profit, const int64_t loss ) {
		const bool negative = profit < loss;
		const uint64_t magnitude = negative
			? static_cast< uint64_t >( loss ) - static_cast< uint64_t >( profit )
			: static_cast< uint64_t >( profit ) - static_cast< uint64_t >( loss );
		const auto labs = magnitude / 5 * 2 + ( magnitude % 5 * 2 + 2 ) / 5;
		const auto psych = magnitude / 5 + ( magnitude % 5 + 2 ) / 5;
		const auto economy = magnitude - labs - psych;
		// Each share is at most two fifths of UINT64_MAX, rounded by at most1,
		// so conversion and subsequent negation are representable signed64.
		const auto signed_share = [ negative ]( const uint64_t share ) {
			return negative ? -static_cast< int64_t >( share ) : static_cast< int64_t >( share );
		};
		return { signed_share( labs ), signed_share( psych ), signed_share( economy ) };
	}
};

}
