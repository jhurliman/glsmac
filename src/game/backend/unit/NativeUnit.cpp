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
#include <type_traits>
#include "gc/Space.h"
#include "util/FinallyGuard.h"
#include "game/backend/base/SmacEcologyNativeSpawn.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include <stdexcept>

namespace game::backend::unit {
namespace {
struct NativeDesignContext {
    int32_t quantum, basic, plan;
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
    return {quantum,speed*quantum,profile.plan,profile.domain,profile.movement,profile.presentation};
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

struct UnitManager::PreparedNativeBirths::Data {
    UnitManager* manager=nullptr;
    std::vector<std::unique_ptr<Unit>> candidates;
    std::vector<Unit*> published;
    std::map<size_t,Unit*> units;
    std::vector<size_t> order;
    std::unordered_map<size_t,unit_update_t> updates;
    PreparedOccupants occupants;
    std::map<Unit*,SmacNativeUnitRecord> records;
    std::optional<map::SmacWorkerKnowledge> knowledge;
    std::optional<faction::SmacNativeFactoryState> factory;
    bool committed=false,notified=false;
};
UnitManager::PreparedNativeBirths::PreparedNativeBirths(std::unique_ptr<Data> data):m_data(std::move(data)) {}
UnitManager::PreparedNativeBirths::~PreparedNativeBirths()=default;
UnitManager::PreparedNativeBirths::PreparedNativeBirths(PreparedNativeBirths&&) noexcept=default;
UnitManager::PreparedNativeBirths& UnitManager::PreparedNativeBirths::operator=(PreparedNativeBirths&&) noexcept=default;
size_t UnitManager::PreparedNativeBirths::Size() const {
    if(!m_data) {throw std::runtime_error("Native birth plan was moved");}return m_data->published.size();
}
const Unit& UnitManager::PreparedNativeBirths::Candidate(size_t index) const {
    if(!m_data || m_data->committed) {throw std::runtime_error("Native birth candidate is not pending");}
    return *m_data->candidates.at(index);
}
void UnitManager::PreparedNativeBirths::Query(const std::function<void()>& query) const {
    if(!m_data || m_data->committed) {throw std::runtime_error("Native birth query is not pending");}
    auto previous=m_data->records;
    auto factory=m_data->factory;
    for(auto& row:previous) {row.second=row.first->Native();}
    util::FinallyGuard restore([&]{
        m_data->manager->m_game->m_simulation->value.native_factory.swap(factory);
        for(const auto& row:previous) {std::get<SmacNativeUnitRecord>(row.first->m_state)=row.second;}
    });
    m_data->manager->m_game->m_simulation->value.native_factory.swap(factory);
    for(const auto& row:m_data->records) {std::get<SmacNativeUnitRecord>(row.first->m_state)=row.second;}
    m_data->manager->QueryPreparedUnits(m_data->units,m_data->order,m_data->occupants,m_data->knowledge,{},query);
}
void UnitManager::PreparedNativeBirths::Commit() noexcept {
    if(!m_data || m_data->committed) {return;}auto& d=*m_data;
    static_assert(std::is_nothrow_copy_assignable_v<SmacNativeUnitRecord>);
    static_assert(std::is_nothrow_swappable_v<decltype(d.units)> && std::is_nothrow_swappable_v<decltype(d.order)>
        && std::is_nothrow_swappable_v<decltype(d.updates)> && std::is_nothrow_swappable_v<decltype(d.factory)>);
    for(const auto& row:d.records) {std::get<SmacNativeUnitRecord>(row.first->m_state)=row.second;}
    d.manager->m_units.swap(d.units);d.manager->m_unit_order.swap(d.order);d.manager->m_unit_updates.swap(d.updates);
    for(auto& row:d.occupants) {row.first->units.swap(row.second);}
    d.manager->m_game->m_simulation->value.native_factory.swap(d.factory);
    for(size_t i=0;i<d.candidates.size();++i) {d.published[i]=d.candidates[i].release();d.published[i]->CommitInitialPlacement(d.published[i]->GetTile());}
    d.manager->m_game->CommitConventionalSight(d.knowledge);d.committed=true;
}
void UnitManager::PreparedNativeBirths::Notify(GSE_CALLABLE) {
    if(!m_data || !m_data->committed || m_data->notified) {throw std::runtime_error("Native birth notifications are not pending");}
    auto& d=*m_data;d.notified=true;
    for(auto* unit:d.published) {d.manager->TriggerSpawnHook(GSE_CALL,unit);}
}
UnitManager::PreparedNativeBirths UnitManager::PrepareEcologyNativeBirths(GSE_CALLABLE,const base::SmacNativeSpawnPlan& plan) {
    m_game->RequireSmacGameplayActive();m_game->CheckRW(GSE_CALL);
    if(!m_game->IsRunning() || gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating()
        || !m_unprocessed_units.empty() || !m_game->m_simulation || !m_game->m_simulation->value.native_factory) {
        throw std::runtime_error("Native birth preparation requires its owning running world and retained factory");
    }
    m_game->RequireConventionalSightWorld();
    const auto& prior=GetMap()->GetSmacWorkerKnowledge();
    if(!prior) {throw std::runtime_error("Native birth requires retained map knowledge");}
    auto knowledge=prior->CopyData();const auto& input=plan.Input();
    if(input.map.width!=knowledge.width || input.map.height!=knowledge.height || input.map.horizontal_wrap!=knowledge.horizontal_wrap
        || knowledge.width<10 || knowledge.height<1 || m_units.size()!=m_unit_order.size() || m_units.size()>2048) {
        throw std::runtime_error("Native birth plan geometry or dense registry differs from its world");
    }
    std::vector<Unit*> live;live.reserve(m_unit_order.size());
    for(const auto id:m_unit_order) {auto* unit=GetUnit(id);if(!unit) {throw std::runtime_error("Native birth has a stale dense identity");}live.push_back(unit);}
    ValidateNativeIndexBindings(live);
    auto d=std::make_unique<PreparedNativeBirths::Data>();d->manager=this;d->factory=m_game->m_simulation->value.native_factory;
    d->units=m_units;d->order=m_unit_order;d->updates=m_unit_updates;d->candidates.reserve(plan.Requests().size());
    d->published.reserve(plan.Requests().size());knowledge.units.reserve(knowledge.units.size()+plan.Requests().size());
    const auto index=[&](const map::tile::Tile* tile) {return size_t(tile->coord.y)*(knowledge.width/2)+tile->coord.x/2;};
    const auto distance=[&](const map::tile::Tile* a,const map::tile::Tile* b) {
        auto dx=std::abs(int64_t(a->coord.x)-int64_t(b->coord.x));
        if(knowledge.horizontal_wrap) {dx=std::min(dx,int64_t(knowledge.width)-dx);}
        return dx+std::abs(int64_t(a->coord.y)-int64_t(b->coord.y));
    };
    const auto occupants=[&](map::tile::Tile* tile)->const std::map<size_t,Unit*>& {
        const auto found=d->occupants.find(tile);return found==d->occupants.end()?tile->units:found->second;
    };
    const auto record=[&](Unit* unit)->SmacNativeUnitRecord& {return d->records.try_emplace(unit,unit->Native()).first->second;};
    const auto observation=[&](size_t id)->map::SmacWorkerKnowledge::UnitObservation& {
        const auto found=std::find_if(knowledge.units.begin(),knowledge.units.end(),[id](const auto& row){return row.id==id;});
        if(found==knowledge.units.end()) {throw std::runtime_error("Native sight lost an existing Unit observation");}return *found;
    };
    const auto mark=[&](map::tile::Tile* tile,int32_t observer) {
        for(const auto& row:occupants(tile)) {
            auto* unit=row.second;auto& sight=observation(row.first);sight.visible_to|=static_cast<uint8_t>(1u<<observer);
            if(unit->IsNative()) {auto& value=record(unit);value.observation_mask=sight.visible_to;if(observer>0) {value.flags&=0xfbbfu;}}
        }
    };
    for(const auto& request:plan.Requests()) {
        // Original factory rejects a full source pool before any body, counter,
        // placement, sight or assignment work. Attempts belong to the input plan.
        if(d->units.size()>=2048) {continue;}
        if(d->factory->assignment_reference[0]>=0) {throw std::runtime_error("Loaded native assignment tail requires its retained AI producer");}
        const auto design=NativeDesign(*m_game,request.design);
        auto* def=GetUnitDef(design.presentation);
        const auto x=request.coordinate[0],y=request.coordinate[1];
        if(x<0 || x>=knowledge.width || y<0 || y>=knowledge.height || ((x^y)&1)) {throw std::runtime_error("Native birth coordinate differs from the staged map");}
        auto* tile=GetMap()->GetTile(x,y);
        const uint64_t next=uint64_t(Unit::GetNextId())+d->candidates.size();
        if(next==0 || next>=uint64_t(INT64_MAX) || next>=SIZE_MAX) {throw std::runtime_error("Native port identity pool exhausted");}
        auto value=SmacNativeUnitRecord::InitializeWild(static_cast<int32_t>(d->order.size()),request.design,d->factory->definition_morale);
        value.x=static_cast<int16_t>(x);value.y=static_cast<int16_t>(y);
        ValidateNativeCreation(static_cast<size_t>(next),def,tile,value);
        if(tile->base) {throw std::runtime_error("Native birth on an occupied Base requires mixed-owner stack admission");}
        auto& stack=d->occupants.try_emplace(tile,tile->units).first->second;
        Unit* head=nullptr;
        for(const auto& row:stack) {
            if(!row.second || !row.second->IsNative() || row.second->SimulationOwner()!=0) {throw std::runtime_error("Native birth requires an ordinary same-native stack");}
            if(record(row.second).stack_previous==-1) {if(head) {throw std::runtime_error("Native birth stack has multiple heads");}head=row.second;}
        }
        if(!stack.empty() && !head) {throw std::runtime_error("Native birth stack has no head");}
        if(head) {value.stack_next=static_cast<int16_t>(record(head).id);record(head).stack_previous=static_cast<int16_t>(value.id);}
        auto candidate=std::unique_ptr<Unit>(new Unit(Unit::Unplaced{},this,static_cast<size_t>(next),def,tile,value));
        auto* born=candidate.get();
        if(!d->units.emplace(born->m_id,born).second || !stack.emplace(born->m_id,born).second
            || !d->updates.emplace(born->m_id,unit_update_t{UUO_SPAWN,born}).second) {throw std::runtime_error("Native birth publication identity conflicts");}
        d->order.push_back(born->m_id);d->records.emplace(born,value);knowledge.units.push_back({born->m_id,0,false});
        d->published.push_back(nullptr);d->candidates.push_back(std::move(candidate));
        d->factory->Adjust(request.design,static_cast<uint8_t>(design.plan),1);
        const auto underfoot=knowledge.tiles[index(tile)].claimant;
        if((tile->terraforming&map::tile::TERRAFORMING_SENSOR) && underfoot && *underfoot==0) {
            throw std::runtime_error("Native-owned underfoot sensor needs its extended sight producer");
        }
        // Fresh mask0 intersects destination exploration before disclosure.
        // Native exploration is independent of human remembered improvements.
        for(int dy=-2;dy<=2;++dy) {for(int dx=-2;dx<=2;++dx) {
            if(std::abs(dx)+std::abs(dy)>2 || ((dx^dy)&1)) {continue;}
            auto cx=x+dx;const auto cy=y+dy;
            if(knowledge.horizontal_wrap) {cx=(cx%knowledge.width+knowledge.width)%knowledge.width;}
            if(cx<0 || cx>=knowledge.width || cy<0 || cy>=knowledge.height) {continue;}
            knowledge.tiles[size_t(cy)*(knowledge.width/2)+cx/2].explored_by|=1u;
        }}
        uint8_t disclosed=0;
        for(const auto& row:d->units) {
            auto* other=row.second;if(other->IsNative()) {continue;}
            const auto owner=other->SimulationOwner();auto* position=other->GetTile();
            if(distance(tile,position)>2) {continue;}
            if(position==tile) {throw std::runtime_error("Native birth cannot share a positive-owner square");}
            mark(position,0);disclosed|=static_cast<uint8_t>(1u<<owner);
        }
        for(const auto& row:m_game->GetBM()->GetBases()) {
            auto* base=row.second;if(distance(tile,base->GetTile())>2) {continue;}
            const auto owner=base->GetSmacResourceContext().owner_simulation_id;
            if(base->GetPopulation()>127) {throw std::runtime_error("Native Base memory exceeds its population byte");}
            auto found=std::find_if(knowledge.bases.begin(),knowledge.bases.end(),[&](const auto& b){return b.id==row.first;});
            if(found==knowledge.bases.end()) {knowledge.bases.push_back({row.first,0,{}});found=std::prev(knowledge.bases.end());}
            found->known_by|=1u;found->population[0]=static_cast<uint8_t>(base->GetPopulation());disclosed|=static_cast<uint8_t>(1u<<owner);
        }
        for(int dy=-4;dy<=4;++dy) {for(int dx=-4;dx<=4;++dx) {
            if(std::abs(dx)+std::abs(dy)>4 || ((dx^dy)&1)) {continue;}
            auto cx=x+dx;const auto cy=y+dy;
            if(knowledge.horizontal_wrap) {cx=(cx%knowledge.width+knowledge.width)%knowledge.width;}
            if(cx<0 || cx>=knowledge.width || cy<0 || cy>=knowledge.height) {continue;}
            const auto* sensor=GetMap()->GetTile(cx,cy);const auto claim=knowledge.tiles[index(sensor)].claimant;
            if((sensor->terraforming&map::tile::TERRAFORMING_SENSOR) && claim && *claim>0) {disclosed|=static_cast<uint8_t>(1u<<*claim);}
        }}
        for(int32_t observer=1;observer<=7;++observer) {if(disclosed&(1u<<observer)) {
            mark(tile,observer);knowledge.tiles[index(tile)].explored_by|=static_cast<uint8_t>(1u<<observer);
            auto& memory=knowledge.remembered[observer];if(memory.empty()) {memory.resize(knowledge.tiles.size());}
            memory[index(tile)]=map::SmacWorkerKnowledge::Improvements{tile->features,tile->terraforming,false,true};
        }}
        // The ecology attribution write is after the full factory return.
        if(request.ecology_owner) {record(born).ecology_owner_tag=*request.ecology_owner;}
    }
    for(auto& row:d->records) {
        // Candidate queries must expose the final prepared record, not a body
        // whose later stack/sight/tag writes are held only in the transaction.
        if(std::any_of(d->candidates.begin(),d->candidates.end(),[&](const auto& c){return c.get()==row.first;})) {
            std::get<SmacNativeUnitRecord>(row.first->m_state)=row.second;
        }
    }
    d->knowledge=map::SmacWorkerKnowledge::Create(knowledge);
    return PreparedNativeBirths(std::move(d));
}

}
