#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

namespace game::backend::map::smac_terrain {

using BandAt=std::function<std::optional<uint8_t>(int,int)>;
using Lighting=std::array<int,5>; // C,W,N,E,S signed palette-index lighting.

// Native ordinary generated lookup, independent of pixel heights/normals.
// The measured table admits slope deltas -3..3 and face indices0..3.
std::optional<int> FaceLighting(int first_delta,int second_delta,unsigned face);

// Caller owns topology/admission. Absence and sea contribute zero face light;
// missing/sea corner contributors force the native boundary slope code.
// Returns absence for a missing/sea center or an unsupported generated slope,
// rather than fabricating RGB light or silently clamping an unproved profile.
std::optional<Lighting> OrdinaryLighting(int x,int y,const BandAt& band_at);

}
