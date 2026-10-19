#include "UnitManager.h"
#include "Unit.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "game/FrontendRequest.h"
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace game::backend::unit {
namespace {
using Order = ConventionalUnitState::WorkOrder;
using namespace map::tile;
int32_t SourceId(Order order) {
    switch(order) {
        case Order::Farm:return 0; case Order::Mine:return 2;
        case Order::Solar:return 3; case Order::Forest:return 4;
        case Order::Road:return 5;
        default:throw std::runtime_error("No terraforming source for this work order");
    }
}
uint16_t Improvement(Order order) {
    switch(order) {
        case Order::Farm:return TERRAFORMING_FARM; case Order::Mine:return TERRAFORMING_MINE;
        case Order::Solar:return TERRAFORMING_SOLAR; case Order::Forest:return TERRAFORMING_FOREST;
        case Order::Road:return TERRAFORMING_ROAD;
        default:throw std::runtime_error("No terraforming improvement for this work order");
    }
}
bool Available(const settings::SmacTechnologyReference& reference,const settings::SmacTechnologyRules& rules,
    const faction::SmacFactionTechnologyState& owned) {
    using Kind=settings::SmacTechnologyReference::Kind;
    if(reference.kind==Kind::NONE){return true;}
    if(reference.kind==Kind::DISABLED){return false;}
    if(reference.kind!=Kind::TECHNOLOGY_ID || !reference.technology_id){throw std::runtime_error("Unresolved terraforming prerequisite");}
    const auto id=*reference.technology_id;
    if(id<0 || id>=89){throw std::runtime_error("Invalid terraforming prerequisite");}
    if(id==88 || !owned.owned[id]){return false;}
    const auto& row=rules.technologies[id];
    const auto first=rules.ResolveToken(row.prerequisites[0]).kind,second=rules.ResolveToken(row.prerequisites[1]).kind;
    if(first==Kind::UNKNOWN || second==Kind::UNKNOWN){throw std::runtime_error("Unresolved owned terraforming technology");}
    return first!=Kind::DISABLED && (second!=Kind::DISABLED || first==Kind::NONE);
}
}

UnitManager::ConventionalCommands UnitManager::QueryConventionalCommands(Unit* actor) const {
    ConventionalCommands result;
    try {
        if(!actor || GetUnit(actor->m_id)!=actor || !actor->IsConventional()){return result;}
        actor->RequireConventionalReady();
        result.package=ResolveConventional(actor->Conventional(),actor->m_owner).package;
        if(!m_game->IsRunning() || m_game->m_canonical_turn_failed || m_game->m_canonical_turn_in_progress
            || m_game->m_canonical_continuation || actor->m_owner->GetIndex()!=m_game->GetSlotNum()
            || actor->m_owner->GetPlayer()!=m_game->GetPlayer()){return result;}
        if(actor->Conventional().IsHeld()) {
            (void)PrepareHumanOrder(actor,HumanOrderCommand::Activate,m_game->GetSlotNum());
            result.actions.push_back(7);return result;
        }
        if(result.package==ConventionalUnitContext::Package::Former) {
            for(const auto order:{Order::Farm,Order::Road,Order::Forest,Order::Solar,Order::Mine,Order::None}) {
                if(order==Order::None && actor->Conventional().work_order==Order::None){continue;}
                try {
                    ValidateConventionalWork(actor,order,m_game->GetSlotNum(),false);
                    result.actions.push_back(static_cast<int32_t>(order));
                } catch(const std::runtime_error&) {} catch(const std::invalid_argument&) {}
            }
        } else if(result.package==ConventionalUnitContext::Package::Colony && m_game->GetBM()) {
            m_game->GetBM()->ValidateSmacFounding(*actor);
            (void)m_game->GetSmacBasePresentations();
            result.actions.push_back(6);
        }
    } catch(const std::runtime_error&) {result.actions.clear();}
      catch(const std::invalid_argument&) {result.actions.clear();}
    return result;
}

UnitManager::PreparedConventionalWork UnitManager::PrepareConventionalWork(Unit* actor,Order requested,size_t caller,bool dispatch) const {
    m_game->RequireSmacGameplayActive();
    if(!actor || GetUnit(actor->m_id)!=actor || !actor->IsConventional()) {throw std::runtime_error("Terraforming requires a registered conventional Former");}
    actor->RequireConventionalReady();
    m_game->RequireConventionalSightWorld();
    m_game->RequireSmacLocalHumanOwner(actor->m_owner);
    const auto context=ResolveConventional(actor->Conventional(),actor->m_owner);
    if(!m_game->IsRunning() || caller!=m_game->GetSlotNum() || actor->m_owner->GetIndex()!=caller
        || actor->m_owner->GetPlayer()!=m_game->GetPlayer() || context.package!=ConventionalUnitContext::Package::Former
        || m_game->m_simulation->value.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {
        throw std::runtime_error("Terraforming requires the actual local Former in the supported no-project campaign");
    }
    const auto stats=actor->QueryStats(SmacMoraleMode::Mode1);
    if(!GetMap()->GetSmacWorkerKnowledge()){throw std::runtime_error("Terraforming requires owned map knowledge");}
    auto* tile=actor->GetTile(); const auto* map=GetMap();
    if(!tile || tile->coord.x>=map->GetWidth() || tile->coord.y>=map->GetHeight()
        || map->GetTile(tile->coord.x,tile->coord.y)!=tile || tile->IsLocked() || actor->m_animation_id
        || !tile->units.count(actor->m_id) || tile->units.at(actor->m_id)!=actor) {
        throw std::runtime_error("Terraforming requires exact unlocked tile membership");
    }
    PreparedConventionalWork result{tile,{},std::nullopt,std::nullopt};
    auto next=actor->Conventional();
    if(next.IsHeld()) {throw std::runtime_error("Activate held Former before work commands");}
    if(!dispatch && requested==Order::None) {
        next.work_order=Order::None; result.units.emplace_back(actor,next); return result;
    }
    const auto order=dispatch?next.work_order:requested;
    if(order==Order::None || (dispatch && stats.remaining_allowance<=0)){return result;}
    const auto& global=m_game->GetState()->m_settings.global;
    const auto& rules=global.GetSmacTerraformingRules();
    const auto& technologies=global.GetSmacTechnologyRules();
    const auto* owned=m_game->m_simulation->value.technology.Find(next.owner_simulation_id);
    if(!rules || !technologies || !owned){throw std::runtime_error("Terraforming requires frozen actions and owned technology");}
    const auto id=SourceId(order); const auto& action=rules->actions.at(id);
    if(action.source_id!=id || !Available(technologies->ResolveToken(action.land_prerequisite),*technologies,*owned)) {
        throw std::runtime_error("Terraforming action prerequisite is unavailable");
    }
    constexpr auto allowed=TERRAFORMING_ROAD|TERRAFORMING_FARM|TERRAFORMING_SOLAR|TERRAFORMING_MINE|TERRAFORMING_FOREST;
    if(tile->is_water_tile || tile->base || tile->features || (tile->terraforming & ~allowed)
        || tile->rockiness!=ROCKINESS_FLAT) {
        throw std::runtime_error("Terraforming requires ordinary flat land without a Base or special features");
    }
    // Action admission is deliberately narrower than the generic replacement
    // routine. Forest removal has separate mineral/climate effects; upgrades
    // from an existing Farm/Road require their own resolved action admission.
    if((tile->terraforming & Improvement(order)) || (tile->terraforming & TERRAFORMING_FOREST)) {
        throw std::runtime_error("Terraforming upgrade or forest-removal continuation is not admitted");
    }
    int64_t cost=action.work_time;
    const auto rocky=static_cast<int32_t>(tile->rockiness-ROCKINESS_FLAT);
    if(order==Order::Solar){cost+=int64_t(rocky)*cost/2;}
    if(order==Order::Road){cost+=(tile->features&FEATURE_RIVER)?1:0;cost+=(tile->terraforming&TERRAFORMING_FOREST)?2:rocky;}
    if(cost<=0 || cost>INT32_MAX/2){throw std::runtime_error("Terraforming time is outside the supported positive signed threshold domain");}
    if(!dispatch){next.work_order=order;result.units.emplace_back(actor,next);return result;}
    // The actor receives all matching progress bytes, then only its own rate.
    // Helper movement is not spent. Byte wrapping occurs at each addition.
    for(const auto& entry:tile->units) {
        auto* other=entry.second;
        if(!other || GetUnit(entry.first)!=other || other->m_id!=entry.first || other->GetTile()!=tile
            || !other->IsConventional() || other->m_owner!=actor->m_owner) {
            throw std::runtime_error("Terraforming requires a canonical same-owner conventional stack");
        }
        other->QueryStats(other->Conventional().render_morale_mode);
        if(other==actor || other->Conventional().work_order!=order){continue;}
        auto helper=other->Conventional();
        next.work_progress=static_cast<uint8_t>(next.work_progress+helper.work_progress);
        helper.work_progress=0;result.units.emplace_back(other,helper);
    }
    next.work_progress=static_cast<uint8_t>(next.work_progress+2); // admitted ordinary ability-free Former, no projects
    next.spent=static_cast<uint8_t>(stats.full_allowance);
    if(next.work_progress>=cost*2) {
        constexpr auto group=TERRAFORMING_SENSOR|TERRAFORMING_FOREST|TERRAFORMING_CONDENSER|TERRAFORMING_MIRROR|TERRAFORMING_BOREHOLE|TERRAFORMING_MINE|TERRAFORMING_SOLAR;
        auto bits=tile->terraforming;
        if(order==Order::Mine || order==Order::Solar){bits&=~group;}
        if(order==Order::Forest){bits&=~(TERRAFORMING_FARM|TERRAFORMING_SOIL_ENRICHER|TERRAFORMING_MINE|TERRAFORMING_SOLAR|TERRAFORMING_CONDENSER|TERRAFORMING_MIRROR|TERRAFORMING_BOREHOLE);}
        bits|=Improvement(order);result.terraforming=bits;
        next.work_order=Order::None;next.work_progress=0;
        for(auto& entry:result.units){entry.second.work_order=Order::None;}
        result.knowledge=m_game->PrepareConventionalWorkMemory(*actor,bits);
    }
    result.units.emplace_back(actor,next);return result;
}

void UnitManager::ValidateConventionalWork(Unit* actor,Order order,size_t caller,bool dispatch) const {
    (void)PrepareConventionalWork(actor,order,caller,dispatch);
}
void UnitManager::SetConventionalWork(GSE_CALLABLE,Unit* actor,Order order,size_t caller,bool dispatch) {
    m_game->CheckRW(GSE_CALL);
    auto prepared=PrepareConventionalWork(actor,order,caller,dispatch);
    if(prepared.units.empty()){return;}
    auto updates=m_unit_updates;
    for(const auto& entry:prepared.units) {
        auto& pending=updates.try_emplace(entry.first->m_id,unit_update_t{UUO_NONE,entry.first}).first->second;
        if(pending.unit!=entry.first || (pending.ops&UUO_DESPAWN)){throw std::runtime_error("Terraforming has a stale Unit update");}
        pending.ops=static_cast<unit_update_op_t>(pending.ops|UUO_REFRESH);
    }
    // Stage owned atlas/sprite payloads before any Unit, terrain or knowledge write.
    std::unique_ptr<map::Map::PreparedTerrainUpdate> render;
    if(prepared.terraforming){render=GetMap()->PrepareOrdinaryTerrainUpdate(prepared.tile,*prepared.terraforming);}
    std::optional<std::vector<FrontendRequest>> frontend;
    if(prepared.terraforming) {
        frontend.emplace(*m_game->m_pending_frontend_requests);
        if(render) {
            FrontendRequest request(FrontendRequest::FR_UPDATE_TERRAIN);
            NEW(request.data.update_terrain.update,map::TerrainRenderUpdate,GetMap()->GetTerrainUpdatePayload(*render));
            frontend->push_back(request);
        }
        FrontendRequest request(FrontendRequest::FR_UPDATE_TILES);
        const FrontendRequest::tile_updates_t changed{{prepared.tile,GetMap()->GetTileState(prepared.tile)}};
        NEW(request.data.update_tiles.tile_updates,FrontendRequest::tile_updates_t,changed);
        frontend->push_back(request);
    }
    static_assert(std::is_nothrow_copy_assignable_v<ConventionalUnitState>);
    static_assert(noexcept(m_unit_updates.swap(updates)));
    for(const auto& entry:prepared.units){entry.first->CommitConventionalMove(entry.second,prepared.tile);}
    if(prepared.terraforming){prepared.tile->terraforming=*prepared.terraforming;}
    if(render){GetMap()->CommitOrdinaryTerrainUpdate(*render);}
    m_game->CommitConventionalSight(prepared.knowledge);
    m_unit_updates.swap(updates);
    if(frontend){m_game->m_pending_frontend_requests->swap(*frontend);}
}
}
