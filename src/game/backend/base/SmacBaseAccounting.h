#pragma once
#include "SmacStartupBase.h"
#include "game/backend/unit/SmacUnitInitialization.h"
#include <functional>

namespace game::backend::base {
struct SmacBaseAccountingStore {
    enum class Space : uint8_t {Bases,Factions,Units,Global};
    Space space;
    uint32_t offset;
    uint8_t size;
    uint32_t value,caller;
};
using SmacBaseAccountingObserver=std::function<void(const SmacBaseAccountingStore&)>;
struct SmacNutrientAccountingInputs {
    int32_t selected_base,facility_base,incoming,outgoing,food_per_citizen,summary_mode;
};
// Complete 0x4ff0a0. Mutates selected intake/consumption/surplus/storage and
// mode1 owner aggregate; returns the independently written GROWTH scratch.
int32_t AccountSmacBaseNutrients(std::vector<SmacStartupBaseRecord>& bases,std::array<faction::SmacStartupFaction,8>& actors,
    const SmacNutrientAccountingInputs& inputs,const SmacBaseAccountingObserver& observer={});
// Slots follow ascending addresses 0x91867c..0x9186a8: support, incoming N/M/E/
// auxiliary, outgoing N/M/E/auxiliary and three other convoy scratch words.
using SmacOpeningSupportScratch=std::array<int32_t,12>;
// Full 0x4feb20 in the opening no-owned-Unit profile. Foreign records are read
// only at owner byte14, before interpreting any other potentially poisoned field.
// Own Units reject; general support/convoy processing remains a separate stage.
void PrepareSmacOpeningSupport(const std::vector<SmacStartupBaseRecord>& bases,int32_t selected_base,
    const std::vector<unit::OriginalUnitBytes>& units,SmacOpeningSupportScratch& scratch,
    const SmacBaseAccountingObserver& observer={});
struct SmacMineralProjectHosts {int32_t singularity=-1,bulk=-1,pholus=-1,elevator=-1,voice=-1;};
struct SmacMineralAccountingInputs {
    int32_t selected_base,facility_base,incoming,outgoing,support;
    uint8_t human_mask;
    int32_t global_difficulty,sea_level,sea_rise_rule,native_life;
    uint32_t primary_rules,secondary_rules;
    SmacMineralProjectHosts projects;
};
// Complete 0x4ff1e0 with the original literal DWORD arithmetic.
// Upstream allocation owns raw intake/terraforming; stock and faction records
// are preserved. Production/ecology algorithms share the existing checked
// callers' core, with explicit original wrapping at raw-record boundaries.
void AccountSmacBaseMinerals(std::vector<SmacStartupBaseRecord>& bases,const std::array<faction::SmacStartupFaction,8>& actors,
    const SmacMineralAccountingInputs& inputs,const SmacBaseAccountingObserver& observer={});
}
