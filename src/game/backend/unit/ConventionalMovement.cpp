#include "UnitManager.h"

#include "Unit.h"
#include "SmacGroundEdgeCost.h"
#include "SmacScoutKeyboardAdmission.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/Base.h"
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace game::backend::unit {
namespace {
smac_edge::Tile EdgeTile( const map::tile::Tile& tile, const base::Base* admitted_hq ) {
	using namespace map::tile;
	constexpr auto features = FEATURE_RIVER | FEATURE_XENOFUNGUS;
	constexpr auto terraforming = TERRAFORMING_ROAD | TERRAFORMING_MAG_TUBE | TERRAFORMING_FOREST;
	if ( tile.is_water_tile || ( tile.base && tile.base != admitted_hq ) || tile.rockiness < ROCKINESS_FLAT || tile.rockiness > ROCKINESS_ROCKY
		|| ( tile.features & ~features ) != 0 || ( tile.terraforming & ~terraforming ) != 0
		|| tile.coord.x > INT32_MAX || tile.coord.y > INT32_MAX ) {
		throw std::runtime_error( "Conventional movement requires admitted ordinary land without base, landmark or interaction features" );
	}
	uint32_t flags = tile.base ? smac_edge::Base : 0;
	if ( tile.features & FEATURE_RIVER ) { flags |= smac_edge::River; }
	if ( tile.features & FEATURE_XENOFUNGUS ) { flags |= smac_edge::Fungus; }
	if ( tile.terraforming & TERRAFORMING_ROAD ) { flags |= smac_edge::Road; }
	if ( tile.terraforming & TERRAFORMING_MAG_TUBE ) { flags |= smac_edge::Tube; }
	if ( tile.terraforming & TERRAFORMING_FOREST ) { flags |= smac_edge::Forest; }
	// Explicit GLSMAC land-equivalence policy. Original bands3..7 have equal
	// cost in the admitted kernel; this does not quantize port meter heights.
	return { static_cast< int32_t >( tile.coord.x ), static_cast< int32_t >( tile.coord.y ), 3,
		static_cast< int32_t >( tile.rockiness - ROCKINESS_FLAT ), flags };
}
}

UnitManager::PreparedConventionalMove UnitManager::PrepareConventionalMove( Unit* unit, map::tile::Tile* destination, size_t caller ) const {
	if ( !unit || GetUnit( unit->m_id ) != unit || !unit->IsConventional() ) {
		throw std::runtime_error( "Conventional movement requires a registered conventional unit" );
	}
	unit->RequireConventionalReady();
	const auto* state = m_game->GetState();
	const auto& local = state->m_settings.local;
	if ( !m_game->IsRunning() || state->m_connection || local.game_mode != settings::LocalSettings::GM_SINGLEPLAYER
		|| local.network_type != settings::LocalSettings::NT_NONE || local.network_role != settings::LocalSettings::NR_NONE ) {
		throw std::runtime_error( "Conventional movement supports local single-player only" );
	}
	const auto& stored = unit->Conventional();
	const auto context = ResolveConventional( stored, unit->m_owner ); // validates current binding before native owner use
	const auto stats = QueryConventionalUnit( stored, context, SmacMoraleMode::Mode1 );
	const auto& simulation = m_game->m_simulation->value;
	const auto* participant = simulation.FindSlot( caller );
	if ( !participant || simulation.roster.size() != 1 || participant->role != roster::Role::Single
		|| caller != m_game->GetSlotNum() || unit->m_owner != &state->m_slots->GetSlot( caller ) ) {
		throw std::runtime_error( "Conventional movement requires the accepted local single-player controller" );
	}
	if ( simulation.movement_capability != faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain ) {
		throw std::runtime_error( "Conventional movement requires explicit no-project ordinary-terrain campaign admission" );
	}
	auto* source = unit->GetTile();
	const auto* map = GetMap();
	const auto canonical = [ & ]( const map::tile::Tile* tile ) {
		return tile && tile->coord.x < map->GetWidth() && tile->coord.y < map->GetHeight()
			&& ( tile->coord.x & 1 ) == ( tile->coord.y & 1 ) && map->GetTile( tile->coord.x, tile->coord.y ) == tile;
	};
	if ( map->GetWidth() > INT32_MAX || map->GetHeight() > INT32_MAX || !canonical( source ) || !canonical( destination ) ) {
		throw std::runtime_error( "Conventional movement requires canonical bounded map tiles" );
	}
	using namespace smac_scout_key;
	if ( !CanAct( { unit->m_id, stored.owner_simulation_id, static_cast< int32_t >( source->coord.x ), static_cast< int32_t >( source->coord.y ),
		stored.spent, stats.full_allowance, Status::Ordinary }, participant->simulation_id,
		static_cast< int32_t >( map->GetWidth() ), static_cast< int32_t >( map->GetHeight() ) ) ) {
		throw std::runtime_error( "Conventional unit cannot act: foreign, out of map or exhausted" );
	}
	// Actionability precedes edge pricing, including a zero-cost tube edge.
	if ( source == destination || !source->IsAdjactentTo( destination ) || source->IsLocked() || destination->IsLocked()
		|| unit->m_animation_id || !destination->units.empty() ) {
		throw std::runtime_error( "Conventional movement requires an unlocked adjacent empty destination and no animation" );
	}
	const auto membership = source->units.find( unit->m_id );
	if ( membership == source->units.end() || membership->second != unit ) { throw std::runtime_error( "Conventional source membership is inconsistent" ); }
	// No combat/ZOC/stack policy is inferred. Reject any foreign native owner
	// in either endpoint or its neighborhood before invoking a movement kernel.
	for ( const auto* endpoint : { source, destination } ) {
		const auto foreign = [ & ]( const map::tile::Tile* tile ) {
			for ( const auto& occupant : tile->units ) { if ( !occupant.second || occupant.second->m_owner != unit->m_owner ) { return true; } }
			return false;
		};
		if ( foreign( endpoint ) ) { throw std::runtime_error( "Conventional movement does not admit foreign-unit interactions" ); }
		for ( const auto* neighbour : endpoint->neighbours ) {
			if ( foreign( neighbour ) ) { throw std::runtime_error( "Conventional movement does not admit possible enemy zones of control" ); }
		}
	}
	const auto design = m_game->ResolveConventionalDesign( stored, *m_game->m_simulation );
	const auto& catalog = state->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
	const auto& weapon = catalog.weapons.weapons.at( design.weapon );
	const auto& social = *simulation.social.Find( stored.owner_simulation_id );
	// The measured HQ extension is deliberately narrower than the base-free route.
	// HEADQUARTERS is the explicit port facility identity, not an English-name or
	// source-slot classification. The saved no-project capability remains required.
	const base::Base* admitted_hq = nullptr;
	const auto& bases = m_game->GetBM()->GetBases();
	if ( !bases.empty() ) {
		if ( bases.size() != 1 || m_units.size() != 1 || stored.design_id != 2 || stored.stored_morale != 1 || stored.damage != 0
			|| design.chassis != 0 || stats.quantum != 3 || stats.basic_allowance != 3 || stats.full_allowance != 3
			|| context.retained_morale != 0 || social.retained->at( 7 ) != 0 ) {
			throw std::runtime_error( "Friendly HQ movement requires one healthy neutral design-2 Infantry Scout with quantum3" );
		}
		const auto& entry = *bases.begin();
		const auto* hq = entry.second;
		const auto* definition = m_game->GetBM()->GetFacilityDef( "HEADQUARTERS" );
		if ( !hq || hq->m_id != entry.first || hq->GetGame() != m_game || hq->m_owner != unit->m_owner
			|| hq->m_faction != unit->m_owner->GetPlayer()->GetFaction() || hq->m_pops.size() != 1
			|| !definition || hq->m_facilities.size() != 1 || hq->m_facilities.begin()->first != "HEADQUARTERS"
			|| hq->m_facilities.begin()->second != definition ) {
			throw std::runtime_error( "Friendly HQ movement requires a canonical same-owner population1 HQ with no other facilities" );
		}
		const auto* center = hq->GetTile();
		const auto* outside = center == source ? destination : source;
		using namespace map::tile;
		if ( ( center != source && center != destination ) || center->base != hq || outside->base
			|| center->rockiness != ROCKINESS_FLAT || outside->rockiness != ROCKINESS_FLAT
			|| center->features != FEATURE_NONE || center->terraforming != TERRAFORMING_NONE
			|| ( outside->features != FEATURE_NONE && outside->features != FEATURE_XENOFUNGUS )
			|| ( outside->terraforming != TERRAFORMING_NONE && outside->terraforming != TERRAFORMING_ROAD && outside->terraforming != TERRAFORMING_MAG_TUBE )
			|| ( outside->features != FEATURE_NONE && outside->terraforming != TERRAFORMING_NONE ) ) {
			throw std::runtime_error( "Friendly HQ movement requires the HQ endpoint and adjacent flat ordinary road tube or fungus land" );
		}
		admitted_hq = hq;
	}
	const auto edge_source = EdgeTile( *source, admitted_hq ), edge_destination = EdgeTile( *destination, admitted_hq );
	const smac_edge::Query query{
		{ design.id, static_cast< smac_edge::Chassis >( design.chassis ), static_cast< int32_t >( catalog.chassis.chassis.at( design.chassis ).triad ),
			weapon.offense, static_cast< int32_t >( weapon.mode ), design.plan, design.abilities, stats.basic_allowance },
		stats.quantum, stored.owner_simulation_id, social.retained->at( 7 ), false, false,
		static_cast< int32_t >( map->GetWidth() ), static_cast< int32_t >( map->GetHeight() ), true, edge_source, edge_destination };
	const auto cost = smac_edge::QueryGroundEdgeCost( query );
	auto result = smac_move::ResolveEmptyGroundStep( { unit->m_id, stored.spent, stored.failed_move }, stats.quantum, stats.full_allowance,
		cost, ( edge_destination.features & smac_edge::Fungus ) != 0, simulation.gameplay_random );
	auto changed = stored;
	changed.spent = static_cast< uint8_t >( result.unit.spent ); changed.failed_move = result.unit.prior_failure_marker;
	QueryConventionalUnit( changed, context, stored.render_morale_mode );
	return { unit, source, destination, changed, std::move( result ) };
}

void UnitManager::ValidateConventionalMove( Unit* unit, map::tile::Tile* destination, size_t caller ) const {
	(void)PrepareConventionalMove( unit, destination, caller ); // owned copied RNG only
}

void UnitManager::MoveConventionalUnit( GSE_CALLABLE, Unit* unit, map::tile::Tile* destination, size_t caller ) {
	m_game->CheckRW( GSE_CALL );
	const auto prepared = PrepareConventionalMove( unit, destination, caller );
	const auto update = m_unit_updates.find( unit->m_id );
	if ( update != m_unit_updates.end() && ( update->second.unit != unit || ( update->second.ops & UUO_DESPAWN ) ) ) {
		throw std::runtime_error( "Conventional movement has a stale pending presentation record" );
	}
	// Same owner call, no callbacks/yields between preparation and commit.
	// Allocate the destination node before touching any membership or authority.
	std::map< size_t, Unit* > staged_destination;
	const bool moved = prepared.result.outcome == smac_move::Outcome::Moved;
	if ( moved ) { staged_destination.emplace( unit->m_id, unit ); }
	// try_emplace completes all update-node/bucket allocation first. With the
	// existing std::less<size_t> map, node transfer below cannot allocate or run
	// user code. Every remaining authority write is a trivial nonthrowing copy.
	auto& pending = m_unit_updates.try_emplace( unit->m_id, unit_update_t{ UUO_NONE, unit } ).first->second;
	if ( moved ) {
		destination->units.insert( staged_destination.extract( unit->m_id ) );
		prepared.source->units.erase( unit->m_id );
	}
	static_assert( std::is_nothrow_copy_assignable_v< ConventionalUnitState > );
	static_assert( std::is_nothrow_copy_assignable_v< SmacGameplayRandomState > );
	unit->CommitConventionalMove( prepared.state, moved ? destination : prepared.source );
	m_game->m_simulation->value.gameplay_random = prepared.result.random;
	pending.ops = static_cast< unit_update_op_t >( pending.ops | UUO_REFRESH );
	// PushUpdates later publishes the committed record. No animation callback
	// owns movement, no GSE callbacks execute here, and no network rollback is used.
}

void UnitManager::ResetConventionalSpent( GSE_CALLABLE, Unit* unit ) {
	m_game->CheckRW( GSE_CALL );
	if ( !unit || GetUnit( unit->m_id ) != unit || !unit->IsConventional() ) { throw std::runtime_error( "Conventional upkeep requires a registered unit" ); }
	unit->QueryStats( SmacMoraleMode::Mode1 );
	auto next = unit->Conventional();
	const auto reset = smac_move::ResetSpentForCheckedUnitUpkeep( { unit->m_id, next.spent, next.failed_move } );
	next.spent = static_cast< uint8_t >( reset.spent );
	QueueUnitUpdate( unit, UUO_REFRESH ); // allocation before byte update
	unit->CommitConventionalMove( next, unit->GetTile() );
}

}
