#pragma once

#include "util/random/Types.h"

namespace types { class Buffer; }

namespace game::backend::world_snapshot {

// V2 world snapshots carry this block after the frozen rules profile.
// Every four-word JSF32 state, including its absorbing zero state, is retained.
void WriteRandomState( types::Buffer& buffer, const util::random::state_t& state );
util::random::state_t ReadRandomState( types::Buffer& buffer );

}
