#pragma once
#include "SmacGenerationRegions.h"
#include "SmacStartingSite.h"

namespace game::backend::map::smac_generation {
struct StartingContext {
    int32_t difficulty;
    bool human,look_first;
    int32_t site_coefficient_u=11,site_coefficient_v=61;
    std::array<uint8_t,128> presence{};
    std::vector<smac_start::Base> bases;
    std::vector<smac_start::Unit> units;
};
struct StartingCacheStore {uint32_t offset;uint8_t before,after;};
using StartingCacheObserver=std::function<void(const StartingCacheStore&)>;
// Compose search with current complete generated geography, raw rating cache,
// resource/site resolvers and region metadata. Caller owns detached startup
// state; cache writes are staged in it, including on refusal/cancellation.
smac_start::Result SelectGeneratedStartingSite(State& state,Geometry geometry,const Regions& regions,
    const StartingContext& context,const StartingCacheObserver& stores={},
    const smac_start::Observe& observe={},const std::function<bool()>& cancelled={},const smac_start::ObserveDraw& draws={});
}
