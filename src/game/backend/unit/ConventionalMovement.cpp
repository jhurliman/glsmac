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
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace game::backend::unit {
namespace {
bool ForeignBase(const Unit* actor,const map::tile::Tile* tile,const base::BaseManager& manager) {
	if(!actor || !tile || !tile->base) {return false;}
	for(const auto& row:manager.GetBases()) {if(row.second==tile->base) {return row.second->m_owner!=actor->m_owner;}}
	return false;
}
smac_edge::Tile EdgeTile( const map::tile::Tile& tile, const base::Base* admitted_base ) {
	using namespace map::tile;
	constexpr auto features = FEATURE_RIVER | FEATURE_XENOFUNGUS;
	constexpr auto terraforming = TERRAFORMING_ROAD | TERRAFORMING_MAG_TUBE | TERRAFORMING_FOREST
		| TERRAFORMING_FARM | TERRAFORMING_SOLAR | TERRAFORMING_MINE;
	if ( tile.is_water_tile || ( tile.base && tile.base != admitted_base ) || tile.rockiness < ROCKINESS_FLAT || tile.rockiness > ROCKINESS_ROCKY
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
	m_game->RequireConventionalSightWorld();
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
	// Active participants and the local controller are distinct. This port
	// route uses the accepted local participant, never an inferred faction slot.
	const auto* participant = simulation.FindSlot( caller );
	if ( !participant || !participant->active || participant->role != roster::Role::Single
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
		|| unit->m_animation_id ) {
		throw std::runtime_error( "Conventional movement requires an unlocked adjacent destination and no animation" );
	}
	for (const auto& entry:destination->units) {
		const auto* other=entry.second;
		if(!other || GetUnit(entry.first)!=other || other->m_id!=entry.first || other->GetTile()!=destination
			|| !other->IsConventional() || other->m_owner!=unit->m_owner || other->Conventional().owner_simulation_id!=stored.owner_simulation_id) {
			throw std::runtime_error("Conventional occupied entry requires a canonical same-owner stack; hostile entry needs combat");
		}
		other->QueryStats(other->Conventional().render_morale_mode);
	}
	if(!destination->units.empty() && (destination->features & map::tile::FEATURE_XENOFUNGUS)) {
		throw std::runtime_error("Occupied fungus entry requires its distinct exemption and encounter-history continuation");
	}
	const auto membership = source->units.find( unit->m_id );
	if ( membership == source->units.end() || membership->second != unit ) { throw std::runtime_error( "Conventional movement requires exact source membership" ); }
	for ( const auto& entry : source->units ) {
		const auto* other = entry.second;
		if ( !other || GetUnit( entry.first ) != other || other->m_id != entry.first || other->GetTile() != source
			|| !other->IsConventional() || other->m_owner != unit->m_owner || other->Conventional().owner_simulation_id != stored.owner_simulation_id ) {
			throw std::runtime_error( "Conventional departure requires a canonical same-owner stack" );
		}
		other->QueryStats( other->Conventional().render_morale_mode );
	}
	const auto design = m_game->ResolveConventionalDesign( stored, *m_game->m_simulation );
	const auto& catalog = state->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
	const auto& weapon = catalog.weapons.weapons.at( design.weapon );
	const auto& social = *simulation.social.Find( stored.owner_simulation_id );
	if ( context.package != ConventionalUnitContext::Package::Combat
		&& ( ( source->features | destination->features ) & map::tile::FEATURE_XENOFUNGUS ) ) {
		throw std::runtime_error( "Noncombat runtime fungus encounter admission is not established" );
	}
	// Owned Bases impose no extra ordinary nonfungus terrain or population rule.
	// Fungus still uses the narrower, independently admitted endpoint-HQ path.
	const base::Base* admitted_hq = nullptr;
	const base::Base* admitted_source_base = nullptr;
	const base::Base* admitted_destination_base = nullptr;
	const auto& bases = m_game->GetBM()->GetBases();
	if ( !bases.empty() ) {
		const bool infantry = design.chassis == 0 && stats.basic_allowance == 3;
		// This context is produced only after resolving the canonical Spartan
		// participant and its actual selected owned design, never slot arithmetic.
		const bool selected_rover = context.presentation == ConventionalUnitContext::Presentation::SelectedSpartanRover
			&& design.chassis == 1 && stats.basic_allowance == 6;
		// Damage and effective morale have already produced full_allowance.
		// Base proximity adds no second health or stored-morale admission gate.
		if ( stats.quantum != 3 || ( !infantry && !selected_rover ) ) {
			throw std::runtime_error( "Movement with Bases requires admitted Infantry or selected Spartan Rover with quantum3" );
		}
		if ( !( ( source->features | destination->features ) & map::tile::FEATURE_XENOFUNGUS ) ) {
			for ( const auto& entry : bases ) {
				const auto* base = entry.second;
				const auto* center = base ? base->GetTile() : nullptr;
				if ( !base || base->m_id != entry.first || base->GetGame() != m_game || !canonical( center ) || center->base != base ) {
					throw std::runtime_error( "Ordinary movement requires canonical registered Base centers" );
				}
				if ( center == source || center == destination || source->IsAdjactentTo( center ) || destination->IsAdjactentTo( center ) ) {
					if ( base->m_owner != unit->m_owner || base->m_faction != unit->m_owner->GetPlayer()->GetFaction() ) {
						throw std::runtime_error( "Ordinary movement does not admit foreign Base interactions" );
					}
				}
				if ( center == source ) { admitted_source_base = base; }
				if ( center == destination ) { admitted_destination_base = base; }
			}
		} else if ( !source->base && !destination->base ) {
			throw std::runtime_error( "Fungus movement with Bases requires an admitted endpoint HQ" );
		} else {
			// Select by endpoint identity, not global count or first record. Locate
			// the borrowed pointer in the canonical manager before dereferencing it.
			const auto* endpoint_base = source->base ? source->base : destination->base;
			const auto found = std::find_if( bases.begin(), bases.end(), [ & ]( const auto& entry ) { return entry.second == endpoint_base; } );
			if ( !endpoint_base || found == bases.end() ) { throw std::runtime_error( "Friendly HQ movement requires a registered endpoint HQ" ); }
			const auto& entry = *found;
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
			// An endpoint HQ is at distance0/1 from this edge. The original fungus
			// branch consumes nearest-any-base distance <=1, not the chosen base ID;
			// another base winning an equal-distance tie cannot invalidate it.
			admitted_hq = hq;
		}
	}
	if ( auto refusal=m_game->PrepareConventionalZoc(*unit,destination) ) {
		return {unit,source,destination,stored,std::nullopt,std::nullopt,false,std::move(refusal)};
	}
	// Without an endpoint HQ the original nearest-base result may retain prior state,
	// and destination fungus can enter the unsupported native-life/CRT encounter path.
	if ( !admitted_hq && ( destination->features & map::tile::FEATURE_XENOFUNGUS ) ) {
		throw std::runtime_error( "Conventional destination fungus requires admitted nearest-base context" );
	}
	std::optional< int32_t > nearest_distance;
	const bool update_nearest = simulation.movement_history && ( destination->features & map::tile::FEATURE_XENOFUNGUS );
	if ( update_nearest ) {
		const auto& history = *simulation.movement_history;
		history.ValidateDimensions( map->GetWidth(), map->GetHeight() );
		if ( bases.size() > map->GetWidth() * map->GetHeight() / 2 ) { throw std::runtime_error( "Too many canonical nearest-base records" ); }
		std::vector< SmacMovementHistory::BasePosition > positions;
		positions.reserve( bases.size() );
		for ( const auto& entry : bases ) {
			const auto* base = entry.second;
			const auto* center = base ? base->GetTile() : nullptr;
			if ( !base || base->m_id != entry.first || base->GetGame() != m_game || !canonical( center ) || center->base != base ) {
				throw std::runtime_error( "Nearest-base history requires canonical registered Base centers" );
			}
			positions.push_back( { entry.first, static_cast< int32_t >( center->coord.x ), static_cast< int32_t >( center->coord.y ) } );
		}
		nearest_distance = SmacMovementHistory::NearestAnyBase( destination->coord.x, destination->coord.y,
			static_cast< int32_t >( map->GetWidth() ), true, positions, history.NearestDistance() ).distance;
		// This is only the existing endpoint-HQ bypass. Distant encounter entry
		// remains unavailable; no CRT roll or native insertion is activated.
		if ( !nearest_distance || *nearest_distance > 1 ) { throw std::runtime_error( "Admitted HQ does not establish the fungus encounter bypass" ); }
	}
	const auto edge_source = EdgeTile( *source, admitted_hq ? admitted_hq : admitted_source_base );
	const auto edge_destination = EdgeTile( *destination, admitted_hq ? admitted_hq : admitted_destination_base );
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
	auto sight = result.outcome == smac_move::Outcome::Moved
		? m_game->PrepareConventionalSight(unit->m_id,stored.owner_simulation_id,destination,source) : std::nullopt;
	return { unit, source, destination, changed, std::move( result ), nearest_distance, update_nearest, std::move(sight) };
}

void UnitManager::ValidateConventionalMove( Unit* unit, map::tile::Tile* destination, size_t caller ) const {
	m_game->RequireSmacGameplayActive();
	if(HasForeignOccupant(unit,destination)) {(void)PrepareConventionalAttack(unit,destination,caller);return;}
	if(ForeignBase(unit,destination,*m_game->GetBM())) {(void)m_game->GetBM()->PrepareSmacCapture(unit,destination,caller);return;}
	(void)PrepareConventionalMove( unit, destination, caller ); // owned copied RNG only
}

void UnitManager::MoveConventionalUnit( GSE_CALLABLE, Unit* unit, map::tile::Tile* destination, size_t caller ) {
	m_game->RequireSmacGameplayActive();
	m_game->CheckRW( GSE_CALL );
	if(HasForeignOccupant(unit,destination)) {ApplyConventionalAttack(GSE_CALL,PrepareConventionalAttack(unit,destination,caller));return;}
	if(ForeignBase(unit,destination,*m_game->GetBM())) {m_game->GetBM()->ApplySmacCapture(GSE_CALL,m_game->GetBM()->PrepareSmacCapture(unit,destination,caller));return;}
	auto prepared = PrepareConventionalMove( unit, destination, caller );
	if ( !prepared.result ) {
		m_game->CommitConventionalSight(prepared.sight); return;
	}
	const auto placement = PreparePlacementHistory( *unit, destination );
	const auto update = m_unit_updates.find( unit->m_id );
	if ( update != m_unit_updates.end() && ( update->second.unit != unit || ( update->second.ops & UUO_DESPAWN ) ) ) {
		throw std::runtime_error( "Conventional movement has a stale pending presentation record" );
	}
	// Same owner call, no callbacks/yields between preparation and commit.
	// Allocate the destination node before touching any membership or authority.
	std::map< size_t, Unit* > staged_destination;
	const bool moved = prepared.result->outcome == smac_move::Outcome::Moved;
	if ( moved ) { staged_destination.emplace( unit->m_id, unit ); }
	// try_emplace completes all update-node/bucket allocation first. With the
	// existing std::less<size_t> map, node transfer below cannot allocate or run
	// user code. Every remaining authority write is a trivial nonthrowing copy.
	auto& pending = m_unit_updates.try_emplace( unit->m_id, unit_update_t{ UUO_NONE, unit } ).first->second;
	// An accepted failed partial attempt also retains the caller query result.
	// Validation above was side-effect free; no unsupported request gets here.
	if ( prepared.update_nearest ) { m_game->m_simulation->value.movement_history->RetainNearest( prepared.nearest_distance ); }
	if ( moved ) {
		destination->units.insert( staged_destination.extract( unit->m_id ) );
		prepared.source->units.erase( unit->m_id );
		if ( placement.first ) { placement.first->MarkPlaced( placement.second ); }
	}
	static_assert( std::is_nothrow_copy_assignable_v< ConventionalUnitState > );
	static_assert( std::is_nothrow_copy_assignable_v< SmacGameplayRandomState > );
	unit->CommitConventionalMove( prepared.state, moved ? destination : prepared.source );
	m_game->m_simulation->value.gameplay_random = prepared.result->random;
	m_game->CommitConventionalSight(prepared.sight);
	pending.ops = static_cast< unit_update_op_t >( pending.ops | UUO_REFRESH );
	// PushUpdates later publishes the committed record. No animation callback
	// owns movement, no GSE callbacks execute here, and no network rollback is used.
}

void UnitManager::ResetConventionalSpent( GSE_CALLABLE, Unit* unit ) {
	m_game->CheckRW( GSE_CALL );
	if ( !unit || GetUnit( unit->m_id ) != unit || !unit->IsConventional() ) { throw std::runtime_error( "Conventional upkeep requires a registered unit" ); }
	m_game->RequireSmacProductionOwner(unit->m_owner);
	unit->QueryStats( SmacMoraleMode::Mode1 );
	auto next = unit->Conventional();
	const auto reset = smac_move::ResetSpentForCheckedUnitUpkeep( { unit->m_id, next.spent, next.failed_move } );
	next.spent = static_cast< uint8_t >( reset.spent );
	QueueUnitUpdate( unit, UUO_REFRESH ); // allocation before byte update
	unit->CommitConventionalMove( next, unit->GetTile() );
}

}
