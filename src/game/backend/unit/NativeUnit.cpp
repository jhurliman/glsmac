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
    using Stack=ConventionalUnitState::Stack;
    const auto state=[](const Unit* unit)->std::optional<Stack> {
        if(unit->IsNative()) {const auto& n=unit->Native();return Stack{int16_t(n.id),n.stack_next,n.stack_previous};}
        if(unit->IsConventional()) {return unit->Conventional().stack;}return std::nullopt;
    };
    for(size_t index=0;index<ordered.size();++index) {
        const auto* unit=ordered[index];const auto binding=state(unit);if(!binding) {continue;}
        binding->Validate();
        if(ordered.size()>2048 || binding->id!=index) {throw std::runtime_error("Source stack index differs from dense Unit order");}
        const auto linked=[&](int16_t target)->const Unit* {
            if(target<0) {return nullptr;}
            if(size_t(target)>=ordered.size() || !state(ordered[target]) || state(ordered[target])->id!=target || ordered[target]->GetTile()!=unit->GetTile()) {
                throw std::runtime_error("Stack reference is not a bound same-tile Unit record");
            }return ordered[target];
        };
        const auto* next=linked(binding->next);const auto* previous=linked(binding->previous);
        if((next && state(next)->previous!=binding->id) || (previous && state(previous)->next!=binding->id)) {
            throw std::runtime_error("Source stack references are not reciprocal");
        }
        if(binding->previous==-1) {
            size_t count=0;const Unit* cursor=unit;
            while(cursor) {
                if(++count>ordered.size()) {throw std::runtime_error("Source stack contains a cycle");}
                cursor=linked(state(cursor)->next);
            }
            if(count!=unit->GetTile()->units.size()) {throw std::runtime_error("Source stack omits physical tile members");}
        } else {
            size_t count=0;const Unit* cursor=unit;
            while(cursor) {
                if(++count>ordered.size()) {throw std::runtime_error("Source stack has no head");}
                cursor=linked(state(cursor)->previous);
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
    std::map<Unit*,ConventionalUnitState::Stack> conventional_stacks;
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
    std::map<Unit*,std::optional<ConventionalUnitState::Stack>> previous_stacks;
    for(const auto& row:m_data->conventional_stacks) {previous_stacks.emplace(row.first,row.first->Conventional().stack);}
    auto factory=m_data->factory;
    for(auto& row:previous) {row.second=row.first->Native();}
    util::FinallyGuard restore([&]{
        m_data->manager->m_game->m_simulation->value.native_factory.swap(factory);
        for(const auto& row:previous) {std::get<SmacNativeUnitRecord>(row.first->m_state)=row.second;}
        for(const auto& row:previous_stacks) {std::get<ConventionalUnitState>(row.first->m_state).stack=row.second;}
    });
    m_data->manager->m_game->m_simulation->value.native_factory.swap(factory);
    for(const auto& row:m_data->records) {std::get<SmacNativeUnitRecord>(row.first->m_state)=row.second;}
    for(const auto& row:m_data->conventional_stacks) {std::get<ConventionalUnitState>(row.first->m_state).stack=row.second;}
    m_data->manager->QueryPreparedUnits(m_data->units,m_data->order,m_data->occupants,m_data->knowledge,{},query);
}
void UnitManager::PreparedNativeBirths::Commit() noexcept {
    if(!m_data || m_data->committed) {return;}auto& d=*m_data;
    static_assert(std::is_nothrow_copy_assignable_v<SmacNativeUnitRecord>);
    static_assert(std::is_nothrow_swappable_v<decltype(d.units)> && std::is_nothrow_swappable_v<decltype(d.order)>
        && std::is_nothrow_swappable_v<decltype(d.updates)> && std::is_nothrow_swappable_v<decltype(d.factory)>);
    for(const auto& row:d.records) {std::get<SmacNativeUnitRecord>(row.first->m_state)=row.second;}
    for(const auto& row:d.conventional_stacks) {std::get<ConventionalUnitState>(row.first->m_state).stack=row.second;}
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
    const auto& geometry=plan.Input().map;
    return PrepareNativeBirths(GSE_CALL,geometry.width,geometry.height,geometry.horizontal_wrap,[&](const NativeBirthFactory& factory){
        for(const auto& request:plan.Requests()) {factory.create(request.design,request.coordinate,request.ecology_owner);}
    });
}
UnitManager::PreparedNativeBirths UnitManager::PrepareNativeBirths(GSE_CALLABLE,int32_t width,int32_t height,bool horizontal_wrap,
    const std::function<void(const NativeBirthFactory&)>& produce) {
    m_game->RequireSmacGameplayActive();m_game->CheckRW(GSE_CALL);
    if(!m_game->IsRunning() || gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating()
        || !m_unprocessed_units.empty() || !m_game->m_simulation || !m_game->m_simulation->value.native_factory) {
        throw std::runtime_error("Native birth preparation requires its owning running world and retained factory");
    }
    m_game->RequireConventionalSightWorld();
    const auto& prior=GetMap()->GetSmacWorkerKnowledge();
    if(!prior) {throw std::runtime_error("Native birth requires retained map knowledge");}
    auto knowledge=prior->CopyData();
    if(!produce) {throw std::runtime_error("Native birth requires a synchronous producer");}
    if(width!=knowledge.width || height!=knowledge.height || horizontal_wrap!=knowledge.horizontal_wrap
        || knowledge.width<10 || knowledge.height<1 || m_units.size()!=m_unit_order.size() || m_units.size()>2048) {
        throw std::runtime_error("Native birth plan geometry or dense registry differs from its world");
    }
    std::vector<Unit*> live;live.reserve(m_unit_order.size());
    for(const auto id:m_unit_order) {auto* unit=GetUnit(id);if(!unit) {throw std::runtime_error("Native birth has a stale dense identity");}live.push_back(unit);}
    ValidateNativeIndexBindings(live);
    auto d=std::make_unique<PreparedNativeBirths::Data>();d->manager=this;d->factory=m_game->m_simulation->value.native_factory;
    d->units=m_units;d->order=m_unit_order;d->updates=m_unit_updates;
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
    using Stack=ConventionalUnitState::Stack;
    const auto stack_state=[&](Unit* unit)->Stack {
        if(unit->IsNative()) {const auto& n=record(unit);return {int16_t(n.id),n.stack_next,n.stack_previous};}
        if(!unit->IsConventional()) {throw std::runtime_error("Native birth requires an admitted conventional/native stack");}
        auto found=d->conventional_stacks.find(unit);if(found!=d->conventional_stacks.end()) {return found->second;}
        if(const auto known=unit->SourceStack()) {return d->conventional_stacks.emplace(unit,*known).first->second;}
        if(occupants(unit->GetTile()).size()!=1) {throw std::runtime_error("Native birth requires retained legacy stack order");}
        const auto index=std::find(d->order.begin(),d->order.end(),unit->m_id);
        if(index==d->order.end()) {throw std::runtime_error("Native birth lost conventional dense identity");}
        return d->conventional_stacks.emplace(unit,Stack{int16_t(index-d->order.begin()),-1,-1}).first->second;
    };
    const auto set_stack=[&](Unit* unit,const Stack& value) {
        value.Validate();
        if(unit->IsNative()) {auto& n=record(unit);n.id=value.id;n.stack_next=value.next;n.stack_previous=value.previous;}
        else {d->conventional_stacks[unit]=value;}
    };
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
    NativeBirthFactory factory;
    factory.create=[&](int32_t design_id,std::array<int32_t,2> coordinate,std::optional<uint8_t> ecology_owner)->int32_t {
        // Original factory rejects a full source pool before any body, counter,
        // placement, sight or assignment work. Attempts belong to the input plan.
        if(d->units.size()>=2048) {return -1;}
        if(ecology_owner && *ecology_owner>7) {throw std::runtime_error("Native ecology attribution exceeds faction domain");}
        if(d->factory->assignment_reference[0]>=0) {throw std::runtime_error("Loaded native assignment tail requires its retained AI producer");}
        const auto design=NativeDesign(*m_game,design_id);
        auto* def=GetUnitDef(design.presentation);
        const auto x=coordinate[0],y=coordinate[1];
        if(x<0 || x>=knowledge.width || y<0 || y>=knowledge.height || ((x^y)&1)) {throw std::runtime_error("Native birth coordinate differs from the staged map");}
        auto* tile=GetMap()->GetTile(x,y);
        const uint64_t next=uint64_t(Unit::GetNextId())+d->candidates.size();
        if(next==0 || next>=uint64_t(INT64_MAX) || next>=SIZE_MAX) {throw std::runtime_error("Native port identity pool exhausted");}
        auto value=SmacNativeUnitRecord::InitializeWild(static_cast<int32_t>(d->order.size()),design_id,d->factory->definition_morale);
        value.x=static_cast<int16_t>(x);value.y=static_cast<int16_t>(y);
        ValidateNativeCreation(static_cast<size_t>(next),def,tile,value);
        if(tile->base) {throw std::runtime_error("Native birth on an occupied Base requires mixed-owner stack admission");}
        auto& stack=d->occupants.try_emplace(tile,tile->units).first->second;
        if(knowledge.occupancy) {knowledge.occupancy->PlaceUnit({x,y},0,stack.empty());}
        Unit* head=nullptr;
        for(const auto& row:stack) {
            if(!row.second) {throw std::runtime_error("Native birth stack has a null member");}
            if(stack_state(row.second).previous==-1) {if(head) {throw std::runtime_error("Native birth stack has multiple heads");}head=row.second;}
        }
        if(!stack.empty() && !head) {throw std::runtime_error("Native birth stack has no head");}
        if(head) {auto changed=stack_state(head);value.stack_next=changed.id;changed.previous=value.id;set_stack(head,changed);}
        auto candidate=std::unique_ptr<Unit>(new Unit(Unit::Unplaced{},this,static_cast<size_t>(next),def,tile,value));
        auto* born=candidate.get();
        if(!d->units.emplace(born->m_id,born).second || !stack.emplace(born->m_id,born).second
            || !d->updates.emplace(born->m_id,unit_update_t{UUO_SPAWN,born}).second) {throw std::runtime_error("Native birth publication identity conflicts");}
        d->order.push_back(born->m_id);d->records.emplace(born,value);knowledge.units.push_back({born->m_id,0,false});
        d->published.push_back(nullptr);d->candidates.push_back(std::move(candidate));
        d->factory->Adjust(design_id,static_cast<uint8_t>(design.plan),1);
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
            // Co-located positive Units receive their owner disclosure below;
            // the original constructor does not add native-observer bit0 here.
            if(position!=tile) {mark(position,0);}disclosed|=static_cast<uint8_t>(1u<<owner);
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
        if(ecology_owner) {record(born).ecology_owner_tag=*ecology_owner;}
        return value.id;
    };
    const auto by_index=[&](int32_t id)->Unit* {
        if(id<0 || size_t(id)>=d->order.size()) {throw std::runtime_error("Native factory operation has an invalid dense index");}
        auto* unit=d->units.at(d->order[id]);
        if(!unit->IsNative() || record(unit).id!=id) {throw std::runtime_error("Native factory operation requires a bound native record");}
        return unit;
    };
    factory.capacity=[&](int32_t id) {
        const auto& value=record(by_index(id));(void)NativeDesign(*m_game,value.design);
        return value.design==9 ? int32_t(value.stored_morale)+1:0;
    };
    factory.sentry=[&](int32_t id) {
        auto& value=record(by_index(id));value.order=1;value.waypoint_x[0]=-1;value.waypoint_y[0]=0;
    };
    factory.promote_stack=[&](int32_t id) {
        auto* unit=by_index(id);auto& value=record(unit);
        if(value.stack_previous<0) {return;}
        auto* head=unit;size_t traversed=0;
        while(stack_state(head).previous>=0) {
            if(++traversed>d->order.size()) {throw std::runtime_error("Native promotion encountered a cyclic stack");}
            const auto parent=stack_state(head).previous;
            if(parent<0 || size_t(parent)>=d->order.size()) {throw std::runtime_error("Native promotion lost its parent");}
            head=d->units.at(d->order[parent]);
            if(head->GetTile()!=unit->GetTile()) {throw std::runtime_error("Native promotion crosses tile boundaries");}
        }
        const auto any_index=[&](int16_t id){if(id<0 || size_t(id)>=d->order.size()) {throw std::runtime_error("Native promotion lost a neighbor");}return d->units.at(d->order[id]);};
        auto* previous=any_index(value.stack_previous);auto previous_state=stack_state(previous);previous_state.next=value.stack_next;set_stack(previous,previous_state);
        if(value.stack_next>=0) {auto* next=any_index(value.stack_next);auto state=stack_state(next);state.previous=value.stack_previous;set_stack(next,state);}
        auto head_state=stack_state(head);value.stack_previous=-1;value.stack_next=head_state.id;head_state.previous=value.id;set_stack(head,head_state);
    };
    factory.record=[&](int32_t id){return record(by_index(id));};
    factory.knowledge=[&]()->const map::SmacWorkerKnowledge::Data& {return knowledge;};
    produce(factory);
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
