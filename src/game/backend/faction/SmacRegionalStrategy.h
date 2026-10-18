#pragma once

#include <array>
#include <cstdint>
#include <functional>

namespace game::backend::faction {

struct SmacRegionalStrategyContext {
    int32_t owner,turn,world_tiles,total_bases,explore,build;
    bool leading_human_dominance,retained_land_region_concentration;
};
struct SmacRegionalStrategyRegion {
    struct Peer {
        uint8_t bases;
        uint16_t units,attack;
        uint32_t relation;
    };
    // All positive faction slots, including the owner. These are the retained
    // peer inputs at this owner's pass, not freshly recounted foreign armies.
    std::array<Peer,8> peers{};
    int32_t stored_area=0,maximum_rating=0;
    uint16_t owned_suitable=0,covered_suitable=0,neutral_suitable=0,flags=0;
};
using SmacRegionalStrategyRow=std::array<uint8_t,128>;
// Called only at a native fresh-route boundary, in peer/mode order. The live
// adapter must supply actual Base/component routing and its retained workspace.
using SmacRegionalStrategyRoute=std::function<bool(int32_t peer,uint8_t region,int32_t mode)>;

// Produces strategy codes only. The unrelated bilateral-policy flag4 is not
// synthesized; this is not a replacement for the complete regional flag word.
// Caller stages publication until the entire owner-summary pass succeeds.
SmacRegionalStrategyRow RefreshSmacRegionalStrategy(const SmacRegionalStrategyContext& context,
    const std::array<SmacRegionalStrategyRegion,128>& regions,const SmacRegionalStrategyRow& previous,
    const SmacRegionalStrategyRoute& route);
}
