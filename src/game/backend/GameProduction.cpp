#include "Game.h"

#include "Player.h"
#include "slot/Slot.h"
#include "slot/Slots.h"
#include "base/Base.h"
#include "map/Map.h"
#include "map/tile/Tile.h"
#include "resource/SmacMineralCostFactor.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend {

void Game::RequireSmacLocalHumanOwner( const slot::Slot* owner ) const {
	const auto& participant = RequireSmacProductionOwner( owner );
	if ( !m_player || m_state->GetConnection() || m_player->GetRole() != Player::PR_SINGLE
		|| ( participant.role != roster::Role::Single && participant.role != roster::Role::Player ) ) {
		throw std::runtime_error( "Canonical calculation requires the explicit local human-controlled roster" );
	}
}

const faction::SmacSimulationParticipant& Game::RequireSmacProductionOwner( const slot::Slot* owner ) const {
	ValidateCurrentSimulation();
	if ( !m_simulation || !owner ) { throw std::runtime_error( "Canonical production requires a bound owner" ); }
	for ( const auto& participant : m_simulation->value.roster ) {
		if ( &m_state->m_slots->GetSlot( participant.slot ) == owner ) { return participant; }
	}
	throw std::runtime_error( "Canonical production owner is not an admitted participant" );
}

base::ProductionItem Game::MakeSmacProductionItem( const slot::Slot* owner, const base::ProductionItemKind kind, const int32_t source_id ) const {
	const auto& participant = RequireSmacProductionOwner( owner );
	base::ProductionItem item{ kind, kind == base::ProductionItemKind::Stockpile ? "" : "smac:" + std::to_string( static_cast< int64_t >( kind ) ) + ":" + std::to_string( source_id ), source_id };
	item.Validate();
	const auto& global = m_state->m_settings.global;
	if ( !global.GetSmacProductionRules() || !global.GetSmacPredefinedUnitRules() ) { throw std::runtime_error( "Canonical production catalogs are unavailable" ); }
	if ( kind == base::ProductionItemKind::Unit ) {
		(void)ResolveConventionalDesign( { source_id, participant.simulation_id, 1, 0, 0, false,
			unit::ConventionalUnitState::Home::None, unit::SmacMoraleMode::Mode0 }, *m_simulation );
	}
	else if ( global.GetSmacProductionRules()->facilities.at( source_id - 1 ).source_id != source_id ) {
		throw std::runtime_error( "Canonical production source identity mismatch" );
	}
	return item;
}

void Game::ValidateSmacProductionState( const slot::Slot* owner, const base::ProductionState& state ) const {
	state.Validate();
	if ( !state.queue ) { return; } // explicitly unresolved legacy record
	const auto check = [ & ]( const base::ProductionItem& item ) {
		(void)MakeSmacProductionItem( owner, item.kind, *item.smac_source_id );
	};
	check( state.current_item ); check( state.baseline_item );
	for ( const auto& item : *state.queue ) { check( item ); }
}

bool Game::HasSmacProductionPrerequisite( const slot::Slot* owner, const base::ProductionItem& item ) const {
	item.Validate();
	if ( !item.smac_source_id ) { throw std::runtime_error( "Production prerequisite requires an explicit source identity" ); }
	const auto& participant = RequireSmacProductionOwner( owner );
	(void)MakeSmacProductionItem( owner, item.kind, *item.smac_source_id );
	const auto& global = m_state->m_settings.global;
	const auto& technologies = *global.GetSmacTechnologyRules();
	settings::SmacTechnologyReference reference;
	if ( item.kind == base::ProductionItemKind::Unit ) {
		if ( *item.smac_source_id >= 64 ) { return true; } // own custom designs skip predefined prerequisite
		const auto& units = global.GetSmacPredefinedUnitRules()->catalog.units;
		const auto row = std::find_if( units.begin(), units.end(), [ & ]( const auto& row ) { return row.source_id == static_cast< uint32_t >( *item.smac_source_id ); } );
		if ( row == units.end() ) { throw std::runtime_error( "Predefined production source is unavailable" ); }
		reference = row->prerequisite;
	}
	else { reference = technologies.ResolveToken( global.GetSmacProductionRules()->facilities.at( *item.smac_source_id - 1 ).prerequisite ); }
	using Kind = settings::SmacTechnologyReference::Kind;
	if ( reference.kind == Kind::NONE ) { return true; }
	if ( reference.kind == Kind::DISABLED ) { return false; }
	if ( reference.kind == Kind::UNKNOWN || !reference.technology_id ) { throw std::runtime_error( "Production prerequisite token is unresolved" ); }
	const auto id = *reference.technology_id;
	if ( id < 0 || id >= 88 ) { return false; }
	const auto* owned = m_simulation->value.technology.Find( participant.simulation_id );
	if ( !owned ) { throw std::runtime_error( "Production technology ownership is unavailable" ); }
	if ( !owned->owned[ id ] ) { return false; }
	const auto& definition = technologies.technologies[ id ];
	const auto first = technologies.ResolveToken( definition.prerequisites[ 0 ] ).kind;
	const auto second = technologies.ResolveToken( definition.prerequisites[ 1 ] ).kind;
	if ( first == Kind::UNKNOWN || second == Kind::UNKNOWN ) { throw std::runtime_error( "Production technology definition is unresolved" ); }
	return first != Kind::DISABLED && ( second != Kind::DISABLED || first == Kind::NONE );
}

