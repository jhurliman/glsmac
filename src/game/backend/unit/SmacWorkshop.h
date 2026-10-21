#pragma once
#include "SmacStartupDesignState.h"
#include "SmacOpeningDesigns.h"
#include "game/backend/faction/SmacDesignCounts.h"
#include <map>
#include <utility>

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
// Live editor ability admission for a new tuple; saved aliases/reselection retain their
// separate technology-loss semantics. Throws without changing the draft.
void ValidateSmacWorkshopEditorAbilities(const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner);
// Original compatibility is independent of technology admission.
bool SmacWorkshopAbilityCompatible(uint32_t ability,const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog);
struct SmacWorkshopAbilitySelection {uint32_t mask;std::array<int32_t,2> slots;};
// Original ascending removal preserves selector slots when no removal occurs.
SmacWorkshopAbilitySelection CleanSmacWorkshopAbilities(const SmacWorkshopDraft& draft,
    std::array<int32_t,2> slots,bool second_ability,const SmacPredefinedUnitCatalog& catalog);
// Full-paint cleanup after retained selection. Availability loss alone does not
// discard saved abilities; Neural capacity and component compatibility still apply.
SmacWorkshopAbilitySelection CleanSmacWorkshopRetainedAbilities(const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner);
struct SmacWorkshopEditor {
    SmacWorkshopDraft draft;
    std::array<int32_t,2> ability_slots{{-1,-1}};
};
int32_t SmacWorkshopMovement(const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog);
enum class SmacWorkshopStatusKind { Basic, BasedOnOld, Previous, New, Modifying };
struct SmacWorkshopStatus {int32_t selected;SmacWorkshopStatusKind kind;};
// Original43d730 status paint also selects a provisional slot. Read retained
// owner caches lazily; this transition neither constructs nor saves a record.
SmacWorkshopStatus QuerySmacWorkshopStatus(const SmacOriginalDesignPool& pool,const SmacWorkshopDraft& draft,
    int32_t owner,const std::optional<faction::SmacDesignCounts>& units,
    const std::optional<faction::SmacDesignCounts>& production);
// Original5932d0 formatting from the editable tuple, with the main painter's
// optional trailing space and ascending installed ability abbreviations.
std::string FormatSmacWorkshopStats(const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,bool main_caption=false);
// The editor preference and current owner capability are explicit authority,
// not defaults inferred from a tuple or an existing saved record.
struct SmacWorkshopEditContext {
    bool automatic_suggestions,second_ability;
    std::array<bool,24> available_abilities;
    SmacWorkshopEditContext(bool automatic,bool second,std::array<bool,24> available)
        : automatic_suggestions(automatic),second_ability(second),available_abilities(std::move(available)) {}
};
SmacWorkshopEditContext QuerySmacWorkshopEditContext(const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner,
    bool automatic_suggestions);
struct SmacWorkshopAbilityMenu {
    enum class Guard { None, SecondTechnology, Missile } guard;
    std::vector<uint32_t> choices; // Stored modal values, beginning with24 (None).
};
SmacWorkshopAbilityMenu QuerySmacWorkshopAbilities(const SmacWorkshopEditor& editor,uint32_t slot,
    const SmacWorkshopEditContext& context,const SmacPredefinedUnitCatalog& catalog);
struct SmacWorkshopAbilityCost {int64_t quantity;int32_t formula,domain;};
// formula=-1..-7 selects localized arithmetic tokens,0 has no formula;
// domain=-1 means no domain suffix. No mineral-factor or strength substitution.
std::optional<SmacWorkshopAbilityCost> QuerySmacWorkshopAbilityCost(int32_t code,uint32_t flags,
    int32_t triad,uint32_t weapon_cost,uint32_t armor_cost,uint32_t chassis_speed);
