#pragma once

#include "game/backend/map/SmacWorkerKnowledge.h"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>
#include <utility>

namespace game::backend::unit {
// Ephemeral event effects, never reconstructed from accumulated saved masks.
// A repeated disclosure still clears flags even when its mask bit was set.
struct PreparedConventionalSight {
    std::optional<map::SmacWorkerKnowledge> knowledge;
    std::vector<uint64_t> disclosed_to_humans;
    std::vector<std::array<int32_t,2>> physical_contacts;
    uint32_t physical_contact_mask=0;
    PreparedConventionalSight(std::optional<map::SmacWorkerKnowledge> value = std::nullopt)
        : knowledge(std::move(value)) {}
};
}
