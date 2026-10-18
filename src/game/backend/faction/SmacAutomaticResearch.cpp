#include "SmacAutomaticResearch.h"

#include <algorithm>
#include <stdexcept>

namespace game::backend::faction {
namespace {
int32_t Signed(uint32_t value) {
    return value <= INT32_MAX ? int32_t(value) : int32_t(int64_t(value)-0x100000000LL);
}
bool Owns(const SmacAutomaticResearchInputs& in, int32_t id) {
    if (id == -1) { return true; }
    if (id < 0 || id >= 88) { return false; }
    const auto& row = in.technologies[id];
    return row.prerequisites[0] != -2 && (row.prerequisites[1] != -2 || row.prerequisites[0] == -1)
        && (row.possession & (uint32_t(1) << in.faction_id));
}
class Depth {
public:
    explicit Depth(const SmacAutomaticResearchInputs& input) : in(input) {}
    int32_t Get(int32_t id) {
        if (id < 0 || id >= 89) { return 0; }
        if (depth[id]) { return depth[id]; }
        if (visiting[id]) { throw std::invalid_argument("Cyclic automatic research prerequisites"); }
        visiting[id] = true;
        const auto& prerequisites = in.technologies[id].prerequisites;
        depth[id] = 1 + std::max(Get(prerequisites[0]), Get(prerequisites[1]));
        visiting[id] = false;
        return depth[id];
    }
private:
    const SmacAutomaticResearchInputs& in;
    std::array<int32_t,89> depth{};
    std::array<bool,89> visiting{};
};
}

int32_t SelectSmacAutomaticResearch(const SmacAutomaticResearchInputs& in,
    SmacGameplayRandomState& random, const std::function<int32_t(int32_t)>& value) {
    if (in.faction_id < 0 || in.faction_id > 7) { throw std::invalid_argument("Invalid automatic research faction"); }
    if (!in.faction_id) { return -1; }
    Depth depth(in);
    int32_t selected = -1, best = -999;
    for (int32_t id = 0; id < 89; ++id) {
        const auto& prerequisites = in.technologies[id].prerequisites;
        if (!Owns(in, prerequisites[0]) || !Owns(in, prerequisites[1]) || Owns(in,id)) { continue; }
        auto score = value(id);
        if (in.blind) {
            if (in.human && (in.priorities[0] || in.priorities[2]) && id == in.former_prerequisite) { return id; }
            score = Signed(uint32_t(score) * 256u) / depth.Get(id);
        }
        // Endpoint and product arithmetic wrap in the original 32-bit range
        // helper. Even zero-width human draws advance the shared stream.
        const auto endpoint = Signed(uint32_t(score) + 1u);
        if (in.human) {
            const auto low = std::min(0,endpoint), high = std::max(0,endpoint);
            const auto width = uint32_t(high) - uint32_t(low);
            const auto draw = random.NextSharedLow16();
            score = Signed(uint32_t(low) + ((width * draw) >> 16));
        }
        else if (score > 0) {
            // The measured AI domain excludes an overflowing V+1 endpoint.
            if (endpoint <= 0) { throw std::invalid_argument("AI research range exceeds measured domain"); }
            score = random.NextCrtRaw15() % endpoint;
        }
        else { score = 0; }
        if (score > best) { best = score; selected = id; }
    }
    return selected;
}

}
