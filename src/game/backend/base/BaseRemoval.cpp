#include "BaseManager.h"
#include "Base.h"
#include "SmacPopulationStep.h"
#include "SmacWorkedSquareCollection.h"
#include "SmacFactionDisposition.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/SmacRestartSearch.h"
#include "game/backend/map/SmacRestartPreprocessing.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/SmacRestartUnitPackage.h"
#include "gc/Space.h"
#include <algorithm>
#include <memory>
#include <type_traits>

namespace game::backend::base {

BaseManager::SmacDispositionPlan BaseManager::PrepareLastSmacBaseDisposition(const Base& base) const {
    if(!m_game->IsRunning() || m_dispatching_updates || GetBase(base.m_id)!=&base || base.m_pops.size()!=1) {
        THROW("Faction disposition requires the current size-one Base");
    }
    const auto* map=m_game->GetMap();const auto* center=base.GetTile();
    if(!center || center->tiles!=map->GetTilesPtr() || center->base!=&base) {THROW("Faction disposition has no canonical center");}
    return PrepareSmacFactionDisposition(base.m_owner,&base,0,m_game->m_simulation->value.gameplay_random);
}
BaseManager::SmacDispositionPlan BaseManager::PrepareSmacFactionDisposition(slot::Slot* owner_slot,const Base* removed,int32_t captor,const SmacGameplayRandomState& random) const {
    m_game->RequireSmacLocalHumanOwner(owner_slot);
    const auto owner=m_game->RequireSmacProductionOwner(owner_slot).simulation_id;
    for(const auto& row:m_bases) {if(row.second!=removed && row.second->m_owner==owner_slot) {THROW("Faction disposition requires no surviving owned Base");}}
    auto& state=m_game->m_simulation->value;
    const auto participant=std::find_if(state.roster.begin(),state.roster.end(),[&](const auto& row){return row.simulation_id==owner;});
    const auto technology=std::find_if(state.technology.factions.begin(),state.technology.factions.end(),[&](const auto& row){return row.admission.simulation_id==owner;});
    if(participant==state.roster.end() || !participant->active || technology==state.technology.factions.end()) {THROW("Faction disposition has no active saved owner");}
    SmacDispositionPlan plan{static_cast<size_t>(participant-state.roster.begin()),static_cast<size_t>(technology-state.technology.factions.begin()),
        m_game->PrepareSmacNonzeroInitialization(owner),state.hostility,nullptr,{},random,{},{}};
    plan.hostility.Reinitialize(owner);
    const auto* map=m_game->GetMap();
    if(!map->GetSmacWorkerKnowledge()) {THROW("Faction disposition has no saved knowledge");}
    const auto& global=m_game->GetState()->m_settings.global;
    if(removed) {plan.economy.regional_base_counts=m_game->PrepareCanonicalRegionalBaseDelta(*removed,-1,map->GetTerritoryTopology());}
    plan.territory=m_game->PrepareBaseTerritory(nullptr,removed,false,&plan.topology);
    if(!plan.territory || !plan.topology || !map->GetResourcePolicy() || !plan.topology->Areas()
        || !plan.topology->Postpass() || !plan.topology->SiteRatings() || !plan.topology->LandmarkDetails()
        || !global.map.GetSmacOceanCoverage() || m_game->GetTurnId()==0 || m_game->GetTurnId()>INT32_MAX) {
        THROW("Faction restart requires owned terrain geography site and landmark history");
    }
    const auto& topology=*plan.topology;
    const auto knowledge=plan.territory->CopyData();
    const map::smac_site::Geometry geometry{topology.Width(),topology.Height(),topology.Wraps()};
    const auto index=[&](map::smac_site::Coordinate point) {return size_t(point[1])*(geometry.width/2)+point[0]/2;};
    std::vector<const Base*> survivors;std::vector<map::smac_site::Coordinate> centers;
    for(const auto id:m_base_order) {
        const auto* current=GetBase(id);if(current==removed) {continue;}
        if(!current) {THROW("Restart Base order is incomplete");}
        survivors.push_back(current);centers.push_back({{static_cast<int32_t>(current->GetTile()->coord.x),static_cast<int32_t>(current->GetTile()->coord.y)}});
    }
    std::array<int32_t,128> presence{};
    for(const auto& entry:state.economy) {
        if(entry.first<1 || entry.first>7) {continue;}
        const auto& counts=entry.first==owner ? plan.economy.regional_base_counts : entry.second.regional_base_counts;
        if(!counts) {THROW("Faction restart lacks retained regional Base counts");}
        for(size_t region=0;region<presence.size();++region) {presence[region]+=counts->at(region)!=0;}
    }
    // All victim Units are absent from the future world, including a transient
    // manufactured Pod. Keep current entities untouched throughout preparation.
    std::vector<bool> occupied(topology.Regions().size(),false);
    for(const auto point:centers) {occupied[index(point)]=true;}
    for(const auto& entry:m_game->GetUM()->GetUnits()) {
        const auto* current=entry.second;if(current->m_owner==owner_slot) {continue;}
        current->RequireConventionalReady();const auto* tile=current->GetTile();
        occupied[index({{static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y)}})]=true;
    }
    const auto altitude=[&](const map::tile::Tile* tile) {
        if(!tile->elevation.center) {THROW("Restart terrain lacks elevation");}
        return static_cast<uint8_t>(map->GetResourcePolicy()->Altitude(*tile));
    };
    const auto wet=[&](map::smac_site::Coordinate point) {
        const auto* tile=map->GetTile(point[0],point[1]);
        if(altitude(tile)<3) {return false;}
        if(tile->moisture<map::tile::MOISTURE_ARID || tile->moisture>map::tile::MOISTURE_RAINY
            || tile->rockiness<map::tile::ROCKINESS_FLAT || tile->rockiness>map::tile::ROCKINESS_ROCKY) {THROW("Restart terrain lacks decoded land ordinals");}
        return tile->moisture>map::tile::MOISTURE_ARID && tile->rockiness<=map::tile::ROCKINESS_ROLLING
            && !(tile->features&map::tile::FEATURE_XENOFUNGUS);
    };
    const auto search=map::smac_restart::Search({geometry,static_cast<int32_t>(m_game->GetTurnId()),owner,captor,
        unit::SmacStartupDesignState::OriginalDifficulty(owner_slot->GetPlayer()->GetDifficultyLevel()),
        map::smac_restart::ScaleFromGeometry(geometry),*global.map.GetSmacOceanCoverage()},plan.random,[&](auto point) {
        map::smac_restart::Candidate candidate;const auto at=index(point);const auto* tile=map->GetTile(point[0],point[1]);
        candidate.elevation=altitude(tile);candidate.rockiness=tile->is_water_tile ? 0 : tile->rockiness-map::tile::ROCKINESS_FLAT;
        candidate.region=topology.Regions()[at];candidate.fungus=bool(tile->features&map::tile::FEATURE_XENOFUNGUS);
        candidate.occupied=occupied[at];candidate.valid_occupant=occupied[at];
        candidate.area=topology.Areas()->at(candidate.region);candidate.fertility=topology.Postpass()->fertility.at(candidate.region);
        candidate.present_factions=presence[candidate.region];candidate.territory_owner=knowledge.tiles[at].claimant.value_or(-1);
        candidate.landmark_bucket=map::smac_restart::LandmarkBucket(geometry,point,[&](auto square){return topology.LandmarkDetails()->at(index(square));});
        for(int32_t i=0;i<21;++i) {
            const auto square=SmacWorkedSquareCollection::Resolve({point,geometry.width,geometry.height,geometry.wrap},i);
            if(square && wet(*square)) {++candidate.wet_neighbors;}
        }
        for(size_t i=0;i<survivors.size();++i) {
            const auto position=centers[i];if(topology.Regions()[index(position)]!=candidate.region) {continue;}
            auto dx=std::abs(point[0]-position[0]);if(geometry.wrap) {dx=std::min(dx,geometry.width-dx);}
            const auto dy=std::abs(point[1]-position[1]);const auto distance=std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
            if(distance<=candidate.nearest_distance) {
                candidate.nearest=map::smac_restart::Base{survivors[i]->GetSmacResourceContext().owner_simulation_id,candidate.region};candidate.nearest_distance=distance;
            }
        }
        return candidate;
    },[&](auto point){plan.topology=map->PrepareSiteRatingQuery(*plan.topology,centers,m_game->GetTurnId(),point);});
    plan.random=search.random;
    // Original guards run after search and retain its draws/cache writes.
    if(!search.site || state.do_or_die || participant->restart_prohibited || m_game->GetTurnId()>100) {return plan;}
    plan.site=map->GetTile((*search.site)[0],(*search.site)[1]);
    const auto package=unit::SmacRestartUnitPackage::Select(static_cast<int32_t>(m_game->GetTurnId()),state.designs,
        global.GetSmacPredefinedUnitRules()->catalog,*technology,*global.GetSmacTechnologyRules());
    for(const auto id:package.creation_order) {
        plan.units.push_back({m_game->ResolveSmacProductionPresentation(owner_slot,id),
            {id,owner,1,0,0,false,unit::ConventionalUnitState::Home::None,unit::SmacMoraleMode::Mode0}});
    }
    plan.economy.rates={0,5};return plan;
}
void BaseManager::CommitSmacFactionDisposition(SmacDispositionPlan& plan) noexcept {
    auto& state=m_game->m_simulation->value;const auto owner=state.roster[plan.participant].simulation_id;
    static_assert(std::is_nothrow_copy_assignable_v<faction::SmacEconomyState> && std::is_nothrow_copy_assignable_v<SmacGameplayRandomState>);
    plan.economy.design_counts=state.economy.at(owner).design_counts;
    state.economy.at(owner)=plan.economy;state.technology.factions[plan.technology].cached_threshold.reset();
    if(state.capabilities) {state.capabilities->factions[owner-1]={};}
    state.hostility.owners.swap(plan.hostility.owners);state.gameplay_random=plan.random;
    state.roster[plan.participant].active=plan.site!=nullptr;
    if(!plan.site && state.IsHuman(owner)) {state.human_defeat_request=owner;}
}

