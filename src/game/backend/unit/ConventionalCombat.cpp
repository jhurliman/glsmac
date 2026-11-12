#include "UnitManager.h"
#include "SmacUnitFacing.h"
#include "game/ScoutBattlePresentation.h"
#include "Unit.h"
#include "Def.h"
#include "SmacScoutKeyboardAdmission.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/FacilityDef.h"
#include "gc/Space.h"
#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace game::backend::unit {

bool UnitManager::HasForeignOccupant(const Unit* actor,const map::tile::Tile* destination) const {
	if(!actor || !destination || GetUnit(actor->m_id)!=actor) {return false;}
	for(const auto& entry:destination->units) {
		const auto* resident=GetUnit(entry.first);
		if(resident && resident==entry.second && resident->m_owner!=actor->m_owner) {return true;}
	}
	return false;
}

namespace {
int32_t CombatWord(int64_t value) {
    if(value<0 || value>INT32_MAX) {throw std::overflow_error("Ground combat exceeds nonnegative signed32 domain");}
    return static_cast<int32_t>(value);
}
int32_t CombatNegate(int32_t value) {
    if(value==INT32_MIN) {throw std::overflow_error("Ground defense penalty exceeds signed32");}return -value;
}
int32_t CombatPercent(int32_t value,int32_t percent) {return CombatWord(int64_t(value)*(INT64_C(100)+percent)/100);}
struct BattleUnit {
    int32_t owner,reactor,health,stored,morale,spent,basic,full,weapon,armor,planet,design;
    ConventionalUnitContext::Package package;
};
}

