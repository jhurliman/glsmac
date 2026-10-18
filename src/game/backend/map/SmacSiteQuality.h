#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include "SmacSiteGeometry.h"

namespace game::backend::map::smac_site {

// Decoded original inputs. Resource and site presence are resolver outputs;
// moisture/rockiness/elevation are original ordinals, not port Tile enums.
struct Tile {
	uint8_t elevation, moisture, rockiness, region;
	bool fungus, monolith, forest, river, excluded, base;
	bool resource, site, coastal;
	bool planet, crater, uranium, jungle;
	uint8_t landmark_detail;
};
using Query = std::function<Tile(Coordinate)>;
using Areas = std::array<int32_t,128>;
struct Score {
	int32_t rating, raw, points, production, fungus_production, rivers;
	int32_t water, deep_water, outer_bases, rocky_neighbors;
	bool large_water, coastal;
	Coordinate last;
};

// RE-SMAC-SITE-QUALITY-001 with verified startup and late-turn extension.
// Visits the measured inner21 order then the outer28. Does not read/write a
// cached rating, infer stored region areas, or consume either RNG stream.
// Land-only entry; nonnegative signed32 turns, including startup turn0.
Score LandQuality(Geometry geometry, Coordinate center, int32_t turn, const Areas& areas, const Query& query);
// Shared land/water query, including the RE-SMAC-SEA-SITE-QUALITY-001 water
// center, outer-Base multiplier and rejection branches. Includes the original
// turn>150 land-fungus adjustment before rejection/clamping.
Score Quality(Geometry geometry, Coordinate center, int32_t turn, const Areas& areas, const Query& query);

}
