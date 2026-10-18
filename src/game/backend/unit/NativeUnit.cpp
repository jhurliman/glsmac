#include "UnitManager.h"
#include "Unit.h"
#include "StaticDef.h"
#include "MoraleSet.h"
#include "SmacConventionalStats.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/map/Map.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend::unit {
namespace {
struct NativeDesignContext {
    int32_t quantum, basic;
    SmacMovementDomain domain;
    movement_type_t movement;
    const char* presentation;
};
NativeDesignContext NativeDesign(const Game& game, int32_t id) {
    const auto* state=game.GetState();
    if(!state) {throw std::runtime_error("Native query requires frozen world settings");}
    const auto& global=state->m_settings.global;
    const auto& units=global.GetSmacPredefinedUnitRules();
    const auto& movement=global.GetSmacMovementRules();
    if(global.GetRulesetId()!="smac" || !global.IsRulesetLocked() || !units || !movement || id<8 || id>10) {
        throw std::runtime_error("Native query requires frozen SMAC native design and movement authority");
    }
    struct Expected {int32_t chassis, speed, abilities, plan, carry; SmacMovementDomain domain; movement_type_t movement; const char* presentation;};
    static const Expected expected[]{
        {0,1,0,1,0,SmacMovementDomain::Land,MT_LAND,"MindWorms"},
        {3,4,0,7,4,SmacMovementDomain::Sea,MT_WATER,"IsleOfTheDeep"},
        {7,8,32,4,0,SmacMovementDomain::Air,MT_AIR,"LocustsOfChiron"}
    };
    const auto& profile=expected[id-8];
    const auto& catalog=units->catalog;
    const auto design=std::find_if(catalog.units.begin(),catalog.units.end(),[id](const auto& row){return row.source_id==id;});
    if(design==catalog.units.end() || !design->active || design->chassis_id!=profile.chassis || design->weapon_id!=12
        || design->defense_id!=9 || design->reactor!=1 || design->abilities!=profile.abilities || design->plan!=profile.plan || design->carry!=profile.carry
        || catalog.chassis.chassis.size()<=static_cast<size_t>(profile.chassis) || catalog.weapons.weapons.size()<=12
        || catalog.defenses.defenses.size()<=9 || catalog.reactors.reactors.empty()) {
        throw std::runtime_error("Native query requires the produced Mind Worm, Isle or Locust design");
    }
    const auto byte=[](int32_t raw) {const auto value=static_cast<uint32_t>(raw)&255u;return value>127 ? int32_t(value)-256 : int32_t(value);};
    const auto& chassis=catalog.chassis.chassis[profile.chassis];
    if(static_cast<int32_t>(chassis.triad)!=static_cast<int32_t>(profile.domain) || chassis.speed!=profile.speed || chassis.missile || chassis.range!=0
        || byte(catalog.weapons.weapons[12].offense)!=-1 || byte(catalog.defenses.defenses[9].rating)!=-1
        || catalog.reactors.reactors[0].power!=1) {
        throw std::runtime_error("Native query requires evidenced chassis/Psi/reactor component semantics");
    }
    int32_t speed=chassis.speed;
    if(profile.domain==SmacMovementDomain::Air) {
        speed+=2*design->reactor;
        if(design->abilities&32) {speed=4*speed/5;}
    }
    const auto quantum=movement->Quantum();
    return {quantum,speed*quantum,profile.domain,profile.movement,profile.presentation};
}
}

