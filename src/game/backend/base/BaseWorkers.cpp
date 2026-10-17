#include "Base.h"

#include "BaseManager.h"
#include "SmacAutomaticWorkerAllocation.h"
#include "SmacWorkedSquareCollection.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "gc/Space.h"

namespace game::backend::base {

void BaseManager::RequireSmacWorkerRecalculation() const {
	if ( !m_game->IsRunning() || m_dispatching_updates ) { THROW( "SMAC worker recalculation requires a running world outside update dispatch" ); }
}

void Base::RecalculateSmacWorkers( GSE_CALLABLE, const SmacAllocationRoles& roles ) {
	m_game->CheckRW( GSE_CALL );
	if ( gc_space != m_game->GetGCSpace() || !gc_space->IsAccumulating() ) {
		THROW( "SMAC worker recalculation requires its running owning accumulation" );
	}
	m_game->GetBM()->RequireSmacWorkerRecalculation();
	const auto context = GetSmacResourceContext(); // validates registered Base/current owner before dereference
	auto* state = m_game->GetState();
	if ( state->GetConnection() || m_owner->GetPlayer() != m_game->GetPlayer()
		|| m_owner->GetPlayer()->GetRole() != Player::PR_SINGLE || context.GovernorActive() ) {
		THROW( "SMAC worker recalculation requires the local human with governor inactive" );
	}
	const auto allocation = GetSmacAllocation();
	if ( !allocation ) { THROW( "SMAC worker recalculation requires complete saved allocation authority" ); }
	if ( m_pops.size() > 127 || m_game->GetTurnId() > INT32_MAX ) {
		THROW( "SMAC worker recalculation population/turn exceeds its signed domain" );
	}
	if ( !m_production ) { THROW( "SMAC worker recalculation requires an explicit production category" ); }
	m_production->Validate();
	SmacWorkerProduction production;
	// Scoring consumes only this saved semantic category, never a cost, design
	// lookup or eligibility. This does not validate target buildability/completion.
	switch ( m_production->current_item.kind ) {
		case ProductionItemKind::Unit: production = SmacWorkerProduction::Unit; break;
		case ProductionItemKind::Facility: production = SmacWorkerProduction::Facility; break;
		case ProductionItemKind::Stockpile: production = SmacWorkerProduction::Stockpile; break;
		default: THROW( "SMAC worker recalculation does not admit orbital/project production" );
	}
	// This query authenticates the complete current Unit/Base/map projection.
	// It retains raw selected32 on Base; only availability is taken from the frame.
	const auto availability = m_game->GetSmacWorkerAvailability( *this );
	for ( const auto& item : m_game->GetUM()->GetUnits() ) {
		const auto* unit = item.second;
		if ( !unit || unit->m_id != item.first || !unit->IsConventional()
			|| unit->Conventional().home != unit::ConventionalUnitState::Home::None ) {
			THROW( "SMAC worker recalculation requires authenticated homeless conventional units" );
		}
	}
	// The measured support gate ignores every homeless unit, independent of
	// package, cost or SUPPORT rating; no invented unit classification is needed.
	const int32_t mineral_support = 0;
	const auto& global = state->m_settings.global;
	if ( !global.GetSmacResourceRules() || !global.GetSmacSpecialistRules() || !global.GetSmacTechnologyRules() ) {
		THROW( "SMAC worker recalculation requires frozen resource/specialist/technology rules" );
	}
	const auto catalog = ResolveSmacSpecialistAllocationCatalog( *global.GetSmacSpecialistRules(), *global.GetSmacTechnologyRules(),
		m_game->GetBM()->GetSmacSpecialistKnowledge( *this ) );
	const auto prefix = m_game->GetCanonicalOrbitalIntake( *this );
	const auto* map = m_game->GetMap();
	if ( map->GetWidth() > INT32_MAX || map->GetHeight() > INT32_MAX ) { THROW( "Unsupported worker radius geometry" ); }
	const SmacWorkerRadiusMap radius{ { static_cast< int32_t >( m_tile->coord.x ), static_cast< int32_t >( m_tile->coord.y ) },
		static_cast< int32_t >( map->GetWidth() ), static_cast< int32_t >( map->GetHeight() ), true };
	const SmacWorkerYieldQuery query = [ & ]( int32_t index, const SmacWorkerStepState& ) -> std::optional< SmacWorkerTileYield > {
		const auto coordinates = SmacWorkedSquareCollection::Resolve( radius, index );
		if ( !coordinates ) { return std::nullopt; }
		const auto* tile = map->GetTile( ( *coordinates )[ 0 ], ( *coordinates )[ 1 ] );
		const auto values = m_game->GetCanonicalTileResources( *tile, *this, Game::ResourceQueryPhase::Ordinary );
		SmacWorkerTileYield result{ {}, tile->is_water_tile };
		for ( size_t i = 0; i < values.size(); ++i ) {
			if ( values[ i ] < INT32_MIN || values[ i ] > INT32_MAX ) { THROW( "Worker tile yield exceeds signed32" ); }
			result.resources[ i ] = static_cast< int32_t >( values[ i ] );
		}
		return result;
	};
	// No yield callback publishes a provisional assignment. The ordinary resource
	// subset reads the actual immutable context, not scratch intake/visited bits.
	const auto result = SmacAutomaticWorkerAllocation::Allocate( {
		static_cast< int32_t >( m_pops.size() ), static_cast< int32_t >( m_game->GetTurnId() ),
		global.GetSmacResourceRules()->nutrients_per_citizen, mineral_support, production,
		{ true, false, false, false, false, false, {} }, context.ManageCitizens(), false,
		allocation->selected, availability.unavailable, prefix, allocation->specialists, catalog, std::nullopt
	}, query );
	// Existing publication prepares reverse assignments/Pop projections/queues
	// before one nonthrowing commit. No cache, turn, RNG or knowledge is changed.
	PublishSmacAllocation( GSE_CALL, { result.workers.visited, result.specialists }, roles );
}

}
