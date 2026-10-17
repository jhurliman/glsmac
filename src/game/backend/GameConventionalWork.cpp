#include "Game.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
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

bool Game::HandleConventionalWorkEvent(GSE_CALLABLE,const std::string& name,
    const gse::value::object_properties_t& args,size_t caller,bool apply) {
    const bool request=name=="work_unit",dispatch=name=="continue_unit_work",cancel=name=="cancel_unit_work";
    if(!request && !dispatch && !cancel){return false;}
    const auto found=args.find("unit");
    if(found==args.end() || !found->second || args.size()!=(request?2u:1u)) {
        throw std::runtime_error("Former command requires exactly its actual Unit and optional requested action");
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
