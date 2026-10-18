#pragma once

#include <array>
#include <cstdint>
#include "types/Buffer.h"

namespace game::backend::unit {

// Original simulation-pool identities, not legacy frontend/manager IDs.
// Zero is valid; -1 in a signed link is absence. Binding to the live port
// registry is separate from retaining these complete scalar authorities.
struct SmacNativeUnitRecord {
    static constexpr size_t MAX_WIRE_BYTES = 512;
    uint16_t id = 0;
    int16_t x = -4;
    int16_t y = -4;
    uint32_t state = 0;
    uint16_t flags = 0;
    int16_t design = 8;
    uint16_t aux_word = 0;
    uint8_t owner = 0;
    uint8_t aux_owner_control = 0;
    uint8_t damage = 0;
    uint8_t order = 0;
    uint8_t automation_control = 0;
    uint8_t waypoint_count = 0;
    uint8_t stored_morale = 0;
    uint8_t terraforming_progress = 0;
    uint8_t ecology_owner_tag = 0;
    uint8_t observation_mask = 0;
    uint8_t movement_spent = 0;
    uint8_t facing = 2;
    uint8_t movement_iteration_control = 0;
    uint8_t status_character = 0;
    uint8_t probe_action_control = 0;
    uint8_t probe_target_control = 0;
    int16_t home_base = -1;
    int16_t stack_next = -1;
    int16_t stack_previous = -1;
    std::array<int16_t,4> waypoint_x{{-1,-1,-1,-1}}, waypoint_y{{-1,-1,-1,-1}};
    static SmacNativeUnitRecord InitializeWild(int32_t id, int32_t design, int32_t native_direct_morale);
    static SmacNativeUnitRecord InitializeMindWorm(int32_t id, int32_t native_direct_morale);
    // Caller has already unlinked the removed node. Returns a detached survivor
    // with dense index, surviving stack links and order1 payload repaired.
    SmacNativeUnitRecord AfterUnlinkedIndexErasure(int32_t removed) const;
    void Validate() const;
    bool operator==(const SmacNativeUnitRecord& other) const noexcept;
    types::Buffer Serialize() const;
    static SmacNativeUnitRecord Deserialize(const std::string& wire);
};

struct SmacNativeUnitStats {
    int32_t level, quantum, full_allowance, remaining_allowance, hit_points, maximum_hit_points, basic_allowance;
};

// Owner0/design8..10 and neutral special markers only. Stored morale and ecology
// attribution do not select lifecycle level. No AI, combat or RNG side effects.
int32_t SmacWildNativeLevel(const SmacNativeUnitRecord& record, int32_t global_turn,
    int32_t opposing_faction, int32_t opposing_major_atrocities);
// Compatibility entry point for existing Mind Worm callers.
int32_t SmacWildMindWormLevel(const SmacNativeUnitRecord& record, int32_t global_turn,
    int32_t opposing_faction, int32_t opposing_major_atrocities);

}
