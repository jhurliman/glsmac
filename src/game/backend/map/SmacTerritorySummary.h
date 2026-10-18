#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace game::backend::map::smac_territory {

// RE-SMAC-REGIONAL-LIFECYCLE-002. These are retained territory-pass outputs,
// separate from per-owner military/Base summaries and terrain fertility.
struct TerritorySummary {
    struct Owner {
        std::array<uint16_t,128> tiles{}, known{}, suitable{}, covered_suitable{};
        std::array<uint8_t,128> maximum_rating{}, site_signals{};
        bool operator==(const Owner& other) const;
    };
    std::array<Owner,8> owners{};
    std::array<int32_t,128> maximum_rating{}, site_signals{};
    // Explicit fresh-local policy, preserved by saves. The original AI/editor
    // writer is not implemented; old saves must not acquire an invented value.
    uint8_t search_objectives = 0;
    static constexpr size_t WIRE_BYTES = 8*128*10 + 128*8 + 1;
    std::string Serialize() const;
    static TerritorySummary Deserialize(const std::string& wire);
    bool operator==(const TerritorySummary& other) const;
};

struct SummaryTile {
    uint8_t region, owner, explored_by, moisture, rockiness;
    bool fungus, excluded, special_marker;
    uint8_t effective_rating, site_signal;
};

// Input is the actual row-ordered raw-claim/cache projection. No worker-owner,
// active-faction or land-only admission belongs in this pass.
TerritorySummary Summarize(const std::vector<SummaryTile>& tiles,
    uint8_t all_map_revealed, uint8_t search_objectives);
bool RefreshTerritoryAtGlobalTail(size_t turn);

}