bool Game::IsSmacUnitProductionEligible( const base::Base& base, const base::ProductionItem& item ) const {
	base.GetSmacResourceContext();
	if ( base.GetGame() != this || item.kind != base::ProductionItemKind::Unit || !item.smac_source_id ) {
		throw std::runtime_error( "Queue candidate is not an owned unit production target" );
	}
	const auto& participant = RequireSmacProductionOwner( base.m_owner );
	(void)MakeSmacProductionItem( base.m_owner, item.kind, *item.smac_source_id );
	const auto design = ResolveConventionalDesign( { *item.smac_source_id, participant.simulation_id, 1, 0, 0, false,
		unit::ConventionalUnitState::Home::None, unit::SmacMoraleMode::Mode0 }, *m_simulation );
	if ( !design.active || ( design.faction_exclusion & ( uint32_t( 1 ) << participant.simulation_id ) ) ) { return false; }
	if ( !HasSmacProductionPrerequisite( base.m_owner, item ) ) { return false; }
	const auto& catalog = m_state->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
	if ( catalog.weapons.weapons.at( design.weapon ).mode == loader::txt::WeaponMode::Colonist ) {
		throw std::runtime_error( "Colony queue admission requires the owned no-colony rule" );
	}
	if ( catalog.chassis.chassis.at( design.chassis ).triad == loader::txt::ChassisTriad::Sea ) {
		const auto* map = GetMap();
		static constexpr int offsets[ 8 ][ 2 ] = { { 1,-1 }, { 2,0 }, { 1,1 }, { 0,2 }, { -1,1 }, { -2,0 }, { -1,-1 }, { 0,-2 } };
		for ( const auto& offset : offsets ) {
			int64_t x = int64_t( base.GetTile()->coord.x ) + offset[ 0 ], y = int64_t( base.GetTile()->coord.y ) + offset[ 1 ];
			// Current native map topology is horizontal wrap, vertical clipping.
			if ( x < 0 ) { x += map->GetWidth(); } else if ( x >= int64_t( map->GetWidth() ) ) { x -= map->GetWidth(); }
			if ( x >= 0 && x < int64_t( map->GetWidth() ) && y >= 0 && y < int64_t( map->GetHeight() )
				&& map->GetTile( x, y )->is_water_tile ) { return true; }
		}
		return false;
	}
	return true;
}

int32_t Game::GetSmacProductionCost( const slot::Slot* owner, const base::ProductionItem& item ) const {
	item.Validate();
	if ( !item.smac_source_id ) { throw std::runtime_error( "Production cost requires an explicit source identity" ); }
	const auto& participant = RequireSmacProductionOwner( owner );
	(void)MakeSmacProductionItem( owner, item.kind, *item.smac_source_id );
	RequireSmacLocalHumanOwner( owner );
	const auto* social = m_simulation->value.social.Find( participant.simulation_id );
	if ( !social ) { throw std::runtime_error( "Production social context is unavailable" ); }
	if ( social->phase == faction::SmacFactionSocialState::Phase::Early ) { RequireConventionalStartupClaim(); }
	const auto& global = m_state->m_settings.global;
	const auto& selection = global.map.GetSmacWorldSize();
	if ( !selection ) { throw std::runtime_error( "Production requires accepted world-size category" ); }
	selection->Validate();
	using namespace resource::smac;
	const auto size = selection->category == 0 ? MineralCostWorldSize::Tiny : selection->category == 1 ? MineralCostWorldSize::Small : MineralCostWorldSize::StandardOrLarger;
	// The checked human branch does not consume leader/dominance/network values.
	const int32_t factor = CalculateMineralCostFactor( { true, m_simulation->value.designs.global_difficulty,
		false, false, false, size, global.GetSmacProductionRules()->mineral_multiplier, social->current[ 9 ] } );
	int32_t cached_cost;
	if ( item.kind == base::ProductionItemKind::Unit ) {
		const auto design = ResolveConventionalDesign( { *item.smac_source_id, participant.simulation_id, 1, 0, 0, false,
			unit::ConventionalUnitState::Home::None, unit::SmacMoraleMode::Mode0 }, *m_simulation );
		if ( *item.smac_source_id >= 64 && !design.prototyped ) { throw std::runtime_error( "Unprototyped custom production cost is not admitted" ); }
		cached_cost = design.cost;
	}
	else { cached_cost = global.GetSmacProductionRules()->facilities.at( *item.smac_source_id - 1 ).cost; }
	const int64_t result = int64_t( cached_cost ) * factor;
	if ( cached_cost < 0 || result < 0 || result > INT32_MAX ) { throw std::overflow_error( "Production cost exceeds admitted signed32 range" ); }
	return static_cast< int32_t >( result );
}

}
