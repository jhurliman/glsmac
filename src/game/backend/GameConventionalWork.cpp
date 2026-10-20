#include "Game.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
#include "unit/SmacSelectionDistance.h"
#include "slot/Slot.h"
#include "map/Map.h"
#include "map/tile/Tile.h"
#include "gse/value/Int.h"
#include <algorithm>
#include <limits>
#include <set>

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

std::optional<size_t> Game::SelectLocalConventionalUnit(size_t turn_id,std::optional<size_t> current,
    std::optional<size_t> reference,types::Vec2<size_t> focus,bool retain_current) const {
    if(!IsRunning() || !m_um || !m_map || !m_state || !m_slot || turn_id!=GetTurnId()
        || m_canonical_turn_in_progress || m_canonical_turn_failed || m_canonical_continuation) {return {};}
    if(focus.x>=m_map->GetWidth() || focus.y>=m_map->GetHeight()
        || focus.x>INT32_MAX || focus.y>INT32_MAX) {throw std::runtime_error("Unit selection requires actual map focus and geometry");}
    return SelectLocalConventionalUnitFromCursor(turn_id,current,reference,
        {{int32_t(focus.x),int32_t(focus.y)}},retain_current);
}

std::optional<size_t> Game::SelectLocalConventionalUnitFromCursor(size_t turn_id,std::optional<size_t> current,
    std::optional<size_t> reference,ui_view::Cursor focus,bool retain_current) const {
    if(!IsRunning() || !m_um || !m_map || !m_state || !m_slot || turn_id!=GetTurnId()
        || m_canonical_turn_in_progress || m_canonical_turn_failed || m_canonical_continuation) {return {};}
    RequireSmacLocalHumanOwner(m_slot);
    RequireConventionalSightWorld();
    const auto width=m_map->GetWidth(),height=m_map->GetHeight();
    if(!m_map->GetSmacWorkerKnowledge() || width>INT32_MAX || height>INT32_MAX) {throw std::runtime_error("Unit selection requires actual map focus and geometry");}
    const auto knowledge=m_map->GetSmacWorkerKnowledge()->CopyData();
    const auto valid=[&](const unit::Unit* actor){
        const auto* tile=actor?actor->GetTile():nullptr;
        return tile && tile->coord.x<width && tile->coord.y<height && !((tile->coord.x^tile->coord.y)&1)
            && m_map->GetTile(tile->coord.x,tile->coord.y)==tile;
    };
    auto origin=focus;
    if(reference) {
        const auto* actor=m_um->GetUnit(*reference);
        if(actor && actor->m_owner==m_slot && valid(actor)) {
            const auto& tile=actor->GetTile()->coord;origin={{int32_t(tile.x),int32_t(tile.y)}};
        }
    }
    std::optional<size_t> selected; int32_t best=9999;
    std::set<size_t> seen;
    for(const auto id:m_um->GetUnitRecordOrder()) {
        const auto* actor=m_um->GetUnit(id);
        if(!actor || !seen.insert(id).second) {throw std::runtime_error("Unit selection record order differs from registry");}
        if(actor->m_owner!=m_slot || !valid(actor)) {continue;}
        if(!actor->IsConventional()) {throw std::runtime_error("Native local selection requires ordinary conventional units");}
        actor->RequireConventionalReady();
        if(actor->Conventional().IsHeld()) {continue;}
        // A persisted selected work order holds controller identity between
        // turns even after dispatch exhausts it. This is separate from the
        // nearest-unit helper, whose eligibility still requires allowance.
        if(retain_current && current==id && actor->Conventional().work_order!=unit::ConventionalUnitState::WorkOrder::None) {return id;}
        if(!actor->HasMovesLeft()) {continue;}
        // The enclosing human controller keeps an eligible selection without
        // invoking the nearest-candidate scan. The scan itself only breaks ties.
        if(retain_current && current==id) {return id;}
        const auto position=actor->GetTile()->coord;
        const auto distance=unit::selection_distance::Metric(origin,
            {{int32_t(position.x),int32_t(position.y)}},int32_t(width),knowledge.horizontal_wrap);
        if(distance<best || (distance==best && current==id)) {selected=id;best=distance;}
    }
    if(seen.size()!=m_um->GetUnits().size()) {throw std::runtime_error("Unit selection omitted a registered record");}
    return selected;
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
    using Command=unit::UnitManager::HumanOrderCommand;
    if(name=="unit_hold" || name=="unit_hold_ten" || name=="unit_menu_hold" || name=="unit_activate" || name=="unit_skip_turn") {
        const auto found=args.find("unit");
        if(found==args.end() || !found->second) {throw std::runtime_error("Human order requires its actual Unit");}
        const auto stamp=args.find("turn_id");
        if(name=="unit_skip_turn" && stamp==args.end()) {
            auto* legacy=unit::Unit::Unwrap(found->second);
            if(legacy && !legacy->IsConventional()) {return false;}
        }
        if(args.size()!=2 || stamp==args.end() || !stamp->second || stamp->second->type!=gse::VT_INT
            || static_cast<const gse::value::Int*>(stamp->second)->value<0) {throw std::runtime_error("Human order requires its captured turn identity");}
        // Discard an old queued input before touching a possibly retired wrapper.
        if(static_cast<uint64_t>(static_cast<const gse::value::Int*>(stamp->second)->value)!=GetTurnId()) {return true;}
        auto* actor=unit::Unit::Unwrap(found->second);
        const auto command=name=="unit_hold" ? Command::Hold : name=="unit_hold_ten" ? Command::HoldTen
            : name=="unit_menu_hold" ? Command::MenuHold : name=="unit_activate" ? Command::Activate : Command::Skip;
        if(apply) {m_um->ApplyHumanOrder(GSE_CALL,actor,command,caller);}
        else {(void)m_um->PrepareHumanOrder(actor,command,caller);}
        return true;
    }
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
