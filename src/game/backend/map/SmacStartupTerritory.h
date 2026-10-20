#pragma once
#include "SmacGenerationRegions.h"
#include "game/backend/base/SmacStartupBase.h"

namespace game::backend::map {
struct SmacStartupTerritoryOptions {
    smac_site::Geometry geometry;
    int32_t border=8,pattern=0,coefficient_u=11,coefficient_v=61;
    uint32_t rules=0x1a0e;
    uint8_t human_mask=2;
    int32_t difficulty=3;
    uint32_t dirty=0;
};
struct SmacStartupTerritoryResult {uint32_t dirty;int32_t last_distance;size_t uncached;};
using SmacStartupQualityObserver=std::function<void(smac_site::Coordinate,int32_t)>;
// Complete original 0x538e80 over detached owned records. Goal insertion and
// quality/resource/site arithmetic reuse their existing production rules.
// Partial writes remain staged if a callback or invalid semantic query throws.
SmacStartupTerritoryResult RebuildSmacStartupTerritory(std::vector<std::array<uint8_t,44>>& tiles,
    std::array<faction::SmacStartupFaction,8>& factions,const std::vector<base::SmacStartupBaseRecord>& bases,
    smac_generation::Regions& regions,const SmacStartupTerritoryOptions& options,
    const faction::SmacStartupStoreObserver& stores={},const SmacStartupQualityObserver& queries={});
}
