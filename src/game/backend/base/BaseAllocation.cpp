#include "Base.h"

#include <limits>
#include <set>
#include <type_traits>
#include "BaseManager.h"
#include "FacilityDef.h"
#include "PopDef.h"
#include "SmacWorkedSquareCollection.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"
#include "gc/Space.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slot.h"

namespace game::backend::base {

std::vector< map::tile::coords_t > Base::AllocationCoordinates( const SmacBaseAllocationState& allocation, std::optional< size_t > population ) const {
	allocation.Validate();
	if ( !m_resource_context || allocation.specialists.total_count > 16 ) {
		THROW( "Live SMAC allocation requires canonical context and at most sixteen specialists" );
	}
	const auto* tiles = m_game->GetMap()->GetTilesPtr();
	if ( !m_tile || m_tile->tiles != tiles || tiles->GetWidth() > INT32_MAX || tiles->GetHeight() > INT32_MAX ) {
		THROW( "SMAC allocation has no supported current map" );
	}
	// The current native Tiles topology wraps horizontally, never vertically.
	const SmacWorkerRadiusMap radius{ { static_cast< int32_t >( m_tile->coord.x ), static_cast< int32_t >( m_tile->coord.y ) },
		static_cast< int32_t >( tiles->GetWidth() ), static_cast< int32_t >( tiles->GetHeight() ), true };
	std::vector< map::tile::coords_t > coordinates;
	coordinates.reserve( 20 );
	std::set< std::pair< size_t, size_t > > unique{ { m_tile->coord.x, m_tile->coord.y } };
	for ( int32_t index = 1; index <= 20; ++index ) {
		if ( !( allocation.selected & ( uint32_t( 1 ) << index ) ) ) { continue; }
		const auto resolved = SmacWorkedSquareCollection::Resolve( radius, index );
		if ( !resolved || !unique.emplace( ( *resolved )[ 0 ], ( *resolved )[ 1 ] ).second ) {
			THROW( "Live SMAC allocation requires valid unaliased worker coordinates" );
		}
		if ( m_game->GetMap()->GetTile( ( *resolved )[ 0 ], ( *resolved )[ 1 ] )->base ) {
			THROW( "SMAC allocation worker cannot occupy another Base center" );
		}
		coordinates.emplace_back( ( *resolved )[ 0 ], ( *resolved )[ 1 ] );
	}
	// Center and high bits remain exact storage, not additional Pop positions.
	if ( coordinates.size() + static_cast< size_t >( allocation.specialists.total_count ) != population.value_or( m_pops.size() ) ) {
		THROW( "SMAC allocation projection does not match the existing population" );
	}
	return coordinates;
}

void Base::ValidateAllocationProjection( bool registered, bool attached ) const {
	if ( !m_allocation ) { return; }
	const auto coordinates = AllocationCoordinates( *m_allocation );
	const auto* bm = m_game->GetBM();
	size_t position = 0;
	for ( const auto& item : m_pops ) {
		const auto& pop = item.second;
		if ( item.first != pop.m_id || ( registered || attached ? pop.m_base != this : pop.m_base != nullptr ) ) {
			THROW( "SMAC allocation population identity is invalid" );
		}
		ValidatePopulationRole( pop.m_def, pop.m_worked_tile );
		if ( position < coordinates.size() ) {
			if ( !pop.m_worked_tile || *pop.m_worked_tile != coordinates[ position ] ) { THROW( "SMAC allocation worker projection differs" ); }
		}
		else if ( pop.m_worked_tile || pop.m_def->m_smac_specialist_id != m_allocation->specialists.types[ position - coordinates.size() ] ) {
			THROW( "SMAC allocation specialist projection differs" );
		}
		++position;
	}
	if ( registered ) {
		size_t indexed = 0;
		for ( const auto& claim : bm->m_work_assignments ) {
			if ( claim.second.first != m_id ) { continue; }
			const auto found = m_pops.find( claim.second.second );
			if ( found == m_pops.end() || !found->second.m_worked_tile
				|| claim.first != BaseManager::tile_key_t( found->second.m_worked_tile->x, found->second.m_worked_tile->y ) ) {
				THROW( "SMAC allocation reverse assignment differs" );
			}
			++indexed;
		}
		if ( indexed != coordinates.size() ) { THROW( "SMAC allocation reverse assignment is incomplete" ); }
	}
}

void Base::ValidateSmacAllocation() const {
	if ( !m_allocation ) { return; }
	GetSmacResourceContext();
}

std::optional< SmacBaseAllocationState > Base::GetSmacAllocation() const {
	ValidateSmacAllocation();
	return m_allocation;
}

void Base::RequireGenericPopulationMutation() const {
	if ( m_allocation ) { THROW( "Canonical allocation requires complete native publication" ); }
}

void Base::PublishSmacAllocation( GSE_CALLABLE, const SmacBaseAllocationState& allocation, const SmacAllocationRoles& roles ) {
	PublishSmacWorkerState( GSE_CALL, allocation, roles, GetSmacResourceContext() );
}

struct Base::PreparedSmacWorkerState::Data {
	Base* base;
	BaseManager* manager;
	struct Projection { Pop* pop; const PopDef* def; Pop::worked_tile_t coordinates; };
	std::vector< Projection > projection;
	pops_t additions;
	std::vector< size_t > removals;
	std::vector< pops_t::node_type > retired;
	size_t next_pop_id;
	bool committed = false;
	std::map< BaseManager::tile_key_t, BaseManager::pop_key_t > assignments;
	std::unordered_map< size_t, BaseManager::base_update_t > updates;
	std::unordered_set< Base* > triggers;
	std::unique_lock< std::mutex > lock;
	std::optional< SmacBaseAllocationState > allocation;
	std::optional< SmacBaseResourceContext > context;
	std::optional< ProductionState > production;
	int64_t nutrients;
	std::vector<std::pair<Base*,facilities_t>> facility_changes;
	slot::Slot* captured_owner=nullptr;
};
Base::PreparedSmacWorkerState::PreparedSmacWorkerState( std::unique_ptr< Data > data ) : m_data( std::move( data ) ) {}
Base::PreparedSmacWorkerState::~PreparedSmacWorkerState() = default;
Base::PreparedSmacWorkerState::PreparedSmacWorkerState( PreparedSmacWorkerState&& ) noexcept = default;
void Base::PreparedSmacWorkerState::Commit() noexcept {
	if ( !m_data || m_data->committed ) { return; }
	auto& data = *m_data;
	for ( const auto& next : data.projection ) { next.pop->m_def = next.def; next.pop->m_worked_tile = next.coordinates; }
	while ( !data.additions.empty() ) { data.base->m_pops.insert( data.additions.extract( data.additions.begin() ) ); }
	for ( const auto id : data.removals ) { data.retired.push_back( data.base->m_pops.extract( id ) ); }
	data.base->m_next_pop_id = data.next_pop_id;
	if(data.captured_owner) {data.base->m_owner=data.captured_owner;data.base->m_faction=data.captured_owner->GetPlayer()->GetFaction();}
	data.base->m_allocation.swap( data.allocation );
	data.base->m_resource_context.swap( data.context );
	data.base->m_production.swap( data.production );
	data.base->m_accumulated_nutrients = data.nutrients;
	static_assert(std::is_nothrow_swappable_v<facilities_t>);
	for(auto& change:data.facility_changes) {change.first->m_facilities.swap(change.second);}
	data.manager->m_work_assignments.swap( data.assignments );
	data.manager->m_base_updates.swap( data.updates );
	data.manager->m_updated_bases.swap( data.triggers );
	data.committed = true;
}
void Base::PreparedSmacWorkerState::Finish() {
	if ( !m_data || !m_data->committed ) { THROW( "Prepared Base state has not committed" ); }
	m_data->lock.unlock();
	m_data.reset(); // removed Pop wrappers retire only after all authority is coherent
}

void Base::PublishSmacWorkerState( GSE_CALLABLE, const SmacBaseAllocationState& allocation, const SmacAllocationRoles& roles, const SmacBaseResourceContext& context ) {
	auto prepared = PrepareSmacWorkerState( GSE_CALL, allocation, roles, context, m_production, m_accumulated_nutrients );
	prepared.Commit(); prepared.Finish();
}

Base::PreparedSmacWorkerState Base::PrepareSmacWorkerState( GSE_CALLABLE, const SmacBaseAllocationState& allocation,
	const SmacAllocationRoles& roles, const SmacBaseResourceContext& context,
	const std::optional< ProductionState >& production, int64_t nutrients, std::optional< size_t > candidate_population, std::vector<std::pair<Base*,facilities_t>> facility_changes, slot::Slot* captured_owner, std::vector<Base*> refresh_only, bool finish_upkeep ) {
	const auto population = candidate_population.value_or( m_pops.size() );
	if ( population > 127 ) { THROW( "Candidate population exceeds 127" ); }
	m_game->CheckRW( GSE_CALL );
	if ( gc_space != m_game->GetGCSpace() || !gc_space->IsAccumulating() ) { THROW( "SMAC allocation publication requires its owning accumulation" ); }
	auto* bm = m_game->GetBM();
	if ( bm->m_dispatching_updates ) { THROW( "SMAC allocation publication cannot reenter update dispatch" ); }
	const auto prior_context = GetSmacResourceContext();
	const auto expected_context=finish_upkeep ? prior_context.AfterUpkeep() : prior_context;
	context.Validate();
	auto unchanged_context = context; unchanged_context.governor_options = prior_context.governor_options;
	if(captured_owner) {
		m_game->RequireSmacLocalHumanOwner(captured_owner);
		bm->ValidateSmacResourceContext(context,captured_owner,captured_owner->GetPlayer()->GetFaction());
		if(finish_upkeep || captured_owner==m_owner || prior_context.conquest || !context.conquest || context.conquest->former_owner!=prior_context.owner_simulation_id
			|| context.conquest->assimilation!=50 || population+1!=m_pops.size()) {THROW("Capture publication requires its first ordinary population-loss transition");}
		unchanged_context.owner_simulation_id=prior_context.owner_simulation_id;unchanged_context.conquest=prior_context.conquest;
		if(!(unchanged_context==prior_context)) {THROW("Capture publication changed unrelated Base context");}
	} else if ( !( unchanged_context == expected_context ) || ( context.governor_options != prior_context.governor_options
		&& context.governor_options != ( prior_context.governor_options & ~SmacBaseResourceContext::MANAGE_CITIZENS ) ) ) {
		THROW( "SMAC worker publication may only clear citizen management in Base context" );
	}
	ValidateAllocationProjection( true );
	const auto coordinates = AllocationCoordinates( allocation, population );
	std::array< const PopDef*, 7 > specialists{};
	const auto* worker = bm->GetPopDef( roles.worker );
	if ( !worker || !( worker->m_flags & PopDef::PF_TILE_WORKER ) || worker->m_smac_specialist_id ) { THROW( "SMAC allocation requires an explicit registered worker role" ); }
	for ( size_t i = 0; i < specialists.size(); ++i ) {
		specialists[ i ] = bm->GetPopDef( roles.specialists[ i ] );
		if ( !specialists[ i ] || specialists[ i ]->m_smac_specialist_id != i || ( specialists[ i ]->m_flags & PopDef::PF_TILE_WORKER ) ) {
			THROW( "SMAC allocation requires seven explicit registered positional specialist roles" );
		}
		bm->ValidateSmacPopDef( specialists[ i ] );
	}
	std::vector< PreparedSmacWorkerState::Data::Projection > projection;
	projection.reserve( population );
	pops_t additions; std::vector< size_t > removals;
	auto next_pop_id = m_next_pop_id;
	if ( population > m_pops.size() ) {
		if ( ( !m_pops.empty() && next_pop_id <= m_pops.rbegin()->first ) || next_pop_id > INT64_MAX - ( population - m_pops.size() ) ) { THROW( "Canonical growth Pop IDs are unavailable" ); }
		for ( size_t i = m_pops.size(); i < population; ++i ) {
			// Variant zero is an explicit GLSMAC presentation policy for a fresh Pop.
			additions.emplace( std::piecewise_construct, std::forward_as_tuple( next_pop_id ), std::forward_as_tuple( this, next_pop_id, worker, 0 ) ); ++next_pop_id;
		}
	}
	auto assignments = bm->m_work_assignments;
	// Validate the existing derived index even during explicit first adoption.
	for ( auto it = assignments.begin(); it != assignments.end(); ) {
		if ( it->second.first != m_id ) { ++it; continue; }
		const auto pop = m_pops.find( it->second.second );
		if ( pop == m_pops.end() || !pop->second.m_worked_tile || it->first != BaseManager::tile_key_t( pop->second.m_worked_tile->x, pop->second.m_worked_tile->y ) ) {
			THROW( "Existing population reverse assignment differs" );
		}
		it = assignments.erase( it );
	}
	size_t position = 0;
	const auto append = [ & ]( Pop& pop ) {
		if ( position < coordinates.size() ) {
			const auto& coord = coordinates[ position ];
			if ( !assignments.emplace( BaseManager::tile_key_t( coord.x, coord.y ), BaseManager::pop_key_t( m_id, pop.m_id ) ).second ) {
				THROW( "SMAC allocation worker is claimed by another Base" );
			}
			projection.push_back( { &pop, worker, coord } );
		}
		else { projection.push_back( { &pop, specialists[ allocation.specialists.types[ position - coordinates.size() ] ], {} } ); }
		++position;
	};
	for ( auto& item : m_pops ) {
		auto& pop = item.second;
		if ( pop.m_base != this || pop.m_id != item.first ) { THROW( "Existing population identity is invalid" ); }
		ValidatePopulationRole( pop.m_def, pop.m_worked_tile );
		if ( pop.m_worked_tile ) {
			const auto claim = bm->m_work_assignments.find( { pop.m_worked_tile->x, pop.m_worked_tile->y } );
			if ( claim == bm->m_work_assignments.end() || claim->second != BaseManager::pop_key_t( m_id, pop.m_id ) ) { THROW( "Existing population reverse assignment is incomplete" ); }
		}
		if ( position < population ) { append( pop ); } else { removals.push_back( pop.m_id ); }
	}
	for ( auto& item : additions ) { append( item.second ); }
	// Prepare both native queues before publishing any authority or Pop fields.
	// No script, yield query, queue dispatch, or allocator callback runs here.
	auto updates = bm->m_base_updates;
	auto& update = updates.try_emplace( m_id, BaseManager::base_update_t{ BaseManager::BUO_NONE, this } ).first->second;
	if ( update.base != this || ( update.ops & BaseManager::BUO_DESPAWN ) ) { THROW( "SMAC allocation has an incompatible pending Base update" ); }
	update.ops = static_cast< BaseManager::base_update_op_t >( update.ops | BaseManager::BUO_REFRESH );
	for(auto* related:refresh_only) {
		if(!related || bm->GetBase(related->m_id)!=related) {THROW("Related capture Base is not current");}
		BaseManager::QueueBaseUpdateInto(updates,related,BaseManager::BUO_REFRESH);
	}
	std::set<size_t> changed;
	for(const auto& change:facility_changes) {
		if(!change.first || bm->GetBase(change.first->m_id)!=change.first || change.first->m_owner!=m_owner || !changed.insert(change.first->m_id).second) {
			THROW("Joined facility publication requires distinct current same-owner Bases");
		}
		change.first->GetSmacResourceContext();
		for(const auto& entry:change.second) {
			if(!entry.second || bm->GetFacilityDef(entry.first)!=entry.second || entry.second->m_id!=entry.first) {THROW("Joined facility publication has an invalid definition");}
			bm->ValidateSmacFacilityDef(entry.second);
		}
		BaseManager::QueueBaseUpdateInto(updates,change.first,BaseManager::BUO_REFRESH);
	}
	std::unique_lock lock( bm->m_updated_bases_mutex );
	auto triggers = bm->m_updated_bases;
	triggers.insert( this );
	for(const auto& change:facility_changes) {triggers.insert(change.first);}
	std::optional< SmacBaseAllocationState > replacement = allocation;
	std::optional< SmacBaseResourceContext > replacement_context = context;
	static_assert( std::is_nothrow_swappable_v< decltype( replacement_context ) > );
	// Vec2's scalar copy operations cannot throw; optional's copy assignment
	// itself lacks a noexcept declaration in the supported C++17 library.
	static_assert( std::is_nothrow_copy_constructible_v< map::tile::coords_t > );
	static_assert( std::is_nothrow_copy_assignable_v< map::tile::coords_t > );
	static_assert( std::is_nothrow_swappable_v< decltype( replacement ) > );
	if ( production ) { m_game->ValidateSmacProductionState( captured_owner ? captured_owner : m_owner, *production ); }
	auto data = std::make_unique< PreparedSmacWorkerState::Data >();
	data->base = this; data->manager = bm;
	data->projection = std::move( projection ); data->additions = std::move( additions ); data->removals = std::move( removals );
	data->retired.reserve( data->removals.size() ); data->next_pop_id = next_pop_id; data->assignments = std::move( assignments );
	data->updates = std::move( updates ); data->triggers = std::move( triggers ); data->lock = std::move( lock );
	data->allocation = allocation; data->context = context; data->production = production; data->nutrients = nutrients;
	data->facility_changes=std::move(facility_changes);
	data->captured_owner=captured_owner;
	static_assert( std::is_nothrow_swappable_v< decltype( data->production ) > );
	return PreparedSmacWorkerState( std::move( data ) );
}

}
