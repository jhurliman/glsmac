#pragma once

#include <cstddef>
#include <vector>

namespace types { class Buffer; }
namespace game::backend::slot { class Slot; }

namespace game::backend::world_snapshot {

struct SlotCompletion {
	size_t id;
	bool completed;
};

// Owned wire values only. Runtime lifetime bindings belong to Game.
struct TurnState {
	size_t id;
	std::vector< SlotCompletion > players;
};

TurnState CaptureTurnState( size_t id, const std::vector< slot::Slot >& slots );
void WriteTurnState( types::Buffer& buffer, const TurnState& state );
TurnState ReadTurnState( types::Buffer& buffer, const std::vector< slot::Slot >& slots );
// Exact occupied roster, canonical indices, Player links and ordered records.
void ValidateTurnRoster( const TurnState& state, const std::vector< slot::Slot >& slots );

}
