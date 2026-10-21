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

UnitManager::PreparedConventionalAttack UnitManager::PrepareConventionalAttack(Unit* actor,map::tile::Tile* destination,size_t caller) const {
	if(!actor || GetUnit(actor->m_id)!=actor || !actor->IsConventional()) {throw std::runtime_error("Scout attack requires a current conventional actor");}
	m_game->RequireConventionalSightWorld();actor->RequireConventionalReady();
	auto* state=m_game->GetState();
	const auto& simulation=m_game->m_simulation->value;
	const auto* participant=simulation.FindSlot(caller);
	if(!m_game->IsRunning() || state->GetConnection() || state->m_settings.local.game_mode!=settings::LocalSettings::GM_SINGLEPLAYER
		|| state->m_settings.local.network_type!=settings::LocalSettings::NT_NONE || state->m_settings.local.network_role!=settings::LocalSettings::NR_NONE
		|| !participant || !participant->active || participant->role!=roster::Role::Single || caller!=m_game->GetSlotNum()
		|| actor->m_owner!=&state->m_slots->GetSlot(caller) || simulation.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {
		throw std::runtime_error("Scout attack requires the admitted local human controller and ordinary no-project campaign");
	}
	const auto* map=GetMap();auto* source=actor->GetTile();
	const auto ordinary=[&](const map::tile::Tile* tile) {
		return tile && tile->tiles==map->GetTilesPtr() && tile->coord.x<map->GetWidth() && tile->coord.y<map->GetHeight()
			&& (tile->coord.x & 1)==(tile->coord.y & 1)
			&& map->GetTile(tile->coord.x,tile->coord.y)==tile && !tile->is_water_tile && !tile->IsLocked()
			&& tile->rockiness==map::tile::ROCKINESS_FLAT && tile->features==map::tile::FEATURE_NONE && tile->terraforming==map::tile::TERRAFORMING_NONE;
	};
	if(!ordinary(source) || !ordinary(destination) || source->base || source==destination || !source->IsAdjactentTo(destination)
		|| source->units.size()!=1 || source->units.begin()->first!=actor->m_id || source->units.begin()->second!=actor
		|| destination->units.size()!=1 || actor->m_animation_id) {throw std::runtime_error("Scout battle requires adjacent ordinary flat singleton squares without source Base or animation");}
	const auto target=destination->units.begin();auto* defender=GetUnit(target->first);
	if(!defender || defender!=target->second || defender->m_id!=target->first || defender->GetTile()!=destination || !defender->IsConventional()
		|| defender->m_owner==actor->m_owner || defender->m_animation_id) {throw std::runtime_error("Scout battle requires a current foreign singleton defender");}
	defender->RequireConventionalReady();m_game->RequireSmacLocalHumanOwner(actor->m_owner);m_game->RequireSmacLocalHumanOwner(defender->m_owner);
	auto* defending_base=destination->base;
	if(defending_base) {
		const auto& bases=m_game->GetBM()->GetBases();
		const auto found=std::find_if(bases.begin(),bases.end(),[&](const auto& row){return row.second==defending_base;});
		if(found==bases.end() || found->first!=defending_base->m_id || defending_base->GetTile()!=destination
			|| defending_base->m_owner!=defender->m_owner) {throw std::runtime_error("Scout Base battle requires a current defender-owned Base");}
		for(const auto& row:defending_base->m_facilities) {
			const auto* facility=row.second;
			if(!facility || !facility->m_smac_source_id || *facility->m_smac_source_id==4 || *facility->m_smac_source_id==5) {
				throw std::runtime_error("Scout Base battle requires the unfortified defense rule");
			}
		}
		(void)defending_base->GetSmacResourceContext();
		if(m_game->GetBM()->m_dispatching_updates) {throw std::runtime_error("Scout Base battle cannot reenter Base update dispatch");}
	}
	if(actor->Conventional().IsHeld()) {throw std::runtime_error("Activate held attacker before directional combat");}
	const auto& rules=*state->m_settings.global.GetSmacPredefinedUnitRules();
	const auto admit=[&](const Unit& unit) {
		const auto& record=unit.Conventional();const auto context=ResolveConventional(record,unit.m_owner);
		const auto design=m_game->ResolveConventionalDesign(record,*m_game->m_simulation);
		const auto& weapon=rules.catalog.weapons.weapons.at(design.weapon);
		const auto& armor=rules.catalog.defenses.defenses.at(design.defense);
		if(context.package!=ConventionalUnitContext::Package::Combat || context.chassis!=settings::SmacMovementRules::GroundChassis::Infantry
			|| design.abilities || context.reactor!=1 || weapon.offense!=1 || armor.rating!=1 || context.quantum!=3
			|| record.work_order!=ConventionalUnitState::WorkOrder::None || record.stored_morale>6
			|| simulation.designs.global_difficulty!=3 || SmacStartupDesignState::OriginalDifficulty(unit.m_owner->GetPlayer()->GetDifficultyLevel())!=3) {
			throw std::runtime_error("Flat Scout battle requires ordinary Gun/Scout Infantry/Fission components at Librarian");
		}
		return unit.QueryStats(SmacMoraleMode::Mode1);
	};
	const auto attack_stats=admit(*actor),defend_stats=admit(*defender);
	if(attack_stats.remaining_allowance<3 || (attack_stats.full_allowance!=3 && attack_stats.full_allowance!=6)) {
		throw std::runtime_error("Partial-movement Scout attack requires its confirmation continuation");
	}
	const auto actor_owner=actor->Conventional().owner_simulation_id,defender_owner=defender->Conventional().owner_simulation_id;
	simulation.hostility.RequireOrdinaryHostility(actor_owner,defender_owner);
	// FANATIC is an uncapped faction flag, independent of the eight-record list.
	const auto* profile=state->m_settings.global.GetSmacFactionStartupRules()->Find(actor->m_owner->GetPlayer()->GetFaction()->m_id);
	if(!profile) {throw std::runtime_error("Scout attack has no frozen faction profile");}
	const bool fanatic=std::any_of(profile->bonuses.begin(),profile->bonuses.end(),[](const auto& entry){return entry.kind==settings::SmacFactionStartupBonus::Kind::FANATIC;});
	auto random=simulation.gameplay_random;std::optional<uint16_t> palette;
	if(!m_game->m_smac_hit_palette_identifier) {
		// Original lazy hit-palette constructor: nonzero shared identifier. Its
		// engine/process lifetime is separate from campaign serialization/reset.
		uint16_t identifier=0;
		do {identifier=static_cast<uint16_t>((uint32_t(random.NextSharedLow16())*65535u)>>16);} while(!identifier);
		palette=identifier;
	}
	const auto& a=actor->Conventional();const auto& d=defender->Conventional();
	const auto& combat=*state->m_settings.global.GetSmacCombatRules();
	const smac_scout::StrengthBonuses bonuses{combat.fanatic_attack,defending_base ? combat.infantry_vs_base : 0,
		defending_base ? combat.intrinsic_base_defense : 0};
	auto result=smac_scout::ResolveFlatScoutAttack({actor->m_id,1,attack_stats.hit_points,a.stored_morale,attack_stats.effective_morale,a.spent},
		{defender->m_id,1,defend_stats.hit_points,d.stored_morale,defend_stats.effective_morale,d.spent},attack_stats.full_allowance,random,{actor->m_id,defender->m_id},fanatic,bonuses);
	auto* survivor=result.deleted_identity==actor->m_id ? defender : actor;auto* loser=survivor==actor ? defender : actor;
	auto candidate=survivor->Conventional();
    if(survivor==actor) {candidate.facing=SmacFacingForStep(*source,*destination);}
    candidate.damage=static_cast<uint8_t>(10-result.survivor.health);
	candidate.stored_morale=static_cast<uint8_t>(result.survivor.stored_morale);candidate.spent=static_cast<uint8_t>(result.survivor.spent);
	(void)QueryConventionalUnit(candidate,ResolveConventional(candidate,survivor->m_owner),candidate.render_morale_mode);
	auto history=simulation.hostility;history.RecordScoutBattle(actor_owner,defender_owner,survivor->Conventional().owner_simulation_id,
		loser->Conventional().owner_simulation_id,loser->Conventional().design_id);
	return {survivor,loser,candidate,std::move(history),std::move(result),palette,survivor==actor ? defending_base : nullptr};
}

void UnitManager::ApplyConventionalAttack(GSE_CALLABLE,PreparedConventionalAttack prepared) {
	if(gc_space!=m_game->GetGCSpace() || !gc_space->IsAccumulating()) {throw std::runtime_error("Scout attack requires its owning accumulation");}
	auto removal=PrepareConventionalRemoval(GSE_CALL,{prepared.loser->m_id},nullptr,prepared.survivor);
	if(prepared.defeated_base) {removal.DiscoverDefendedBase(prepared.defeated_base,prepared.survivor_state.owner_simulation_id);}
	const auto message=prepared.survivor->GetDisplayName()+" defeated "+prepared.loser->GetDisplayName()+".";
    FrontendRequest playback(FrontendRequest::FR_SCOUT_BATTLE);
    NEW(playback.data.scout_battle.plan,scout_presentation::Plan,scout_presentation::Prepare(prepared.result));
    auto frontend=*m_game->m_pending_frontend_requests;frontend.push_back(playback);
	static_assert(std::is_nothrow_copy_assignable_v<SmacGameplayRandomState>);
	// All queues, sight, strings, inventory/history and survivor validation are
	// prepared. No callback or allocation between survivor and casualty authority.
	prepared.survivor->CommitConventionalMove(prepared.survivor_state,prepared.survivor->GetTile());
	m_game->m_simulation->value.hostility.owners.swap(prepared.history.owners);
	m_game->m_simulation->value.gameplay_random=prepared.result.random;
	if(prepared.palette_identifier) {m_game->m_smac_hit_palette_identifier=prepared.palette_identifier;}
	if(prepared.defeated_base) {prepared.defeated_base->m_resource_context->recent_defender_loss=true;prepared.defeated_base->m_resource_context->SyncKnownStatusBits();}
	removal.Commit();m_game->m_pending_frontend_requests->swap(frontend);removal.Notify(GSE_CALL);m_game->Message(message);
}

}