BaseManager::SmacAbandonmentResult BaseManager::DisposeLastSmacBase(GSE_CALLABLE,Base& base,
    unit::Def* transient_definition,unit::SmacMoraleMode mode) {
    auto plan=PrepareLastSmacBaseDisposition(base);
    auto* um=m_game->GetUM();
    std::optional<unit::UnitManager::PreparedConventionalUnit> transient;
    SmacAbandonmentResult result{};
    if(transient_definition) {
        transient.emplace(um->PrepareManufacturedConventionalUnit(GSE_CALL,base,*base.GetProductionState()->current_item.smac_source_id,transient_definition,mode));
        result.newborn_id=transient->Candidate().m_id;
    }
    auto units=um->PrepareFactionUnits(GSE_CALL,base.m_owner,plan.site,plan.units,*plan.territory,transient ? &*transient : nullptr);
    auto bases=m_bases;bases.erase(base.m_id);
    auto order=m_base_order;order.erase(std::remove(order.begin(),order.end(),base.m_id),order.end());
    auto assignments=m_work_assignments;
    for(auto it=assignments.begin();it!=assignments.end();) {if(it->second.first==base.m_id) {it=assignments.erase(it);} else {++it;}}
    auto updates=m_base_updates;QueueBaseUpdateInto(updates,&base,BUO_DESPAWN);
    std::unique_lock lock(m_updated_bases_mutex);auto triggers=m_updated_bases;triggers.erase(&base);
    static_assert(std::is_nothrow_swappable_v<decltype(bases)> && std::is_nothrow_swappable_v<decltype(order)>
        && std::is_nothrow_swappable_v<decltype(assignments)> && std::is_nothrow_swappable_v<decltype(updates)>
        && std::is_nothrow_swappable_v<decltype(triggers)> && std::is_nothrow_copy_assignable_v<faction::SmacEconomyState>);
    m_bases.swap(bases);m_base_order.swap(order);m_work_assignments.swap(assignments);
    m_base_updates.swap(updates);m_updated_bases.swap(triggers);base.GetTile()->base=nullptr;
    CommitSmacFactionDisposition(plan);
    units.Commit();m_game->CommitBaseTerritoryTopology(plan.topology);std::unique_ptr<Base> retired(&base);lock.unlock();
    units.Notify(GSE_CALL);
    m_game->GetState()->TriggerObject(this,"base_despawn",ARGS_F(&base) {{"base",base.Wrap(GSE_CALL)},};});
    return result;
}

