#include "BaseManager.h"
#include "Base.h"
#include "FacilityDef.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/SmacClimate.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/Unit.h"
#include "util/FinallyGuard.h"
#include "gc/Space.h"
#include <algorithm>
#include <type_traits>

namespace game::backend::base {
struct BaseManager::PreparedSmacFloods::Data {
    struct Change {
        Base* base;
        size_t id;
        std::string before;
        Base::facilities_t facilities;
        int32_t population,owner;
        bool changed=false,remove=false;
    };
    BaseManager* manager;
    std::vector<Change> changes;
    std::array<std::optional<uint32_t>,8> before_status,status;
    std::optional<faction::SmacBaseTotals> totals;
    bool committed=false,sealed=false,aggregate=false,failed=false;
    size_t knowledge_step=0;
    std::optional<faction::SmacBaseTotals> before_totals;
    decltype(BaseManager::m_bases) bases;
    decltype(BaseManager::m_base_order) order,before_order;
    decltype(BaseManager::m_work_assignments) assignments;
    decltype(BaseManager::m_base_updates) updates;
    std::unordered_set<Base*> triggers;
    std::map<size_t,std::string> before_bases,before_units;
    std::map<size_t,unit::Unit*> original_units;
    std::map<size_t,Base*> original_bases;
    std::map<size_t,std::pair<int,const unit::Unit*>> before_unit_updates;
    std::map<size_t,std::pair<int,const Base*>> before_base_updates;
    std::vector<size_t> before_unit_order;
    std::map<int32_t,faction::SmacEconomyState> before_economy,economy;
    std::array<std::optional<int64_t>,8> before_targets,targets;
    std::optional<map::SmacWorkerKnowledge> knowledge;
    std::vector<std::unique_ptr<unit::UnitManager::PreparedConventionalRemoval>> units;
    std::vector<Base*> removed;
    std::vector<std::unique_ptr<Base>> retired;
};
BaseManager::PreparedSmacFloods::PreparedSmacFloods(std::unique_ptr<Data> data):m_data(std::move(data)) {}
BaseManager::PreparedSmacFloods::~PreparedSmacFloods()=default;
BaseManager::PreparedSmacFloods::PreparedSmacFloods(PreparedSmacFloods&&) noexcept=default;

BaseManager::PreparedSmacFloods BaseManager::PrepareSmacFloods(GSE_CALLABLE) {
    m_game->CheckRW(GSE_CALL);m_game->RequireSmacGameplayActive();
    if(!m_game->IsRunning() || gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating()
        || m_dispatching_updates || !m_game->m_simulation) {THROW("Flood preparation requires its running owner");}
    auto data=std::make_unique<PreparedSmacFloods::Data>();data->manager=this;
    data->before_status=m_game->m_simulation->value.faction_status.values;data->status=data->before_status;
    data->totals=m_game->m_simulation->value.retained_base_totals;
    data->before_totals=data->totals;data->bases=m_bases;data->order=m_base_order;data->before_order=m_base_order;data->assignments=m_work_assignments;
    data->before_economy=m_game->m_simulation->value.economy;data->economy=data->before_economy;
    data->before_targets=m_game->m_simulation->value.battle_plan_target;data->targets=data->before_targets;
    data->knowledge=m_game->GetMap()->GetSmacWorkerKnowledge();
    for(const auto& row:m_bases) {data->before_bases.emplace(row.first,Base::Serialize(row.second).ToString());data->original_bases.emplace(row);}
    for(const auto& row:m_base_updates) {data->before_base_updates.emplace(row.first,std::make_pair(int(row.second.ops),row.second.base));}
    for(const auto& row:m_game->GetUM()->m_unit_updates) {data->before_unit_updates.emplace(row.first,std::make_pair(int(row.second.ops),row.second.unit));}
    data->before_unit_order=m_game->GetUM()->GetUnitRecordOrder();
    for(const auto id:data->before_unit_order) {data->before_units.emplace(id,unit::Unit::Serialize(m_game->GetUM()->GetUnit(id)).ToString());data->original_units.emplace(id,m_game->GetUM()->GetUnit(id));}
    return PreparedSmacFloods(std::move(data));
}

BaseManager::PreparedSmacFloods::Outcome BaseManager::PreparedSmacFloods::Flood(
    GSE_CALLABLE,Base& base,int32_t old_class,int32_t new_class,map::smac_climate::Streams& random) {
    if(!m_data || m_data->committed) {THROW("Flood plan is absent or already published");}
    auto& data=*m_data;auto* manager=data.manager;auto* game=manager->m_game;
    game->CheckRW(GSE_CALL);game->RequireSmacGameplayActive();
    if(gc_space!=game->GetGCSpace() || !gc_space->IsAccumulating() || !game->IsRunning()
        || manager->GetBase(base.m_id)!=&base || !base.HasSmacResourceContext()
        || !base.GetTile() || base.GetTile()->base!=&base || base.GetTile()->tiles!=game->GetMap()->GetTilesPtr()) {
        THROW("Flood requires a registered canonical Base in its owning accumulation");
    }
    if(old_class<0 || old_class>7 || new_class<0 || new_class>7) {THROW("Flood class is outside retained altitude range");}
    if(old_class<3 || new_class>=3) {return Outcome::Unchanged;}
    auto found=std::find_if(data.changes.begin(),data.changes.end(),[&](const auto& row){return row.base==&base;});
    if(found!=data.changes.end() && found->remove) {THROW("Removed flood Base must leave the candidate registry");}
    const auto& facilities=found==data.changes.end() ? base.m_facilities:found->facilities;
    bool dome=false,objective_facility=false;
    const auto& rules=game->GetState()->m_settings.global.GetSmacCampaignRules();
    for(const auto& row:facilities) {
        manager->ValidateSmacFacilityDef(row.second);
        if(!row.second->m_smac_source_id) {THROW("Flood facility lacks native identity");}
        const auto id=*row.second->m_smac_source_id;dome=dome || id==26;
        if(rules && rules->objective_facility<34 && id==rules->objective_facility) {objective_facility=true;}
    }
    if(dome) {return Outcome::Unchanged;}
    if(!base.m_allocation || !base.m_production) {THROW("Flood damage requires complete native allocation and production authority");}
    const auto context=base.GetSmacResourceContext();const auto owner=context.owner_simulation_id;
    const auto& state=game->m_simulation->value;
    if(!rules || !context.retained_status_word || !data.status.at(owner) || !data.totals) {
        THROW("Flood lacks retained campaign, Base status, owner status or Base-count authority");
    }
    // Network permission and project-host objectives have separate original
    // providers. Do not manufacture either from a human bit, HQ or Do Or Die.
    if(game->GetState()->GetConnection()) {THROW("Flood network removal permission is unavailable");}
    if((rules->primary&0x40000000u)
        && state.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {
        THROW("Flood project objectives require project-host authority");
    }
    const bool objective=(rules->primary&0x20000000u) || (*context.retained_status_word&0x8000u)
        || ((rules->secondary&0x04000000u) && objective_facility);
    auto* facility=manager->GetSmacFacilityDef(26);
    if(!facility) {THROW("Flood requires a registered Pressure Dome definition");}
    manager->ValidateSmacFacilityDef(facility);
    const auto population=found==data.changes.end() ? base.GetPopulation():size_t(found->population);
    if(!population || population>127) {THROW("Flood population exceeds native positive signed byte");}
    if(found==data.changes.end()) {
        data.changes.push_back({&base,base.m_id,Base::Serialize(&base).ToString(),base.m_facilities,static_cast<int32_t>(population),owner});
        found=std::prev(data.changes.end());
    }
    // Complete all allocations before the one original damage draw. Caller
    // supplies a detached stream shared with the enclosing climate passes.
    found->facilities.emplace(facility->m_id,facility);
    const auto loss=std::clamp(static_cast<int32_t>(population)/2,2+random.Crt()%3,10);
    found->population=static_cast<int32_t>(population)-loss;
    if(objective || (found->population<=0 && data.totals->at(owner)<=1)) {
        found->population=std::max(found->population,1);
    }
    found->changed=true;found->remove=found->population<=0;*data.status.at(owner)|=0x80000u;
    return found->remove ? Outcome::Remove:Outcome::Survived;
}

void BaseManager::PreparedSmacFloods::CommitSurvivors(GSE_CALLABLE) {
    if(!m_data || m_data->committed || m_data->aggregate) {THROW("Flood survivor publication cannot publish an aggregate entity plan");}
    auto& data=*m_data;auto* manager=data.manager;auto* game=manager->m_game;
    game->CheckRW(GSE_CALL);game->RequireSmacGameplayActive();
    if(gc_space!=game->GetGCSpace() || !gc_space->IsAccumulating() || !game->IsRunning()
        || manager->m_dispatching_updates || !game->m_simulation
        || game->m_simulation->value.faction_status.values!=data.before_status
        || game->m_simulation->value.retained_base_totals!=data.before_totals) {THROW("Flood owner preimage changed");}
    for(const auto& row:data.changes) {
        if(row.remove) {THROW("Flood Base deletion must join the ordinary removal transaction");}
        if(manager->GetBase(row.id)!=row.base || Base::Serialize(row.base).ToString()!=row.before) {
            THROW("Flood Base preimage changed");
        }
    }
    auto updates=manager->m_base_updates;
    for(const auto& row:data.changes) {if(row.changed) {manager->QueueBaseUpdateInto(updates,row.base,BUO_REFRESH);}}
    static_assert(std::is_nothrow_swappable_v<Base::facilities_t> && std::is_nothrow_swappable_v<decltype(updates)>);
    for(auto& row:data.changes) {if(row.changed) {
        row.base->m_facilities.swap(row.facilities);
        row.base->m_retained_population=row.population==static_cast<int32_t>(row.base->m_pops.size())
            ? std::nullopt:std::optional<size_t>(std::max(row.population,0));
    }}
    manager->m_base_updates.swap(updates);game->m_simulation->value.faction_status.values=data.status;data.committed=true;
    // Worker projection, stores, resource cache, summaries and RNG are retained.
    // The enclosing climate owner publishes its detached stream and flood scratch.
}

void BaseManager::PreparedSmacFloods::Query(const std::function<void()>& query) const {
    if(!m_data || m_data->committed || m_data->failed || !query) {throw std::runtime_error("Climate entity query requires an unpublished plan");}
    auto& d=*m_data;auto* bm=d.manager;auto* game=bm->m_game;auto& state=game->m_simulation->value;
    auto bases=d.bases;auto order=d.order;auto assignments=d.assignments;auto totals=d.totals;auto status=d.status;auto economy=d.economy;auto targets=d.targets;
    struct Saved {Base* base;Base::facilities_t facilities;std::optional<size_t> population;};
    std::vector<Saved> saved;for(const auto& row:d.changes) {saved.push_back({row.base,row.base->m_facilities,row.base->m_retained_population});}
    std::vector<std::pair<map::tile::Tile*,Base*>> centers;for(auto* base:d.removed) {centers.emplace_back(base->GetTile(),base->GetTile()->base);}
    auto previous_knowledge=game->GetMap()->GetSmacWorkerKnowledge();
    util::FinallyGuard restore([&]{
        bm->m_bases.swap(bases);bm->m_base_order.swap(order);bm->m_work_assignments.swap(assignments);
        state.retained_base_totals.swap(totals);state.faction_status.values.swap(status);state.economy.swap(economy);state.battle_plan_target.swap(targets);
        for(auto& row:saved) {row.base->m_facilities.swap(row.facilities);row.base->m_retained_population=row.population;}
        for(const auto& row:centers) {row.first->base=row.second;}
        game->CommitConventionalSight(previous_knowledge);
    });
    bm->m_bases.swap(bases);bm->m_base_order.swap(order);bm->m_work_assignments.swap(assignments);
    state.retained_base_totals.swap(totals);state.faction_status.values.swap(status);state.economy.swap(economy);state.battle_plan_target.swap(targets);
    for(auto& row:d.changes) {row.base->m_facilities=row.facilities;row.base->m_retained_population=row.population==int32_t(row.base->m_pops.size()) ? std::nullopt:std::optional<size_t>(std::max(row.population,0));}
    for(const auto& row:centers) {row.first->base=nullptr;}
    const auto nested=[&](auto&& self,size_t index)->void {
        if(index<d.units.size()) {d.units[index]->Query([&]{self(self,index+1);});}
        else {
            if(d.knowledge && d.knowledge_step==d.units.size()) {auto candidate=d.knowledge;game->CommitConventionalSight(candidate);}
            auto knowledge=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
            for(auto* base:d.removed) {
                knowledge.bases.erase(std::remove_if(knowledge.bases.begin(),knowledge.bases.end(),[&](const auto& row){return row.id==base->m_id;}),knowledge.bases.end());
                if(knowledge.occupancy) {knowledge.occupancy->RemoveBase({int32_t(base->GetTile()->coord.x),int32_t(base->GetTile()->coord.y)});}
            }
            for(const auto& row:d.original_units) {
                if(game->GetUM()->GetUnit(row.first) || !row.second->IsConventional()) {continue;}
                const auto& record=row.second->Conventional();auto& counts=state.economy.at(record.owner_simulation_id).design_counts;
                if(counts) {auto& count=counts->at(record.design_id);count=static_cast<uint8_t>(int32_t(count)-1);}
            }
            std::optional<map::SmacWorkerKnowledge> candidate=map::SmacWorkerKnowledge::Create(knowledge);game->CommitConventionalSight(candidate);query();
        }
    };nested(nested,0);
}

void BaseManager::PreparedSmacFloods::Apply(GSE_CALLABLE,map::smac_climate::World& world,int32_t x,int32_t y,
    int32_t old_class,int32_t new_class,map::smac_climate::Streams& random) {
    if(!m_data || m_data->committed || m_data->sealed || m_data->failed) {THROW("Climate entity plan cannot accept another crossing");}
    auto& d=*m_data;auto* bm=d.manager;auto* game=bm->m_game;game->CheckRW(GSE_CALL);
    if(world.width!=int32_t(game->GetMap()->GetWidth()) || world.height!=int32_t(game->GetMap()->GetHeight())) {THROW("Climate entity world geometry differs");}
    if(world.horizontal_wrap) {x=(x%world.width+world.width)%world.width;}
    if(old_class<0 || old_class>7 || new_class<0 || new_class>7 || !world.At(x,y)
        || world.At(x,y)->elevation!=old_class || ((x^y)&1)) {THROW("Climate entity crossing has invalid coordinates classes or retained surface");}
    if((old_class<3)==(new_class<3)) {return;}
    bool completed=false;util::FinallyGuard failed([&]{if(!completed) {d.failed=true;}});
    d.aggregate=true;
    // Each new subplan is prepared inside every preceding candidate view.
    // Objects stay alive until the complete climate owner publishes and notifies.
    Query([&]{
        auto* tile=game->GetMap()->GetTile(x,y);auto* base=tile->base;
        if(base) {
            if(new_class>=3) {return;}
            const auto outcome=Flood(GSE_CALL,*base,old_class,new_class,random);
            if(outcome!=Outcome::Unchanged) {const auto owner=base->GetSmacResourceContext().owner_simulation_id;++world.flood_counts.at(owner);}
            if(outcome!=Outcome::Remove) {return;}
            Base* replacement=nullptr;int64_t nearest=INT64_MAX;
            for(const auto id:bm->m_base_order) {
                auto* candidate=bm->GetBase(id);if(candidate==base || candidate->m_owner!=base->m_owner) {continue;}
                auto dx=std::abs(int64_t(candidate->GetTile()->coord.x)-x);dx=std::min(dx,int64_t(world.width)-dx);
                const auto dy=std::abs(int64_t(candidate->GetTile()->coord.y)-y);
                const auto distance=std::max(dx,dy)-(std::abs(dx-dy)+2)/4;
                if(distance<=nearest) {nearest=distance;replacement=candidate;}
            }
            if(!replacement) {THROW("Climate removed a Base without its retained surviving home authority");}
            auto homes=game->GetUM()->PrepareWorldUnitRemoval(GSE_CALL,{});homes.PrepareBaseRemovalHomes(*base,*replacement);
            d.units.push_back(std::make_unique<unit::UnitManager::PreparedConventionalRemoval>(std::move(homes)));
        }
    });
    // Apply ordinary Base removal against the old surface, then clear the
    // candidate Base marker before the climate-specific domain sweep.
    Query([&]{
        auto* tile=game->GetMap()->GetTile(x,y);auto* base=tile->base;
        const auto changed=std::find_if(d.changes.begin(),d.changes.end(),[&](const auto& row){return row.base==base;});
        if(base && changed!=d.changes.end() && changed->remove) {
            const auto owner=base->GetSmacResourceContext().owner_simulation_id;
            auto cleanup=game->GetUM()->PrepareBaseCenterRemovalForClass(GSE_CALL,*base,old_class);
            d.units.push_back(std::make_unique<unit::UnitManager::PreparedConventionalRemoval>(std::move(cleanup)));
            d.economy.at(owner).regional_base_counts=game->PrepareCanonicalRegionalBaseDelta(*base,-1,game->GetMap()->GetTerritoryTopology());
            d.totals=faction::AdjustSmacBaseTotal(d.totals,owner,-1);
            for(auto& target:d.targets) {if(target && *target==int64_t(base->m_id)) {target=-1;}}
            d.bases.erase(base->m_id);d.order.erase(std::remove(d.order.begin(),d.order.end(),base->m_id),d.order.end());d.removed.push_back(base);
            for(auto it=d.assignments.begin();it!=d.assignments.end();) {if(it->second.first==base->m_id) {it=d.assignments.erase(it);}else {++it;}}
        }
    });
    Query([&]{
        auto* tile=game->GetMap()->GetTile(x,y);if(tile->base) {return;}
        const auto& catalog=game->GetState()->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
        std::vector<size_t> casualties;const auto& order=game->GetUM()->GetUnitRecordOrder();
        for(auto it=order.rbegin();it!=order.rend();++it) {
            const auto* unit=game->GetUM()->GetUnit(*it);if(unit->GetTile()!=tile) {continue;}
            uint32_t chassis,flags;
            if(unit->IsNative()) {
                const auto design=std::find_if(catalog.units.begin(),catalog.units.end(),[&](const auto& row){return row.source_id==unit->Native().design;});
                if(design==catalog.units.end()) {THROW("Climate Unit lacks its native chassis");}chassis=design->chassis_id;flags=unit->Native().flags;
            } else {chassis=game->ResolveConventionalDesign(unit->Conventional(),*game->m_simulation).chassis;if(!unit->Conventional().native_flags) {THROW("Climate Unit lacks retained exemption flags");}flags=*unit->Conventional().native_flags;}
            const auto domain=static_cast<int32_t>(catalog.chassis.chassis.at(chassis).triad);
            if(domain!=2 && !(flags&0x20u) && ((domain==1)!=(new_class<3))) {casualties.push_back(*it);}
        }
        auto cleanup=game->GetUM()->PrepareWorldUnitRemoval(GSE_CALL,casualties);
        d.units.push_back(std::make_unique<unit::UnitManager::PreparedConventionalRemoval>(std::move(cleanup)));
    });
    // Retain the latest Unit knowledge, then remove only observations/presence
    // belonging to deleted Bases. Cached occupying affiliation is not rebuilt.
    auto saved=d.knowledge;
    try {Query([&]{
        auto knowledge=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
        for(auto* base:d.removed) {
            knowledge.bases.erase(std::remove_if(knowledge.bases.begin(),knowledge.bases.end(),[&](const auto& row){return row.id==base->m_id;}),knowledge.bases.end());
            if(knowledge.occupancy) {knowledge.occupancy->RemoveBase({int32_t(base->GetTile()->coord.x),int32_t(base->GetTile()->coord.y)});}
        }
        d.knowledge=map::SmacWorkerKnowledge::Create(knowledge);d.knowledge_step=d.units.size();
        auto* cell=world.At(x,y);cell->items=(cell->items&~3u)|(game->GetMap()->GetTile(x,y)->base?1u:0u)|(game->GetMap()->GetTile(x,y)->units.empty()?0u:2u);
    });} catch(...) {d.knowledge=std::move(saved);throw;}
    completed=true;
}

void BaseManager::PreparedSmacFloods::Seal(GSE_CALLABLE) {
    if(!m_data || m_data->committed || m_data->sealed || m_data->failed) {THROW("Climate entity plan is absent or already sealed");}
    auto& d=*m_data;auto* bm=d.manager;bm->m_game->CheckRW(GSE_CALL);
    d.updates=bm->m_base_updates;
    for(const auto& row:d.changes) {if(row.changed) {bm->QueueBaseUpdateInto(d.updates,row.base,row.remove ? BUO_DESPAWN:BUO_REFRESH);}}
    {std::unique_lock lock(bm->m_updated_bases_mutex);d.triggers=bm->m_updated_bases;}
    for(auto* base:d.removed) {d.triggers.erase(base);}
    d.retired.resize(d.removed.size());d.sealed=true;Validate(GSE_CALL);
}
void BaseManager::PreparedSmacFloods::Validate(GSE_CALLABLE) const {
    if(!m_data || m_data->committed || !m_data->sealed || m_data->failed) {THROW("Climate entities require a sealed unpublished plan");}
    const auto& d=*m_data;auto* bm=d.manager;auto* game=bm->m_game;game->CheckRW(GSE_CALL);
    if(!game->IsRunning() || gc_space!=game->GetGCSpace() || !gc_space->IsAccumulating() || bm->m_dispatching_updates
        || bm->m_base_order!=d.before_order || game->GetUM()->GetUnitRecordOrder()!=d.before_unit_order
        || game->m_simulation->value.retained_base_totals!=d.before_totals
        || game->m_simulation->value.faction_status.values!=d.before_status
        || game->m_simulation->value.economy!=d.before_economy || game->m_simulation->value.battle_plan_target!=d.before_targets) {
        THROW("Climate entity owner preimage changed");
    }
    if(bm->m_base_updates.size()!=d.before_base_updates.size() || game->GetUM()->m_unit_updates.size()!=d.before_unit_updates.size()) {THROW("Climate pending entity queues changed");}
    for(const auto& row:bm->m_base_updates) {const auto previous=d.before_base_updates.find(row.first);if(previous==d.before_base_updates.end() || previous->second!=std::make_pair(int(row.second.ops),row.second.base)) {THROW("Climate Base queue changed");}}
    for(const auto& row:game->GetUM()->m_unit_updates) {const auto previous=d.before_unit_updates.find(row.first);if(previous==d.before_unit_updates.end() || previous->second!=std::make_pair(int(row.second.ops),static_cast<const unit::Unit*>(row.second.unit))) {THROW("Climate Unit queue changed");}}
    for(const auto& row:d.before_bases) {auto* base=bm->GetBase(row.first);if(!base || base!=d.original_bases.at(row.first) || Base::Serialize(base).ToString()!=row.second) {THROW("Climate Base preimage changed");}}
    for(const auto& row:d.before_units) {auto* unit=game->GetUM()->GetUnit(row.first);if(!unit || unit!=d.original_units.at(row.first) || unit::Unit::Serialize(unit).ToString()!=row.second) {THROW("Climate Unit preimage changed");}}
}
void BaseManager::PreparedSmacFloods::Commit() noexcept {
    if(!m_data || !m_data->sealed || m_data->committed) {return;}
    auto& d=*m_data;auto* bm=d.manager;auto& state=bm->m_game->m_simulation->value;
    state.economy.swap(d.economy); // removal commits apply each actual design debit to this candidate economy
    for(auto& units:d.units) {units->Commit();}
    for(auto& row:d.changes) {if(row.changed) {
        row.base->m_facilities.swap(row.facilities);
        row.base->m_retained_population=row.population==int32_t(row.base->m_pops.size()) ? std::nullopt:std::optional<size_t>(std::max(row.population,0));
    }}
    bm->m_bases.swap(d.bases);bm->m_base_order.swap(d.order);bm->m_work_assignments.swap(d.assignments);bm->m_base_updates.swap(d.updates);
    {std::unique_lock lock(bm->m_updated_bases_mutex);bm->m_updated_bases.swap(d.triggers);}
    state.retained_base_totals=d.totals;state.faction_status.values=d.status;state.battle_plan_target=d.targets;
    for(size_t index=0;index<d.removed.size();++index) {d.removed[index]->GetTile()->base=nullptr;d.retired[index].reset(d.removed[index]);}
    bm->m_game->CommitConventionalSight(d.knowledge);d.committed=true;
}
void BaseManager::PreparedSmacFloods::Notify(GSE_CALLABLE) {
    if(!m_data || !m_data->committed) {THROW("Climate entity notifications require publication");}
    auto& d=*m_data;auto retired=std::move(d.retired);
    for(auto& units:d.units) {units->Notify(GSE_CALL);}
    for(const auto& owned:retired) {auto* base=owned.get();d.manager->m_game->GetState()->TriggerObject(d.manager,"base_despawn",ARGS_F(base) {{"base",base->Wrap(GSE_CALL)},};});}
}

}
