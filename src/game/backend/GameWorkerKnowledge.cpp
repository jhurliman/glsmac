#include "Game.h"

#include <stdexcept>
#include <type_traits>
#include "base/Base.h"
#include "base/BaseManager.h"
#include "map/Map.h"
#include "map/tile/Tiles.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
#include "gc/Space.h"

namespace game::backend {

map::SmacWorkerMapFrame Game::ProjectSmacWorkerKnowledge( const map::SmacWorkerKnowledge& knowledge, int32_t observer ) const {
	using Frame = map::SmacWorkerMapFrame;
	ValidateCurrentSimulation();
	if ( !m_simulation || !m_map || !m_bm || !m_um || m_state->m_settings.global.GetRulesetId() != "smac" ) {
		throw std::runtime_error( "Worker knowledge requires a current canonical SMAC world" );
	}
	const auto data = knowledge.CopyData();
	bool participant = false;
	for ( const auto& record : m_simulation->value.roster ) { if ( record.simulation_id == observer ) { participant = true; break; } }
	if ( !participant ) { throw std::runtime_error( "Worker observer is not a current simulation participant" ); }
	if ( data.width != static_cast< int64_t >( m_map->GetWidth() ) || data.height != static_cast< int64_t >( m_map->GetHeight() ) || !data.horizontal_wrap ) {
		throw std::runtime_error( "Worker knowledge geometry differs from the current periodic-X map" );
	}
	for ( const auto& record : data.units ) {
		if ( record.active_convoy ) { throw std::runtime_error( "Live worker knowledge does not yet admit convoy orders" ); }
	}
	const auto coordinate = [ this ]( const map::tile::Tile* tile ) -> Frame::Coordinate {
		if ( !tile || tile->tiles != m_map->GetTilesPtr() || tile->coord.x >= m_map->GetWidth() || tile->coord.y >= m_map->GetHeight()
			|| ( ( tile->coord.x ^ tile->coord.y ) & 1 ) || m_map->GetTile( tile->coord.x, tile->coord.y ) != tile ) {
			throw std::runtime_error( "Worker entity has no current map coordinate" );
		}
		return { static_cast< int32_t >( tile->coord.x ), static_cast< int32_t >( tile->coord.y ) };
	};
	std::vector< Frame::Base > bases;
	for ( const auto& entry : m_bm->GetBases() ) {
		const auto* base = entry.second;
		if ( !base || base->m_id != entry.first || base->GetGame() != this ) { throw std::runtime_error( "Worker Base registry identity differs" ); }
		const auto position = coordinate( base->GetTile() );
		const auto context = base->GetSmacResourceContext();
		const auto allocation = base->GetSmacAllocation();
		if ( !allocation || base->GetTile()->base != base ) { throw std::runtime_error( "Worker Base lacks allocation authority or center membership" ); }
		bases.push_back( { context.owner_simulation_id, position, allocation->selected & Frame::RADIUS_MASK } );
	}
	std::vector< map::SmacWorkerKnowledge::LiveUnit > units;
	for ( const auto& entry : m_um->GetUnits() ) {
		const auto* unit = entry.second;
		if ( !unit || unit->m_id != entry.first || !unit->IsConventional() ) { throw std::runtime_error( "Worker knowledge requires canonical live units" ); }
		unit->RequireConventionalReady();
		const auto& state = unit->Conventional();
		ResolveConventionalUnit( state, unit->m_owner );
		const auto position = coordinate( unit->GetTile() );
		const auto member = unit->GetTile()->units.find( entry.first );
		if ( member == unit->GetTile()->units.end() || member->second != unit ) { throw std::runtime_error( "Worker unit is missing current tile membership" ); }
		units.push_back( { entry.first, state.owner_simulation_id, position } );
	}
	std::vector< bool > sea;
	sea.reserve( data.tiles.size() );
	// Reverse membership is checked as well: no unregistered center or stack
	// entry may quietly vanish from the derived frame.
	for ( int32_t y = 0; y < data.height; ++y ) { for ( int32_t x = y & 1; x < data.width; x += 2 ) {
		const auto* tile = m_map->GetTile( x, y );
		coordinate( tile );
		sea.push_back( tile->is_water_tile );
		if ( tile->base ) {
			bool found = false;
			for ( const auto& entry : m_bm->GetBases() ) { if ( entry.second == tile->base ) { found = entry.second->GetTile() == tile; break; } }
			if ( !found ) { throw std::runtime_error( "Worker map has an unregistered or displaced Base center" ); }
		}
		for ( const auto& entry : tile->units ) {
			const auto found = m_um->GetUnits().find( entry.first );
			if ( found == m_um->GetUnits().end() || found->second != entry.second || !entry.second || entry.second->GetTile() != tile ) {
				throw std::runtime_error( "Worker map has an unregistered or displaced unit" );
			}
		}
	} }
	return knowledge.Project( observer, sea, bases, units );
}

void Game::ValidateSmacWorkerKnowledge( const map::SmacWorkerKnowledge& knowledge ) const {
	const auto data = knowledge.CopyData();
	for ( int32_t observer = 1; observer <= 7; ++observer ) {
		if ( data.observers[ observer ] ) { (void)ProjectSmacWorkerKnowledge( knowledge, observer ); }
	}
}

void Game::PublishSmacWorkerKnowledge( GSE_CALLABLE, const map::SmacWorkerKnowledge& knowledge ) {
	CheckRW( GSE_CALL );
	if ( gc_space != GetGCSpace() || !gc_space->IsAccumulating() || !IsRunning() ) {
		throw std::runtime_error( "Worker knowledge publication requires its running owning accumulation" );
	}
	ValidateSmacWorkerKnowledge( knowledge );
	std::optional< map::SmacWorkerKnowledge > replacement = knowledge;
	static_assert( std::is_nothrow_swappable_v< decltype( replacement ) > );
	m_map->m_worker_knowledge.swap( replacement );
	m_map->m_worker_knowledge_ready = true;
}

void Game::CompleteSmacWorkerKnowledgeRestore() {
	if ( m_map && m_map->m_worker_knowledge ) {
		ValidateSmacWorkerKnowledge( *m_map->m_worker_knowledge );
		m_map->m_worker_knowledge_ready = true;
	}
}

map::SmacWorkerMapFrame::Evaluation Game::GetSmacWorkerAvailability( const base::Base& base ) const {
	if ( !m_map || !m_map->m_worker_knowledge || !m_map->m_worker_knowledge_ready ) { throw std::runtime_error( "Live worker knowledge is unavailable" ); }
	size_t index = 0;
	for ( const auto& entry : m_bm->GetBases() ) {
		if ( entry.second == &base ) { return ProjectSmacWorkerKnowledge( *m_map->m_worker_knowledge, base.GetSmacResourceContext().owner_simulation_id ).Evaluate( index ); }
		++index;
	}
	throw std::runtime_error( "Worker query Base is not registered in this world" );
}

void Game::RequireWorkerKnowledgeMutation() const {
	if ( m_map && m_map->m_worker_knowledge ) { throw std::runtime_error( "This mutation requires an integrated worker sight/territory producer" ); }
}

}
