#pragma once
#include "game/backend/faction/SmacStartupTechnology.h"
#include "game/backend/map/SmacSiteGeometry.h"
#include <optional>

namespace game::backend::base {
using SmacStartupBaseRecord=std::array<uint8_t,304>;
struct SmacBaseSeedCatalog {
    faction::SmacStartupTechnologyOwnership ownership;
    std::array<std::array<int32_t,2>,89> prerequisites;
    std::array<uint8_t,3> design_costs; // cached catalog costs for Pod/Former/Scout
    int32_t formers_prerequisite;
    std::array<int32_t,38> free_facility_technologies;
    std::vector<int32_t> facility_bonuses; // source FACILITY records, in order
};
struct SmacBaseSeedOptions {
    map::smac_site::Geometry geometry;
    map::smac_site::Coordinate position;
    int32_t owner,base_count,difficulty;
    uint8_t human_mask;
    int32_t map_category,mineral_multiplier,reference_owner;
    bool network,previous_same_owner,extra_population,population_penalty;
    std::string name; // original single-byte localization text, no UTF-8 conversion
};
struct SmacBaseSeedResult {
    int32_t index,count;
    std::optional<std::array<uint8_t,8>> history;
};
// Original turn-zero 0x4fa490..0x4faa7b only; preserves untouched bytes.
// Name/cost/table inputs are owned caller data; later resources are separate.
SmacBaseSeedResult SeedSmacStartupBase(SmacStartupBaseRecord& base,std::vector<std::array<uint8_t,44>>& tiles,
    faction::SmacStartupFaction& owner,const SmacBaseSeedOptions& options,const SmacBaseSeedCatalog& catalog);
// Original 0x4f9c80 and persistent/active goal cleanup. Returns its actual last
// attempted offset coordinate, which the original uses instead of Base center.
map::smac_site::Coordinate PrepareSmacStartupBaseRadius(std::vector<std::array<uint8_t,44>>& tiles,
    std::array<faction::SmacStartupFaction,8>& factions,map::smac_site::Geometry geometry,
    map::smac_site::Coordinate position,int32_t owner,const faction::SmacStartupStoreObserver& stores={});
}