UnitManager::PreparedConventionalAttack UnitManager::PrepareConventionalAttack(Unit* actor,map::tile::Tile* destination,size_t caller) const {
    if(!actor || GetUnit(actor->m_id)!=actor || (!actor->IsConventional() && !actor->IsNative())) {throw std::runtime_error("Ground attack requires a current supported actor");}
    m_game->RequireConventionalSightWorld();
    if(actor->IsConventional()) {actor->RequireConventionalReady();}
    auto* state=m_game->GetState();const auto& simulation=m_game->m_simulation->value;
    const auto* participant=simulation.FindSlot(caller);
    const bool native_actor=actor->IsNative();
    const bool controller=m_game->IsSmacControllerOwner(actor->SimulationOwner());
    if(!m_game->IsRunning() || state->GetConnection() || state->m_settings.local.game_mode!=settings::LocalSettings::GM_SINGLEPLAYER
        || state->m_settings.local.network_type!=settings::LocalSettings::NT_NONE || state->m_settings.local.network_role!=settings::LocalSettings::NR_NONE
        || (native_actor ? !controller : (!participant || !participant->active || actor->m_owner!=&state->m_slots->GetSlot(caller)
            || (!controller && (participant->role!=roster::Role::Single || caller!=m_game->GetSlotNum()))))
        || simulation.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {
        throw std::runtime_error("Ground attack requires its admitted local faction controller");
    }
    const auto* map=GetMap();auto* source=actor->GetTile();
    const auto ordinary=[&](const map::tile::Tile* tile) {
        using namespace map::tile;
        constexpr auto features=FEATURE_RIVER|FEATURE_XENOFUNGUS;
        constexpr auto improvements=TERRAFORMING_ROAD|TERRAFORMING_MAG_TUBE|TERRAFORMING_FOREST|TERRAFORMING_FARM|TERRAFORMING_MINE|TERRAFORMING_SOLAR|TERRAFORMING_SENSOR;
        return tile && tile->tiles==map->GetTilesPtr() && tile->coord.x<map->GetWidth() && tile->coord.y<map->GetHeight()
            && (tile->coord.x&1)==(tile->coord.y&1) && map->GetTile(tile->coord.x,tile->coord.y)==tile && !tile->is_water_tile && !tile->IsLocked()
            && tile->rockiness>=ROCKINESS_FLAT && tile->rockiness<=ROCKINESS_ROCKY && !(tile->features&~features) && !(tile->terraforming&~improvements);
    };
    if(!ordinary(source) || !ordinary(destination) || source==destination || !source->IsAdjactentTo(destination)
        || destination->units.empty() || actor->m_animation_id || !source->units.count(actor->m_id) || source->units.at(actor->m_id)!=actor) {
        throw std::runtime_error("Ground battle requires adjacent canonical land and a foreign defender stack");
    }
    for(const auto& row:source->units) {
        if(GetUnit(row.first)!=row.second || row.second->GetTile()!=source || row.second->SimulationOwner()!=actor->SimulationOwner()) {
            throw std::runtime_error("Ground attacker source stack is not canonical and same-owner");
        }
    }
    auto* defender=destination->units.begin()->second;
    if(!defender || GetUnit(destination->units.begin()->first)!=defender || defender->GetTile()!=destination
        || (!defender->IsConventional() && !defender->IsNative()) || defender->SimulationOwner()==actor->SimulationOwner() || defender->m_animation_id) {
        throw std::runtime_error("Ground battle requires a current foreign defender");
    }
    if(defender->IsConventional()) {defender->RequireConventionalReady();}
    auto* defending_base=destination->base;
    if(defending_base) {
        const auto& bases=m_game->GetBM()->GetBases();
        const auto found=std::find_if(bases.begin(),bases.end(),[&](const auto& row){return row.second==defending_base;});
        if(found==bases.end() || found->first!=defending_base->m_id || defending_base->GetTile()!=destination || defending_base->m_owner!=defender->m_owner) {
            throw std::runtime_error("Ground Base battle requires a current defender-owned Base");
        }
        for(const auto& row:defending_base->m_facilities) {
            if(!row.second || !row.second->m_smac_source_id) {
                throw std::runtime_error("Ground Base battle requires unfortified defense");
            }
        }
        (void)defending_base->GetSmacResourceContext();
        if(m_game->GetBM()->m_dispatching_updates) {throw std::runtime_error("Ground battle cannot reenter Base update dispatch");}
    }
    const auto& catalog=state->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
    const auto read=[&](const Unit& unit,int32_t opposing)->BattleUnit {
        if(unit.IsNative()) {
            const auto& n=unit.Native();const auto q=QueryNativeUnit(n,opposing);
            if(n.owner || n.design!=8 || n.home_base!=-1 || n.order==1) {throw std::runtime_error("Ground PSI battle requires an ordinary wild Mind Worm");}
            return {0,1,q.hit_points,n.stored_morale,q.level,n.movement_spent,q.basic_allowance,q.full_allowance,-1,-1,0,n.design,ConventionalUnitContext::Package::Combat};
        }
        const auto& n=unit.Conventional();const auto c=ResolveConventional(n,unit.m_owner);
        const auto d=m_game->ResolveConventionalDesign(n,*m_game->m_simulation);const auto q=unit.QueryStats(SmacMoraleMode::Mode1);
        if((d.chassis!=0 && d.chassis!=1) || (d.abilities&~0x4000u) || c.reactor!=1 || c.quantum!=3
            || n.stored_morale>6) {
            throw std::runtime_error("Ground battle requires ordinary Fission Infantry/Rover components");
        }
        const auto* social=simulation.social.Find(n.owner_simulation_id);
        if(!social || !social->retained) {throw std::runtime_error("Ground battle requires retained social authority");}
        return {n.owner_simulation_id,1,q.hit_points,n.stored_morale,q.effective_morale,n.spent,q.basic_allowance,q.full_allowance,
            catalog.weapons.weapons.at(d.weapon).offense,catalog.defenses.defenses.at(d.defense).rating,social->retained->at(7),d.id,c.package};
    };
    const auto a=read(*actor,defender->SimulationOwner());auto d=read(*defender,actor->SimulationOwner());
    if(a.package!=ConventionalUnitContext::Package::Combat || a.weapon==0
        || (actor->IsConventional() && (actor->Conventional().IsHeld() || actor->Conventional().work_order!=ConventionalUnitState::WorkOrder::None)) || a.full-a.spent<3 || simulation.designs.global_difficulty!=3) {
        throw std::runtime_error("Ground attack requires an ordinary combat unit with a complete movement point at Librarian");
    }
    bool psi=a.weapon<0 || d.armor<0;
    const auto& combat=*state->m_settings.global.GetSmacCombatRules();
    if(!psi && ((source->features|destination->features)&map::tile::FEATURE_XENOFUNGUS)) {throw std::runtime_error("Conventional fungus defense requires its separate rule");}
    int32_t fortification=2;
    if(defending_base) {
        bool perimeter=false,tachyon=false;
        for(const auto& row:defending_base->m_facilities) {perimeter|=*row.second->m_smac_source_id==4;tachyon|=*row.second->m_smac_source_id==5;}
        fortification=(perimeter ? 4:2)+(tachyon ? 2:0);
    }
    if(a.owner && d.owner) {simulation.hostility.RequireOrdinaryHostility(a.owner,d.owner);}
    bool fanatic=false;
    if(a.owner) {
        const auto* profile=state->m_settings.global.GetSmacFactionStartupRules()->Find(actor->m_owner->GetPlayer()->GetFaction()->m_id);
        if(!profile) {throw std::runtime_error("Ground attack has no frozen faction profile");}
        fanatic=std::any_of(profile->bonuses.begin(),profile->bonuses.end(),[](const auto& b){return b.kind==settings::SmacFactionStartupBonus::Kind::FANATIC;});
    }
    bool sensor_defense=false;
    if(d.owner>0) {
        const auto& knowledge=map->GetSmacWorkerKnowledge();
        if(!knowledge) {throw std::runtime_error("Sensor defense requires recognized current territory");}
        const auto frame=m_game->ProjectSmacWorkerKnowledge(*knowledge,d.owner);
        for(int dy=-4;dy<=4;++dy) {for(int dx=-4;dx<=4;++dx) {
            if(std::abs(dx)+std::abs(dy)>4 || ((dx^dy)&1)) {continue;}
            auto x=(int64_t(destination->coord.x)+dx+map->GetWidth())%map->GetWidth();const auto y=int64_t(destination->coord.y)+dy;
            if(y<0 || y>=int64_t(map->GetHeight())) {continue;}const auto* tile=map->GetTile(x,y);
            if(!(tile->terraforming&map::tile::TERRAFORMING_SENSOR)) {continue;}
            if(!tile->is_water_tile) {
                const auto owner=frame.Observe({static_cast<int32_t>(x),static_cast<int32_t>(y)}).recognized_owner;
                sensor_defense|=!owner || *owner==d.owner;
            } else {
                std::vector<SmacMovementHistory::BasePosition> positions;
                for(const auto& row:m_game->GetBM()->GetBases()) {positions.push_back({row.first,static_cast<int32_t>(row.second->GetTile()->coord.x),static_cast<int32_t>(row.second->GetTile()->coord.y)});}
                const auto nearest=SmacMovementHistory::NearestAnyBase(x,y,map->GetWidth(),true,positions,{});
                if(!nearest.base) {throw std::runtime_error("Sea Sensor defense requires its nearest Base authority");}
                sensor_defense|=m_game->GetBM()->GetBase(*nearest.base)->GetSmacResourceContext().owner_simulation_id==d.owner;
            }
        }}
        if(sensor_defense && !combat.ordinary_land) {throw std::runtime_error("Sensor defense requires the frozen source percentage");}
    }
    const auto conventional_bonuses=[&](const BattleUnit& candidate) {
        smac_scout::StrengthBonuses b{combat.fanatic_attack,defending_base && a.basic==3 ? combat.infantry_vs_base:0,
            defending_base ? combat.intrinsic_base_defense:0,a.weapon,candidate.armor};
        b.base_fortification_factor=defending_base ? fortification:0;
        const bool rough=destination->rockiness==map::tile::ROCKINESS_ROCKY || (destination->terraforming&map::tile::TERRAFORMING_FOREST);
        if(!defending_base) {b.terrain_defense=rough ? 50:0;}
        const bool mobile=!defending_base && !rough && a.basic>3 && candidate.basic<=a.basic;
        const bool noncombat=candidate.weapon==0 && candidate.armor==1;
        if(mobile || noncombat) {
            if(!combat.ordinary_land) {throw std::runtime_error("Ground battle requires frozen mobile/noncombat rules");}
            if(mobile) {b.mobile_attack=combat.ordinary_land->at(settings::SmacCombatRules::MobileAttack);}
        }
        return b;
    };
    const auto strengths_for=[&](const BattleUnit& candidate) {
        if(a.weapon<0 || candidate.armor<0) {
            if(!combat.psi_ratios || !combat.ordinary_land) {throw std::runtime_error("PSI battle requires frozen ratios and PLANET rules");}
            const auto ratios=combat.psi_ratios->at(0);
            auto attack=CombatWord(INT64_C(32)*ratios[0]*(6+a.morale));
            auto intermediate=CombatWord(INT64_C(8)*ratios[1]*(6+std::max(1,candidate.morale)));
            if(defending_base) {intermediate=CombatPercent(intermediate,combat.intrinsic_base_defense);}
            auto defense=CombatWord(int64_t(intermediate)*4);
            if(sensor_defense) {defense=CombatPercent(defense,combat.ordinary_land->at(settings::SmacCombatRules::SensorDefense));}
            const auto planet_bonus=int64_t(a.planet)*combat.ordinary_land->at(settings::SmacCombatRules::PlanetAttack);
            if(planet_bonus<INT32_MIN || planet_bonus>INT32_MAX) {throw std::overflow_error("PSI PLANET percentage exceeds signed32");}
            attack=CombatPercent(attack,static_cast<int32_t>(planet_bonus));
            if(candidate.weapon==0 && candidate.armor==1 && !(a.owner==0 && defending_base)) {
                defense=CombatPercent(defense,CombatNegate(combat.ordinary_land->at(settings::SmacCombatRules::NoncombatDefensePenalty)));
            }
            return std::array<int32_t,2>{attack,defense};
        }
        auto strength=smac_scout::QueryGroundStrengths({actor->m_id,1,a.health,a.stored,a.morale,a.spent},
            {defender->m_id,1,candidate.health,candidate.stored,candidate.morale,candidate.spent},fanatic,conventional_bonuses(candidate));
        if(sensor_defense) {strength[1]=CombatPercent(strength[1],combat.ordinary_land->at(settings::SmacCombatRules::SensorDefense));}
        if(candidate.weapon==0 && candidate.armor==1) {strength[1]=CombatPercent(strength[1],CombatNegate(combat.ordinary_land->at(settings::SmacCombatRules::NoncombatDefensePenalty)));}
        return strength;
    };
    std::vector<Unit*> defending_order{defender};
    if(destination->units.size()>1) {
        defending_order.clear();
        int64_t best=INT64_MIN;
        const auto reference_attack=CombatWord(INT64_C(32)*(psi ? combat.psi_ratios->at(0)[0]:a.weapon)*(6+a.morale));
        Unit* head=nullptr;
        for(const auto& row:destination->units) {
            if(!row.second->SourceStack()) {throw std::runtime_error("Defender selection requires retained source links");}
            if(row.second->SourceStack()->previous<0) {if(head) {throw std::runtime_error("Defending stack has multiple source heads");}head=row.second;}
        }
        while(head) {
            if(defending_order.size()>=destination->units.size() || head->GetTile()!=destination) {throw std::runtime_error("Defending source stack is cyclic or displaced");}
            defending_order.push_back(head);const auto next=head->SourceStack()->next;head=next<0 ? nullptr:GetUnit(m_unit_order.at(next));
        }
        if(defending_order.size()!=destination->units.size()) {throw std::runtime_error("Defending stack has disconnected source links");}
        for(auto* candidate:defending_order) {
            if(!candidate || GetUnit(candidate->m_id)!=candidate || (!candidate->IsConventional() && !candidate->IsNative()) || candidate->SimulationOwner()!=d.owner || candidate->GetTile()!=destination || candidate->m_animation_id) {
                throw std::runtime_error("Defender selection requires a current same-owner conventional stack");
            }
            const auto record=read(*candidate,a.owner);
            const auto strength=strengths_for(record);
            const auto binding=candidate->SourceStack();
            const auto position=std::find(m_unit_order.begin(),m_unit_order.end(),candidate->m_id);
            if(position==m_unit_order.end()) {throw std::runtime_error("Defender selector lost its dense record identity");}
            const int64_t source_id=binding ? binding->id:position-m_unit_order.begin();
            const auto score=smac_scout::QueryGroundDefenderScore(strength[0],strength[1],record.health,10,reference_attack,record.weapon,static_cast<int32_t>(source_id));
            if(score>best) {best=score;defender=candidate;d=record;}
        }
    }
    psi=a.weapon<0 || d.armor<0;
    auto random=simulation.gameplay_random;std::optional<uint16_t> palette;
    const auto* focused=simulation.FindSlot(m_game->GetSlotNum());
    const bool presentation=focused && (focused->simulation_id==a.owner || focused->simulation_id==d.owner);
    if(presentation && !m_game->m_smac_hit_palette_identifier) {
        uint16_t id=0;do {id=static_cast<uint16_t>((uint32_t(random.NextSharedLow16())*65535u)>>16);} while(!id);palette=id;
    }
    const auto participant_a=smac_scout::Participant(actor->m_id,a.reactor,a.health,a.stored,a.morale,a.spent);
    const auto participant_d=smac_scout::Participant(defender->m_id,d.reactor,d.health,d.stored,d.morale,d.spent);
    const smac_scout::References references{actor->m_id,defender->m_id};
    const auto noncombat=d.weapon==0 && d.armor==1;
    map::tile::Tile* escape=nullptr;
    const auto without_elite=[&](const Unit& u,const BattleUnit& b) {
        const auto damage=u.IsNative() ? u.Native().damage:u.Conventional().damage;
        return SmacConventionalAllowance({b.basic,3,SmacMovementDomain::Land,1,damage,b.morale,false,true});
    };
    if(destination->units.size()==1 && !defending_base && defender->IsConventional() && d.weapon!=0 && !defender->Conventional().IsHeld()
        && without_elite(*defender,d)>without_elite(*actor,a)) {
        const auto flags=defender->Conventional().native_flags;
        if(!flags) {throw std::runtime_error("Retreat requires retained original defender flags");}
        if(!(*flags&0x40u)) {
            const auto toward=SmacFacingForStep(*destination,*source);int best=2;
            map::tile::Tile* neighbors[]={destination->NE,destination->E,destination->SE,destination->S,destination->SW,destination->W,destination->NW,destination->N};
            for(int direction=0;direction<8;++direction) {
                auto* tile=neighbors[direction];int angle=std::abs(direction-int(toward));angle=std::min(angle,8-angle);
                if(angle<=best || !ordinary(tile) || (tile->features&map::tile::FEATURE_XENOFUNGUS)) {continue;}
                if(tile->base && tile->base->m_owner!=defender->m_owner) {continue;}
                bool friendly=true;for(const auto& row:tile->units) {friendly=friendly && row.second && row.second->m_owner==defender->m_owner;}
                if(!friendly) {continue;}best=angle;escape=tile;
            }
        }
    }

    const auto strengths=strengths_for(d);auto widths=strengths;
    if(m_game->GetTurnId()<15) {if(!a.owner) {widths[0]/=3;}if(!d.owner) {widths[1]/=2;}}
    auto result=smac_scout::ResolveStrengthAttack(participant_a,participant_d,strengths,widths,random,references,escape!=nullptr);

    auto* survivor=result.deleted_identity==actor->m_id ? defender:actor;auto* loser=survivor==actor ? defender:actor;
    ConventionalUnitState candidate;std::optional<SmacNativeUnitRecord> native;
    if(survivor->IsNative()) {
        native=survivor->Native();if(survivor==actor) {native->state|=4u;}native->damage=static_cast<uint8_t>(10-result.survivor.health);
        native->stored_morale=static_cast<uint8_t>(result.survivor.stored_morale);native->movement_spent=static_cast<uint8_t>(result.survivor.spent);
        if(survivor==actor && (source->features&map::tile::FEATURE_XENOFUNGUS) && native->damage) {--native->damage;}
    } else {
        candidate=survivor->Conventional();if(survivor==actor) {if(candidate.retained_state_word) {*candidate.retained_state_word|=4u;}candidate.facing=SmacFacingForStep(*source,*destination);}
        candidate.damage=static_cast<uint8_t>(10-result.survivor.health);candidate.stored_morale=static_cast<uint8_t>(result.survivor.stored_morale);candidate.spent=static_cast<uint8_t>(result.survivor.spent);
        (void)QueryConventionalUnit(candidate,ResolveConventional(candidate,survivor->m_owner),candidate.render_morale_mode);
    }
    std::vector<size_t> casualties;
    std::vector<std::pair<Unit*,ConventionalUnitState>> collateral;
    std::vector<std::pair<Unit*,SmacNativeUnitRecord>> native_casualties;
    if(!result.defender_retreated) {
        casualties.push_back(loser->m_id);
        if(loser->IsNative()) {auto dead=loser->Native();dead.damage=10;if(loser==actor) {dead.state|=4u;}native_casualties.emplace_back(loser,dead);}
    }
    if(!result.defender_retreated && survivor==actor && destination->units.size()>1) {
        size_t non_exempt=0;bool colony_present=false;
        for(const auto& row:destination->units) {
            const auto profile=read(*row.second,a.owner);colony_present|=profile.package==ConventionalUnitContext::Package::Colony;
            non_exempt+=profile.package!=ConventionalUnitContext::Package::Colony;
        }
        const bool protected_stack=(defending_base || colony_present) && non_exempt>1;
        if(!protected_stack) {
            size_t surviving_non_exempt=0;
            for(const auto& row:destination->units) {
                auto* member=row.second;if(member==defender) {continue;}
                const auto profile=read(*member,a.owner);if(profile.package==ConventionalUnitContext::Package::Colony) {continue;}
                if(member->IsNative()) {
                    auto state=member->Native();state.damage=static_cast<uint8_t>(state.damage+3);state.state|=4u;
                    // The source removal walk sends owner0 through its casualty
                    // handler even when collateral leaves positive health.
                    casualties.push_back(member->m_id);native_casualties.emplace_back(member,state);continue;
                }
                auto state=member->Conventional();
                if(!state.retained_state_word) {throw std::runtime_error("Collateral requires retained original activity status");}
                state.damage=static_cast<uint8_t>(state.damage+3);*state.retained_state_word|=4u;
                if(state.damage>=10) {casualties.push_back(member->m_id);}else {collateral.emplace_back(member,state);++surviving_non_exempt;}
            }
            if(!surviving_non_exempt) {
                for(const auto& row:destination->units) {
                    if(row.second!=defender && read(*row.second,a.owner).package==ConventionalUnitContext::Package::Colony) {casualties.push_back(row.first);}
                }
            }
        }
    }
    if(survivor==actor && !result.defender_retreated) {
        std::stable_sort(casualties.begin(),casualties.end(),[&](size_t left,size_t right) {
            return std::find(defending_order.begin(),defending_order.end(),GetUnit(left))<std::find(defending_order.begin(),defending_order.end(),GetUnit(right));
        });
    }
    auto history=simulation.hostility;
    if(a.owner && d.owner) {for(const auto id:casualties) {const auto* victim=GetUnit(id);
        history.RecordScoutBattle(a.owner,d.owner,survivor->SimulationOwner(),victim->SimulationOwner(),victim->Conventional().design_id);}}
    std::optional<std::pair<int32_t,int32_t>> reward;
    if(loser->IsNative() && survivor==actor && survivor->SimulationOwner()>0) {
        int64_t credits=simulation.economy.at(survivor->SimulationOwner()).credits;
        for(const auto id:casualties) {const auto* victim=GetUnit(id);if(victim->IsNative()) {credits+=10*(QueryNativeUnit(victim->Native(),0).level+1);}}
        reward={{survivor->SimulationOwner(),CombatWord(credits)}};
    }
    ConventionalUnitState retreat_state;
    if(result.defender_retreated) {retreat_state=defender->Conventional();retreat_state.damage=static_cast<uint8_t>(10-result.health_after[1]);}
    return {survivor,loser,candidate,std::move(history),std::move(result),palette,survivor==actor ? defending_base:nullptr,native,reward,
        result.defender_retreated ? escape:nullptr,retreat_state,std::move(casualties),std::move(collateral),std::move(native_casualties)};
}

void UnitManager::ApplyConventionalAttack(GSE_CALLABLE,PreparedConventionalAttack prepared) {
	if(gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating()) {throw std::runtime_error("Scout attack requires its owning accumulation");}
	auto removal=PrepareRemovalForQuery(prepared.casualties,nullptr,prepared.survivor,true);
    removal.PrepareBattleCollateral(prepared.collateral);
    for(const auto& row:prepared.native_casualties) {removal.SetNativeBattleCasualty(row.first,row.second);}
    if(prepared.result.defender_retreated) {removal.PrepareBattleRetreat(prepared.loser,prepared.retreat_destination,prepared.retreat_state);}
    if(prepared.native_survivor) {removal.SetNativeBattleSurvivor(prepared.survivor,*prepared.native_survivor);}
	if(prepared.defeated_base && !prepared.native_survivor) {removal.DiscoverDefendedBase(prepared.defeated_base,prepared.survivor_state.owner_simulation_id);}
	const auto message=prepared.result.defender_retreated ? prepared.loser->GetDisplayName()+" retreated.":prepared.survivor->GetDisplayName()+" defeated "+prepared.loser->GetDisplayName()+".";
    const auto* focused=m_game->m_simulation->value.FindSlot(m_game->GetSlotNum());
    const bool presentation=focused && (focused->simulation_id==prepared.survivor->SimulationOwner() || focused->simulation_id==prepared.loser->SimulationOwner());
    auto frontend=*m_game->m_pending_frontend_requests;
    if(presentation) {
        FrontendRequest playback(FrontendRequest::FR_SCOUT_BATTLE);
        NEW(playback.data.scout_battle.plan,scout_presentation::Plan,scout_presentation::Prepare(prepared.result));frontend.push_back(playback);
    }
	static_assert(std::is_nothrow_copy_assignable_v<SmacGameplayRandomState>);
	// All queues, sight, strings, inventory/history and survivor validation are
	// prepared. No callback or allocation between survivor and casualty authority.
	if(!prepared.native_survivor) {prepared.survivor->CommitConventionalMove(prepared.survivor_state,prepared.survivor->GetTile());}
    if(prepared.reward) {m_game->m_simulation->value.economy.at(prepared.reward->first).credits=prepared.reward->second;}
	m_game->m_simulation->value.hostility.owners.swap(prepared.history.owners);
	m_game->m_simulation->value.gameplay_random=prepared.result.random;
	if(prepared.palette_identifier) {m_game->m_smac_hit_palette_identifier=prepared.palette_identifier;}
	if(prepared.defeated_base) {prepared.defeated_base->m_resource_context->recent_defender_loss=true;prepared.defeated_base->m_resource_context->SyncKnownStatusBits();}
	removal.Commit();m_game->m_pending_frontend_requests->swap(frontend);removal.Notify(GSE_CALL);if(presentation) {m_game->Message(message);}
}

}