SmacNativeUnitStats UnitManager::QueryNativeUnit(const SmacNativeUnitRecord& record,int32_t opposing_faction) const {
    m_game->ValidateCurrentSimulation();
    if(!m_game->m_simulation || m_game->GetTurnId()>INT32_MAX || opposing_faction<0 || opposing_faction>7) {
        throw std::runtime_error("Native query requires a current simulation and bounded turn/context");
    }
    const auto design=NativeDesign(*m_game,record.design);
    int32_t atrocities=0;
    if(opposing_faction) {
        const auto& history=m_game->m_simulation->value.ecology_history;
        if(!history) {throw std::runtime_error("Opposed native level requires retained major-atrocity authority");}
        atrocities=history->major_atrocities.at(opposing_faction);
    }
    const auto level=SmacWildNativeLevel(record,static_cast<int32_t>(m_game->GetTurnId()),opposing_faction,atrocities);
    // Psi suppresses elite movement. Sea damage rounds whole movement points
    // before Q; air bypasses damage scaling. The shared mechanical helper
    // applies these rules after native design/level resolution above.
    const auto full=SmacConventionalAllowance({design.basic,design.quantum,design.domain,1,record.damage,level,false,true});
    return {level,design.quantum,full,std::max(0,full-int32_t(record.movement_spent)),
        std::clamp(10-int32_t(record.damage),0,9999),10,design.basic};
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
    const auto design=NativeDesign(*m_game,record.design);
    (void)SmacWildNativeLevel(record,0,0,0);
    if(id==0 || id>=static_cast<uint64_t>(INT64_MAX) || id>=std::numeric_limits<size_t>::max() || HasUnitId(id)
        || !GetMap() || !tile || tile->tiles!=GetMap()->GetTilesPtr() || tile->coord.x>=GetMap()->GetWidth()
        || tile->coord.y>=GetMap()->GetHeight() || ((tile->coord.x^tile->coord.y)&1)
        || GetMap()->GetTile(tile->coord.x,tile->coord.y)!=tile || tile->units.count(id)
        || record.x!=static_cast<int64_t>(tile->coord.x) || record.y!=static_cast<int64_t>(tile->coord.y)
        || record.damage>=10 || record.home_base!=-1) {
        throw std::runtime_error("Native creation requires canonical placement and homeless live state");
    }
    if(!def || def!=GetUnitDef(design.presentation) || def->m_type!=DT_STATIC || def->GetMovementType()!=design.movement
        || !def->m_moraleset || def->m_moraleset!=GetMoraleSet(def->m_moraleset->m_id)) {
        throw std::runtime_error("Native creation requires the canonical native design presentation");
    }
    for(const auto& entry:m_units) {
        if(entry.second->IsNative() && entry.second->Native().id==record.id) {throw std::runtime_error("Duplicate native source-pool identity");}
    }
    for(const auto& pending:m_unprocessed_units) {
        if(pending.unit->IsNative() && pending.unit->Native().id==record.id) {throw std::runtime_error("Duplicate pending native source-pool identity");}
    }
}
void UnitManager::ValidateNativeIndexBindings(const std::vector<Unit*>& ordered) const {
    for(size_t index=0;index<ordered.size();++index) {
        const auto* unit=ordered[index];
        if(!unit->IsNative()) {continue;}
        const auto& record=unit->Native();
        if(ordered.size()>2048 || record.id!=index) {throw std::runtime_error("Native source index differs from dense Unit order");}
        const auto linked=[&](int16_t target)->const Unit* {
            if(target<0) {return nullptr;}
            if(static_cast<size_t>(target)>=ordered.size() || !ordered[target]->IsNative() || ordered[target]->GetTile()!=unit->GetTile()) {
                throw std::runtime_error("Native stack reference is not a same-tile native record");
            }
            return ordered[target];
        };
        const auto* next=linked(record.stack_next);const auto* previous=linked(record.stack_previous);
        if((next && next->Native().stack_previous!=record.id) || (previous && previous->Native().stack_next!=record.id)) {
            throw std::runtime_error("Native stack references are not reciprocal");
        }
        if(record.stack_previous==-1) {
            size_t count=0;const Unit* cursor=unit;
            while(cursor) {
                if(++count>ordered.size()) {throw std::runtime_error("Native stack contains a cycle");}
                cursor=linked(cursor->Native().stack_next);
            }
            if(count!=unit->GetTile()->units.size()) {throw std::runtime_error("Native stack omits physical tile members");}
        } else {
            // Every node must reach a head, including an otherwise reciprocal cycle.
            size_t count=0;const Unit* cursor=unit;
            while(cursor) {
                if(++count>ordered.size()) {throw std::runtime_error("Native stack has no head");}
                cursor=linked(cursor->Native().stack_previous);
            }
        }
    }
}

}