void BaseManager::ValidateSmacAbandonment( const Base& base ) const {
    if (!m_game->IsRunning() || m_dispatching_updates || GetBase(base.m_id)!=&base || base.m_pops.size()!=1) {
        THROW("Colony abandonment requires the current size-one canonical Base");
    }
    m_game->RequireSmacLocalHumanOwner(base.m_owner);
    base.GetSmacResourceContext();
    const auto& production=base.GetProductionState();
    if (!production || !production->queue || production->current_item.kind!=ProductionItemKind::Unit
        || !production->current_item.smac_source_id) { THROW("Colony abandonment requires its posted production state"); }
    m_game->ValidateSmacProductionState(base.m_owner,*production);
    if (production->minerals<m_game->GetSmacProductionCost(base.m_owner,production->current_item)) {
        THROW("Colony abandonment has not reached the actual completion cost");
    }
    auto* tile=base.GetTile();
    const auto* map=m_game->GetMap();
    if (!tile || !map || tile->tiles!=map->GetTilesPtr() || tile->base!=&base) {THROW("Abandoned Base center differs");}
    const auto position=std::find(m_base_order.begin(),m_base_order.end(),base.m_id);
    if (position==m_base_order.end()) {THROW("Abandoned Base is absent from record order");}
    if (std::next(position)!=m_base_order.end()) {
        const auto* successor=GetBase(*std::next(position));
        if (!successor || successor->m_owner!=base.m_owner) {THROW("Foreign compacted successor continuation is not yet admitted");}
        if (!successor->GetSmacUpkeepCache()) {THROW("Compacted successor lacks its retained resource calculation");}
        const auto population=static_cast<int32_t>(successor->m_pops.size());
        const auto plan=SmacPopulationStep::Prepare(m_game->CalculateCanonicalPopulationRules(*successor,population),population,
            successor->GetAccumulatedNutrients(),successor->GetSmacUpkeepCache()->calculation.nutrients.surplus,false);
        if (!plan.stored_nutrients) {THROW("Compacted successor removal requires the disposition continuation");}
    }
    const bool other_owned=std::any_of(m_base_order.begin(),m_base_order.end(),[&](size_t id){
        const auto* candidate=GetBase(id);
        return candidate && candidate!=&base && candidate->m_owner==base.m_owner;
    });
    if (!other_owned) {(void)PrepareLastSmacBaseDisposition(base);}
}

