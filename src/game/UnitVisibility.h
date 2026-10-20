#pragma once
#include "ActorVisibilitySnapshot.h"

namespace game::actor_visibility {
struct UnitPlan {
    std::optional<uint64_t> body;
    std::vector<uint64_t> stack;
};
// Ordinary local view. Special editor/override, display-option and blink-phase
// contexts retain their own source contracts; no per-member visibility sort.
UnitPlan OrdinaryUnits(const Snapshot& snapshot,int32_t x,int32_t y,
    std::optional<uint64_t> selected,bool control,bool pending,
    const std::vector<uint64_t>* tile_members=nullptr);
}
