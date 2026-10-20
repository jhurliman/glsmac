#pragma once
#include "SmacInitialClimateGeneration.h"

namespace game::backend::map::smac_generation {
// Original 0x5dda90, 0x5dd550 and 0x5db1d0. Owned raw-plane helpers;
// the final-generation controller must still order their surrounding stages.
void NormalizeContours(State& state,Geometry geometry,InitialClimateTrace* trace=nullptr);
void ClimatePostpass(State& state,Geometry geometry,Regions& regions,InitialClimateTrace* trace=nullptr);
void FinalFeatures(State& state,Geometry geometry,InitialClimateTrace* trace=nullptr);
}
