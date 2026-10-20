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
}
