#pragma once

#include "BaseResourceCalculation.h"
#include "game/backend/unit/SmacConventionalStats.h"
#include <cstdint>

namespace game::backend::base {

enum class SmacColonyChoice : uint8_t { Delay, Screen, Accept };

// The Base record already owns the posted mineral store, unchanged baseline,
// old food, target/queue and repaired allocation. Only the first resource pass
// must cross the question: replaying it would post production twice or replace
// the nutrient surplus consumed by the later population stage.
// No pointer, callback, lock or presentation identity is retained here.
struct SmacColonyUpkeepPause {
    SmacBaseResourceCalculation first_calculation;
    unit::SmacMoraleMode morale_mode;
};

}
