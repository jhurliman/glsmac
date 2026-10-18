#include "Game.h"
#include "State.h"

#include <set>
#include <algorithm>
#include <stdexcept>
#include "base/Base.h"
#include "base/BaseManager.h"
#include "map/Map.h"
#include "map/SmacTerritoryRules.h"
#include "gc/Space.h"

namespace game::backend {

std::optional< map::SmacWorkerKnowledge > Game::PrepareBaseTerritory( const base::Base* added,
	const base::Base* removed, bool attached,std::optional<map::GlsmacTerritoryTopology>* prepared_topology ) const {
	if ( !m_map || !m_map->m_worker_knowledge ) { return {}; }
	if ( !IsRunning() || !GetGCSpace()->IsAccumulating() || !m_map->m_worker_knowledge_ready
		|| !m_map->m_territory_topology || ( added && removed ) ) {
		throw std::runtime_error( "Base territory change requires the running owning world and saved topology" );
	}
	const auto& rules = m_state->m_settings.global.GetSmacTerritoryRules();
	if ( !rules ) { throw std::runtime_error( "Base territory change lacks frozen border rules" ); }
	const auto sea = m_map->TerritorySea();
	const auto& topology = *m_map->m_territory_topology;
	if ( topology.Width() != m_map->GetWidth() || topology.Height() != m_map->GetHeight() || !topology.Wraps() ) {
		throw std::runtime_error( "Base territory topology geometry differs" );
	}
	topology.ValidateTerrain( sea );
	const auto knowledge = m_map->m_worker_knowledge->CopyData();
	for ( int32_t observer = 1; observer <= 7; ++observer ) {
		if ( knowledge.observers[observer] ) { (void)ProjectSmacWorkerKnowledge( *m_map->m_worker_knowledge, observer, attached ? added : nullptr ); }
	}
	map::smac_territory::Input input{ knowledge.width, knowledge.height, knowledge.horizontal_wrap,
		rules->BorderDistance(), {}, {} };
	input.tiles.reserve( sea.size() );
	for ( size_t i = 0; i < sea.size(); ++i ) { input.tiles.push_back( { sea[i], topology.Regions()[i] } ); }
	const auto append = [&]( const base::Base* base, bool candidate ) {
		if ( !base || base->m_game != this || !base->m_resource_context || !base->m_allocation ) {
			throw std::runtime_error( "Territory Base requires complete canonical context and allocation" );
		}
		const auto* tile = base->GetTile();
		if ( !tile || tile->tiles != m_map->GetTilesPtr() || tile->coord.x >= m_map->GetWidth()
			|| tile->coord.y >= m_map->GetHeight() || ((tile->coord.x ^ tile->coord.y)&1)
			|| m_map->GetTile(tile->coord.x,tile->coord.y) != tile ) { throw std::runtime_error( "Territory Base has no current center" ); }
		if ( candidate ) {
			if ( m_bm->GetBase(base->m_id) || (attached ? tile->base != base : tile->base != nullptr) ) {
				throw std::runtime_error( "Territory candidate center or identity is already occupied" );
			}
			m_bm->ValidateSmacResourceContext( *base->m_resource_context, base->m_owner, base->m_faction );
			base->ValidateAllocationProjection( false, attached );
		} else { (void)base->GetSmacResourceContext(); }
		input.bases.push_back( { { static_cast<int32_t>(tile->coord.x), static_cast<int32_t>(tile->coord.y) },
			base->m_resource_context->owner_simulation_id } );
	};
	std::set<size_t> seen;
	bool found_removed = !removed;
	for ( const auto id : m_bm->GetBaseRecordOrder() ) {
		const auto* base = m_bm->GetBase( id );
		if ( !base || !seen.insert(id).second ) { throw std::runtime_error( "Territory Base record order is invalid" ); }
		if ( base == removed ) { found_removed = true; continue; }
		append( base, false );
	}
	if ( seen.size() != m_bm->GetBases().size() || !found_removed ) { throw std::runtime_error( "Territory Base order is incomplete" ); }
	if ( added ) { append( added, true ); }
	auto replacement=map::smac_territory::ReplaceClaims( *m_map->m_worker_knowledge, input );
	if ( removed ) {
		auto data=replacement.CopyData();
		data.bases.erase(std::remove_if(data.bases.begin(),data.bases.end(),[&](const auto& observation){return observation.id==removed->m_id;}),data.bases.end());
		replacement=map::SmacWorkerKnowledge::Create(data);
	}
	if(prepared_topology) {
		auto next=topology;
		if(next.SiteRatings()) {
			std::vector<map::smac_site::Coordinate> centers;centers.reserve(input.bases.size());for(const auto& base:input.bases) {centers.push_back(base.coordinate);}
			auto sites=*next.SiteRatings();
			if(added) {sites=sites.FoundBase({{static_cast<int32_t>(added->GetTile()->coord.x),static_cast<int32_t>(added->GetTile()->coord.y)}});}
			if(removed) {sites=sites.RemoveBase({{static_cast<int32_t>(removed->GetTile()->coord.x),static_cast<int32_t>(removed->GetTile()->coord.y)}},centers);}
			next=m_map->PrepareSiteRatingRefill(next.WithSiteRatings(std::move(sites)),centers,GetTurnId());
		}
		next=m_map->PrepareTerritorySummary(next,replacement);
		*prepared_topology=std::move(next);
	}
	return replacement;
}

void Game::CommitBaseTerritory( std::optional< map::SmacWorkerKnowledge >& prepared ) noexcept {
	if ( prepared ) { m_map->m_worker_knowledge.swap( prepared ); }
}
void Game::CommitBaseTerritoryTopology(std::optional<map::GlsmacTerritoryTopology>& topology) noexcept {
	if(topology) {m_map->m_territory_topology.swap(topology);}
}
void Game::CommitBaseTerritory(std::optional<map::SmacWorkerKnowledge>& knowledge,std::optional<map::GlsmacTerritoryTopology>& topology) noexcept {
	CommitBaseTerritory(knowledge);CommitBaseTerritoryTopology(topology);
}
}
