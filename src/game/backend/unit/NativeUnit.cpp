#include "UnitManager.h"
#include "Unit.h"
#include "StaticDef.h"
#include "MoraleSet.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/map/Map.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend::unit {
namespace {
int32_t NativeQuantum(const Game& game) {
    const auto* state=game.GetState();
    if(!state) {throw std::runtime_error("Native query requires frozen world settings");}
    const auto& global=state->m_settings.global;
    const auto& units=global.GetSmacPredefinedUnitRules();
    const auto& movement=global.GetSmacMovementRules();
    if(global.GetRulesetId()!="smac" || !global.IsRulesetLocked() || !units || !movement) {
        throw std::runtime_error("Native query requires frozen SMAC design and movement authority");
    }
    const auto& catalog=units->catalog;
    const auto design=std::find_if(catalog.units.begin(),catalog.units.end(),[](const auto& row){return row.source_id==8;});
    if(design==catalog.units.end() || !design->active || design->chassis_id!=0 || design->weapon_id!=12
        || design->defense_id!=9 || design->reactor!=1 || design->abilities || design->plan!=1 || design->carry!=0
        || catalog.chassis.chassis.empty() || catalog.weapons.weapons.size()<=12
        || catalog.defenses.defenses.size()<=9 || catalog.reactors.reactors.empty()) {
        throw std::runtime_error("Native query requires the produced Mind Worm design8");
    }
    const auto byte=[](int32_t raw) {const auto value=static_cast<uint32_t>(raw)&255u;return value>127 ? int32_t(value)-256 : int32_t(value);};
    const auto& chassis=catalog.chassis.chassis[0];
    if(chassis.triad!=loader::txt::ChassisTriad::Land || chassis.speed!=1 || chassis.missile || chassis.range!=0
        || byte(catalog.weapons.weapons[12].offense)!=-1 || byte(catalog.defenses.defenses[9].rating)!=-1
        || catalog.reactors.reactors[0].power!=1) {
        throw std::runtime_error("Native query requires evidenced Infantry/Psi/reactor component semantics");
    }
    return movement->Quantum();
}
}

SmacNativeUnitStats UnitManager::QueryNativeUnit(const SmacNativeUnitRecord& record,int32_t opposing_faction) const {
    m_game->ValidateCurrentSimulation();
    if(!m_game->m_simulation || m_game->GetTurnId()>INT32_MAX || opposing_faction<0 || opposing_faction>7) {
        throw std::runtime_error("Native query requires a current simulation and bounded turn/context");
    }
    const auto quantum=NativeQuantum(*m_game);
    int32_t atrocities=0;
    if(opposing_faction) {
        const auto& history=m_game->m_simulation->value.ecology_history;
        if(!history) {throw std::runtime_error("Opposed native level requires retained major-atrocity authority");}
        atrocities=history->major_atrocities.at(opposing_faction);
    }
    const auto level=SmacWildMindWormLevel(record,static_cast<int32_t>(m_game->GetTurnId()),opposing_faction,atrocities);
    if(record.damage>=10) {throw std::runtime_error("Living native damage exhausts produced reactor health");}
    // Infantry speed1 gives Q basic/full allowance. The negative Psi weapon
    // suppresses the ordinary elite increment, including at native level6.
    return {level,quantum,quantum,std::max(0,quantum-int32_t(record.movement_spent)),10-record.damage,10};
}

void UnitManager::SyncNativeObservationMasks(const map::SmacWorkerKnowledge& knowledge) noexcept {
    for(const auto& row:knowledge.UnitObservations()) {
        const auto found=m_units.find(row.id);
        if(found!=m_units.end() && found->second->IsNative()) {
            std::get<SmacNativeUnitRecord>(found->second->m_state).observation_mask=row.visible_to;
        }
    }
}

void UnitManager::ValidateNativeCreation(size_t id,const Def* def,const map::tile::Tile* tile,const SmacNativeUnitRecord& record) const {
    record.Validate();
    (void)NativeQuantum(*m_game);
    (void)SmacWildMindWormLevel(record,0,0,0);
    if(id==0 || id>=static_cast<uint64_t>(INT64_MAX) || id>=std::numeric_limits<size_t>::max() || HasUnitId(id)
        || !GetMap() || !tile || tile->tiles!=GetMap()->GetTilesPtr() || tile->coord.x>=GetMap()->GetWidth()
        || tile->coord.y>=GetMap()->GetHeight() || ((tile->coord.x^tile->coord.y)&1)
        || GetMap()->GetTile(tile->coord.x,tile->coord.y)!=tile || tile->units.count(id)
        || record.x!=static_cast<int64_t>(tile->coord.x) || record.y!=static_cast<int64_t>(tile->coord.y)
        || record.damage>=10 || record.home_base!=-1 || record.stack_next!=-1 || record.stack_previous!=-1) {
        throw std::runtime_error("Native creation requires canonical singleton placement and homeless live state");
    }
    if(!def || def!=GetUnitDef("MindWorms") || def->m_type!=DT_STATIC || def->GetMovementType()!=MT_LAND
        || !def->m_moraleset || def->m_moraleset!=GetMoraleSet(def->m_moraleset->m_id)) {
        throw std::runtime_error("Native creation requires the canonical MindWorms presentation");
    }
    for(const auto& entry:m_units) {
        if(entry.second->IsNative() && entry.second->Native().id==record.id) {throw std::runtime_error("Duplicate native source-pool identity");}
    }
    for(const auto& pending:m_unprocessed_units) {
        if(pending.unit->IsNative() && pending.unit->Native().id==record.id) {throw std::runtime_error("Duplicate pending native source-pool identity");}
    }
}
}
