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
        constexpr auto improvements=TERRAFORMING_ROAD|TERRAFORMING_MAG_TUBE|TERRAFORMING_FOREST|TERRAFORMING_FARM|TERRAFORMING_MINE|TERRAFORMING_SOLAR;
        return tile && tile->tiles==map->GetTilesPtr() && tile->coord.x<map->GetWidth() && tile->coord.y<map->GetHeight()
            && (tile->coord.x&1)==(tile->coord.y&1) && map->GetTile(tile->coord.x,tile->coord.y)==tile && !tile->is_water_tile && !tile->IsLocked()
            && tile->rockiness>=ROCKINESS_FLAT && tile->rockiness<=ROCKINESS_ROCKY && !(tile->features&~features) && !(tile->terraforming&~improvements);
    };
    if(!ordinary(source) || !ordinary(destination) || source==destination || !source->IsAdjactentTo(destination)
        || destination->units.size()!=1 || actor->m_animation_id || !source->units.count(actor->m_id) || source->units.at(actor->m_id)!=actor) {
        throw std::runtime_error("Ground battle requires adjacent canonical land and a singleton defender");
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
            if(!row.second || !row.second->m_smac_source_id || *row.second->m_smac_source_id==4 || *row.second->m_smac_source_id==5) {
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
    const auto a=read(*actor,defender->SimulationOwner()),d=read(*defender,actor->SimulationOwner());
    if(a.package!=ConventionalUnitContext::Package::Combat || a.weapon==0
        || (actor->IsConventional() && (actor->Conventional().IsHeld() || actor->Conventional().work_order!=ConventionalUnitState::WorkOrder::None)) || a.full-a.spent<3 || simulation.designs.global_difficulty!=3) {
        throw std::runtime_error("Ground attack requires an ordinary combat unit with a complete movement point at Librarian");
    }
    const bool psi=a.weapon<0 || d.armor<0;
    const auto& combat=*state->m_settings.global.GetSmacCombatRules();
    if(!psi && ((source->features|destination->features)&map::tile::FEATURE_XENOFUNGUS)) {throw std::runtime_error("Conventional fungus defense requires its separate rule");}
    if(psi && (source->base || defending_base)) {throw std::runtime_error("PSI Base defense requires its separate rule");}
    if(a.owner && d.owner) {simulation.hostility.RequireOrdinaryHostility(a.owner,d.owner);}
    bool fanatic=false;
    if(a.owner) {
        const auto* profile=state->m_settings.global.GetSmacFactionStartupRules()->Find(actor->m_owner->GetPlayer()->GetFaction()->m_id);
        if(!profile) {throw std::runtime_error("Ground attack has no frozen faction profile");}
        fanatic=std::any_of(profile->bonuses.begin(),profile->bonuses.end(),[](const auto& b){return b.kind==settings::SmacFactionStartupBonus::Kind::FANATIC;});
    }
    auto random=simulation.gameplay_random;std::optional<uint16_t> palette;
    if(!m_game->m_smac_hit_palette_identifier) {
        uint16_t id=0;do {id=static_cast<uint16_t>((uint32_t(random.NextSharedLow16())*65535u)>>16);} while(!id);palette=id;
    }
    const auto participant_a=smac_scout::Participant(actor->m_id,a.reactor,a.health,a.stored,a.morale,a.spent);
    const auto participant_d=smac_scout::Participant(defender->m_id,d.reactor,d.health,d.stored,d.morale,d.spent);
    const smac_scout::References references{actor->m_id,defender->m_id};
    const auto noncombat=d.weapon==0 && d.armor==1;
    auto result=[&]() {
        if(psi) {
            if(!combat.psi_ratios || !combat.ordinary_land) {throw std::runtime_error("PSI battle requires frozen ratios and PLANET rules");}
            // Supported stock factions have no PSI keyword; the frozen faction
            // parser rejects unsupported scalar tags rather than defaulting one.
            const auto ratios=combat.psi_ratios->at(0);
            auto attack=CombatWord(INT64_C(32)*ratios[0]*(6+a.morale));
            auto defense=CombatWord(INT64_C(32)*ratios[1]*(6+std::max(1,d.morale)));
            const auto planet_bonus=int64_t(a.planet)*combat.ordinary_land->at(settings::SmacCombatRules::PlanetAttack);
            if(planet_bonus<INT32_MIN || planet_bonus>INT32_MAX) {throw std::overflow_error("PSI PLANET percentage exceeds signed32");}
            attack=CombatPercent(attack,static_cast<int32_t>(planet_bonus));
            if(noncombat) {defense=CombatPercent(defense,-combat.ordinary_land->at(settings::SmacCombatRules::NoncombatDefensePenalty));}
            std::array<int32_t,2> widths{attack,defense};
            if(m_game->GetTurnId()<15) {if(!a.owner) {widths[0]/=3;}if(!d.owner) {widths[1]/=2;}}
            return smac_scout::ResolveStrengthAttack(participant_a,participant_d,{attack,defense},widths,random,references);
        }
        smac_scout::StrengthBonuses b{combat.fanatic_attack,defending_base && a.basic==3 ? combat.infantry_vs_base:0,
            defending_base ? combat.intrinsic_base_defense:0,a.weapon,d.armor};
        const bool rough=destination->rockiness==map::tile::ROCKINESS_ROCKY || (destination->terraforming&map::tile::TERRAFORMING_FOREST);
        if(!defending_base) {b.terrain_defense=rough ? 50:0;}
        const bool mobile=!defending_base && !rough && a.basic>3 && d.basic<=a.basic;
        if(mobile || noncombat) {
            if(!combat.ordinary_land) {throw std::runtime_error("Ground battle requires frozen mobile/noncombat rules");}
            if(mobile) {b.mobile_attack=combat.ordinary_land->at(settings::SmacCombatRules::MobileAttack);}
            if(noncombat) {b.noncombat_defense=-combat.ordinary_land->at(settings::SmacCombatRules::NoncombatDefensePenalty);}
        }
        return smac_scout::ResolveGroundAttack(participant_a,participant_d,a.full,random,references,fanatic,b);
    }();
    auto* survivor=result.deleted_identity==actor->m_id ? defender:actor;auto* loser=survivor==actor ? defender:actor;
    ConventionalUnitState candidate;std::optional<SmacNativeUnitRecord> native;
    if(survivor->IsNative()) {
        native=survivor->Native();native->damage=static_cast<uint8_t>(10-result.survivor.health);
        native->stored_morale=static_cast<uint8_t>(result.survivor.stored_morale);native->movement_spent=static_cast<uint8_t>(result.survivor.spent);
        if(survivor==actor && (source->features&map::tile::FEATURE_XENOFUNGUS) && native->damage) {--native->damage;}
    } else {
        candidate=survivor->Conventional();if(survivor==actor) {candidate.facing=SmacFacingForStep(*source,*destination);}
        candidate.damage=static_cast<uint8_t>(10-result.survivor.health);candidate.stored_morale=static_cast<uint8_t>(result.survivor.stored_morale);candidate.spent=static_cast<uint8_t>(result.survivor.spent);
        (void)QueryConventionalUnit(candidate,ResolveConventional(candidate,survivor->m_owner),candidate.render_morale_mode);
    }
    auto history=simulation.hostility;
    if(a.owner && d.owner) {history.RecordScoutBattle(a.owner,d.owner,survivor->SimulationOwner(),loser->SimulationOwner(),loser->Conventional().design_id);}
    std::optional<std::pair<int32_t,int32_t>> reward;
    if(loser->IsNative() && survivor==actor && survivor->SimulationOwner()>0) {
        const auto credits=CombatWord(int64_t(simulation.economy.at(survivor->SimulationOwner()).credits)+10*(QueryNativeUnit(loser->Native(),0).level+1));
        reward={{survivor->SimulationOwner(),credits}};
    }
    return {survivor,loser,candidate,std::move(history),std::move(result),palette,survivor==actor ? defending_base:nullptr,native,reward};
}

void UnitManager::ApplyConventionalAttack(GSE_CALLABLE,PreparedConventionalAttack prepared) {
	if(gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating()) {throw std::runtime_error("Scout attack requires its owning accumulation");}
	auto removal=PrepareRemovalForQuery({prepared.loser->m_id},nullptr,prepared.survivor,true);
    if(prepared.native_survivor) {removal.SetNativeBattleSurvivor(prepared.survivor,*prepared.native_survivor);}
	if(prepared.defeated_base) {removal.DiscoverDefendedBase(prepared.defeated_base,prepared.survivor_state.owner_simulation_id);}
	const auto message=prepared.survivor->GetDisplayName()+" defeated "+prepared.loser->GetDisplayName()+".";
    FrontendRequest playback(FrontendRequest::FR_SCOUT_BATTLE);
    NEW(playback.data.scout_battle.plan,scout_presentation::Plan,scout_presentation::Prepare(prepared.result));
    auto frontend=*m_game->m_pending_frontend_requests;frontend.push_back(playback);
	static_assert(std::is_nothrow_copy_assignable_v<SmacGameplayRandomState>);
	// All queues, sight, strings, inventory/history and survivor validation are
	// prepared. No callback or allocation between survivor and casualty authority.
	if(!prepared.native_survivor) {prepared.survivor->CommitConventionalMove(prepared.survivor_state,prepared.survivor->GetTile());}
    if(prepared.reward) {m_game->m_simulation->value.economy.at(prepared.reward->first).credits=prepared.reward->second;}
	m_game->m_simulation->value.hostility.owners.swap(prepared.history.owners);
	m_game->m_simulation->value.gameplay_random=prepared.result.random;
	if(prepared.palette_identifier) {m_game->m_smac_hit_palette_identifier=prepared.palette_identifier;}
	if(prepared.defeated_base) {prepared.defeated_base->m_resource_context->recent_defender_loss=true;prepared.defeated_base->m_resource_context->SyncKnownStatusBits();}
	removal.Commit();m_game->m_pending_frontend_requests->swap(frontend);removal.Notify(GSE_CALL);m_game->Message(message);
}

}
