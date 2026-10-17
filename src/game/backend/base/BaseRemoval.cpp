#include "BaseManager.h"
#include "Base.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "gc/Space.h"
#include <algorithm>
#include <memory>
#include <type_traits>

namespace game::backend::base {

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
    if (!replacement) {THROW("Last-owned-Base starvation requires the restart/disposition path");}
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
