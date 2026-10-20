#pragma once
#include "SmacStartupFaction.h"
#include "game/backend/settings/SmacFactionStartupRules.h"

namespace game::backend::faction {
using SmacStartupTechnologyOwnership=std::array<uint8_t,89>;
// Original turn-zero named acquisition only. No RNG, research scoring, Future
// Tech count, design construction, visibility, notification or live publication.
void GrantSmacStartupNamedTechnology(SmacStartupFaction& record,SmacStartupTechnologyOwnership& ownership,
    int32_t faction,int32_t technology,int32_t source=0);
// Consume the parser's original ordered eight-record projection; keep repeats.
void GrantSmacStartupNamedTechnologies(SmacStartupFaction& record,SmacStartupTechnologyOwnership& ownership,
    int32_t faction,const settings::SmacFactionStartupRule& profile);
}
