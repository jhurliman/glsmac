#pragma once

#include "SmacSiteGeometry.h"
#include "game/backend/SmacGameplayRandomState.h"
#include <functional>
#include <optional>

namespace game::backend::map::smac_restart {
using Coordinate=smac_site::Coordinate;
struct Input {
	smac_site::Geometry geometry;
	int32_t turn, owner, captor, difficulty, scale, ocean;
};
struct Base {int32_t owner;uint8_t region;};
// Outputs of candidate preprocessing over the caller's prepared future world.
// Area/fertility/presence are retained regional metadata, never fresh recounts.
struct Candidate {
	uint8_t elevation=0,rockiness=0,region=0;
	bool fungus=false,occupied=false,valid_occupant=false;
	int32_t wet_neighbors=0,area=0,fertility=0,present_factions=0,landmark_bucket=0,territory_owner=-1;
	std::optional<Base> nearest;
	int32_t nearest_distance=9999;
};
using Preprocess=std::function<Candidate(Coordinate)>;
// Invoked at the native rating stage, before Q/territory/captor gates. It must
// read or prepare a cache write even if the computed rating is zero. Nonzero
// restart bypasses the following minimum-rating test, hence no returned score.
using ResolveRating=std::function<void(Coordinate)>;
enum class Outcome {
	Terrain,MinimumDistance,Altitude,Occupied,NorthMargin,SouthMargin,WetNeighborhood,
	RelaxingDistance,Fertility,Area,Presence,Landmark,Claimed,CaptorRegion,Limit,Selected
};
const char* OutcomeName(Outcome outcome);
struct Attempt {size_t number;Coordinate point;uint32_t crt;Outcome outcome;};
using Observe=std::function<void(const Attempt&)>;
struct Result {
	std::optional<Coordinate> site;
	SmacGameplayRandomState random;
	int32_t nearest_distance;
	size_t pairs,rating_reads;
};
// RE-SMAC-RESTART-SEARCH-001 human nonzero-turn candidate loop. The original
// Do-or-Die/history/turn100 guards are later caller decisions, not early exits
// here. Failure returns the consumed CRT state and terminal preprocessing
// scratch. Input RNG is unchanged; the caller publishes the prepared result.
Result Search(const Input& input,const SmacGameplayRandomState& initial,const Preprocess& preprocess,
	const ResolveRating& rating,const Observe& observe={});
}
