#pragma once

#include <cstdint>
#include <string>

namespace game::frontend::resource {

// A display decision never changes the authoritative signed value. Nonzero
// amounts without an exact asset use one identifying icon and an exact label.
struct ResourceDisplay {
	size_t sprite_value = 0;
	std::string label;

	static ResourceDisplay For( const int64_t value, const size_t available_sprites ) {
		if ( value == 0 ) { return {}; }
		if ( value > 0 && static_cast< uint64_t >( value ) <= available_sprites ) {
			return { static_cast< size_t >( value ), "" };
		}
		return { available_sprites ? size_t( 1 ) : size_t( 0 ), std::to_string( value ) };
	}
};

}
