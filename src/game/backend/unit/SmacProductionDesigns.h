#pragma once
#include "SmacStartupDesignState.h"
#include "game/backend/faction/SmacTechnologyState.h"
#include <optional>

namespace game::backend::unit {
struct SmacProductionDesignContext {
    int32_t owner;
    std::optional<int32_t> peer_attack,peer_defense;
    std::optional<std::array<int32_t,2>> land_psi_ratio;
    int32_t psi_percent;
    bool dream_twister,neural_amplifier;
};
struct SmacProductionDesignList {
    std::array<int32_t,128> order{},keys{}; // sorted parallel arrays, including placeholders
    size_t count=0; // published prefix, not the count of eligible production rows
};
SmacCustomDesign ReadSmacProductionDesign(const SmacOriginalDesign& row,int32_t id);
bool HasSmacProductionDesignTechnology(const SmacOriginalDesign& row,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner);
int32_t SmacProductionArmorScore(int32_t armor,const SmacProductionDesignContext& context);
SmacProductionDesignList BuildSmacProductionDesignList(const SmacOriginalDesignPool& pool,
    const SmacPredefinedUnitCatalog& catalog,const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner,const SmacProductionDesignContext& context);
}
