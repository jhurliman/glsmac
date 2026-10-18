#include "UnitManager.h"
#include "Unit.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/map/Map.h"
#include "game/backend/slot/Slot.h"
#include "gc/Space.h"
#include "util/FinallyGuard.h"
#include <set>
#include <type_traits>

namespace game::backend::unit {
struct UnitManager::PreparedDefectionUnits::Data {
    struct Change {Unit* unit;slot::Slot* before_owner;slot::Slot* owner;ConventionalUnitState before,state;};
    UnitManager* manager;
    std::vector<Change> changes;
    std::unordered_map<size_t,unit_update_t> updates;
    bool committed=false;
};
UnitManager::PreparedDefectionUnits::PreparedDefectionUnits(std::unique_ptr<Data> data):m_data(std::move(data)) {}
UnitManager::PreparedDefectionUnits::~PreparedDefectionUnits()=default;
UnitManager::PreparedDefectionUnits::PreparedDefectionUnits(PreparedDefectionUnits&&) noexcept=default;
UnitManager::PreparedDefectionUnits& UnitManager::PreparedDefectionUnits::operator=(PreparedDefectionUnits&&) noexcept=default;
UnitManager::PreparedDefectionUnits UnitManager::PrepareDefectionUnits(GSE_CALLABLE,slot::Slot* recipient,
    const std::vector<std::pair<size_t,ConventionalUnitState>>& changes) {
    m_game->CheckRW(GSE_CALL);m_game->RequireSmacGameplayActive();m_game->RequireSmacLocalHumanOwner(recipient);
    if(gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating() || !m_game->IsRunning()) {THROW("Unit defection requires its owning running accumulation");}
    const auto owner=m_game->RequireSmacProductionOwner(recipient).simulation_id;
    auto data=std::make_unique<PreparedDefectionUnits::Data>();data->manager=this;data->updates=m_unit_updates;
    data->changes.reserve(changes.size());std::set<size_t> seen;
    for(const auto& change:changes) {
        auto* unit=GetUnit(change.first);
        if(!unit || !unit->IsConventional() || unit->m_id!=change.first || !seen.insert(change.first).second || unit->m_animation_id) {THROW("Defection requires distinct registered idle conventional Units");}
        m_game->RequireSmacLocalHumanOwner(unit->m_owner);
        const auto& before=unit->Conventional();const auto& next=change.second;
        if(before.owner_simulation_id<1 || before.owner_simulation_id>7 || before.owner_simulation_id==owner || next.owner_simulation_id!=owner) {THROW("Defection requires a positive distinct recipient");}
        (void)ResolveConventional(before,unit->m_owner);(void)ResolveConventional(next,recipient);
        const auto* tile=unit->GetTile();
        if(!tile || tile->tiles!=m_game->GetMap()->GetTilesPtr() || !tile->units.count(unit->m_id) || tile->units.at(unit->m_id)!=unit) {THROW("Defection Unit has inconsistent map membership");}
        const auto& economy=m_game->m_simulation->value.economy;
        for(const auto id:{before.owner_simulation_id,owner}) {
            const auto found=economy.find(id);
            if(found==economy.end() || !found->second.design_counts) {THROW("Defection requires owned active design-count history");}
        }
        auto& update=data->updates.try_emplace(unit->m_id,unit_update_t{UUO_NONE,unit}).first->second;
        if(update.unit!=unit || (update.ops & UUO_DESPAWN)) {THROW("Defection Unit has a stale pending publication");}
        update.ops=static_cast<unit_update_op_t>(update.ops|UUO_OWNER_CHANGE);
        data->changes.push_back({unit,unit->m_owner,recipient,before,next});
    }
    return PreparedDefectionUnits(std::move(data));
}
void UnitManager::PreparedDefectionUnits::Query(const std::function<void()>& query) const {
    if(!m_data || m_data->committed || !query) {THROW("Defection query requires an unpublished plan");}
    const auto& data=*m_data;
    bool active=false;
    const auto apply=[&](bool forward) {
        for(const auto& change:data.changes) {
            data.manager->AdjustDesignCount(*change.unit,-1);
            change.unit->m_owner=forward ? change.owner:change.before_owner;
            change.unit->CommitConventionalMove(forward ? change.state:change.before,change.unit->GetTile());
            data.manager->AdjustDesignCount(*change.unit,1);
        }
    };
    util::FinallyGuard restore([&]{if(active) {apply(false);}});
    active=true;apply(true);query();
}
void UnitManager::PreparedDefectionUnits::Commit() noexcept {
    if(!m_data || m_data->committed) {return;}
    auto& data=*m_data;
    static_assert(std::is_nothrow_swappable_v<decltype(data.updates)>);
    for(const auto& change:data.changes) {
        data.manager->AdjustDesignCount(*change.unit,-1);
        change.unit->m_owner=change.owner;
        change.unit->CommitConventionalMove(change.state,change.unit->GetTile());
        data.manager->AdjustDesignCount(*change.unit,1);
    }
    data.manager->m_unit_updates.swap(data.updates);data.committed=true;
}
}
