#pragma once
#include "SmacStartupCapability.h"
#include "game/backend/base/SmacBaseNaming.h"
#include "game/backend/base/SmacFirstWorker.h"
#include "game/backend/map/SmacCompleteTerrainGeneration.h"
#include "game/backend/unit/SmacPredefinedDesigns.h"
#include "game/backend/unit/SmacStartupGroundUnit.h"

namespace game::backend::faction {
// Decoded original definition inputs. Named grants are ordered and may repeat;
// numeric initial-tech counts are handled by later research, not named grants.
struct SmacGeneratedFactionInput {
    std::string identity;
    SmacStartupPersonality personality;
    std::vector<int32_t> named_technologies,facilities;
    std::array<int32_t,11> social;
    int32_t drone,talent;
    base::SmacBaseNameCatalog names;
};
struct SmacGeneratedStartupRules {
    std::array<SmacGeneratedFactionInput,7> factions;
    base::SmacOpeningYieldRules yields;
    SmacCapabilityCatalog capabilities;
    unit::SmacDesignComponents components;
    std::vector<unit::SmacPredefinedDesign> predefined;
    unit::SmacEarlyDesignNames design_names;
    int32_t formers_technology;
};
struct SmacGeneratedStartup {
    map::smac_generation::State terrain;
    map::smac_generation::Regions regions;
    std::array<SmacStartupFaction,8> actors{};
    std::vector<base::SmacStartupBaseRecord> bases;
    std::vector<unit::SmacNativeUnitRecord> units;
    unit::SmacOriginalDesignPool designs{};
    SmacStartupTechnologyOwnership ownership{};
};
struct SmacGeneratedStartupObservers {
    SmacStartupObserver random;
    std::function<void(int32_t,const SmacGeneratedStartup&)> founded;
    base::SmacBaseAccountingObserver worker_stores;
    std::function<void(const base::SmacFirstWorkerQuery&)> worker_queries;
    std::function<void(int32_t,int32_t)> worker_scores;
};
// Detached ordinary local stock SMAC startup: human slot1, all seven active,
// difficulty3, Standard Rules, no extra configured UNITs or Look First. Copies
// completed generated geography, composes actual gameplay producers and returns
// complete records through setup return. Source inputs are not modified and no
// live world is published. Broader startup profiles require separate admission.
SmacGeneratedStartup CompleteSmacGeneratedStartup(const map::smac_generation::State& terrain,
    const map::smac_generation::Regions& regions,map::smac_generation::Geometry geometry,
    const SmacGeneratedStartupRules& rules,const std::string& channel_prefix,
    const SmacGeneratedStartupObservers& observers={});
}
