#pragma once

#include <cstdint>

namespace game::backend::faction {
// Mutable world authorities, initially copied from the accepted Planet header.
// Category admission is separate from signed byte decoding; sea offset is not
// a rendered altitude or the pending future sea-level change.
struct SmacEcologyWorld {
    int32_t native_life, cloud_cover, sea_level_offset;
    void Validate() const;
    bool operator==(const SmacEcologyWorld& other) const noexcept {
        return native_life==other.native_life && cloud_cover==other.cloud_cover && sea_level_offset==other.sea_level_offset;
    }
};
enum class SmacPerihelionTransition { None, Starts, Ends };
struct SmacPerihelionUpdate { bool active; SmacPerihelionTransition transition; };
// The perihelion slice of maintenance, after other resource-event maintenance.
// No RNG/announcement/event generation or missed-year reconstruction occurs.
SmacPerihelionUpdate AdvanceSmacPerihelion(bool active,int32_t turn,int32_t mission_year,bool maintenance);
}
