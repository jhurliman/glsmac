#include "BaseManager.h"
#include "Base.h"
#include "FacilityDef.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/SmacClimate.h"
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
    bool committed=false;
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
    if(!m_data || m_data->committed) {THROW("Flood plan is absent or already published");}
    auto& data=*m_data;auto* manager=data.manager;auto* game=manager->m_game;
    game->CheckRW(GSE_CALL);game->RequireSmacGameplayActive();
    if(gc_space!=game->GetGCSpace() || !gc_space->IsAccumulating() || !game->IsRunning()
        || manager->m_dispatching_updates || !game->m_simulation
        || game->m_simulation->value.faction_status.values!=data.before_status
        || game->m_simulation->value.retained_base_totals!=data.totals) {THROW("Flood owner preimage changed");}
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
            ? std::nullopt:std::optional<size_t>(row.population);
    }}
    manager->m_base_updates.swap(updates);game->m_simulation->value.faction_status.values=data.status;data.committed=true;
    // Worker projection, stores, resource cache, summaries and RNG are retained.
    // The enclosing climate owner publishes its detached stream and flood scratch.
}
}
