#pragma once
#include "game/backend/SmacGameplayRandomState.h"
#include <array>
#include <functional>
#include <string>
#include <vector>
#include "game/backend/map/SmacSiteGeometry.h"

namespace game::backend::faction {
// Complete original startup storage. Scratch is NOT technology ownership.
struct SmacStartupFaction {
    std::array<uint8_t,8380> record{};
    std::array<uint8_t,512> scratch{};
};
struct SmacStartupPersonality {int32_t fight,tech,power,growth,wealth,energy;};
struct SmacFactionPrefixOptions {
    int32_t faction,difficulty;
    uint8_t human_mask,active_mask;
    uint32_t rules,secondary;
    bool network;
    int32_t starting_energy;
    SmacStartupPersonality definition;
    // Already encoded original single-byte localized strings, not UTF-8.
    std::string channel_prefix,channel_separator;
    int32_t mode=-1;
};
struct SmacStartupDraw {uint32_t caller,before,after;uint16_t value;};
using SmacStartupObserver=std::function<void(const SmacStartupDraw&)>;
struct SmacFactionPrefixResult {uint8_t active_mask;bool searching;};
// Original turn-zero 0x5c89bc..0x5c9051 only; preserves all untouched bytes.
// Supplied RNG continues from terrain generation. No site, package or grants.
SmacFactionPrefixResult InitializeSmacFactionPrefix(SmacStartupFaction& state,
    const SmacFactionPrefixOptions& options,SmacGameplayRandomState& random,
    const SmacStartupObserver& observer={});
struct SmacSitePreparation {
    map::smac_site::Geometry geometry;
    map::smac_site::Coordinate position;
    int32_t faction,region_area,difficulty;
    uint32_t rules;
};
struct SmacStartupTileStore {uint32_t offset;uint8_t size;uint32_t value,caller;};
using SmacStartupStoreObserver=std::function<void(const SmacStartupTileStore&)>;
// Original 0x5c9ad1..0x5c9dd5 after site selection: strategy and initial sight.
// Raw caller-owned records; this does not implement later Unit visibility.
void PrepareSmacStartingSite(SmacStartupFaction& faction,std::vector<std::array<uint8_t,44>>& tiles,
    const SmacSitePreparation& options,SmacGameplayRandomState& random,
    const SmacStartupObserver& observer={},const SmacStartupStoreObserver& stores={});
}
