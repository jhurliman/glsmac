#pragma once

#include "game/backend/settings/SmacTechnologyRules.h"
#include <array>
#include <vector>

namespace game::backend::faction {

// Owned-value inputs to the original council/dominance predicates. These are
// not saved state and never refresh ranking, population or history on a query.
struct SmacCouncilBase {int32_t owner,population;};
int32_t SmacCouncilVotes(int32_t owner,const std::vector<SmacCouncilBase>& bases,
    bool owns_empath_guild,bool owns_clinical_immortality,const std::vector<int32_t>& modifiers);

struct SmacVictoryContext {
    int32_t turn=0,voice_host=-1;
    uint32_t rules=0;
    uint8_t humans=0;
    std::array<int32_t,8> votes{},economic_deadlines{},last_base_expansion_or_advice_turn{};
    std::array<std::array<bool,89>,8> technologies{};
    std::array<std::array<uint32_t,8>,8> relations{};
};
struct SmacVictoryRequirements {
    settings::SmacTechnologyReference supreme_leader;
    // Current loaded catalog requirements, never hardcoded installed IDs.
    int32_t psi_gate,voice_of_planet;
};
// Exclusion0 means none; -1 is the original dominance caller's history-word
// read. Positive exclusions use directed Pact bits. Other negatives reject.
int32_t SmacPotentialMajority(const SmacVictoryContext& context,int32_t query,int32_t exclude,
    const SmacVictoryRequirements& requirements,const settings::SmacTechnologyRules& catalog);
bool SmacVictoryPressure(const SmacVictoryContext& context,const SmacVictoryRequirements& requirements);

struct SmacDominanceQuery {
    int32_t owner,leader,retained_base_count,difficulty,mode,objectives,leader_score,runner_up_score;
};
bool SmacLeadingHumanDominance(const SmacVictoryContext& context,const SmacDominanceQuery& query,
    const SmacVictoryRequirements& requirements,const settings::SmacTechnologyRules& catalog);
}
