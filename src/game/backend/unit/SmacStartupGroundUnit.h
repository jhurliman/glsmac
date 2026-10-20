#pragma once
#include "SmacUnitStackPlacement.h"
#include "game/backend/base/SmacStartupBase.h"

namespace game::backend::unit {
struct SmacStartupGroundUnitOptions {
    map::smac_site::Geometry geometry;
    map::smac_site::Coordinate position;
    int32_t owner,design,role,morale,display_faction;
    bool editor=false;
    // Caller-owned catalog/sight admission. Extended/sensor/ability paths
    // require their own producers; they cannot silently use ordinary sight.
    uint32_t abilities=0;
    bool extended_sight=false,sensor_sight=false;
};
struct SmacStartupUnitFactoryState {
    uint32_t dirty=0;
    int32_t cached_base=-1,last_distance=9999;
    SmacUnitPlacementInvalidation placement;
};
struct SmacStartupUnitMetadataStore {
    enum class Space : uint8_t {Units,Global};
    Space space;
    uint32_t offset;
    uint8_t size;
    uint32_t value,caller;
};
using SmacStartupUnitMetadataObserver=std::function<void(const SmacStartupUnitMetadataStore&)>;
// Ordinary positive-owner ground creation at 0x5d8520, including ordinary
// 0x5cdaa0 sight. Foreign objects inside sight probes and late AI creation reject.
// Raw pool/global mutations are staged; no live registry or snapshot publication.
int32_t CreateSmacStartupGroundUnit(std::vector<std::array<uint8_t,44>>& tiles,std::vector<SmacNativeUnitRecord>& units,
    faction::SmacStartupFaction& actor,const std::vector<base::SmacStartupBaseRecord>& bases,
    SmacStartupUnitFactoryState& state,const SmacStartupGroundUnitOptions& options,
    const faction::SmacStartupStoreObserver& stores={},const SmacStartupUnitMetadataObserver& metadata={});
}