// Accepted mapped-picker caller bodies, including same-value modal returns.
// Does not allocate, save, recenter the browse list or alter the selected record.
SmacWorkshopEditor ChangeSmacWorkshopComponent(const SmacWorkshopEditor& before,SmacWorkshopComponent kind,
    uint32_t accepted,const SmacWorkshopEditContext& context,const SmacPredefinedUnitCatalog& catalog);
// Slot-entry guards precede the accepted modal result. >=24 means None.
// The chooser separately owns technology, compatibility and duplicate filtering.
struct SmacWorkshopAbilityEdit {SmacWorkshopEditor editor;bool admitted;};
SmacWorkshopAbilityEdit SetSmacWorkshopAbilitySlot(const SmacWorkshopEditor& before,uint32_t slot,int32_t accepted,
    const SmacWorkshopEditContext& context,const SmacPredefinedUnitCatalog& catalog);
// Generic modal return boundary: invalid/cancelled hover preserves old choice;
// an admitted mapped reactor index becomes a one-based rating.
int32_t ResolveSmacWorkshopPickerResult(int32_t old_choice,int32_t mapped_hover,bool valid,SmacWorkshopComponent kind);
// Original custom-record construction. Preserves name, byte47 and prerequisite;
// technology/picker legality and automatic naming belong to the caller.
void BuildSmacWorkshopRecord(SmacOriginalDesignPool& pool,size_t index,const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner);
// Installed-component naming (including PSI), retained selected plan/chassis/reactor context
// and own-bank/intrinsic collisions. Draft operands need not match that record.
// Returns the untruncated native buffer; saving owns the31-byte boundary.
std::string NameSmacWorkshopDesign(const SmacOriginalDesignPool& pool,size_t index,const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,int32_t owner);
std::string DisplaySmacWorkshopName(const SmacOriginalDesignPool& pool,const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,int32_t owner);
struct SmacWorkshopRename {SmacStartupDesignState state;SmacWorkshopDraft draft;std::string default_name;};
// Action5 clears selected named-bit2 before showing an automatically named
// default. Cancel preserves that effect; accepted text edits only the draft.
SmacWorkshopRename OpenSmacWorkshopRename(const SmacStartupDesignState& state,const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,int32_t owner);
SmacWorkshopRename AnswerSmacWorkshopRename(const SmacStartupDesignState& state,const SmacWorkshopDraft& draft,
    const std::string& displayed_default,bool accepted,const std::string& submitted,
    const SmacPredefinedUnitCatalog& catalog,int32_t owner);
// Explicit post-capability-refresh inputs. Address keys identify original
// scalar contracts; no executable or captured Faction buffer is a runtime input.
// Missing authority is an error, never an implicit zero.
struct SmacWorkshopHumanContext {
    std::map<uint32_t,int32_t> fields;
    int32_t innate_morale;
    uint8_t threat; // Ordinary human entry is0;1..3 are declared stage controls.
    SmacWorkshopHumanContext(std::map<uint32_t,int32_t> inputs,int32_t morale,uint8_t stage_threat=0)
        : fields(std::move(inputs)),innate_morale(morale),threat(stage_threat) {}
};
struct SmacWorkshopHumanStep {
    SmacWorkshopDraft proposal,published;
    int32_t category,source_id,alias,mode,filter;
};
struct SmacWorkshopHumanPreview {
    SmacStartupDesignState state;
    SmacWorkshopDraft draft;
    int32_t mode=1,filter=-1;
    std::vector<SmacWorkshopHumanStep> steps;
};
// Original597de0 + preview597390, conventional stock-human domain.
// Caller owns capability refresh, provisional tuple and selector readback.
SmacWorkshopHumanPreview PrepareSmacWorkshopHumanPreview(const SmacStartupDesignState& state,
    const SmacWorkshopDraft& provisional,const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner,
    const SmacWorkshopHumanContext& context,const faction::SmacDesignCounts& units,
    const faction::SmacDesignCounts& production);

