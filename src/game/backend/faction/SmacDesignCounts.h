#pragma once
#include <array>
#include <cstdint>
#include <vector>
namespace game::backend::faction {
using SmacDesignCounts = std::array<uint8_t,512>;
struct SmacProductionReferenceQueue {int32_t owner,pending_count;std::vector<int32_t> targets;};
// Original owner summary counts current plus declared pending entries. Negative
// count skips the entire queue; foreign queues and undeclared tail are ignored.
SmacDesignCounts RecountSmacProductionReferences(int32_t owner,const std::vector<SmacProductionReferenceQueue>& bases);
// Per-Base detach/attach against retained bytes, never a recount or stale repair.
SmacDesignCounts ChangeSmacProductionReferences(const SmacDesignCounts& initial,
    const std::vector<int32_t>& removed,const std::vector<int32_t>& added);

}
