#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <optional>
#include "types/Buffer.h"

namespace game::backend::settings {

// Frozen signed source percentages: RULES22,28,30 and PSI ratios18..20. Arithmetic admission belongs
// to the combat caller; negative or modded source values are not defaulted.
struct SmacCombatRules {
	static constexpr size_t MAX_WIRE_BYTES = 4 * 14 + 7 + 6 * 14 + 7 + 7 * 14;
    enum LandBonus { MobileAttack, RoughDefense, NoncombatDefensePenalty, SensorDefense, PlanetAttack, TranceDefense, EmpathAttack };
    using LandBonuses=std::array<int32_t,7>;
	int32_t intrinsic_base_defense, infantry_vs_base, fanatic_attack;
	using PsiRatios = std::array<std::array<int32_t,2>,3>; // land/sea/air; offense, defense
	// Old V1 port profiles never retained these inputs. Absence must not become
	// installed defaults when loading a campaign with potentially modified rules.
	std::optional<PsiRatios> psi_ratios;
    std::optional<LandBonuses> ordinary_land; // V3; earlier campaigns retain unknown inputs
	bool operator==(const SmacCombatRules& other) const {
		return intrinsic_base_defense==other.intrinsic_base_defense && infantry_vs_base==other.infantry_vs_base
			&& fanatic_attack==other.fanatic_attack && psi_ratios==other.psi_ratios && ordinary_land==other.ordinary_land;
	}
	types::Buffer Serialize() const {
		types::Buffer buffer;buffer.WriteInt(3);buffer.WriteInt(intrinsic_base_defense);
		buffer.WriteInt(infantry_vs_base);buffer.WriteInt(fanatic_attack);
		buffer.WriteBool(psi_ratios.has_value());
		if(psi_ratios) {for(const auto& ratio:*psi_ratios) {for(const auto value:ratio) {buffer.WriteInt(value);}}}
        buffer.WriteBool(ordinary_land.has_value());
        if(ordinary_land) {for(const auto value:*ordinary_land) {buffer.WriteInt(value);}}
		return buffer;
	}
	static SmacCombatRules Deserialize(types::Buffer buffer) {
		if(buffer.lenw>MAX_WIRE_BYTES) {throw std::runtime_error("SMAC combat profile exceeds wire budget");}
		const auto version=buffer.ReadInt();
		if(version!=1 && version!=2 && version!=3) {throw std::runtime_error("Unsupported SMAC combat profile");}
		const auto integer=[&]() {
			const auto input=buffer.ReadInt();
			if(input<INT32_MIN || input>INT32_MAX) {throw std::runtime_error("SMAC combat input exceeds signed32");}
			return static_cast<int32_t>(input);
		};
		SmacCombatRules result{integer(),integer(),integer()};
		if(version>=2 && buffer.ReadBool()) {
			result.psi_ratios.emplace();
			for(auto& ratio:*result.psi_ratios) {for(auto& value:ratio) {value=integer();}}
		}
        if(version>=3 && buffer.ReadBool()) {result.ordinary_land.emplace();for(auto& value:*result.ordinary_land) {value=integer();}}
		if(buffer.lenr!=buffer.lenw) {throw std::runtime_error("Trailing SMAC combat profile fields");}
		return result;
	}
};

}