// Original5bdb60 ordering: active included, every inactive own slot,
// excluded own with/without cached Units, then excluded intrinsics.
// A missing count row is acceptable only when no excluded active own row needs it.
std::vector<int32_t> BrowseSmacWorkshopDesigns(const SmacStartupDesignState& state,
    const SmacPredefinedUnitCatalog& catalog,const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner,const std::optional<faction::SmacDesignCounts>& units);

// Original4490b0(id,0), used by card selection and Apply reselection.
// Does not run normal-open recommendations or compatibility cleanup.
struct SmacWorkshopReselection {
    SmacStartupDesignState state;
    SmacWorkshopDraft draft;
    std::array<int32_t,2> ability_slots{{-1,-1}};
};
SmacWorkshopReselection PrepareSmacWorkshopReselection(const SmacStartupDesignState& state,int32_t requested,
    const std::string& retained_name,const SmacPredefinedUnitCatalog& catalog,int32_t owner);
struct SmacWorkshopObsolete {SmacWorkshopReselection selection;size_t strip_offset;};
// Direct595ac0 design/history portion. Unit removal and custom Base queue
// surgery must be staged by the live caller before publishing this state.
SmacStartupDesignState RetireSmacWorkshopDesign(const SmacStartupDesignState& state,int32_t selected,
    const SmacPredefinedUnitCatalog& catalog,int32_t owner);
// Action6 toggles stored exclusion/flags before navigating the old browse list.
// Draft components are not saved; inactive reselection preserves draft name.
SmacWorkshopObsolete PrepareSmacWorkshopObsolete(const SmacStartupDesignState& state,int32_t selected,
    const std::string& retained_name,const std::vector<int32_t>& old_list,size_t strip_offset,const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner,
    const std::optional<faction::SmacDesignCounts>& units);

enum class SmacWorkshopSlotKind {Alias,Build,IntrinsicFailure};
struct SmacWorkshopSlot {int32_t selected;SmacWorkshopSlotKind kind;bool confirmation;};
// Original 441e20 tuple matching and two-stage allocation. Consumes retained
// count bytes; no recount, retirement or design mutation occurs here.
SmacWorkshopSlot ChooseSmacWorkshopSlot(const SmacOriginalDesignPool& pool,const SmacWorkshopDraft& draft,
    const faction::SmacDesignCounts& units,const faction::SmacDesignCounts& production,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner);
struct SmacWorkshopCommit {
    int32_t design_id;
    bool intrinsic_name_warning;
};
// Original 441e20 matching pass. Component picker legality is a separate
// caller requirement; custom matches ignore stored names and prerequisites.
// No match => no mutation. Switching to an excluded intrinsic preserves the
// shared record's remaining flags and other owners' exclusion bits.
std::optional<SmacWorkshopCommit> ReuseSmacWorkshopDesign(SmacStartupDesignState& state,
    const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner);
// Read-only installed-language label for admitted unnamed legacy opening rows.
std::string SmacWorkshopDesignName(const SmacCustomDesign& design,const SmacPredefinedUnitCatalog& catalog,int32_t owner);

// Detached common commit. A confirmation result preserves the input state.
// Accepting it prepares the old tuple's history and identifies the retirement
// whose Units/queues the live caller must stage before publishing this state.
enum class SmacWorkshopNotice { None=0, IntrinsicType=1, IntrinsicName=2, UnchangedIntrinsic=3 };
struct SmacWorkshopPreparation {
    SmacStartupDesignState state;
    SmacWorkshopCommit result;
    bool success=false,confirmation=false;
    std::optional<int32_t> retirement;
    SmacWorkshopNotice notice=SmacWorkshopNotice::None;
};
SmacWorkshopPreparation PrepareSmacWorkshopCommit(const SmacStartupDesignState& state,const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner,const faction::SmacDesignCounts& units,
    const faction::SmacDesignCounts& production,bool confirm=false);

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
