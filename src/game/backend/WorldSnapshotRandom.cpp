#include "WorldSnapshotRandom.h"

#include "types/Buffer.h"

#include <limits>
#include <stdexcept>

namespace game::backend::world_snapshot {
namespace {
constexpr auto algorithm = "jsf32-v1";
}

void WriteRandomState( types::Buffer& buffer, const util::random::state_t& state ) {
	buffer.WriteString( algorithm );
	for ( const auto word : { state.a, state.b, state.c, state.d } ) { buffer.WriteInt( word ); }
}

util::random::state_t ReadRandomState( types::Buffer& buffer ) {
	if ( buffer.ReadString() != algorithm ) { throw std::runtime_error( "Unsupported world snapshot RNG algorithm" ); }
	util::random::state_t result = {};
	for ( auto* word : { &result.a, &result.b, &result.c, &result.d } ) {
		const auto value = buffer.ReadInt();
		if ( value < 0 || value > std::numeric_limits< uint32_t >::max() ) {
			throw std::runtime_error( "World snapshot RNG word is outside uint32 range" );
		}
		*word = static_cast< uint32_t >( value );
	}
	return result;
}

}
