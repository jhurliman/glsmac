#include "BaseManager.h"

#include "Base.h"
#include "PopDef.h"
#include "FacilityDef.h"
#include "SmacAutomaticWorkerAllocation.h"
#include "SmacWorkedSquareCollection.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "gc/Space.h"
#include <algorithm>
#include <limits>
#include <memory>

namespace game::backend::base {

SmacBaseUpkeepCache BaseManager::PrepareNewBaseResourceCache( const Base& candidate,
    const std::array<int32_t,3>& intake, bool startup, std::optional<size_t> consumed_unit ) const {
    if(startup) {m_game->RequireConventionalStartupClaim();}
    if(!m_game->m_simulation || !candidate.m_resource_context || GetBase(candidate.m_id)
        || candidate.GetGame()!=m_game || candidate.m_pops.empty() || candidate.m_pops.size()>127 || !m_game->GetUM()) {
        THROW("Initial resource cache requires the prepared new Base");
    }
    const auto* social=m_game->m_simulation->value.social.Find(candidate.m_resource_context->owner_simulation_id);
    const auto phase=startup ? faction::SmacFactionSocialState::Phase::Early : faction::SmacFactionSocialState::Phase::Refreshed;
    if(!social || social->phase!=phase) {THROW("Initial resource cache has no current owner SUPPORT");}
    // The existing candidate/knowledge projection proves that a fresh Base ID
    // has no home links. Retain every surviving actual record as Ignored, using
    // the actual current rating for the allowance; do not invent retained Early
    // social ratings or an empty global Unit roster.
    const auto population=static_cast<int32_t>(candidate.m_pops.size());
    auto support=SmacUnitSupport::Recalculate(social->current.at(2),population,{});
    const auto* um=m_game->GetUM();support.units.reserve(um->GetUnits().size());
    for(const auto id:um->GetUnitRecordOrder()) {
        const auto* current=um->GetUnit(id);
        if(!current || !current->IsConventional() || current->m_id!=id) {THROW("Initial support has an invalid Unit record");}
        if(current->Conventional().home_base_id==candidate.m_id || (startup && current->Conventional().home_base_id)) {
            THROW("Initial resource cache cannot assume an existing home is absent");
        }
        if(consumed_unit!=id) {support.units.push_back({id,SmacSupportDisposition::Ignored});}
    }
    int32_t multiplier_steps=0;
    for(const auto& entry:candidate.m_facilities) {
        if(!entry.second || GetFacilityDef(entry.first)!=entry.second) {THROW("Initial resource cache facility is not registered");}
        ValidateSmacFacilityDef(entry.second);
        if(!entry.second->m_smac_source_id || *entry.second->m_smac_source_id<1 || *entry.second->m_smac_source_id>33) {
            THROW("Initial resource cache requires resolved local facility roles");
        }
        const auto source=*entry.second->m_smac_source_id;
        if(source>=19 && source<=22) {++multiplier_steps;}
    }
    const auto& rules=m_game->GetState()->m_settings.global.GetSmacResourceRules();
    if(!rules) {THROW("Initial resource cache has no frozen nutrient demand");}
    return {FinishSmacResourceIntake({intake[0],intake[1],intake[2]},int64_t(rules->nutrients_per_citizen)*population,
        candidate.m_accumulated_nutrients,support.mineral_support,multiplier_steps),std::move(support)};
}

Base* BaseManager::SpawnStartingSmacBase( GSE_CALLABLE, slot::Slot* owner, map::tile::Tile* tile,
    const std::string& worker_id, const std::array< std::string, 7 >& specialist_ids ) {
    m_game->CheckRW( GSE_CALL );
    m_game->RequireConventionalStartupClaim();
    const auto& local = m_game->GetState()->m_settings.local;
    if ( gc_space != m_game->GetGCSpace() || !gc_space->IsAccumulating() || m_dispatching_updates
        || m_game->GetState()->GetConnection() || !owner || !owner->GetPlayer()
        || local.game_mode != settings::LocalSettings::GM_SINGLEPLAYER
        || local.network_type != settings::LocalSettings::NT_NONE || local.network_role != settings::LocalSettings::NR_NONE
        || !tile || tile->tiles != m_game->GetMap()->GetTilesPtr() || tile->base || tile->is_water_tile
        || m_bases.size() >= 512 || Base::GetNextId() == std::numeric_limits<size_t>::max() ) {
        THROW("First Base requires the owning local new-world startup and an empty land center");
    }
    const auto context = PrepareStartingResourceContext(owner);
    const auto& global = m_game->GetState()->m_settings.global;
    const auto* profile = global.GetSmacFactionStartupRules()->Find(owner->GetPlayer()->GetFaction()->m_id);
    const auto* technology = m_game->m_simulation->value.technology.Find(context.owner_simulation_id);
    if (!profile || !technology || !global.GetSmacSpecialistRules() || !global.GetSmacResourceRules()) {
        THROW("First Base requires complete frozen owner/resource/specialist inputs");
    }
    const auto* worker = GetPopDef(worker_id);
    if (!worker || !(worker->m_flags & PopDef::PF_TILE_WORKER) || worker->m_smac_specialist_id) {
        THROW("First Base requires an explicit registered worker role");
    }
    std::array<const PopDef*,7> specialists{};
    for (size_t i=0;i<specialists.size();++i) {
        specialists[i]=GetPopDef(specialist_ids[i]);
        if (!specialists[i] || specialists[i]->m_smac_specialist_id!=i || (specialists[i]->m_flags & PopDef::PF_TILE_WORKER)) {
            THROW("First Base requires all seven registered positional specialist roles");
        }
        ValidateSmacPopDef(specialists[i]);
    }
    Base::facilities_t facilities;
    const auto grant = [&](int32_t source) {
        const FacilityDef* selected=nullptr;
        for (const auto& entry:m_base_facilitydefs) {
            if (entry.second->m_smac_source_id==source) { selected=entry.second; break; }
        }
        if (!selected) { THROW("First Base facility source is not registered"); }
        ValidateSmacFacilityDef(selected);
        facilities.emplace(selected->m_id,selected);
    };
    grant(1); // The exact first-owner-Base check precedes this HQ grant.
    for (const auto& bonus:profile->Records()) {
        if (bonus.kind==settings::SmacFactionStartupBonus::Kind::FACILITY) {
            grant(std::clamp(bonus.parameter,1,38));
        }
    }
    auto target=m_game->MakeSmacProductionItem(owner,ProductionItemKind::Unit,1);
    if (!m_game->HasSmacProductionPrerequisite(owner,target)) {
        const auto difficulty=unit::SmacStartupDesignState::OriginalDifficulty(owner->GetPlayer()->GetDifficultyLevel());
        target=m_game->MakeSmacProductionItem(owner,ProductionItemKind::Unit,difficulty<=1 ? 0 : 2);
    }
    const int64_t credit=m_game->GetSmacProductionCost(owner,target)/2;
    const ProductionState production{target,credit,target,credit,std::vector<ProductionItem>{}};
    m_game->ValidateSmacProductionState(owner,production);
    // This private candidate never publishes its provisional Pop. The actual
    // allocation below replaces it before the first identity or center write.
    std::unique_ptr<Base> candidate(new Base(Base::ConstructionKind::PreparedStartup,m_game,Base::GetNextId(),owner,
        owner->GetPlayer()->GetFaction(),tile,"",facilities,{{1,Pop(nullptr,1,specialists[0],0)}},2,0,production,context,std::nullopt));
    auto knowledge=m_game->PrepareStartingBaseKnowledge(*candidate);
    const auto catalog=ResolveSmacSpecialistAllocationCatalog(*global.GetSmacSpecialistRules(),*global.GetSmacTechnologyRules(),technology->owned);
    bool aerospace=false;
    for (const auto& item:facilities) { aerospace |= item.second->m_smac_source_id==29; }
    // The saved campaign excludes projects; authenticating every existing
    // startup Unit in PrepareStartingBaseKnowledge also proves all are homeless.
    // No retained SUPPORT rating is invented for this zero-support initial call.
    const auto prefix=m_game->m_simulation->value.GetSatelliteCounts(context.owner_simulation_id).Intake(1,aerospace,false);
    const SmacWorkerRadiusMap radius{{static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y)},
        static_cast<int32_t>(m_game->GetMap()->GetWidth()),static_cast<int32_t>(m_game->GetMap()->GetHeight()),true};
    const SmacWorkerYieldQuery query=[&](int32_t index,const SmacWorkerStepState&) -> std::optional<SmacWorkerTileYield> {
        const auto point=SmacWorkedSquareCollection::Resolve(radius,index);
        if (!point) {return std::nullopt;}
        const auto* square=m_game->GetMap()->GetTile((*point)[0],(*point)[1]);
        const auto values=m_game->CalculateCanonicalTileResources(*square,*candidate,Game::ResourceQueryPhase::Ordinary,true);
        SmacWorkerTileYield result{{},square->is_water_tile};
        for (size_t i=0;i<3;++i) {
            if(values[i]<INT32_MIN || values[i]>INT32_MAX) {THROW("First worker yield exceeds signed32");}
            result.resources[i]=static_cast<int32_t>(values[i]);
        }
        return result;
    };
    // Actual creator seed: selected/count zero. Unused packed slots are fresh
    // zero storage in GLSMAC, not a claim about reused original Base records.
    const auto result=SmacAutomaticWorkerAllocation::Allocate({1,0,global.GetSmacResourceRules()->nutrients_per_citizen,0,
        SmacWorkerProduction::Unit,context.WorkerPriorities(profile->worker_priorities),context.ManageCitizens(),false,
        0,knowledge.unavailable,prefix,{{},0},catalog,std::nullopt},query);
    candidate->m_allocation=SmacBaseAllocationState{result.workers.visited,result.specialists};
    const auto coordinates=candidate->AllocationCoordinates(*candidate->m_allocation);
    if (coordinates.empty()) {
        candidate->m_pops.at(1)=Pop(nullptr,1,specialists.at(result.specialists.types[0]),0);
    } else {
        candidate->m_pops.at(1)=Pop(nullptr,1,worker,0,m_game->GetMap()->GetTile(coordinates[0].x,coordinates[0].y));
    }
    candidate->ValidateAllocationProjection(false);
    // Initial food is zero. Preserve the actual first N-calculation's deficit
    // marker without posting a turn of food or mineral production.
    candidate->m_smac_upkeep_cache=PrepareNewBaseResourceCache(*candidate,result.workers.intake,true);
    candidate->m_accumulated_nutrients=candidate->m_smac_upkeep_cache->calculation.nutrients.normalized_stored;
    PrepareBaseName(candidate.get());
    auto* result_base=candidate.get();
    const auto regional=m_game->PrepareCanonicalRegionalBaseDelta(*result_base,1,knowledge.topology);
    auto& economy=m_game->m_simulation->value.economy.at(context.owner_simulation_id);
    RegisterBase(result_base); // prepares every allocating registry/update copy
    candidate.release();
    result_base->CommitStartupPlacement();
    m_game->CommitStartingBaseKnowledge(knowledge);
    economy.regional_base_counts=regional;
    m_game->GetState()->TriggerObject(this,"base_spawn",ARGS_F(result_base) {
        {"base",result_base->Wrap(GSE_CALL)},
    }; });
    return result_base;
}

}
