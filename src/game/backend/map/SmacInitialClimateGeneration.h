#pragma once
#include "SmacPretemperatureGeneration.h"

namespace game::backend::map::smac_generation {
struct InitialClimateTrace {
    // Synchronous observers/progress only; do not mutate or reenter the plane.
    uint64_t stores=0,crt_draws=0,shared_events=0,pumps=0;
    std::function<void(uint32_t,unsigned,uint32_t)> on_store;
    std::function<void(uint32_t,uint32_t,uint16_t)> on_crt;
    std::function<void(bool,uint32_t,uint32_t,int32_t,int32_t,int32_t)> on_shared;
    std::function<void()> checkpoint;
};
struct RiverSources {int32_t quota,placed,attempts;};
// Settings are resolved categories[4]/[5]/[6], ocean category[1], and
// reset_words[1] (extra climate). These mutate the same owned raw plane.
void InitialTemperature(State& state,Geometry geometry,const WorldRules& rules,InitialClimateTrace* trace=nullptr);
RiverSources SeedRiverSources(State& state,Geometry geometry,const WorldRules& rules,InitialClimateTrace* trace=nullptr);
void InitialFungus(State& state,Geometry geometry,const WorldRules& rules,uint32_t clear_mask=0x288050,InitialClimateTrace* trace=nullptr);
// Ordered continuation to original 0x5e02be. Individual kernels leave stage
// bookkeeping to their caller; this controller marks the completed boundary.
RiverSources GenerateInitialClimate(State& state,Geometry geometry,const WorldRules& rules,uint32_t clear_mask=0x288050,InitialClimateTrace* trace=nullptr);
}
