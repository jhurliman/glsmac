#pragma once
#include "SmacBaseAccounting.h"

namespace game::backend::base {
// Original commerce/citizen scratch words, ascending 0x9185f8..0x918670.
using SmacOpeningEnergyScratch=std::array<int32_t,31>;
struct SmacOpeningCitizenInputs {uint8_t human_mask;int32_t definition_drone,definition_talent;};
struct SmacOpeningEnergyInputs {SmacOpeningCitizenInputs citizens;int32_t incoming,outgoing,flat_labs;};
// Complete 4ffa00, selected/indexed Base0: one settled owned Base, neutral
// social ratings, no Units/projects/mood facilities, mode0. Specialist exclusion
// and produced psych are inputs. HQ/Perimeter/Node are admitted.
void CalculateSmacOpeningCitizens(std::vector<SmacStartupBaseRecord>& bases,const std::array<faction::SmacStartupFaction,8>& actors,
    SmacOpeningEnergyScratch& scratch,const SmacOpeningCitizenInputs& inputs,const SmacBaseAccountingObserver& observer={});
// Complete 500880 including the citizen tail: first HQ, no specialists or
// trade-eligible relationships, neutral opening profile. Consumes raw worker
// intake and convoy scratch; does not credit treasury or advance production.
void AccountSmacOpeningEnergy(std::vector<SmacStartupBaseRecord>& bases,const std::array<faction::SmacStartupFaction,8>& actors,
    SmacOpeningEnergyScratch& scratch,const SmacOpeningEnergyInputs& inputs,const SmacBaseAccountingObserver& observer={});
}
