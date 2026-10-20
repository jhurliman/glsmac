#pragma once
#include "SmacNativeUnitRecord.h"
#include <vector>

namespace game::backend::unit {
struct SmacUnitPlacementInvalidation {int32_t unit=-1,x=0,y=0;};
// Synchronous borrowed view of an owned dense original Unit pool and raw tiles.
// No reallocation, identity erasure, visibility or live registry publication.
struct SmacUnitPlacementWorld {
    std::vector<SmacNativeUnitRecord>& units;
    std::vector<std::array<uint8_t,44>>& tiles;
    int32_t width,height;
    SmacUnitPlacementInvalidation& invalidation;
};
// Native 0x5d81d0 / 0x5d80f0. Caller normalizes coordinates and maintains a
// consistent occupancy/stack pool. Corrupt/global-repair paths are rejected.
void PlaceSmacUnit(SmacUnitPlacementWorld& world,size_t unit,int32_t x,int32_t y,int32_t triad);
// Removed record ends at (-1,-1) with no links, as in the original. This is a
// transient state; existing validated live serialization remains unchanged.
void UnlinkSmacUnit(SmacUnitPlacementWorld& world,size_t unit);
}
