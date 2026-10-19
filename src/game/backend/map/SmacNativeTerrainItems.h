#pragma once
#include "tile/Types.h"
#include <array>
#include <utility>
namespace game::backend::map {
inline uint32_t NativeTerrainItems(tile::terraforming_t terraforming,tile::feature_t features) {
    uint32_t result=0;
    const std::array<std::pair<tile::terraforming_t,uint32_t>,13> mapping{{
        {tile::TERRAFORMING_ROAD,4},{tile::TERRAFORMING_MAG_TUBE,8},{tile::TERRAFORMING_MINE,0x10},
        {tile::TERRAFORMING_SOLAR,0x40},{tile::TERRAFORMING_FARM,0x8000},{tile::TERRAFORMING_SOIL_ENRICHER,0x80000},
        {tile::TERRAFORMING_FOREST,0x200000},{tile::TERRAFORMING_CONDENSER,0x400000},
        {tile::TERRAFORMING_MIRROR,0x800000},{tile::TERRAFORMING_BOREHOLE,0x1000000},
        {tile::TERRAFORMING_SENSOR,0x80000000u},{tile::TERRAFORMING_BUNKER,0x800},{tile::TERRAFORMING_AIRBASE,0x40000}
    }};
    for(const auto& entry:mapping) {if(terraforming&entry.first) {result|=entry.second;}}
    if(features&tile::FEATURE_XENOFUNGUS) {result|=0x20;}
    if(features&tile::FEATURE_RIVER) {result|=0x80;}
    return result;
}
}
