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
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/slot/Slot.h"
#include "gc/Space.h"
#include <algorithm>
#include <limits>
#include <memory>

namespace game::backend::base {
void BaseManager::ValidateSmacFounding(const unit::Unit& colony) const {
	m_game->RequireSmacGameplayActive();
    using namespace map::tile;
    if (!m_game->IsRunning() || m_game->GetTurnId()==0 || m_game->GetTurnId()>INT32_MAX
        || m_dispatching_updates || m_game->GetUM()->GetUnit(colony.m_id)!=&colony || !colony.IsConventional()
        || colony.m_owner->GetPlayer()!=m_game->GetPlayer()) {THROW("Founding requires the current local running Colony Pod");}
    m_game->RequireSmacLocalHumanOwner(colony.m_owner);
    colony.RequireConventionalReady();
    const auto resolved=m_game->ResolveConventionalUnit(colony.Conventional(),colony.m_owner);
    if (resolved.package!=unit::ConventionalUnitContext::Package::Colony || !colony.HasMovesLeft() || colony.m_animation_id) {
        THROW("Founding requires an available ordinary land Colony Pod");
    }
    const auto& global=m_game->GetState()->m_settings.global;
    if (!global.IsOrdinarySmacCampaign()
        || m_game->m_simulation->value.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {
        THROW("Running founding currently admits the ordinary generated no-project campaign");
    }
    auto* tile=colony.GetTile();
    if (!tile || tile->tiles!=m_game->GetMap()->GetTilesPtr() || tile->base || tile->is_water_tile || tile->IsLocked()
        || tile->rockiness==ROCKINESS_ROCKY || (tile->features & (FEATURE_XENOFUNGUS|FEATURE_MONOLITH))
        || m_bases.size()>=512 || Base::GetNextId()==std::numeric_limits<size_t>::max()) {
        THROW("Colony founding site is unavailable");
    }
    for (const auto* adjacent:tile->neighbours) {if (adjacent->base) {THROW("Founding cannot adjoin an existing Base");}}
    const auto& knowledge=m_game->GetMap()->GetSmacWorkerKnowledge();
    if (!knowledge) {THROW("Founding requires saved exploration and territory");}
    // Full current entity and directed-observer authentication, before reading
    // the raw claimant. Foreign diplomatic choices remain a separate UI path.
    const auto frame=m_game->ProjectSmacWorkerKnowledge(*knowledge,colony.Conventional().owner_simulation_id).CopyData();
    const auto& square=frame.tiles.at(size_t(tile->coord.y)*(frame.width/2)+tile->coord.x/2);
    const bool existing=std::any_of(m_bases.begin(),m_bases.end(),[&](const auto& row){return row.second->m_owner==colony.m_owner;});
    if (existing && square.claimant && *square.claimant!=colony.Conventional().owner_simulation_id) {
        THROW("Foreign-territory founding requires an explicit diplomatic choice");
    }
}

Base* BaseManager::FoundSmacBase(GSE_CALLABLE,unit::Unit& colony,const std::array<std::string,8>& roles) {
    m_game->CheckRW(GSE_CALL);
    if (gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Founding requires the owning accumulation");}
    ValidateSmacFounding(colony);
    auto* owner=colony.m_owner; auto* tile=colony.GetTile();
    const auto simulation_id=colony.Conventional().owner_simulation_id;
    const auto& global=m_game->GetState()->m_settings.global;
    const auto* profile=global.GetSmacFactionStartupRules()->Find(owner->GetPlayer()->GetFaction()->m_id);
    const auto* technology=m_game->m_simulation->value.technology.Find(simulation_id);
    const auto* social=m_game->m_simulation->value.social.Find(simulation_id);
    if (!profile || !technology || !social || !social->retained) {THROW("Founding requires actual retained owner inputs");}
    SmacBaseResourceContext context{simulation_id,false,0,0,SmacBaseResourceContext::NewGameGovernorOptions(m_game->m_simulation->value.designs.global_difficulty),false};
    ValidateSmacResourceContext(context,owner,owner->GetPlayer()->GetFaction());
    const auto* worker=GetPopDef(roles[0]);
    if (!worker || !(worker->m_flags & PopDef::PF_TILE_WORKER) || worker->m_smac_specialist_id) {THROW("Founding worker role is invalid");}
    ValidateSmacPopDef(worker);
    std::array<const PopDef*,7> specialists{};
    for (size_t i=0;i<7;++i) {
        specialists[i]=GetPopDef(roles[i+1]);
        if (!specialists[i] || specialists[i]->m_smac_specialist_id!=i || (specialists[i]->m_flags & PopDef::PF_TILE_WORKER)) {THROW("Founding specialist role is invalid");}
        ValidateSmacPopDef(specialists[i]);
    }
    Base::facilities_t facilities;
    const auto grant=[&](int32_t source) {
        const FacilityDef* selected=nullptr;
        for (const auto& entry:m_base_facilitydefs) {if(entry.second->m_smac_source_id==source) {selected=entry.second;break;}}
        if (!selected) {THROW("Founding facility source is not registered");}
        ValidateSmacFacilityDef(selected); facilities.emplace(selected->m_id,selected);
    };
    const bool first=std::none_of(m_bases.begin(),m_bases.end(),[&](const auto& row){return row.second->m_owner==owner;});
    const auto turn=static_cast<int32_t>(m_game->GetTurnId());
    const int32_t population=first ? std::clamp(static_cast<int32_t>((int64_t(turn)+49)/50),1,5) : 1;
    if(first) {
        grant(1);
        if(turn>=75) {grant(3);}
        if(turn>=100) {grant(6);}
        if(turn>=150) {grant(7);}
    }
    for (const auto& bonus:profile->Records()) {
        if (bonus.kind==settings::SmacFactionStartupBonus::Kind::FACILITY) {grant(std::clamp(bonus.parameter,1,38));}
    }
    const auto target=m_game->SelectSmacFoundingProduction(owner,tile);
    const int64_t credit=social->retained->at(2)>=-1 ? global.GetSmacProductionRules()->retool_exemption : 0;
    const ProductionState production{target,credit,target,credit,std::vector<ProductionItem>{}};
    m_game->ValidateSmacProductionState(owner,production);
    Base::pops_t pops;
    for(int32_t i=1;i<=population;++i) {pops.emplace(i,Pop(nullptr,i,specialists[0],0));}
    std::unique_ptr<Base> candidate(new Base(Base::ConstructionKind::PreparedStartup,m_game,Base::GetNextId(),owner,
        owner->GetPlayer()->GetFaction(),tile,"",facilities,pops,population+1,0,production,context,std::nullopt));
    candidate->m_allocation=SmacBaseAllocationState{0,{{},population}};
    std::optional<map::GlsmacTerritoryTopology> topology;
    auto knowledge=m_game->PrepareBaseTerritory(candidate.get(),nullptr,false,&topology);
    if (!knowledge) {THROW("Founding requires prepared saved territory");}
    // The normal projector authenticates actual entities before a private
    // candidate projection removes the consumed Pod and appends the new Base.
    auto frame=m_game->ProjectSmacWorkerKnowledge(*knowledge,simulation_id).CopyData();
    auto data=knowledge->CopyData();
    const auto observation=std::find_if(data.units.begin(),data.units.end(),[&](const auto& row){return row.id==colony.m_id;});
    if (observation==data.units.end()) {THROW("Founding Pod has no observation record");}
    frame.units.erase(frame.units.begin()+std::distance(data.units.begin(),observation));
    data.units.erase(observation);
    const map::SmacWorkerMapFrame::Coordinate center{static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y)};
    frame.bases.push_back({simulation_id,center,0});
    const auto unavailable=map::SmacWorkerMapFrame::Create(frame).Evaluate(frame.bases.size()-1).unavailable;
    knowledge=map::SmacWorkerKnowledge::Create(data); // exploration/memory unchanged
    const auto catalog=ResolveSmacSpecialistAllocationCatalog(*global.GetSmacSpecialistRules(),*global.GetSmacTechnologyRules(),technology->owned);
    bool aerospace=false; for (const auto& item:facilities) {aerospace |= item.second->m_smac_source_id==29;}
    const auto prefix=m_game->m_simulation->value.GetSatelliteCounts(simulation_id).Intake(population,aerospace,false);
    const SmacWorkerRadiusMap radius{center,frame.width,frame.height,true};
    const SmacWorkerYieldQuery query=[&](int32_t index,const SmacWorkerStepState&) -> std::optional<SmacWorkerTileYield> {
        const auto coordinate=SmacWorkedSquareCollection::Resolve(radius,index);
        if (!coordinate) {return {};}
        const auto* square=m_game->GetMap()->GetTile((*coordinate)[0],(*coordinate)[1]);
        const auto values=m_game->CalculateCanonicalTileResources(*square,*candidate,Game::ResourceQueryPhase::Ordinary,false,true);
        SmacWorkerTileYield result{{},square->is_water_tile};
        for(size_t i=0;i<3;++i) {if(values[i]<INT32_MIN || values[i]>INT32_MAX) {THROW("Founding yield exceeds signed32");} result.resources[i]=static_cast<int32_t>(values[i]);}
        return result;
    };
    // No Unit can already be homed to this unregistered fresh Base ID: the full
    // current projection above validated every home. The consumed Pod retains
    // its old home until this one transaction removes it.
    const auto result=SmacAutomaticWorkerAllocation::Allocate({population,turn,
        global.GetSmacResourceRules()->nutrients_per_citizen,0,SmacWorkerProduction::Unit,
        context.WorkerPriorities(profile->worker_priorities),context.ManageCitizens(),false,0,unavailable,prefix,{{},0},catalog,std::nullopt},query);
    candidate->m_allocation=SmacBaseAllocationState{result.workers.visited,result.specialists};
    const auto coordinates=candidate->AllocationCoordinates(*candidate->m_allocation);
    for(size_t i=0;i<size_t(population);++i) {
        candidate->m_pops.at(i+1)=i<coordinates.size() ? Pop(nullptr,i+1,worker,0,m_game->GetMap()->GetTile(coordinates[i].x,coordinates[i].y))
            : Pop(nullptr,i+1,specialists.at(result.specialists.types[i-coordinates.size()]),0);
    }
    candidate->ValidateAllocationProjection(false);
    candidate->m_smac_upkeep_cache=PrepareNewBaseResourceCache(*candidate,result.workers.intake,false,colony.m_id);
    candidate->m_accumulated_nutrients=candidate->m_smac_upkeep_cache->calculation.nutrients.normalized_stored;
    auto removal=m_game->GetUM()->PrepareConventionalRemoval(GSE_CALL,{colony.m_id});
    PrepareBaseName(candidate.get());
    auto* base=candidate.get();
    const auto regional=m_game->PrepareCanonicalRegionalBaseDelta(*base,1,m_game->GetMap()->GetTerritoryTopology());
    auto& economy=m_game->m_simulation->value.economy.at(simulation_id);
	const auto totals=faction::AdjustSmacBaseTotal(m_game->m_simulation->value.retained_base_totals,simulation_id,1);
	auto victory=m_game->m_simulation->value.victory_history;
	if(victory) {
		if(m_game->GetTurnId()>INT32_MAX) {THROW("Founding history turn exceeds signed32");}
		victory->RecordBaseExpansion(simulation_id,static_cast<int32_t>(m_game->GetTurnId()));
	}
    RegisterBase(base); // every allocating registry/queue operation precedes writes
    candidate.release(); base->CommitStartupPlacement(); removal.Commit();
    m_game->CommitBaseTerritory(knowledge,topology);
    economy.regional_base_counts=regional;
	m_game->m_simulation->value.retained_base_totals=totals;
	if(victory) {m_game->m_simulation->value.victory_history=std::move(victory);}
    removal.Notify(GSE_CALL); // retired wrappers are owned even if a hook throws
    m_game->GetState()->TriggerObject(this,"base_spawn",ARGS_F(base) {{"base",base->Wrap(GSE_CALL)},};});
    return base;
}
}
