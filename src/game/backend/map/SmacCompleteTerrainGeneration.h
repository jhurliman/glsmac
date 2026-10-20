#pragma once
#include "SmacLandmarkGeneration.h"
#include "SmacPreparedLandmarkProfiles.h"

namespace game::backend::map::smac_generation {
struct GenerationClocks {uint32_t boreholes,final_climate;};
struct GeneratedTerrain {
    PretemperatureResult attempts;
    Landmarks landmarks;
    smac_terrain::landmark_tables::PreparedHeightProfiles profiles;
};
void FinalClimate(State& state,Geometry geometry,const WorldRules& rules,Regions& regions,uint32_t clock,InitialClimateTrace* trace=nullptr);
// Complete the prelandmark continuation to original 0x5e038e, before startup.
GeneratedTerrain CompleteTerrain(State& state,Geometry geometry,const WorldRules& rules,Regions& regions,GenerationClocks clocks,InitialClimateTrace* tail_trace=nullptr);
// Full native producer from supplied initial storage and RNG; no startup yet.
// Optional trace covers the tail beginning at the first landmark, not prefixes.
GeneratedTerrain GenerateTerrain(State& state,Geometry geometry,const WorldRules& rules,Regions& regions,GenerationClocks clocks,InitialClimateTrace* tail_trace=nullptr);
}
