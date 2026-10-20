#pragma once
#include "SmacOpeningEnergy.h"
#include "SmacWorkedSquareCollection.h"
#include "game/backend/faction/SmacStartupTechnology.h"

namespace game::backend::base {
struct SmacOpeningYieldRules {
    std::array<std::array<int32_t,3>,9> resources;
    std::array<std::array<int32_t,2>,89> prerequisites;
    std::array<int32_t,3> limit_technologies;
    int32_t platform_technology,food_per_citizen,mine_nutrients,unroaded_mine_limit;
};
struct SmacFirstWorkerOptions {int32_t width,height;bool horizontal_wrap;int32_t pattern;uint32_t primary_rules;};
struct SmacFirstWorkerScratch {int32_t food_sufficient,remaining,unavailable;std::array<int32_t,25> reasons;};
struct SmacFirstWorkerQuery {bool dispatcher;int32_t resource,x,y,value;};
// Complete 4fd650 in the unmanaged human population-one opening profile:
// single founded Base, no Units/contacts/satellites/projects/events, empty or
// center-only initial selection, no retained specialists, Unit target.
// Calculates raw yields and scores from current records using existing rules.
int32_t CalculateSmacFirstWorker(const std::vector<std::array<uint8_t,44>>& tiles,SmacStartupBaseRecord& base,
    const faction::SmacStartupFaction& actor,const faction::SmacStartupTechnologyOwnership& ownership,
    const SmacOpeningYieldRules& rules,const SmacFirstWorkerOptions& options,SmacFirstWorkerScratch& scratch,
    const SmacBaseAccountingObserver& observer={},const std::function<void(const SmacFirstWorkerQuery&)>& query_observer={},
    const std::function<void(int32_t,int32_t)>& score_observer={});
struct SmacFirstBaseScratch {
    SmacOpeningSupportScratch support;SmacFirstWorkerScratch worker;SmacOpeningEnergyScratch energy;int32_t growth;
};
struct SmacFirstBaseOptions {
    SmacFirstWorkerOptions worker;SmacOpeningCitizenInputs citizens;
    int32_t global_difficulty,sea_level,sea_rise_rule,native_life,flat_labs;uint32_t secondary_rules;
};
// Compose the complete resource calculation for the fresh first human HQ,
// before the Base caller's dirty OR2. Exactly one Base and no Units; projects,
// events and broader campaign contexts are outside this opening profile.
int32_t CalculateSmacFirstBase(const std::vector<std::array<uint8_t,44>>& tiles,std::vector<SmacStartupBaseRecord>& bases,
    std::array<faction::SmacStartupFaction,8>& actors,const std::vector<unit::OriginalUnitBytes>& units,
    const faction::SmacStartupTechnologyOwnership& ownership,const SmacOpeningYieldRules& rules,
    const SmacFirstBaseOptions& options,SmacFirstBaseScratch& scratch,const SmacBaseAccountingObserver& observer={},
    const std::function<void(const SmacFirstWorkerQuery&)>& query_observer={},const std::function<void(int32_t,int32_t)>& score_observer={});
}
