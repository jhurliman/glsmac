#pragma once
#include "SmacStartupDesignState.h"

namespace game::backend::unit {
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
// Original 441e20 first path: Scout or its Infantry/Synthmetal edit, no
// abilities, Fission. Reuses intrinsic tuples before allocating the last slot
// of an empty owner bank. Occupied-bank replacement and custom rename need
// their own live-unit/production-count prompt policy and are not admitted here.
// Throws before publication on invalid/unavailable/unsupported drafts.
SmacWorkshopCommit CommitSmacWorkshopFirstDesign(SmacStartupDesignState& state,
    const SmacWorkshopDraft& draft, const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies, const faction::SmacFactionTechnologyState& owner);
}
