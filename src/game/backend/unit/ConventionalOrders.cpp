#include "UnitManager.h"
#include "Unit.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include <stdexcept>

namespace game::backend::unit {
ConventionalUnitState UnitManager::PrepareHumanOrder(Unit* actor,HumanOrderCommand command,size_t caller) const {
    m_game->RequireSmacGameplayActive();
    if(!actor || GetUnit(actor->m_id)!=actor || !actor->IsConventional()) {throw std::runtime_error("Human order requires a registered conventional unit");}
    actor->RequireConventionalReady();
    m_game->RequireConventionalSightWorld();m_game->RequireSmacLocalHumanOwner(actor->m_owner);
    if(!m_game->IsRunning() || m_game->m_canonical_turn_failed || m_game->m_canonical_turn_in_progress || m_game->m_canonical_continuation
        || caller!=m_game->GetSlotNum() || actor->m_owner->GetIndex()!=caller || actor->m_owner->GetPlayer()!=m_game->GetPlayer()) {
        throw std::runtime_error("Human order requires actual local owner and stable turn");
    }
    auto* tile=actor->GetTile();auto* map=GetMap();
    if(!tile || tile->coord.x>=map->GetWidth() || tile->coord.y>=map->GetHeight() || ((tile->coord.x^tile->coord.y)&1)
        || map->GetTile(tile->coord.x,tile->coord.y)!=tile || tile->IsLocked() || actor->m_animation_id
        || !tile->units.count(actor->m_id) || tile->units.at(actor->m_id)!=actor) {throw std::runtime_error("Human order requires current unlocked tile membership");}
    const auto stats=actor->QueryStats(SmacMoraleMode::Mode1);
    auto next=actor->Conventional();
    if(command==HumanOrderCommand::Skip) {
        if(next.IsHeld() || stats.remaining_allowance<=0 || stats.full_allowance>255) {throw std::runtime_error("Skip requires an actionable unit");}
        next.spent=static_cast<uint8_t>(stats.full_allowance);return next;
    }
    if(command==HumanOrderCommand::Activate) {
        if(!next.IsHeld()) {throw std::runtime_error("Activate requires a held unit; Former cancellation is a separate command");}
        next.order=ConventionalUnitState::Order::None;return next;
    }
    if(command!=HumanOrderCommand::Hold && command!=HumanOrderCommand::HoldTen && command!=HumanOrderCommand::MenuHold) {throw std::runtime_error("Unknown human order command");}
    // Only ordinary supported ground designs are admitted by ResolveConventional.
    // Stranded ground-on-water is refused; water-Base/air/transport need their own receipts.
    if(tile->is_water_tile) {throw std::runtime_error("Human Hold requires supported ground surface");}
    next.order=ConventionalUnitState::Order::Hold;next.work_order=ConventionalUnitState::WorkOrder::None;
    if(command!=HumanOrderCommand::MenuHold) {next.order_countdown=command==HumanOrderCommand::HoldTen?10:0;}
    next.Validate();return next;
}
void UnitManager::ApplyHumanOrder(GSE_CALLABLE,Unit* actor,HumanOrderCommand command,size_t caller) {
    m_game->CheckRW(GSE_CALL);
    const auto next=PrepareHumanOrder(actor,command,caller);
    QueueUnitUpdate(actor,UUO_REFRESH); // allocate before committing record bytes
    actor->CommitConventionalMove(next,actor->GetTile());
}
}
