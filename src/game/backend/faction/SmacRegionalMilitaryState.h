#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace game::backend::faction {

// Per-owner summary outputs, not live sums at research-query time. Native
// ushort counters cover all128 regions in every positive faction slot,
// including currently unoccupied slots. Regional Base counts have their
// separate founding/removal delta lifetime in SmacEconomyState.
struct SmacRegionalMilitaryState {
    using Flags=std::array<uint16_t,128>;
    struct Owner {
        std::array<uint16_t,128> units{}, attack{};
        // Contribution bits from the owned-unit phase only, not the final
        // regional flag word after Base/peer/strategy processing. V18 lacked
        // this history; an actual owner summary can establish its own row.
        std::optional<std::array<uint16_t,128>> unit_phase_flags;
        // Base-stage snapshot, including the prior retained bit8. Missing
        // history cannot be recovered from current Base/terrain placement.
        std::optional<Flags> base_phase_flags;
        bool operator==(const Owner& other) const {return units==other.units && attack==other.attack && unit_phase_flags==other.unit_phase_flags && base_phase_flags==other.base_phase_flags;}
    };
    struct Contribution {
        uint8_t region, plan, domain;
        bool land;
        int32_t attack, armor, morale;
        uint8_t order;
        bool skip_strength;
        bool valid_base = false;
    };
    // One already admitted surviving Unit; the caller owns cleanup, actual
    // component/morale queries and reverse record order. No combat-odds or HP.
    static void Accumulate(Owner& owner,const Contribution& unit);
    static void ResetBaseFlags(Flags& flags,uint32_t turn,int32_t owner);
    static void AccumulateBaseFlag(Flags& flags,uint8_t region,bool immediate_sea);
    std::array<Owner,7> factions{};
    static constexpr size_t V18_WIRE_BYTES = 7*128*2*2;
    static constexpr size_t V19_WIRE_BYTES = V18_WIRE_BYTES + 7*(1+128*2);
    static constexpr size_t WIRE_BYTES = V19_WIRE_BYTES + 7*(1+128*2);
    static constexpr uint16_t UNIT_PHASE_MASK = 2 | 2048 | 4096 | 8192;
    static constexpr uint16_t BASE_PHASE_MASK = 8 | 128;
    bool operator==(const SmacRegionalMilitaryState& other) const {return factions==other.factions;}
};

}
