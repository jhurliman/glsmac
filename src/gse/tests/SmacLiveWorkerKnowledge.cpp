#include "SmacLiveWorkerKnowledge.h"
#include "ConventionalUnits.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/WorldSnapshotWorkerKnowledge.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "game/FrontendRequest.h"
#include "gse/value/Callable.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include "gse/callable/Native.h"
#include "util/FinallyGuard.h"
#include <functional>
#include <memory>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::base;
using namespace game::backend::unit;
using Knowledge = map::SmacWorkerKnowledge;
using Frame = map::SmacWorkerMapFrame;

bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }
bool KnowledgeFence( const std::function< void() >& call ) {
	try { call(); } catch ( const std::exception& e ) { return std::string( e.what() ).find( "worker sight/territory producer" ) != std::string::npos; } return false;
}
Value* Call( GSE_CALLABLE, value::Object* object, const std::string& name, const value::function_arguments_t& args = {} ) {
	return static_cast< value::Callable* >( object->Get( name ) )->Run( GSE_CALL, args );
}
Base::SmacAllocationRoles Roles() {
	Base::SmacAllocationRoles roles; roles.worker = "knowledge-worker";
	for ( size_t i = 0; i < 7; ++i ) { roles.specialists[ i ] = "knowledge-specialist-" + std::to_string( i ); }
	return roles;
}
void Definitions( Game* game ) {
	auto* bm = game->GetBM(); const auto roles = Roles();
	bm->DefinePop( new PopDef( roles.worker, "Worker", {}, {}, PopDef::PF_TILE_WORKER ) );
	for ( uint8_t i = 0; i < 7; ++i ) { bm->DefinePop( new PopDef( roles.specialists[ i ], "Specialist", {}, {}, PopDef::PF_NONE, i ) ); }
	MoraleSet::morale_values_t names; for ( int i = 0; i < 7; ++i ) { names.emplace_back( "level" + std::to_string( i ) ); }
	auto* um = game->GetUM(); um->DefineMoraleSet( new MoraleSet( "knowledge-morale", names ) );
	um->DefineUnit( new StaticDef( "knowledge-scout", um->GetMoraleSet( "knowledge-morale" ), "Scout", MT_LAND, 999,
		new CvrScoutRender( 2, 0, 100, 100, { 1, 1 }, { 128, 128 } ) ) );
}
Base* MakeBase( GSE_CALLABLE, Game* game, size_t id, size_t slot, int32_t simulation, size_t x, size_t y, uint32_t selected ) {
	auto* bm = game->GetBM(); auto* owner = &game->GetState()->m_slots->GetSlot( slot );
	Base::pops_t pops;
	pops.emplace( 4, Pop( nullptr, 4, bm->GetPopDef( Roles().specialists[ 1 ] ), 1 ) );
	pops.emplace( 91, Pop( nullptr, 91, bm->GetPopDef( Roles().specialists[ 1 ] ), 0 ) );
	auto* base = new Base( game, id, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( x, y ), "Knowledge Base", {}, pops, 100, 19, {},
		SmacBaseResourceContext{ simulation, false, 0, 0, 0x80000040u, false } );
	bm->SpawnBase( GSE_CALL, base );
	SmacBaseAllocationState allocation{ selected, { {}, 1 } };
	for ( size_t i = 0; i < 16; ++i ) { allocation.specialists.types[ i ] = ( i + 1 ) % 7; }
	base->PublishSmacAllocation( GSE_CALL, allocation, Roles() ); return base;
}
Unit* Spawn( GSE_CALLABLE, Game* game, size_t id, size_t slot, int32_t simulation, size_t x, size_t y ) {
	return game->GetUM()->SpawnConventionalUnit( GSE_CALL, id, game->GetUM()->GetUnitDef( "knowledge-scout" ), &game->GetState()->m_slots->GetSlot( slot ),
		game->GetMap()->GetTile( x, y ), { 2, simulation, 1, 0, 0, false, ConventionalUnitState::Home::None, SmacMoraleMode::Mode0 } );
}
struct World { Base* local; Base* foreign; Unit* local_unit; Unit* foreign_unit; };
World Populate( GSE_CALLABLE, Game* game, bool sea ) {
	Definitions( game );
	// Explicit synthetic terrain supplied to the actual Tile authority; no map
	// generator, visibility producer or original identity mapping is inferred.
	for ( size_t y = 0; y < game->GetMap()->GetHeight(); ++y ) { for ( size_t x = y & 1; x < game->GetMap()->GetWidth(); x += 2 ) {
		auto* tile = game->GetMap()->GetTile( x, y );
		for ( auto* corner : tile->elevation.corners ) { *corner = 100; }
	} }
	if ( sea ) { for ( auto* corner : game->GetMap()->GetTile( 4, 2 )->elevation.corners ) { *corner = -1000; } }
	for ( size_t y = 0; y < 8; ++y ) { for ( size_t x = y & 1; x < 8; x += 2 ) { game->GetMap()->GetTile( x, y )->Update(); } }
	return { MakeBase( GSE_CALL, game, 901, 0, 1, 2, 2, 0x80000003u ), MakeBase( GSE_CALL, game, 800, 6, 2, 4, 4, 0x40000101u ),
		Spawn( GSE_CALL, game, 91, 0, 1, 6, 6 ), Spawn( GSE_CALL, game, 17, 6, 2, 3, 3 ) };
}
size_t Index( size_t x, size_t y ) { return y * 4 + x / 2; }
Knowledge::Data Data() {
	Knowledge::Data data{ 8, 8, true, true, 0, {}, {}, {} };
	data.observers[ 1 ] = Knowledge::Observer{}; data.observers[ 2 ] = Knowledge::Observer{};
	data.tiles.resize( 32, { 6, std::nullopt } );
	data.tiles[ Index( 0, 2 ) ].explored_by = 4;
	data.tiles[ Index( 1, 3 ) ].claimant = 2;
	data.tiles[ Index( 1, 1 ) ].claimant = 0;
	// Deliberately opposite the actual manager's ascending ID order.
	data.units = { { 91, 0, false }, { 17, 2, false } }; return data;
}
struct Saved {
	std::string snapshot, base, other, unit, other_unit, knowledge, jsf;
	SmacGameplayRandomState pair;
	size_t requests, unit_id, base_id;
};
Saved Save( ConventionalUnitFixture& fixture, const World& world ) {
	const auto& knowledge = fixture.Game()->GetMap()->GetSmacWorkerKnowledge();
	return { fixture.Snapshot(), Base::Serialize( world.local ).ToString(), Base::Serialize( world.foreign ).ToString(), Unit::Serialize( world.local_unit ).ToString(),
		Unit::Serialize( world.foreign_unit ).ToString(), knowledge ? knowledge->Serialize() : "", fixture.Game()->GetRandom()->GetStateString(), fixture.GameplayRandom(),
		fixture.QueuedFrontendRequests().size(), Unit::GetNextId(), Base::GetNextId() };
}
bool Unchanged( ConventionalUnitFixture& fixture, const World& world, const Saved& before ) {
	const auto now = Save( fixture, world );
	return now.snapshot == before.snapshot && now.base == before.base && now.other == before.other && now.unit == before.unit && now.other_unit == before.other_unit
		&& now.knowledge == before.knowledge && now.jsf == before.jsf && now.pair == before.pair && now.requests == before.requests
		&& now.unit_id == before.unit_id && now.base_id == before.base_id;
}
}

void AddSmacLiveWorkerKnowledgeTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC live worker knowledge joins actual entities and terrain without query effects", []( GSE*, context::Context* ) -> std::string {
		for ( bool sea : { false, true } ) {
			ConventionalUnitFixture fixture; fixture.Configure();
			const auto result = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, sea ); auto data = Data();
				for ( int scenario = 0; scenario < 4; ++scenario ) {
					data.units[ 1 ].visible_to = scenario == 1 ? 0 : 2;
					data.observers[ 1 ]->relations[ 2 ] = { scenario == 2, scenario == 3 };
					game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( data ) ); const auto before = Save( fixture, world );
					const auto actual = game->GetSmacWorkerAvailability( *world.local );
					uint32_t expected = 1u | ( 1u << 6 ) | ( 1u << 10 ) | ( 1u << 13 ) | ( 1u << 20 );
					if ( sea ) { expected |= 1u << 2; }
					if ( scenario != 1 && scenario != 2 ) { expected |= 1u << 3; }
					if ( scenario == 3 ) { expected |= 1u << 5; }
					GT_ASSERT( actual.unavailable == expected && actual.selected_radius == 3, "live IDs/terrain/foreign allocation or visibility projected incorrectly" );
					GT_ASSERT( actual.squares[ 5 ]->claimant == 2 && actual.squares[ 5 ]->recognized_owner == ( scenario == 3 ? std::optional< int32_t >( 2 ) : std::nullopt )
						&& actual.squares[ 7 ]->claimant == 0 && !actual.squares[ 7 ]->recognized_owner, "raw/native claims conflated with recognition" );
					GT_ASSERT( world.local->GetSmacAllocation()->selected == 0x80000003u && world.foreign->GetSmacAllocation()->selected == 0x40000101u,
						"consumer projection truncated saved selected32" );
					(void)game->GetSmacWorkerAvailability( *world.foreign );
					GT_ASSERT( Unchanged( fixture, world, before ), "observational query changed world, queues, IDs or either RNG" );
				}
				const auto installed = game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
				auto replacement = *world.foreign->GetSmacAllocation(); replacement.selected = 0x40000001u; replacement.specialists.total_count = 2;
				world.foreign->PublishSmacAllocation( GSE_CALL, replacement, Roles() );
				GT_ASSERT( !( game->GetSmacWorkerAvailability( *world.local ).unavailable & ( 1u << 2 ) )
					&& game->GetMap()->GetSmacWorkerKnowledge()->Serialize() == installed, "live allocation replacement was hidden behind a copied Base list" );
				GT_OK();
			} ); GT_ASSERT( result.empty(), "live fixture failed: " + result );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live worker knowledge rejects replacement before publishing any state", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, false );
			game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( Data() ) ); const auto before = Save( fixture, world );
			for ( int scenario = 0; scenario < 7; ++scenario ) {
				auto data = Data();
				if ( scenario == 0 ) { data.width = 16; data.tiles.resize( 64 ); }
				if ( scenario == 1 ) { data.horizontal_wrap = false; }
				if ( scenario == 2 ) { data.units.pop_back(); }
				if ( scenario == 3 ) { data.units.push_back( { 99, 0, false } ); }
				if ( scenario == 4 ) { data.units[ 1 ].id = 99; }
				if ( scenario == 5 ) { data.units[ 1 ].active_convoy = true; }
				if ( scenario == 6 ) { data.observers[ 7 ] = Knowledge::Observer{}; }
				const auto candidate = Knowledge::Create( data );
				GT_ASSERT( Rejects( [ & ] { game->PublishSmacWorkerKnowledge( GSE_CALL, candidate ); } ) && Unchanged( fixture, world, before ),
					"invalid replacement changed installed knowledge/entities/queues" );
			}
			// Presence for another admitted observer never manufactures this Base's row.
			auto data = Data(); data.observers[ 1 ].reset(); game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( data ) );
			GT_ASSERT( Rejects( [ & ] { game->GetSmacWorkerAvailability( *world.local ); } ), "missing observer row silently inherited another participant" );
			(void)game->GetSmacWorkerAvailability( *world.foreign ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker knowledge authenticates forward and reverse entity membership", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, false ); const auto knowledge = Knowledge::Create( Data() );
			game->PublishSmacWorkerKnowledge( GSE_CALL, knowledge ); const auto before = Save( fixture, world );
			for ( int scenario = 0; scenario < 5; ++scenario ) {
				auto* tile = world.foreign_unit->GetTile(); auto* spare = game->GetMap()->GetTile( 6, 4 );
				const auto center = world.foreign->GetTile()->base; auto* owner = world.foreign_unit->m_owner;
				if ( scenario == 0 ) { tile->units.erase( world.foreign_unit->m_id ); }
				if ( scenario == 1 ) { spare->units.emplace( 17, world.foreign_unit ); }
				if ( scenario == 2 ) { world.foreign->GetTile()->base = nullptr; }
				if ( scenario == 3 ) { spare->base = world.foreign; }
				if ( scenario == 4 ) { world.foreign_unit->m_owner = world.local_unit->m_owner; }
				const bool query_rejected = Rejects( [ & ] { game->GetSmacWorkerAvailability( *world.local ); } );
				const bool publish_rejected = Rejects( [ & ] { game->PublishSmacWorkerKnowledge( GSE_CALL, knowledge ); } );
				tile->units[ 17 ] = world.foreign_unit; spare->units.clear(); spare->base = nullptr; world.foreign->GetTile()->base = center; world.foreign_unit->m_owner = owner;
				GT_ASSERT( query_rejected && publish_rejected && Unchanged( fixture, world, before ), "unbound live entity admitted or rejection changed stored state" );
			}
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker knowledge requires canonical allocation and conventional unit authority", []( GSE*, context::Context* ) -> std::string {
		for ( bool legacy_unit : { false, true } ) {
			ConventionalUnitFixture fixture; fixture.Configure();
			const auto result = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, false ); auto data = Data();
				auto* owner = world.local->m_owner; auto* tile = game->GetMap()->GetTile( 7, 5 );
				if ( legacy_unit ) {
					auto* legacy = new Unit( GSE_CALL, game->GetUM(), 57, world.local_unit->m_def, owner, tile, 1, 1, 1, false );
					game->GetUM()->SpawnUnit( GSE_CALL, legacy ); data.units.push_back( { 57, 0, false } );
				}
				else {
					auto* unallocated = new Base( game, 902, owner, owner->GetPlayer()->GetFaction(), tile, "No allocation", {}, {}, 1, 0, {},
						SmacBaseResourceContext{ 1, false, 0, 0, 0, false } ); game->GetBM()->SpawnBase( GSE_CALL, unallocated );
				}
				const auto snapshot = fixture.Snapshot(); const auto next_unit = Unit::GetNextId(), next_base = Base::GetNextId(); const auto queued = fixture.QueuedFrontendRequests().size();
				GT_ASSERT( Rejects( [ & ] { game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( data ) ); } ) && !game->GetMap()->GetSmacWorkerKnowledge()
					&& fixture.Snapshot() == snapshot && Unit::GetNextId() == next_unit && Base::GetNextId() == next_base && fixture.QueuedFrontendRequests().size() == queued,
					"legacy Unit or missing full Base allocation admitted by inventing canonical state" ); GT_OK();
			} ); GT_ASSERT( result.empty(), "authority fixture failed: " + result );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live worker knowledge restores after both actual manager queues", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture source; source.Configure(); std::string snapshot, knowledge, base, unit; Frame::Evaluation expected{};
		auto result = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = source.Game(); const auto world = Populate( GSE_CALL, game, true );
			game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( Data() ) );
			snapshot = source.Snapshot(); knowledge = game->GetMap()->GetSmacWorkerKnowledge()->Serialize(); base = Base::Serialize( world.local ).ToString(); unit = Unit::Serialize( world.foreign_unit ).ToString();
			expected = game->GetSmacWorkerAvailability( *world.local ); GT_OK();
		} ); GT_ASSERT( result.empty(), "source snapshot failed: " + result );
		for ( bool no_current : { false, true } ) {
			ConventionalUnitFixture destination; destination.Configure(); destination.GameplayRandom( { 99, 101 } );
			result = destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				destination.Restore( GSE_CALL, snapshot, no_current ); auto* game = destination.Game();
				auto* restored_base = game->GetBM()->GetBase( 901 ); auto* restored_unit = game->GetUM()->GetUnit( 17 );
				GT_ASSERT( restored_base && restored_unit && game->GetMap()->GetSmacWorkerKnowledge()->Serialize() == knowledge
					&& Base::Serialize( restored_base ).ToString() == base && Unit::Serialize( restored_unit ).ToString() == unit
					&& game->GetSmacWorkerAvailability( *restored_base ) == expected, "body restore lost knowledge/entity authority or published before pending registries" );
				GT_ASSERT( restored_base->GetSmacAllocation()->selected == 0x80000003u && restored_base->GetSmacAllocation()->specialists.types[ 15 ] == 2,
					"restore recreated active-only allocation instead of full saved authority" );
				GT_ASSERT( destination.GameplayRandom() == source.GameplayRandom(), "restore changed paired RNG" ); GT_OK();
			} ); GT_ASSERT( result.empty(), "restored fixture failed: " + result );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live worker knowledge fences entity terrain and turn mutation before effects", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, false );
			game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( Data() ) ); const auto before = Save( fixture, world );
			auto* target = game->GetMap()->GetTile( 7, 5 ); auto* um = game->GetUM(); auto* bm = game->GetBM(); bool called = false;
			std::vector< std::function< void() > > mutations{
				[ & ] { world.local_unit->SetTile( GSE_CALL, target ); },
				[ & ] { um->ValidateConventionalMove( world.local_unit, target, 0 ); },
				[ & ] { um->MoveConventionalUnit( GSE_CALL, world.local_unit, target, 0 ); },
				[ & ] { std::unique_ptr< const std::string > error( um->MoveUnitToTile( GSE_CALL, world.local_unit, target, [ & ] { called = true; } ) ); },
				[ & ] { Spawn( GSE_CALL, game, Unit::GetNextId(), 0, 1, 7, 5 ); },
				[ & ] { um->DespawnUnit( GSE_CALL, world.local_unit->m_id ); },
				[ & ] { um->SpawnUnit( GSE_CALL, world.local_unit ); },
				[ & ] { bm->DespawnBase( GSE_CALL, world.local->m_id ); },
				[ & ] { bm->SpawnBase( GSE_CALL, world.local ); },
				[ & ] { std::unique_ptr< Base > rejected( new Base( game, Base::GetNextId(), world.local->m_owner, world.local->m_faction, target, "Rejected", {}, {} ) ); },
				[ & ] { target->Clear(); }, [ & ] { target->Deserialize( target->Serialize() ); },
				[ & ] { game->GetMap()->GetTilesPtr()->Clear(); }, [ & ] { game->GetMap()->GetTilesPtr()->Resize( 8, 8 ); },
				[ & ] { game->GetMap()->GetTilesPtr()->FixTopBottomRows( game->GetRandom() ); },
				[ & ] { game->Event( GSE_CALL, "", {} ); }, [ & ] { game->CompleteTurn( GSE_CALL, 0 ); },
				[ & ] { game->AdvanceTurn( 2 ); },
				[ & ] { auto tiles = game->GetMap()->GetTilesPtr()->Serialize(); (void)game->GetMap()->LoadFromBuffer( tiles ); },
				[ & ] { Call( GSE_CALL, static_cast< value::Object* >( um->Wrap( GSE_CALL ) ), "despawn_unit", { world.local_unit->Wrap( GSE_CALL ) } ); },
				[ & ] { Call( GSE_CALL, static_cast< value::Object* >( bm->Wrap( GSE_CALL ) ), "despawn_base", { world.local->Wrap( GSE_CALL ) } ); }
			};
			for ( const auto& mutation : mutations ) {
				GT_ASSERT( KnowledgeFence( mutation ) && !called && !target->IsLocked() && !world.local_unit->m_animation_id && Unchanged( fixture, world, before ),
					"guarded mutation changed IDs/membership/queues/terrain/turn/RNG or ran callback" );
			}
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker knowledge fences already queued event callbacks", []( GSE*, context::Context* ) -> std::string {
		for ( bool installed : { false, true } ) {
			ConventionalUnitFixture fixture; fixture.Configure(); auto calls = std::make_shared< size_t >( 0 );
			const auto result = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, false );
				fixture.QueueProbeEvent( GSE_CALL, [ calls, game ] { ++*calls; game->GetRandom()->GetUInt(); } );
				if ( installed ) { game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( Data() ) ); }
				const auto before = Save( fixture, world );
				if ( installed ) {
					GT_ASSERT( KnowledgeFence( [ & ] { fixture.DrainEvents(); } ) && *calls == 0 && Unchanged( fixture, world, before ),
						"captured event ran validation/resolution/apply or spent RNG after knowledge publication" );
				}
				else { fixture.DrainEvents(); GT_ASSERT( *calls == 3 && game->GetRandom()->GetStateString() != before.jsf, "absent-knowledge control did not execute all real handler phases" ); }
				GT_OK();
			} ); GT_ASSERT( result.empty(), "event fixture failed: " + result );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live worker knowledge remains unavailable during actual restored Start callback", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture source; source.Configure(); std::string snapshot; Base* original = nullptr; Frame::Evaluation expected{};
		auto result = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			const auto world = Populate( GSE_CALL, source.Game(), true ); original = world.local;
			source.Game()->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( Data() ) );
			snapshot = source.Snapshot(); expected = source.Game()->GetSmacWorkerAvailability( *original ); GT_OK();
		} ); GT_ASSERT( result.empty(), "source setup failed: " + result );
		ConventionalUnitFixture destination; destination.Configure();
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = destination.Game(); size_t starts = 0; bool pending = false, query_blocked = false, spawn_blocked = false, base_blocked = false;
			destination.RestoreThroughStart( GSE_CALL, snapshot, [ & ]( GSE_CALLABLE ) {
				++starts; pending = game->GetMap()->GetSmacWorkerKnowledge().has_value() && game->GetUM()->GetUnits().empty() && game->GetBM()->GetBases().empty();
				// No destination Base has been published yet. The readiness gate
				// must reject before inspecting the supplied Base reference.
				try { (void)game->GetSmacWorkerAvailability( *original ); } catch ( const std::runtime_error& e ) { query_blocked = std::string( e.what() ) == "Live worker knowledge is unavailable"; }
				const auto next_unit = Unit::GetNextId(), next_base = Base::GetNextId(); const auto requests = destination.QueuedFrontendRequests().size();
				spawn_blocked = Rejects( [ & ] { Spawn( GSE_CALL, game, next_unit, 0, 1, 7, 5 ); } );
				auto* owner = &game->GetState()->m_slots->GetSlot( 0 );
				base_blocked = Rejects( [ & ] { std::unique_ptr< Base > rejected( new Base( game, next_base, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 7, 5 ), "Start rejection", {}, {} ) ); } );
				if ( Unit::GetNextId() != next_unit || Base::GetNextId() != next_base || destination.QueuedFrontendRequests().size() != requests ) { throw std::runtime_error( "start rejection changed IDs or queues" ); }
			} );
			GT_ASSERT( starts == 1 && pending && query_blocked && spawn_blocked && base_blocked, "restored start escaped knowledge/producer readiness gate" );
			GT_ASSERT( game->GetUM()->GetUnits().size() == 2 && game->GetBM()->GetBases().size() == 2
				&& game->GetSmacWorkerAvailability( *game->GetBM()->GetBase( 901 ) ) == expected, "actual InitComplete did not publish knowledge after both manager drains" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker knowledge world prefix bounds before copying and map admission", []( GSE*, context::Context* ) -> std::string {
		using namespace game::backend::world_snapshot;
		const auto knowledge = Knowledge::Create( Data() ); types::Buffer valid;
		WriteWorkerKnowledge( valid, knowledge, true ); valid.WriteInt( 713 );
		GT_ASSERT( ReadWorkerKnowledge( valid, true ) == std::optional< Knowledge >( knowledge ) && valid.ReadInt() == 713, "bounded prefix did not preserve following body" );
		for ( size_t length : { Knowledge::MAX_WIRE_BYTES, Knowledge::MAX_WIRE_BYTES + 1, Knowledge::MAX_WIRE_BYTES + 4096 } ) {
			types::Buffer input; input.WriteBool( true ); const auto boundary = input.lenw; input.WriteString( std::string( length, 'x' ) );
			GT_ASSERT( Rejects( [ & ] { ReadWorkerKnowledge( input, true ); } ), "invalid knowledge frame admitted" );
			GT_ASSERT( ( length > Knowledge::MAX_WIRE_BYTES && input.lenr == boundary ) || ( length == Knowledge::MAX_WIRE_BYTES && input.lenr == input.lenw ),
				"outer length rejection conflated with bounded nested codec rejection" );
		}
		types::Buffer non_smac; non_smac.WriteBool( true ); const auto boundary = non_smac.lenw; non_smac.WriteString( knowledge.Serialize() );
		GT_ASSERT( Rejects( [ & ] { ReadWorkerKnowledge( non_smac, false ); } ) && non_smac.lenr == boundary, "non-SMAC presence consumed the payload" );
		types::Buffer output; output.WriteInt( 91 ); const auto before = output.ToString();
		GT_ASSERT( Rejects( [ & ] { WriteWorkerKnowledge( output, knowledge, false ); } ) && output.ToString() == before, "rejected writer appended a partial prefix" );
		types::Buffer map; map.WriteInt( 8 ); map.WriteInt( 8 ); ValidateWorkerKnowledgeGeometry( knowledge, map.ToString() );
		for ( const auto dimensions : { std::pair< int, int >{ 6, 8 }, { 8, 6 }, { -1, 8 } } ) {
			types::Buffer bad; bad.WriteInt( dimensions.first ); bad.WriteInt( dimensions.second );
			GT_ASSERT( Rejects( [ & ] { ValidateWorkerKnowledgeGeometry( knowledge, bad.ToString() ); } ), "knowledge/map geometry mismatch reached map decoding" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC live worker knowledge absence preserves existing explicit entity lifecycle", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.Configure();
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); const auto world = Populate( GSE_CALL, game, false );
			GT_ASSERT( !game->GetMap()->GetSmacWorkerKnowledge() && Rejects( [ & ] { game->GetSmacWorkerAvailability( *world.local ); } ), "absence invented worker knowledge" );
			auto* target = game->GetMap()->GetTile( 7, 5 ); world.local_unit->SetTile( GSE_CALL, target );
			GT_ASSERT( target->units.at( 91 ) == world.local_unit && !game->GetMap()->GetTile( 6, 6 )->units.count( 91 ), "absence changed existing relocation" );
			game->GetUM()->DespawnUnit( GSE_CALL, 91 ); game->GetBM()->DespawnBase( GSE_CALL, 901 );
			GT_ASSERT( !game->GetUM()->GetUnit( 91 ) && !game->GetBM()->GetBase( 901 ) && !target->units.count( 91 ), "absence changed manager removal" );
			target->Clear(); GT_OK();
		} );
	} );
}
}
