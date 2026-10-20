#pragma once
#include "SmacSiteQuality.h"
#include <vector>

namespace game::backend::map::smac_site {
struct RawQualityContext {
    Geometry geometry;
    int32_t pattern;
    uint32_t rules;
    int32_t coefficient_u=11,coefficient_v=61;
};
// Decode current original records for turn-zero shared quality. No cache
// stores, RNG, installed-file header, live Tile objects or retained pointers.
Score RawQuality(const std::vector<std::array<uint8_t,44>>& records,const Areas& areas,
    const RawQualityContext& context,Coordinate point);
}
