#pragma once
#include "game/backend/SmacGameplayRandomState.h"
#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace game::backend::map::smac_generation {
using Record=std::array<uint8_t,44>;
using Categories=std::array<int32_t,7>;
struct Rules {int32_t land_base,land_modifier,islands;};
enum class Stage {Allocated,ContinentLoop};
enum class Entry {First,Retry};
struct Draw {
    enum class Kind {Crt,Shared};
    Kind kind;
    uint32_t before,after;
    int32_t value;
};
using Observer=std::function<void(const Draw&)>;

// Detached owned inputs and continuation. No port Tile pointers, JSF32 stream,
// live-world publication or implicit seed choice enter this generation prefix.
// Geometry is already allocated: resolving category 0 does not resize records.
struct State {
    std::vector<Record> records;
    Categories categories;
    SmacGameplayRandomState random;
    int32_t resource_pattern=0;
    std::array<int32_t,3> reset_words{}; // original 9b8b04,9b8b08,9b8b18
    int32_t topology=0; // original 9b8b14: zero is round
    int32_t land_target=0,land_progress=0;
    std::array<int32_t,2> generation_flags{};
    std::array<int32_t,11> working_land_table{};
    int32_t continent_attempts=0,attempt_iterations=0,continent_minimum=0;
    Stage stage=Stage::Allocated;
};

// Original 0x5a8450 only: preserve bytes 6/7, reset the other record fields,
// clear three map words and consume one shared pattern draw, even if empty.
void ResetRecords(State& state,const Observer& observer={});
// Original 0x5e0100..0x5e01da. Retry carries the continent-attempt counter (EBP),
// while restarting per-attempt iteration/progress. Stops before continent growth.
void Bootstrap(State& state,const Rules& rules,Entry entry,const Observer& observer={});
}
