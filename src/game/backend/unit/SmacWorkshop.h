#pragma once
#include "SmacStartupDesignState.h"

namespace game::backend::unit {
struct SmacPrototypePriceContext {
    int32_t owner_difficulty;
    bool free_prototype, queried_base_skunkworks;
    std::optional<std::array<int32_t,3>> percentages;
};
struct SmacPrototypePrice {int32_t rows;bool surcharge_present;};
// Original 5d9810: ordinary stored rows plus rounded ability-free component
// surcharge. Context belongs to the design owner and actual queried Base.
SmacPrototypePrice SmacDesignProductionRows(const SmacCustomDesign& design,
    const SmacPredefinedUnitCatalog& catalog,const SmacPrototypePriceContext& context);

enum class SmacWorkshopComponent { Chassis, Weapon, Armor, Reactor };
struct SmacWorkshopChoice {
    uint32_t component_id; // source identity, not the visible card ordinal
    bool technology_available; // compatibility is a separate editor check
};
std::vector<SmacWorkshopChoice> SmacWorkshopChoices(SmacWorkshopComponent kind,
    const SmacPredefinedUnitCatalog& catalog, const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner);

struct SmacWorkshopDraft {
    int32_t selected_design = 2;
    uint32_t chassis = 0, weapon = 0, armor = 0, abilities = 0;
    uint8_t reactor = 1;
    std::string name; // empty requests the installed-language automatic name
};
struct SmacWorkshopCommit {
    int32_t design_id;
    bool intrinsic_name_warning;
};
// Original 441e20 matching pass. Component picker legality is a separate
// caller requirement; custom matches ignore stored names and prerequisites.
// No match => no mutation. Throws before mutation for unsupported intrinsic
// obsolescence (its shared runtime overlay is not yet retained).
std::optional<SmacWorkshopCommit> ReuseSmacWorkshopDesign(SmacStartupDesignState& state,
    const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner);
// Read-only installed-language label for admitted unnamed legacy opening rows.
std::string SmacWorkshopDesignName(const SmacCustomDesign& design,const SmacPredefinedUnitCatalog& catalog,int32_t owner);

// Original 441e20 first path: Scout or its Infantry/Synthmetal edit, no
// abilities, Fission. Reuses intrinsic tuples before allocating the last slot
// of an empty owner bank. Existing tuples use the matching pass above.
// Occupied-bank allocation/retirement and the standalone Rename dialog still
// require their live-unit/production-count prompt policy.
// Throws before publication on invalid/unavailable/unsupported drafts.
SmacWorkshopCommit CommitSmacWorkshopFirstDesign(SmacStartupDesignState& state,
    const SmacWorkshopDraft& draft, const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies, const faction::SmacFactionTechnologyState& owner);
}
