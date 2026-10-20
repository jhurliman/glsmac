#pragma once
#include "SmacGenerationRegions.h"

namespace game::backend::map::smac_generation {
using WorldRules=std::array<int32_t,31>;
inline constexpr WorldRules DEFAULT_WORLD_RULES{{384,256,12,24,1,2,4,8,8,12,14,14,8,8,5,5,3,1,15,10,15,10,10,25,1,3,6,12,18,24,36}};
struct Attempt {
    int32_t entry,carried,iterations,continent_calls,hills;
    bool rejected;
};
struct PretemperatureResult {
    std::vector<Attempt> attempts;
    int32_t retries=0;
    bool accepted=false;
};
// Optional detached-stage traces. Observer covers bootstrap/noise/polar-choice
// draws; shaping covers grow/altitude draws and morph byte stores; region covers
// its own stores. Bootstrap record-reset stores are not included in these traces.
// All callbacks are synchronous and must not mutate/reenter the inputs.
struct GenerationTrace {
    ContinentTrace* shaping=nullptr;
    RegionTrace* regions=nullptr;
    Observer random;
};
void MorphContours(State& state,Geometry geometry,const WorldRules& rules,Regions& regions,const GenerationTrace& trace={});
// Genuine owned-record pipeline from initial allocation to the first temperature
// entry. Retries retain RNG, continent ordinal and region/summary history.
PretemperatureResult GeneratePretemperature(State& state,Geometry geometry,const WorldRules& rules,Regions& regions,const GenerationTrace& trace={});
}
