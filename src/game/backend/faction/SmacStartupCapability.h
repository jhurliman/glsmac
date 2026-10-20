#pragma once
#include "SmacStartupTechnology.h"

namespace game::backend::faction {
struct SmacCapabilityComponent {int32_t power,prerequisite;};
struct SmacCapabilityChassis {uint8_t speed,domain;int32_t prerequisite;};
struct SmacCapabilityCatalog {
    std::array<SmacCapabilityComponent,21> weapons;
    std::array<SmacCapabilityComponent,10> armor;
    std::array<SmacCapabilityChassis,9> chassis;
    uint16_t mind_worm_flags;
    uint8_t mind_worm_exclusion,mind_worm_plan;
    int32_t mind_worm_prerequisite;
};
struct SmacCapabilityInputs {
    int32_t owner;uint32_t rules;
    int32_t psi_percent,psi_offense,psi_defense,dream_twister_owner,neural_amplifier_owner;
};
struct SmacCapabilityStore {uint32_t offset;uint8_t size;uint32_t value,caller;};
using SmacCapabilityObserver=std::function<void(const SmacCapabilityStore&)>;
// Complete original 0x577140. Rebuilds own and foreign capability caches from
// catalogs, effective ownership and retained peer caches. No RNG or live publication.
void RefreshSmacStartupCapabilities(std::array<SmacStartupFaction,8>& actors,
    const SmacStartupTechnologyOwnership& ownership,const std::array<std::array<int32_t,2>,89>& prerequisites,
    const SmacCapabilityCatalog& catalog,const SmacCapabilityInputs& inputs,const SmacCapabilityObserver& observer={});
}
