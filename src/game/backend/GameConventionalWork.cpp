#include "Game.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
#include "slot/Slot.h"
#include "map/Map.h"
#include "map/tile/Tile.h"
#include "gse/value/Int.h"

namespace game::backend {

std::optional<map::SmacWorkerKnowledge> Game::PrepareConventionalWorkMemory(const unit::Unit& actor,uint16_t terraforming) const {
    RequireConventionalSightWorld();
    if(!m_map || !m_map->m_worker_knowledge || !m_map->m_worker_knowledge_ready) {
        throw std::runtime_error("Terraforming completion requires owned map memory");
    }
    auto data=m_map->m_worker_knowledge->CopyData();
    const auto owner=actor.Conventional().owner_simulation_id;
    auto& memory=data.remembered.at(owner);
    const auto* tile=actor.GetTile();
    const auto index=size_t(tile->coord.y)*(data.width/2)+tile->coord.x/2;
    if(memory.size()!=data.tiles.size() || !(data.tiles.at(index).explored_by & (1u<<owner))) {
        throw std::runtime_error("Terraforming completion requires the acting owner's actual explored/memory inputs");
    }
    memory.at(index)=map::SmacWorkerKnowledge::Improvements{tile->features,terraforming,tile->base!=nullptr,!tile->units.empty()};
    return map::SmacWorkerKnowledge::Create(data);
}

bool Game::QueueSelectedConventionalWork(GSE_CALLABLE,size_t unit_id,size_t turn_id) {
    if(!IsRunning() || !m_um || !m_state || !m_slot || turn_id!=GetTurnId()
        || m_canonical_turn_in_progress || m_canonical_turn_failed || m_canonical_continuation) {return false;}
    auto* actor=m_um->GetUnit(unit_id);
    if(!actor || !actor->IsConventional() || actor->IsConventionalPending() || actor->m_owner!=m_slot
        || actor->Conventional().work_order==unit::ConventionalUnitState::WorkOrder::None
        || !actor->HasMovesLeft()) {return false;}
    Event(GSE_CALL,"continue_unit_work",{{"unit",actor->Wrap(GSE_CALL)},
        {"turn_id",VALUE(gse::value::Int,,turn_id)}});
    return true;
}

bool Game::HandleConventionalWorkEvent(GSE_CALLABLE,const std::string& name,
    const gse::value::object_properties_t& args,size_t caller,bool apply) {
    const bool request=name=="work_unit",dispatch=name=="continue_unit_work",cancel=name=="cancel_unit_work";
    if(!request && !dispatch && !cancel){return false;}
    const auto found=args.find("unit");
    if(found==args.end() || !found->second || args.size()!=((request||dispatch)?2u:1u)) {
        throw std::runtime_error("Former command requires its actual Unit and exact action or continuation stamp");
    }
    if(dispatch) {
        const auto stamp=args.find("turn_id");
        if(stamp==args.end() || !stamp->second || stamp->second->type!=gse::VT_INT
            || static_cast<const gse::value::Int*>(stamp->second)->value<0) {
            throw std::runtime_error("Former continuation requires its captured turn identity");
        }
        // Check before touching a potentially retired wrapper. An old queued
        // controller visit cannot consume newly reset movement on a later turn.
        if(static_cast<uint64_t>(static_cast<const gse::value::Int*>(stamp->second)->value)!=GetTurnId()) {return true;}
    }
    auto* actor=unit::Unit::Unwrap(found->second);
    using Order=unit::ConventionalUnitState::WorkOrder;
    auto order=Order::None;
    if(request) {
        const auto action=args.find("action");
        if(action==args.end() || !action->second || action->second->type!=gse::VT_INT) {
            throw std::runtime_error("Former action requires an explicit integer identity");
        }
        const auto value=static_cast<const gse::value::Int*>(action->second)->value;
        if(value<static_cast<int32_t>(Order::Farm) || value>static_cast<int32_t>(Order::Mine)) {
            throw std::runtime_error("Former action is outside the admitted set");
        }
        order=static_cast<Order>(value);
    }
    if(apply){m_um->SetConventionalWork(GSE_CALL,actor,order,caller,dispatch);}
    else{m_um->ValidateConventionalWork(actor,order,caller,dispatch);}
    return true;
}
}
