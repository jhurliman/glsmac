#include "BaseManager.h"
#include "Base.h"
#include "SmacPopulationStep.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/SmacRestartUnitPackage.h"
#include "gc/Space.h"
#include <algorithm>
#include <memory>
#include <type_traits>

namespace game::backend::base {

struct BaseManager::SmacDispositionPlan {
    size_t participant, technology;
    faction::SmacEconomyState economy;
    faction::SmacHostileState hostility;
    map::tile::Tile* site=nullptr;
    std::vector<std::pair<unit::Def*,unit::ConventionalUnitState>> units;
};

BaseManager::SmacDispositionPlan BaseManager::PrepareLastSmacBaseDisposition(const Base& base) const {
    if(!m_game->IsRunning() || m_dispatching_updates || GetBase(base.m_id)!=&base || base.m_pops.size()!=1) {
        THROW("Faction disposition requires the current size-one Base");
    }
    m_game->RequireSmacLocalHumanOwner(base.m_owner);
    const auto owner=base.GetSmacResourceContext().owner_simulation_id;
    for(const auto& row:m_bases) {if(row.second!=&base && row.second->m_owner==base.m_owner) {THROW("Faction disposition requires the last owned Base");}}
    auto& state=m_game->m_simulation->value;
    const auto participant=std::find_if(state.roster.begin(),state.roster.end(),[&](const auto& row){return row.simulation_id==owner;});
    const auto technology=std::find_if(state.technology.factions.begin(),state.technology.factions.end(),[&](const auto& row){return row.admission.simulation_id==owner;});
    if(participant==state.roster.end() || !participant->active || technology==state.technology.factions.end()) {THROW("Faction disposition has no active saved owner");}
    SmacDispositionPlan plan{static_cast<size_t>(participant-state.roster.begin()),static_cast<size_t>(technology-state.technology.factions.begin()),
        m_game->PrepareSmacNonzeroInitialization(owner),state.hostility,nullptr,{}};
    plan.hostility.Reinitialize(owner);
    const auto* map=m_game->GetMap();auto* center=base.GetTile();
    if(!center || center->tiles!=map->GetTilesPtr() || center->base!=&base || !map->GetSmacWorkerKnowledge()) {THROW("Faction disposition has no canonical center or knowledge");}
    if(state.do_or_die || participant->restart_prohibited || m_game->GetTurnId()>100) {return plan;}
    const auto& global=m_game->GetState()->m_settings.global;
    const auto package=unit::SmacRestartUnitPackage::Select(static_cast<int32_t>(m_game->GetTurnId()),state.designs,
        global.GetSmacPredefinedUnitRules()->catalog,*technology,*global.GetSmacTechnologyRules());
    // Explicit GLSMAC placement policy: maximize distance from surviving Bases
    // and the lost center, with row-major ties. This is not the original search
    // or a fixture coordinate. It consumes neither gameplay nor map RNG.
    int64_t best=-1;
    for(size_t y=0;y<map->GetHeight();++y) {for(size_t x=y&1;x<map->GetWidth();x+=2) {
        auto* tile=map->GetTile(x,y);
        if(tile->base || tile->is_water_tile || tile->IsLocked() || tile->rockiness==map::tile::ROCKINESS_ROCKY
            || (tile->features & (map::tile::FEATURE_XENOFUNGUS|map::tile::FEATURE_MONOLITH))) {continue;}
        if(std::any_of(tile->units.begin(),tile->units.end(),[&](const auto& row){return row.second->m_owner!=base.m_owner;})) {continue;}
        if(std::any_of(tile->neighbours.begin(),tile->neighbours.end(),[&](const auto* adjacent){return adjacent->base && adjacent->base!=&base;})) {continue;}
        int64_t nearest=INT64_MAX;
        for(const auto& row:m_bases) {
            const auto* other=row.second->GetTile();
            auto dx=std::abs(int64_t(x)-int64_t(other->coord.x));dx=std::min(dx,int64_t(map->GetWidth())-dx);
            const auto dy=std::abs(int64_t(y)-int64_t(other->coord.y));nearest=std::min(nearest,std::max(dx,dy));
        }
        if(nearest>best) {best=nearest;plan.site=tile;}
    }}
    if(!plan.site) {return plan;}
    for(const auto id:package.creation_order) {
        plan.units.push_back({m_game->ResolveSmacProductionPresentation(base.m_owner,id),
            {id,owner,1,0,0,false,unit::ConventionalUnitState::Home::None,unit::SmacMoraleMode::Mode0}});
    }
    plan.economy.rates={0,5};return plan;
}

BaseManager::SmacAbandonmentResult BaseManager::DisposeLastSmacBase(GSE_CALLABLE,Base& base,
    unit::Def* transient_definition,unit::SmacMoraleMode mode) {
    auto plan=PrepareLastSmacBaseDisposition(base);
    auto territory=m_game->PrepareBaseTerritory(nullptr,&base,false);
    if(!territory) {THROW("Faction disposition requires staged territory");}
    auto* um=m_game->GetUM();
    std::optional<unit::UnitManager::PreparedConventionalUnit> transient;
    SmacAbandonmentResult result{};
    if(transient_definition) {
        transient.emplace(um->PrepareManufacturedConventionalUnit(GSE_CALL,base,*base.GetProductionState()->current_item.smac_source_id,transient_definition,mode));
        result.newborn_id=transient->Candidate().m_id;
    }
    auto units=um->PrepareFactionUnits(GSE_CALL,base.m_owner,plan.site,plan.units,*territory,transient ? &*transient : nullptr);
    auto bases=m_bases;bases.erase(base.m_id);
    auto order=m_base_order;order.erase(std::remove(order.begin(),order.end(),base.m_id),order.end());
    auto assignments=m_work_assignments;
    for(auto it=assignments.begin();it!=assignments.end();) {if(it->second.first==base.m_id) {it=assignments.erase(it);} else {++it;}}
    auto updates=m_base_updates;QueueBaseUpdateInto(updates,&base,BUO_DESPAWN);
    std::unique_lock lock(m_updated_bases_mutex);auto triggers=m_updated_bases;triggers.erase(&base);
    auto& state=m_game->m_simulation->value;
    const auto lost_owner=base.GetSmacResourceContext().owner_simulation_id;
    auto& economy=state.economy.at(lost_owner);
    static_assert(std::is_nothrow_swappable_v<decltype(bases)> && std::is_nothrow_swappable_v<decltype(order)>
        && std::is_nothrow_swappable_v<decltype(assignments)> && std::is_nothrow_swappable_v<decltype(updates)>
        && std::is_nothrow_swappable_v<decltype(triggers)> && std::is_nothrow_copy_assignable_v<faction::SmacEconomyState>);
    m_bases.swap(bases);m_base_order.swap(order);m_work_assignments.swap(assignments);
    m_base_updates.swap(updates);m_updated_bases.swap(triggers);base.GetTile()->base=nullptr;
    economy=plan.economy;state.technology.factions[plan.technology].cached_threshold.reset();
    state.hostility.owners.swap(plan.hostility.owners);
    state.roster[plan.participant].active=plan.site!=nullptr;
    if(!plan.site) {state.human_defeat_request=lost_owner;}
    units.Commit();std::unique_ptr<Base> retired(&base);lock.unlock();
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
    auto territory=m_game->PrepareBaseTerritory(nullptr,&base,false);
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
    auto territory=m_game->PrepareBaseTerritory(nullptr,&base,false);
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
    m_game->CommitBaseTerritory(territory);
    std::unique_ptr<Base> retired(&base);
    lock.unlock();
    // Stable IDs are unchanged; only record order compacts. Retire Pop/Base
    // wrappers even if the postcommit notification throws.
    m_game->GetState()->TriggerObject(this,"base_despawn",ARGS_F(&base) {{"base",base.Wrap(GSE_CALL)},};});
}
}