BaseManager::SmacAbandonmentResult BaseManager::AbandonSmacBase( GSE_CALLABLE, Base& base,
    unit::Def* definition, unit::SmacMoraleMode mode ) {
    m_game->CheckRW(GSE_CALL);
    if (gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating() || !m_game->IsRunning()
        || m_dispatching_updates || GetBase(base.m_id)!=&base || base.m_pops.size()!=1) {
        THROW("Colony abandonment requires the current size-one canonical Base");
    }
    ValidateSmacAbandonment(base);
    const auto& production=base.GetProductionState();
    auto* tile=base.GetTile(); const auto* map=m_game->GetMap();
    const auto position=std::find(m_base_order.begin(),m_base_order.end(),base.m_id);
    SmacAbandonmentResult result{};
    if (std::next(position)!=m_base_order.end()) {
        const auto* successor=GetBase(*std::next(position));
        result.successor_id=successor->m_id; result.successor_cache=successor->GetSmacUpkeepCache();
    }
    std::optional<map::GlsmacTerritoryTopology> topology;
    auto territory=m_game->PrepareBaseTerritory(nullptr,&base,false,&topology);
    if (!territory) {THROW("Colony abandonment requires saved worker knowledge and territory");}
    Base* replacement=nullptr;
    int64_t nearest=INT64_MAX;
    for (const auto id:m_base_order) {
        auto* candidate=GetBase(id);
        if (!candidate) {THROW("Abandonment Base record order is invalid");}
        candidate->GetSmacResourceContext();
        if (candidate==&base || candidate->m_owner!=base.m_owner) {continue;}
        const auto* center=candidate->GetTile();
        auto dx=std::abs(int64_t(center->coord.x)-int64_t(tile->coord.x));
        const auto dy=std::abs(int64_t(center->coord.y)-int64_t(tile->coord.y));
        dx=std::min(dx,int64_t(map->GetWidth())-dx);
        const auto distance=std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
        if (distance<=nearest) {replacement=candidate;nearest=distance;}
    }
    if (!replacement) {return DisposeLastSmacBase(GSE_CALL,base,definition,mode);}
    const auto regional=m_game->PrepareCanonicalRegionalBaseDelta(base,-1,map->GetTerritoryTopology());
    auto& economy=m_game->m_simulation->value.economy.at(base.GetSmacResourceContext().owner_simulation_id);
    auto* um=m_game->GetUM();
    if (!um) {THROW("Colony abandonment requires its Unit manager");}
    um->RequireConventionalUnitsReady();
    auto newborn=um->PrepareManufacturedConventionalUnit(GSE_CALL,base,*production->current_item.smac_source_id,definition,mode);
    newborn.PrepareBaseAbandonment(base,*replacement,*territory);
    result.newborn_id=newborn.Candidate().m_id;

    auto bases=m_bases; bases.erase(base.m_id);
    auto order=m_base_order; order.erase(order.begin()+std::distance(m_base_order.begin(),position));
    auto assignments=m_work_assignments;
    for (auto it=assignments.begin();it!=assignments.end();) {
        if (it->second.first==base.m_id) {it=assignments.erase(it);} else {++it;}
    }
    auto updates=m_base_updates; QueueBaseUpdateInto(updates,&base,BUO_DESPAWN);
    std::unique_lock lock(m_updated_bases_mutex);
    auto triggers=m_updated_bases; triggers.erase(&base);
    static_assert(std::is_nothrow_swappable_v<decltype(bases)> && std::is_nothrow_swappable_v<decltype(order)>
        && std::is_nothrow_swappable_v<decltype(assignments)> && std::is_nothrow_swappable_v<decltype(updates)>
        && std::is_nothrow_swappable_v<decltype(triggers)>);
    // Every registry, queue, home and knowledge allocation is complete. Commit
    // the same factory record, clearing only its home; old homes use replacement.
    m_bases.swap(bases); m_base_order.swap(order); m_work_assignments.swap(assignments);
    m_base_updates.swap(updates); m_updated_bases.swap(triggers); tile->base=nullptr;
    newborn.Commit(); // also commits the joined sight/claims and old home repairs
    m_game->CommitBaseTerritoryTopology(topology);
    economy.regional_base_counts=regional;
    std::unique_ptr<Base> retired(&base);
    lock.unlock();
    newborn.Notify(GSE_CALL);
    m_game->GetState()->TriggerObject(this,"base_despawn",ARGS_F(&base) {{"base",base.Wrap(GSE_CALL)},};});
    return result;
}

