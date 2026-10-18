#pragma once

#include <array>
#include <cstdint>
#include <map>

namespace game::backend::faction {

// RE-SMAC-HOSTILE-STATE-LIFETIME-001. Stable logical fields, not the original
// memory layout. Paired counters retain their neutral names until all consumers
// are identified; neither is a derived count of surviving Units.
struct SmacHostileDirection {
	uint32_t primary = 0, secondary = 0;
	int32_t transition_turn = -1;
	uint32_t paired_a = 0, paired_b = 0;
	bool operator==(const SmacHostileDirection& other) const;
};
struct SmacHostileOwner {
	std::array<SmacHostileDirection,8> directions{};
	std::array<uint16_t,512> losses{};
	uint32_t integrity = 0, bookkeeping = 0;
	static SmacHostileOwner NewWorld(const std::array<uint32_t,8>& retained_secondary);
	bool operator==(const SmacHostileOwner& other) const;
};
struct SmacHostileState {
	std::map<int32_t,SmacHostileOwner> owners;
	bool operator==(const SmacHostileState& other) const;
	void Reinitialize(int32_t owner);
	void RequireOrdinaryHostility(int32_t actor,int32_t target) const;
	void RecordScoutBattle(int32_t actor,int32_t target,int32_t winner,int32_t loser,int32_t lost_design);
};

}
