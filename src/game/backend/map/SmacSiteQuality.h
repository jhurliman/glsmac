#pragma once

#include <array>
#include <cstdint>
#include <functional>

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
struct Geometry { int32_t width, height; bool wrap; };
using Coordinate = std::array<int32_t,2>;
using Query = std::function<Tile(Coordinate)>;
using Areas = std::array<int32_t,128>;
struct Score {
	int32_t rating, raw, points, production, fungus_production, rivers;
	int32_t water, deep_water, outer_bases, rocky_neighbors;
	bool large_water, coastal;
	Coordinate last;
};

// RE-SMAC-SITE-QUALITY-001: pure land-center query for ordinary turns1..100.
// Visits the measured inner21 order then the outer28. Does not read/write a
// cached rating, infer stored region areas, or consume either RNG stream.
// Sea-centered and late-turn branches are separate, unclosed contracts.
Score LandQuality(Geometry geometry, Coordinate center, int32_t turn, const Areas& areas, const Query& query);

}
