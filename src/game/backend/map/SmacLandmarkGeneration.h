#pragma once
#include "SmacInitialClimateGeneration.h"
#include <optional>

namespace game::backend::map::smac_generation {
enum class Landmark {Jungle,Crater,Mount,Mesa,Ridge,Uranium,Ruins,Nexus,Boreholes,Sargasso,Dunes,Freshwater,Geothermal};
struct LandmarkLabel {Landmark kind;Coordinate position;};
struct Landmarks {
    std::vector<LandmarkLabel> labels;
    std::optional<Coordinate> mount_center;
    // Caller supplies already encoded/localized name bytes. Each native slot
    // has 32 bytes including NUL. This returns coordinates + zero-filled slots.
    std::string EncodeNames(const std::array<std::string,13>& names) const;
};
const std::array<std::string,13>& StockLandmarkNames();
void PlaceLandmark(State& state,Geometry geometry,const WorldRules& rules,Regions& regions,Landmarks& landmarks,
    Landmark kind,Coordinate center={{-1,-1}},uint32_t clock=100,bool suppress_mount_name=false,InitialClimateTrace* trace=nullptr);
}
