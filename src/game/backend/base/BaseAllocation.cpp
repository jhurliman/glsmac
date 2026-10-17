#include "Base.h"

#include <limits>
#include <set>
#include <type_traits>
#include "BaseManager.h"
#include "PopDef.h"
#include "SmacWorkedSquareCollection.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"
#include "gc/Space.h"

namespace game::backend::base {

std::vector< map::tile::coords_t > Base::AllocationCoordinates( const SmacBaseAllocationState& allocation ) const {
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
	if ( coordinates.size() + static_cast< size_t >( allocation.specialists.total_count ) != m_pops.size() ) {
		THROW( "SMAC allocation projection does not match the existing population" );
	}
	return coordinates;
}

void Base::ValidateAllocationProjection( bool registered ) const {
	if ( !m_allocation ) { return; }
	const auto coordinates = AllocationCoordinates( *m_allocation );
	const auto* bm = m_game->GetBM();
	size_t position = 0;
	for ( const auto& item : m_pops ) {
		const auto& pop = item.second;
		if ( item.first != pop.m_id || ( registered ? pop.m_base != this : pop.m_base != nullptr ) ) {
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
	m_game->CheckRW( GSE_CALL );
	if ( gc_space != m_game->GetGCSpace() || !gc_space->IsAccumulating() ) { THROW( "SMAC allocation publication requires its owning accumulation" ); }
	auto* bm = m_game->GetBM();
	if ( bm->m_dispatching_updates ) { THROW( "SMAC allocation publication cannot reenter update dispatch" ); }
	GetSmacResourceContext();
	ValidateAllocationProjection( true );
	const auto coordinates = AllocationCoordinates( allocation );
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
	struct Projection { Pop* pop; const PopDef* def; Pop::worked_tile_t coordinates; };
	std::vector< Projection > projection;
	projection.reserve( m_pops.size() );
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
	for ( auto& item : m_pops ) {
		auto& pop = item.second;
		if ( pop.m_base != this || pop.m_id != item.first ) { THROW( "Existing population identity is invalid" ); }
		ValidatePopulationRole( pop.m_def, pop.m_worked_tile );
		if ( pop.m_worked_tile ) {
			const auto claim = bm->m_work_assignments.find( { pop.m_worked_tile->x, pop.m_worked_tile->y } );
			if ( claim == bm->m_work_assignments.end() || claim->second != BaseManager::pop_key_t( m_id, pop.m_id ) ) { THROW( "Existing population reverse assignment is incomplete" ); }
		}
		if ( position < coordinates.size() ) {
			const auto& coord = coordinates[ position ];
			if ( !assignments.emplace( BaseManager::tile_key_t( coord.x, coord.y ), BaseManager::pop_key_t( m_id, pop.m_id ) ).second ) {
				THROW( "SMAC allocation worker is claimed by another Base" );
			}
			projection.push_back( { &pop, worker, coord } );
		}
		else { projection.push_back( { &pop, specialists[ allocation.specialists.types[ position - coordinates.size() ] ], {} } ); }
		++position;
	}
	// Prepare both native queues before publishing any authority or Pop fields.
	// No script, yield query, queue dispatch, or allocator callback runs here.
	auto updates = bm->m_base_updates;
	auto& update = updates.try_emplace( m_id, BaseManager::base_update_t{ BaseManager::BUO_NONE, this } ).first->second;
	if ( update.base != this || ( update.ops & BaseManager::BUO_DESPAWN ) ) { THROW( "SMAC allocation has an incompatible pending Base update" ); }
	update.ops = static_cast< BaseManager::base_update_op_t >( update.ops | BaseManager::BUO_REFRESH );
	std::lock_guard lock( bm->m_updated_bases_mutex );
	auto triggers = bm->m_updated_bases;
	triggers.insert( this );
	std::optional< SmacBaseAllocationState > replacement = allocation;
	// Vec2's scalar copy operations cannot throw; optional's copy assignment
	// itself lacks a noexcept declaration in the supported C++17 library.
	static_assert( std::is_nothrow_copy_constructible_v< map::tile::coords_t > );
	static_assert( std::is_nothrow_copy_assignable_v< map::tile::coords_t > );
	static_assert( std::is_nothrow_swappable_v< decltype( replacement ) > );
	// Commit: no allocation, callbacks, IDs, node replacement or RNG draws.
	for ( const auto& next : projection ) { next.pop->m_def = next.def; next.pop->m_worked_tile = next.coordinates; }
	m_allocation.swap( replacement );
	bm->m_work_assignments.swap( assignments );
	bm->m_base_updates.swap( updates );
	bm->m_updated_bases.swap( triggers );
}

}
