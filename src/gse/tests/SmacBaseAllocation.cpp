#include "SmacBaseAllocation.h"
#include "ConventionalUnits.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/base/SmacWorkedSquareCollection.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/WorldSnapshotRules.h"
#include "game/FrontendRequest.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Callable.h"
#include "gse/callable/Native.h"
#include "util/FinallyGuard.h"
#include <algorithm>
#include <functional>
#include <memory>

namespace gse::tests {
using namespace game::backend;
using namespace game::backend::base;

// Read-only observation of the actual deferred native queues, not a substitute
// for enqueueing or a production injection hook.
class BaseAllocationFixture {
public:
	static std::vector< std::pair< size_t, unsigned > > Queues( BaseManager* manager ) {
		std::vector< std::pair< size_t, unsigned > > result;
		for ( const auto& item : manager->m_base_updates ) { result.emplace_back( item.first, item.second.ops ); }
		for ( const auto* item : manager->m_updated_bases ) { result.emplace_back( item->m_id, 256 ); }
		std::sort( result.begin(), result.end() ); return result;
	}
};
namespace {
bool Rejects( const std::function< void() >& f ) { try { f(); } catch ( const std::exception& ) { return true; } return false; }
Value* Call( GSE_CALLABLE, value::Object* object, const std::string& name, const value::function_arguments_t& args = {} ) {
	return static_cast< value::Callable* >( object->Get( name ) )->Run( GSE_CALL, args );
}
Base::SmacAllocationRoles Roles() {
	Base::SmacAllocationRoles result; result.worker = "allocation-worker";
	for ( size_t i = 0; i < 7; ++i ) { result.specialists[ i ] = "allocation-role-" + std::to_string( i ); }
	return result;
}
Base* MakeBase( GSE_CALLABLE, Game* game, const std::string& name = "Allocation" ) {
	auto* bm = game->GetBM(); const auto roles = Roles();
	bm->DefinePop( new PopDef( roles.worker, "Same text", {}, {}, PopDef::PF_TILE_WORKER ) );
	for ( uint8_t i = 0; i < 7; ++i ) { bm->DefinePop( new PopDef( roles.specialists[ i ], "Same text", {}, {}, PopDef::PF_NONE, i ) ); }
	auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
	Base::pops_t pops;
	for ( size_t id : { 40u, 1u, 8u } ) { pops.emplace( id, Pop( nullptr, id, bm->GetPopDef( roles.specialists[ 1 ] ), id % 2 ) ); }
	auto* base = new Base( game, 901, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 2, 2 ), name, {}, pops, 71, 29, {}, SmacBaseResourceContext{ 1, true, 37, -11, 0x80000040u } );
	bm->SpawnBase( GSE_CALL, base ); return base;
}
SmacBaseAllocationState Allocation() {
	SmacBaseAllocationState result{ 0x80000007u, { {}, 1 } };
	for ( size_t i = 0; i < 16; ++i ) { result.specialists.types[ i ] = ( i + 1 ) % 7; }
	return result;
}
}

void AddSmacBaseAllocationTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC live Base allocation projects onto stable Pop identities and both queues", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* bm = game->GetBM(); auto* base = MakeBase( GSE_CALL, game );
			std::vector< Pop* > pops; std::vector< value::Object* > wrappers;
			for ( auto& entry : base->m_pops ) { pops.push_back( &entry.second ); wrappers.push_back( static_cast< value::Object* >( entry.second.Wrap( GSE_CALL ) ) ); }
			const auto rng = game->GetRandom()->GetStateString(); const auto id = Base::GetNextId(); const auto pending = fixture.QueuedFrontendRequests().size();
			const auto state = Allocation(); base->PublishSmacAllocation( GSE_CALL, state, Roles() );
			GT_ASSERT( base->GetSmacAllocation() == std::optional< SmacBaseAllocationState >( state ), "authority lost high bits or inactive slots" );
			GT_ASSERT( bm->GetWorkingPop( { 3, 1 } ) == pops[ 0 ] && bm->GetWorkingPop( { 4, 2 } ) == pops[ 1 ] && !pops[ 2 ]->GetWorkedTileCoords(), "worker radius/specialist projection order changed" );
			auto next = state; next.selected = 0x40000004u; next.specialists.total_count = 2; next.specialists.types[ 0 ] = 5; next.specialists.types[ 1 ] = 0;
			base->PublishSmacAllocation( GSE_CALL, next, Roles() );
			GT_ASSERT( !bm->GetWorkingPop( { 3, 1 } ) && bm->GetWorkingPop( { 4, 2 } ) == pops[ 0 ], "replacement reverse index was not swapped" );
			for ( size_t i = 0; i < pops.size(); ++i ) {
				GT_ASSERT( &base->m_pops.at( pops[ i ]->m_id ) == pops[ i ] && Pop::Unwrap( wrappers[ i ] ) == pops[ i ] && pops[ i ]->m_variant == pops[ i ]->m_id % 2, "publication replaced a Pop node/variant/wrapper" );
			}
			GT_ASSERT( pops[ 1 ]->m_def->m_smac_specialist_id == 5 && pops[ 2 ]->m_def->m_smac_specialist_id == 0, "unavailable stored specialist type was normalized" );
			GT_ASSERT( Base::GetNextId() == id && game->GetRandom()->GetStateString() == rng && fixture.QueuedFrontendRequests().size() == pending, "publication dispatched frontend work, IDs, or RNG" );
			const auto queued = BaseAllocationFixture::Queues( bm );
			GT_ASSERT( queued.size() == 2 && queued[ 0 ].first == 901 && queued[ 1 ] == std::make_pair( size_t( 901 ), 256u ), "publication did not coalesce one entry in each queue" );
			const auto& updates = fixture.Updates(); size_t refreshes = 0;
			for ( const auto& update : updates ) { if ( update.type == game::FrontendRequest::FR_BASE_UPDATE && update.data.base_update.base_id == 901 ) {
				++refreshes; GT_ASSERT( update.data.base_update.pops->size() == 3, "frontend did not read complete committed population" );
			} }
			GT_ASSERT( refreshes == 1, "multiple publications emitted partial or duplicate frontend refreshes" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live Base allocation rejects invalid projections atomically", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* bm = game->GetBM(); auto* base = MakeBase( GSE_CALL, game );
			base->PublishSmacAllocation( GSE_CALL, Allocation(), Roles() );
			const auto before = Base::Serialize( base ).ToString(); const auto queues = BaseAllocationFixture::Queues( bm ); const auto fr = fixture.QueuedFrontendRequests().size();
			for ( int scenario = 0; scenario < 7; ++scenario ) {
				auto state = Allocation(); auto roles = Roles();
				if ( scenario == 0 ) { state.specialists.total_count = 17; }
				if ( scenario == 1 ) { state.specialists.types[ 15 ] = 7; }
				if ( scenario == 2 ) { state.selected &= ~uint32_t( 2 ); }
				if ( scenario == 3 ) { state.selected = ( state.selected & ~uint32_t( 2 ) ) | ( 1u << 13 ); } // y=-1
				if ( scenario == 4 ) { roles.worker = roles.specialists[ 0 ]; }
				if ( scenario == 5 ) { roles.specialists[ 6 ] = roles.specialists[ 5 ]; }
				if ( scenario == 6 ) { roles.specialists[ 0 ] = "absent"; }
				GT_ASSERT( Rejects( [ & ] { base->PublishSmacAllocation( GSE_CALL, state, roles ); } ), "invalid live allocation admitted" );
				GT_ASSERT( Base::Serialize( base ).ToString() == before && BaseAllocationFixture::Queues( bm ) == queues && fixture.QueuedFrontendRequests().size() == fr
					&& bm->GetWorkingPop( { 3, 1 } ) == &base->m_pops.at( 1 ) && bm->GetWorkingPop( { 4, 2 } ) == &base->m_pops.at( 8 ), "rejection changed authority, projection, reverse index, or queues" );
			}
			// A real other Base's claim, rather than an unavailable-mask guess.
			auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
			auto* other = new Base( game, 902, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 6, 6 ), "Other", {},
				{ { 2, Pop( nullptr, 2, bm->GetPopDef( Roles().worker ), 0, game->GetMap()->GetTile( 3, 3 ) ) } }, 3, 0, {}, base->GetSmacResourceContext() );
			bm->SpawnBase( GSE_CALL, other ); auto conflict = Allocation(); conflict.selected = ( conflict.selected & ~uint32_t( 2 ) ) | ( 1u << 3 );
			const auto with_other = BaseAllocationFixture::Queues( bm );
			GT_ASSERT( Rejects( [ & ] { base->PublishSmacAllocation( GSE_CALL, conflict, Roles() ); } ) && Base::Serialize( base ).ToString() == before
				&& BaseAllocationFixture::Queues( bm ) == with_other && bm->GetWorkingPop( { 3, 3 } ) == &other->m_pops.at( 2 ), "cross-Base collision changed either allocation" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live Base allocation excludes actual Base centers and narrow-map aliases", []( GSE*, context::Context* ) -> std::string {
		for ( bool narrow : { false, true } ) {
			ConventionalUnitFixture fixture; fixture.Configure();
			const auto result = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* bm = game->GetBM();
				auto* base = MakeBase( GSE_CALL, game );
				if ( narrow ) {
					// Destroy the original Base before changing the fixture map domain.
					bm->DespawnBase( GSE_CALL, base->m_id ); game->GetMap()->GetTilesPtr()->Resize( 2, 8 );
					auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
					Base::pops_t pops; for ( size_t id : { 1u, 8u, 40u } ) { pops.emplace( id, Pop( nullptr, id, bm->GetPopDef( Roles().specialists[ 1 ] ) ) ); }
					base = new Base( game, 903, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 0, 2 ), "Narrow", {}, pops, 71, 0, {}, SmacBaseResourceContext{ 1, false, 0, 0, 0 } );
					bm->SpawnBase( GSE_CALL, base );
				}
				else {
					auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
					auto* other = new Base( game, 904, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 3, 1 ), "Center", {}, {}, 1, 0, {}, base->GetSmacResourceContext() );
					bm->SpawnBase( GSE_CALL, other );
				}
				const auto before = Base::Serialize( base ).ToString(); const auto queues = BaseAllocationFixture::Queues( bm );
				auto state = Allocation(); if ( narrow ) { state.selected = 1u | ( 1u << 1 ) | ( 1u << 7 ); }
				GT_ASSERT( Rejects( [ & ] { base->PublishSmacAllocation( GSE_CALL, state, Roles() ); } ) && Base::Serialize( base ).ToString() == before
					&& BaseAllocationFixture::Queues( bm ) == queues, "implicit center or alias became a committed worker" ); GT_OK();
			} ); GT_ASSERT( result.empty(), "fixture failed: " + result );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live Base allocation rejects late restore frames before center and ID effects", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		const auto original_id = Base::GetNextId(); util::FinallyGuard ids( [ = ] { Base::SetNextId( original_id ); } );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* bm = game->GetBM(); auto* base = MakeBase( GSE_CALL, game );
			base->PublishSmacAllocation( GSE_CALL, Allocation(), Roles() ); const auto bytes = Base::Serialize( base ).ToString();
			types::Buffer suffix; suffix.WriteBool( true ); suffix.WriteString( Allocation().Serialize().ToString() );
			const auto prefix = bytes.substr( 0, bytes.size() - suffix.lenw );
			bm->DespawnBase( GSE_CALL, base->m_id ); const auto next = Base::GetNextId(); const auto queues = BaseAllocationFixture::Queues( bm );
			for ( int scenario = 0; scenario < 5; ++scenario ) {
				types::Buffer body( prefix ); body.WriteBool( true ); const auto bound = body.lenw;
				auto allocation = Allocation(); if ( scenario == 0 ) { allocation.specialists.total_count = 2; }
				if ( scenario == 1 ) { allocation.specialists.types[ 0 ] = 4; }
				if ( scenario == 2 ) { allocation.selected = 0x8000000du; }
				const auto payload = allocation.Serialize().ToString();
				body.WriteString( payload + ( scenario == 3 ? "x" : "" ) ); if ( scenario == 4 ) { body.WriteBool( false ); }
				GT_ASSERT( Rejects( [ & ] { std::unique_ptr< Base > rejected( Base::Deserialize( body, game ) ); } ) && !game->GetMap()->GetTile( 2, 2 )->base
					&& !bm->GetBase( 901 ) && Base::GetNextId() == next && BaseAllocationFixture::Queues( bm ) == queues, "invalid allocation decode published center, identity, or queue effects" );
				if ( scenario == 3 ) { GT_ASSERT( body.lenr == bound, "allocation cap failure consumed the payload" ); }
			}
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC live Base allocation protects its worker from later Base construction and restore", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		const auto original_id = Base::GetNextId(); util::FinallyGuard ids( [ = ] { Base::SetNextId( original_id ); } );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* bm = game->GetBM(); auto* base = MakeBase( GSE_CALL, game );
			base->PublishSmacAllocation( GSE_CALL, Allocation(), Roles() ); const auto before = Base::Serialize( base ).ToString();
			const auto queues = BaseAllocationFixture::Queues( bm ); auto* tile = game->GetMap()->GetTile( 3, 1 ); const auto id = Base::GetNextId();
			GT_ASSERT( Rejects( [ & ] { std::unique_ptr< Base > rejected( new Base( game, id, base->m_owner, base->m_faction, tile, "Conflicting", {}, {} ) ); } ), "later Base constructed over a canonical worker" );
			types::Buffer body; body.WriteInt( id ); body.WriteInt( 0 ); body.WriteString( base->m_faction->m_id ); body.WriteInt( 3 ); body.WriteInt( 1 ); body.WriteString( "Conflicting restore" );
			body.WriteInt( 0 ); body.WriteInt( 0 ); body.WriteInt( 1 ); body.WriteInt( 0 ); body.WriteString( SerializeProductionState( {} ).ToString() );
			body.WriteString( SerializeSmacBaseResourceContext( {} ).ToString() ); body.WriteBool( false );
			GT_ASSERT( Rejects( [ & ] { std::unique_ptr< Base > rejected( Base::Deserialize( body, game ) ); } ) && !tile->base && Base::GetNextId() == id
				&& Base::Serialize( base ).ToString() == before && BaseAllocationFixture::Queues( bm ) == queues && bm->GetWorkingPop( tile->coord ) == &base->m_pops.at( 1 ), "opposite-order rejection changed center, ID, old projection, or queues" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live Base allocation refuses update callback reentry and releases dispatch guard", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* bm = game->GetBM(); auto* base = MakeBase( GSE_CALL, game );
			base->PublishSmacAllocation( GSE_CALL, Allocation(), Roles() ); const auto before = Base::Serialize( base ).ToString();
			bool rejected = false;
			const auto listener = game->On( GSE_CALL, "update_base", NATIVE_CALL( &rejected, base ) {
				rejected = Rejects( [ & ] { base->PublishSmacAllocation( GSE_CALL, Allocation(), Roles() ); } );
				throw std::runtime_error( "controlled update callback failure" ); return VALUE( value::Undefined );
			} ) );
			util::FinallyGuard remove( [ & ] { game->Off( GSE_CALL, "update_base", listener ); } );
			GT_ASSERT( Rejects( [ & ] { bm->TriggerUpdates( GSE_CALL ); } ) && rejected && Base::Serialize( base ).ToString() == before, "dispatch reentry changed allocation or did not reject" );
			base->PublishSmacAllocation( GSE_CALL, Allocation(), Roles() ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live Base allocation gates generic mutation and validates actual reads", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = MakeBase( GSE_CALL, game ); base->PublishSmacAllocation( GSE_CALL, Allocation(), Roles() );
			const auto before = Base::Serialize( base ).ToString(); const auto rng = game->GetRandom()->GetStateString();
			auto* wrapper = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
			auto* input = VALUEEXT( value::Object, GSE_CALL, ( value::object_properties_t{ { "type", VALUE( value::String,, Roles().specialists[ 1 ] ) } } ) );
			GT_ASSERT( Rejects( [ & ] { Call( GSE_CALL, wrapper, "create_pop", { input } ); } ) && Rejects( [ & ] { base->RemovePop( 1 ); } )
				&& Rejects( [ & ] { base->AddPop( Pop( base, 77, game->GetBM()->GetPopDef( Roles().specialists[ 1 ] ) ) ); } )
				&& Rejects( [ & ] { base->ChangePopType( GSE_CALL, 1, Roles().specialists[ 0 ] ); } )
				&& Rejects( [ & ] { base->SetPopAssignment( GSE_CALL, &base->m_pops.at( 1 ), Roles().specialists[ 1 ], nullptr ); } ), "generic mutator bypassed complete allocation publication" );
			GT_ASSERT( Base::Serialize( base ).ToString() == before && game->GetRandom()->GetStateString() == rng, "rejected generic mutation consumed RNG or changed population" );
			// Public legacy fields cannot silently become a second authority.
			auto& pop = base->m_pops.at( 40 ); const auto* def = pop.m_def; pop.m_def = game->GetBM()->GetPopDef( Roles().specialists[ 0 ] );
			util::FinallyGuard restore( [ & ] { pop.m_def = def; } );
			GT_ASSERT( Rejects( [ & ] { Base::Serialize( base ); } ) && Rejects( [ & ] { base->GetSmacAllocation(); } )
				&& Rejects( [ & ] { Call( GSE_CALL, wrapper, "get_pops" ); } ) && Rejects( [ & ] { Call( GSE_CALL, wrapper, "get_intake" ); } ), "read/save accepted a divergent projection" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live Base allocation prepares all storage before authority commit", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = MakeBase( GSE_CALL, game, "ALLOCATION_FAILURE_CONTROL" );
			const auto before = Base::Serialize( base ).ToString(); const auto queues = BaseAllocationFixture::Queues( game->GetBM() );
			// Copied production/test TUs inject a one-shot late allocation failure.
			constexpr bool expect_precommit_failure = false;
			const auto failed = Rejects( [ & ] { base->PublishSmacAllocation( GSE_CALL, Allocation(), Roles() ); } );
			GT_ASSERT( failed == expect_precommit_failure, "precommit control did not reach expected branch" );
			if ( failed ) {
				GT_ASSERT( Base::Serialize( base ).ToString() == before && BaseAllocationFixture::Queues( game->GetBM() ) == queues
					&& !game->GetBM()->GetWorkingPop( { 3, 1 } ) && !game->GetBM()->GetWorkingPop( { 4, 2 } ), "allocation failure published partial fields, index, or queues" );
				base->PublishSmacAllocation( GSE_CALL, Allocation(), Roles() );
			}
			GT_ASSERT( base->GetSmacAllocation() == std::optional< SmacBaseAllocationState >( Allocation() ), "retry failed to publish complete authority" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live Base allocation restore preserves exact state without founding or recalculation", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture source; source.Configure(); std::string saved, expected;
		const auto result = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* base = MakeBase( GSE_CALL, source.Game() ); base->PublishSmacAllocation( GSE_CALL, Allocation(), Roles() );
			expected = Base::Serialize( base ).ToString(); saved = source.Snapshot(); GT_OK();
		} ); GT_ASSERT( result.empty(), "fixture failed: " + result );
		for ( bool no_current : { false, true } ) {
			ConventionalUnitFixture destination; destination.Configure(); destination.RetainedMorale( 6, 6 );
			const auto restored = destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				size_t founded = 0; auto* bm = destination.Game()->GetBM();
				bm->On( GSE_CALL, "base_spawn", NATIVE_CALL( &founded ) { ++founded; return VALUE( value::Undefined ); } ) );
				destination.Restore( GSE_CALL, saved, no_current ); auto* base = bm->GetBase( 901 );
				GT_ASSERT( base && !founded && Base::Serialize( base ).ToString() == expected && base->GetSmacAllocation() == std::optional< SmacBaseAllocationState >( Allocation() ), "deferred world restore recalculated or normalized allocation" );
				GT_ASSERT( bm->GetWorkingPop( { 3, 1 } ) == &base->m_pops.at( 1 ) && bm->GetWorkingPop( { 4, 2 } ) == &base->m_pops.at( 8 ) && base->m_pops.at( 40 ).m_variant == 0, "restore lost ID/variant/assignment projection" ); GT_OK();
			} ); GT_ASSERT( restored.empty(), "restore failed: " + restored );
		}
		GT_OK();
	} );
}
}
