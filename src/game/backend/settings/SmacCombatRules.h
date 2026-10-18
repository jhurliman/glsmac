#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include "types/Buffer.h"

namespace game::backend::settings {

// Frozen signed source percentages: RULES22,28,30. Arithmetic admission belongs
// to the combat caller; negative or modded source values are not defaulted.
struct SmacCombatRules {
	static constexpr size_t MAX_WIRE_BYTES = 4 * 14;
	int32_t intrinsic_base_defense, infantry_vs_base, fanatic_attack;
	bool operator==(const SmacCombatRules& other) const {
		return intrinsic_base_defense==other.intrinsic_base_defense && infantry_vs_base==other.infantry_vs_base
			&& fanatic_attack==other.fanatic_attack;
	}
	types::Buffer Serialize() const {
		types::Buffer buffer;buffer.WriteInt(1);buffer.WriteInt(intrinsic_base_defense);
		buffer.WriteInt(infantry_vs_base);buffer.WriteInt(fanatic_attack);return buffer;
	}
	static SmacCombatRules Deserialize(types::Buffer buffer) {
		if(buffer.lenw>MAX_WIRE_BYTES || buffer.ReadInt()!=1) {throw std::runtime_error("Unsupported SMAC combat profile");}
		std::array<int32_t,3> values;
		for(auto& value:values) {
			const auto input=buffer.ReadInt();
			if(input<INT32_MIN || input>INT32_MAX) {throw std::runtime_error("SMAC combat percentage exceeds signed32");}
			value=static_cast<int32_t>(input);
		}
		if(buffer.lenr!=buffer.lenw) {throw std::runtime_error("Trailing SMAC combat profile fields");}
		return {values[0],values[1],values[2]};
	}
};

}
