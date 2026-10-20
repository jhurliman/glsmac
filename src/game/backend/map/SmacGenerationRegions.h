#pragma once
#include "SmacContinentGeneration.h"

namespace game::backend::map::smac_generation {
struct Regions {
    std::array<uint8_t,128*28> records{};
    uint32_t secondary_flags=0;
    int32_t summary_width=0,summary_height=0;
    uint8_t summary_initial=165;
    std::vector<uint8_t> summary;
    int32_t Area(size_t region) const;
};
struct RegionTrace {
    uint64_t tile_stores=0,region_stores=0,pumps=0;
    std::function<void(uint32_t,unsigned,uint32_t)> on_tile_store,on_region_store;
    std::function<void()> checkpoint;
};
// Original 0x5b4200, including capacity merges, retained region history,
// freshwater, adjacency and summary. Callbacks must not mutate/reenter inputs.
void RebuildRegions(State& state,Geometry geometry,Regions& regions,RegionTrace* trace=nullptr);
}