void BaseManager::RemoveStarvedSmacBase(GSE_CALLABLE, Base& base) {
    m_game->CheckRW(GSE_CALL);
    if (gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating() || !m_game->IsRunning()
        || m_dispatching_updates || GetBase(base.m_id)!=&base || base.m_pops.size()!=1) {
        THROW("Starvation removal requires the current size-one canonical Base");
    }
    m_game->RequireSmacLocalHumanOwner(base.m_owner);
    base.GetSmacResourceContext();
    auto* tile=base.GetTile();
    const auto* map=m_game->GetMap();
    if (!tile || !map || tile->tiles!=map->GetTilesPtr() || tile->base!=&base) {THROW("Starving Base center differs");}
    Base* replacement=nullptr;
    int64_t nearest=INT64_MAX;
    for (const auto id:m_base_order) {
        auto* candidate=GetBase(id);
        if (!candidate) {THROW("Starvation Base record order is invalid");}
        candidate->GetSmacResourceContext();
        if (candidate==&base || candidate->m_owner!=base.m_owner) {continue;}
        const auto* position=candidate->GetTile();
        auto dx=std::abs(int64_t(position->coord.x)-int64_t(tile->coord.x));
        const auto dy=std::abs(int64_t(position->coord.y)-int64_t(tile->coord.y));
        // Native GLSMAC topology wraps X. The staged territory query below
        // authenticates that saved topology before any authority changes.
        dx=std::min(dx,int64_t(map->GetWidth())-dx);
        const auto distance=std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
        if (distance<=nearest) {replacement=candidate;nearest=distance;}
    }
    if (!replacement) {(void)DisposeLastSmacBase(GSE_CALL,base,nullptr,unit::SmacMoraleMode::Mode0);return;}
    const auto regional=m_game->PrepareCanonicalRegionalBaseDelta(base,-1,map->GetTerritoryTopology());
    auto& economy=m_game->m_simulation->value.economy.at(base.GetSmacResourceContext().owner_simulation_id);
    std::optional<map::GlsmacTerritoryTopology> topology;
    auto territory=m_game->PrepareBaseTerritory(nullptr,&base,false,&topology);
    if (!territory) {THROW("Canonical starvation requires saved worker knowledge and territory");}

    auto* um=m_game->GetUM();
    um->RequireConventionalUnitsReady();
    auto unit_updates=um->m_unit_updates;
    std::vector<std::pair<unit::Unit*,unit::ConventionalUnitState>> homes;
    homes.reserve(um->m_units.size());
    for (const auto id:um->m_unit_order) {
        auto* current=um->GetUnit(id);
        if (!current || !current->IsConventional()) {THROW("Starvation requires current conventional Unit records");}
        auto state=current->Conventional();
        if (state.home_base_id!=base.m_id) {continue;}
        // Accepted live homes already require the same owner. Capture-created
        // foreign home links are not admitted by this world's Unit resolver.
        if (current->m_owner!=base.m_owner) {THROW("Foreign captured home repair is not admitted");}
        state.home=unit::ConventionalUnitState::Home::Base;
        state.home_base_id=replacement->m_id;
        (void)m_game->ResolveConventionalUnit(state,current->m_owner);
        homes.emplace_back(current,state);
        auto& update=unit_updates.try_emplace(id,unit::UnitManager::unit_update_t{unit::UnitManager::UUO_NONE,current}).first->second;
        if (update.ops & unit::UnitManager::UUO_DESPAWN) {THROW("Rehomed Unit has a pending removal");}
        update.ops=static_cast<unit::UnitManager::unit_update_op_t>(update.ops|unit::UnitManager::UUO_REFRESH);
    }
    auto bases=m_bases; bases.erase(base.m_id);
    auto order=m_base_order;
    order.erase(std::remove(order.begin(),order.end(),base.m_id),order.end());
    auto assignments=m_work_assignments;
    for (auto it=assignments.begin();it!=assignments.end();) {
        if(it->second.first==base.m_id) {it=assignments.erase(it);} else {++it;}
    }
    auto updates=m_base_updates; QueueBaseUpdateInto(updates,&base,BUO_DESPAWN);
    std::unique_lock lock(m_updated_bases_mutex);
    auto triggers=m_updated_bases; triggers.erase(&base);
    static_assert(std::is_nothrow_copy_assignable_v<unit::ConventionalUnitState>);
    static_assert(std::is_nothrow_swappable_v<decltype(bases)> && std::is_nothrow_swappable_v<decltype(order)>
        && std::is_nothrow_swappable_v<decltype(assignments)> && std::is_nothrow_swappable_v<decltype(updates)>
        && std::is_nothrow_swappable_v<decltype(unit_updates)> && std::is_nothrow_swappable_v<decltype(triggers)>);
    for (const auto& home:homes) {std::get<unit::ConventionalUnitState>(home.first->m_state)=home.second;}
    um->m_unit_updates.swap(unit_updates);
    m_bases.swap(bases); m_base_order.swap(order); m_work_assignments.swap(assignments);
    m_base_updates.swap(updates); m_updated_bases.swap(triggers); tile->base=nullptr;
    m_game->CommitBaseTerritory(territory,topology);
    economy.regional_base_counts=regional;
    std::unique_ptr<Base> retired(&base);
    lock.unlock();
    // Stable IDs are unchanged; only record order compacts. Retire Pop/Base
    // wrappers even if the postcommit notification throws.
    m_game->GetState()->TriggerObject(this,"base_despawn",ARGS_F(&base) {{"base",base.Wrap(GSE_CALL)},};});
}
}
