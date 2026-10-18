#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace game::backend::faction {

// Per-owner summary outputs, not live sums at research-query time. Native
// ushort counters cover all128 regions in every positive faction slot,
// including currently unoccupied slots. Regional Base counts have their
// separate founding/removal delta lifetime in SmacEconomyState.
struct SmacRegionalMilitaryState {
    struct Owner {
        std::array<uint16_t,128> units{}, attack{};
        bool operator==(const Owner& other) const {return units==other.units && attack==other.attack;}
    };
    struct Contribution {
        uint8_t region, plan, domain;
        bool land;
        int32_t attack, armor, morale;
        uint8_t order;
        bool skip_strength;
    };
    // One already admitted surviving Unit; the caller owns cleanup, actual
    // component/morale queries and reverse record order. No combat-odds or HP.
    static void Accumulate(Owner& owner,const Contribution& unit);
    std::array<Owner,7> factions{};
    static constexpr size_t WIRE_BYTES = 7*128*2*2;
    bool operator==(const SmacRegionalMilitaryState& other) const {return factions==other.factions;}
};

}
