#include "Backend.h"
#include "fixtures/SmacRulesSource.h"
#include "fixtures/WorldSnapshotRules.h"
#include "GLSMAC.h"

#include "task/gsetests/GSETests.h"
#include "gse/GSE.h"
#include "gse/ExecutionPointer.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Object.h"
#include "gse/value/Callable.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/context/GlobalContext.h"
#include "gc/Space.h"
#include "game/backend/Game.h"
#include "game/backend/Random.h"
#include "game/backend/Player.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/SpriteRender.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/animation/AnimationManager.h"
#include "game/backend/resource/ResourceManager.h"
#include "game/backend/resource/Resource.h"
#include "game/backend/map/tile/TileManager.h"
#include "game/backend/State.h"
#include "game/backend/connection/Client.h"
#include "types/Packet.h"
#include "game/backend/event/Event.h"
#include "game/backend/event/EventHandler.h"
#include "game/backend/connection/Connection.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/FacilityDef.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/faction/FactionManager.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/MapState.h"
#include "game/backend/map/tile/Tiles.h"
#include "util/FinallyGuard.h"

#include <memory>
#include <exception>
#include <limits>
#include <stdexcept>
#include <set>
#include <thread>

namespace gse {
namespace tests {

class RecordingConnection : public game::backend::connection::Connection {
public:
	explicit RecordingConnection( gc::Space* space ) : Connection( space, network::CM_CLIENT, nullptr ) {}
	void UpdateSlot( size_t, game::backend::slot::Slot*, bool ) override {}
	void SendMessage( const std::string& ) override {}
	std::vector< std::string > sent;
protected:
	void SendGameEvents( const game_events_t& events ) override {
		for ( const auto& event : events ) { sent.push_back( event.serialized_data ); }
	}
};

// Real backend objects and event dispatch; no game start, renderer, network
// traffic, or original-game files are involved in these tests.
class BackendFixture {
public:
	BackendFixture()
		: m_random( new game::backend::Random( &m_game, 12345 ) )
		, m_gse( CreateRuntime() )
		, m_glsmac( new GLSMAC( m_gse ) ) {
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this ]() {
			auto* state = new game::backend::State( m_gse->GetGCSpace(), m_glsmac->m_ctx, m_glsmac.get() );
			m_gse->AddRootObject( state );
			state->SetGame( &m_game );
			m_game.SetState( state );
			state->m_slots->Resize( 3 );
			state->GetFM()->Add( new game::backend::faction::Faction( "GAIANS", "Gaia's Stepdaughters" ) );
			m_map.reset( new game::backend::map::Map( &m_game, 8, 8 ) );
			m_map->GetTilesPtr()->Clear();
			for ( size_t y = 0 ; y < 8 ; y++ ) {
				for ( size_t x = y & 1 ; x < 8 ; x += 2 ) {
					m_map->GetTile( x, y )->Update();
				}
			}
			m_game.m_map = m_map.get();
			m_game.m_random = m_random.get();
			m_game.m_pending_frontend_requests = &m_frontend_requests;
			m_game.m_bm = new game::backend::base::BaseManager( &m_game );
		} );
	}

	~BackendFixture() {
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this ]() {
			m_game.GetState()->DetachConnection();
		} );
		// State and BaseManager belong to this GSE's collector. Destroy its
		// space while the game, map, and frontend request queue are still alive.
		m_glsmac.reset();
		m_gse = nullptr;
		m_game.m_state = nullptr;
		m_game.m_bm = nullptr;
		m_game.m_um = nullptr;
		m_game.m_tm = nullptr;
		m_game.m_rm = nullptr;
		m_game.m_am = nullptr;
		m_game.m_map = nullptr;
		m_game.m_random = nullptr;
		m_game.m_pending_frontend_requests = nullptr;
	}

	game::backend::Game* GetGame() {
		return &m_game;
	}

	RecordingConnection* UseClientConnection( gc::Space* space ) {
		auto* connection = new RecordingConnection( space );
		m_game.GetState()->SetConnection( connection );
		return connection;
	}

	void RegisterEvent( const std::string& name, game::backend::event::EventHandler* handler ) {
		m_game.m_event_handlers.emplace( name, handler );
	}

	void ProcessEvents() { m_game.ProcessEvents(); }
	void FlushEvents() { m_game.GetState()->GetConnection()->ProcessPending( true ); }
	size_t PendingWireEvents() { return m_game.GetState()->GetConnection()->m_pending_game_events.size(); }
	size_t WaitingResponses() { return m_game.m_events_waiting_for_responses.size(); }
	void WithoutDisconnectIO( game::backend::connection::Connection* connection, const std::function< void() >& operation ) {
		// Exercise the real protocol-error path as if a disconnect were already
		// pending, without submitting a network request from an asset-free test.
		const auto saved = connection->m_mt_ids.disconnect;
		connection->m_mt_ids.disconnect = 1;
		util::FinallyGuard restore( [ & ]() { connection->m_mt_ids.disconnect = saved; } );
		operation();
	}

	void SetRunning( const bool running ) {
		m_game.m_game_state = running ? game::backend::Game::GS_RUNNING : game::backend::Game::GS_NONE;
	}

	void EnableWorldSnapshot() {
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this ]() {
			m_game.m_rm = new game::backend::resource::ResourceManager( &m_game );
			m_game.m_um = new game::backend::unit::UnitManager( &m_game );
			m_game.m_am = new game::backend::animation::AnimationManager( &m_game );
		} );
	}

	std::string WorldSnapshot() const { return m_game.SerializeWorldSnapshot(); }
	std::string WorldManagers() const {
		types::Buffer result, resources, units, bases, animations;
		m_game.GetRM()->Serialize( resources );
		m_game.GetUM()->Serialize( units );
		m_game.GetBM()->Serialize( bases );
		m_game.GetAM()->Serialize( animations );
		for ( const auto* part : { &resources, &units, &bases, &animations } ) { result.WriteString( part->ToString() ); }
		return result.ToString();
	}
	void PrepareWorldDownload( game::backend::connection::Client* client, const size_t size ) {
		m_game.m_game_state = game::backend::Game::GS_PREPARING_MAP;
		m_game.ConfigureWorldDownload( client );
		// Seed the final-chunk state, avoiding a request to a real socket. The
		// packet itself still runs through Client::ProcessEvent and Game's handler.
		client->m_download_state.is_downloading = true;
		client->m_download_state.total_size = static_cast< int >( size );
		client->m_download_state.downloaded_size = 0;
		client->m_download_state.buffer.clear();
	}
	bool IsPreparingMap() const { return m_game.m_game_state == game::backend::Game::GS_PREPARING_MAP; }
	bool HasMap() const { return m_game.m_map != nullptr; }
	bool IsDownloadFinished( game::backend::connection::Client* client ) const {
		return !client->m_download_state.is_downloading && client->m_download_state.buffer.empty();
	}
	bool IsAccumulating() const { return m_gse->GetGCSpace()->IsAccumulating(); }
	void ProcessAccumulations() {
		if ( IsAccumulating() ) { throw std::logic_error( "download fixture must drain outside accumulation" ); }
		m_gse->GetGCSpace()->ProcessAccumulations();
	}

	void WithRW( const std::function< void() >& f ) {
		m_game.WithRW( f );
	}

	const std::vector< game::FrontendRequest >& PushBaseUpdates() {
		// Frontend spawn messages need tile coordinates, but no rendered assets.
		if ( !m_map->m_map_state ) {
			NEW( m_map->m_map_state, game::backend::map::MapState );
			m_map->m_map_state->dimensions = { 8, 8 };
			const common::mt_flag_t canceled = false;
			m_map->m_map_state->LinkTileStates( canceled );
		}
		m_frontend_requests.clear();
		m_game.GetBM()->PushUpdates();
		return m_frontend_requests;
	}

	void EnableRandom() {
		m_random.reset( new game::backend::Random( &m_game, 123 ) );
		m_game.m_random = m_random.get();
	}

	const std::vector< game::FrontendRequest >& PushUnitUpdates() {
		PushBaseUpdates(); // Initializes the asset-free map's frontend coordinates.
		m_game.GetUM()->PushUpdates();
		return m_frontend_requests;
	}

	Value* RunScript( GSE_CALLABLE, const std::string& path ) {
		return m_gse->RunScript( GSE_CALL, path );
	}

	std::string WithMutableScriptValues( const std::function< std::string( GSE_CALLABLE ) >& check ) {
		return WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			std::string result;
			std::exception_ptr error;
			m_game.WithRW( [ & ]() {
				try { result = check( GSE_CALL ); }
				catch ( ... ) { error = std::current_exception(); }
			} );
			if ( error ) { std::rethrow_exception( error ); }
			return result;
		} );
	}

	std::string WithScriptValues( const std::function< std::string( GSE_CALLABLE ) >& check ) {
		std::string result;
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this, &check, &result ]() {
			try {
				gse::ExecutionPointer ep;
				result = check( m_gse->GetGCSpace(), m_game.GetState()->m_ctx, {}, ep );
			}
			catch ( const std::exception& error ) {
				// Report malformed values after leaving GC accumulation normally.
				result = error.what();
			}
		} );
		return result;
	}

	void EnableTurns() {
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this ]() {
			m_game.m_um = new game::backend::unit::UnitManager( &m_game );
		} );
	}

	void EnableYieldFixture() {
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this ]() {
			using namespace game::backend;
			m_game.m_rm = new resource::ResourceManager( &m_game );
			m_game.m_tm = new map::tile::TileManager( &m_game );
			for ( const auto* id : { "NUTRIENTS", "MINERALS", "ENERGY" } ) {
				m_game.m_rm->DefineResource( new resource::Resource( id, id, {} ) );
			}
			for ( const size_t slot : { 0, 2 } ) {
				NEWV( player, Player, "Test player", Player::PR_SINGLE, m_game.GetFaction( "GAIANS" ), "" );
				m_game.GetState()->AddPlayer( player );
				m_game.GetState()->m_slots->GetSlot( slot ).SetPlayer( player, 0, "" );
			}
		} );
	}

	void DestroyMapBeforeManagers() {
		// The registered State reaches Game's map and tile wrappers. Retire the
		// published pointer and destroy them while collection is excluded.
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this ]() {
			m_game.m_map = nullptr;
			m_map.reset();
		} );
	}

	size_t FrontendRequestCount() const { return m_frontend_requests.size(); }

	void DefinePop( const std::string& id, const std::string& name, const game::backend::base::PopDef::pop_flags_t flags ) {
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this, &id, &name, flags ]() {
			m_game.GetBM()->DefinePop( new game::backend::base::PopDef( id, name, {}, {}, flags ) );
		} );
	}

	void DefineFacility( game::backend::base::FacilityDef* def ) {
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this, def ]() {
			m_game.GetBM()->DefineFacility( def );
		} );
	}

	game::backend::base::Base* AddBaseWithPop( const size_t id, const size_t pop_id, const size_t x = 4 ) {
		using namespace game::backend::base;
		Base* base = nullptr;
		m_gse->GetGCSpace()->Accumulate( m_gse, [ & ]() {
			base = new Base( &m_game, id, &m_game.GetState()->m_slots->GetSlot( 2 ), m_game.GetFaction( "GAIANS" ),
				m_game.GetMap()->GetTile( x, 4 ), "Test base " + std::to_string( id ), {},
				{ { pop_id, Pop( nullptr, pop_id, m_game.GetBM()->GetPopDef( "WORKER" ), 0 ) } }, pop_id + 1 );
			gse::ExecutionPointer ep;
			m_game.GetBM()->SpawnBase( m_gse->GetGCSpace(), m_game.GetState()->m_ctx, {}, ep, base );
		} );
		return base;
	}

	void RestoreBaseManager( types::Buffer& snapshot ) {
		std::exception_ptr error;
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this, &snapshot, &error ]() {
			try {
				gse::ExecutionPointer ep;
				m_game.GetBM()->Deserialize( m_gse->GetGCSpace(), m_game.GetState()->m_ctx, {}, ep, snapshot );
			}
			catch ( ... ) {
				// Buffer errors are native exceptions; leave accumulation normally
				// before reporting them so the fixture's GC state stays consistent.
				error = std::current_exception();
			}
		} );
		if ( error ) {
			std::rethrow_exception( error );
		}
	}

private:
	static GSE* CreateRuntime() {
		// GLSMAC owns this runtime and destroys it through tracked DELETE.
		NEWV( runtime, GSE );
		return runtime;
	}

	game::backend::Game m_game;
	std::vector< game::FrontendRequest > m_frontend_requests;
	std::unique_ptr< game::backend::map::Map > m_map;
	std::unique_ptr< game::backend::Random > m_random;
	GSE* m_gse; // owned by m_glsmac
	std::unique_ptr< GLSMAC > m_glsmac;
};

namespace {

using namespace game::backend::map::tile;

class CountedFacilityDef : public game::backend::base::FacilityDef {
public:
	CountedFacilityDef( size_t& destructions, const std::string& id )
		: FacilityDef( id, id, "", 1, {} )
		, m_destructions( destructions ) {}
	~CountedFacilityDef() override {
		m_destructions++;
	}
private:
	size_t& m_destructions;
};

std::string CheckTileSnapshot( const Tile& expected, const Tile& actual ) {
	const auto location = " at " + expected.ToString();
	GT_ASSERT( actual.coord.x == expected.coord.x, "tile x changed" + location );
	GT_ASSERT( actual.coord.y == expected.coord.y, "tile y changed" + location );
	GT_ASSERT( *actual.elevation.bottom == *expected.elevation.bottom, "bottom elevation changed" + location );
	GT_ASSERT( *actual.elevation.left == *expected.elevation.left, "left elevation changed" + location );
	GT_ASSERT( *actual.elevation.top == *expected.elevation.top, "top elevation changed" + location );
	GT_ASSERT( *actual.elevation.right == *expected.elevation.right, "right elevation changed" + location );
	GT_ASSERT( *actual.elevation.center == *expected.elevation.center, "center elevation changed" + location );
	GT_ASSERT( actual.moisture == expected.moisture, "moisture changed" + location );
	GT_ASSERT( actual.rockiness == expected.rockiness, "rockiness changed" + location );
	GT_ASSERT( actual.bonus == expected.bonus, "bonus changed" + location );
	GT_ASSERT( actual.features == expected.features, "features changed" + location );
	GT_ASSERT( actual.terraforming == expected.terraforming, "terraforming changed" + location );
	GT_ASSERT( actual.is_water_tile == expected.is_water_tile, "water classification changed" + location );
	GT_OK();
}

}

static void AddEventTransportTests( task::gsetests::GSETests* task ) {
	task->AddTest(
		"test optimistic population removal sends its original reference to the authority",
		GT() {
			using namespace game::backend::base;
			using namespace game::backend::event;
			const auto next_id = Base::GetNextId();
			util::FinallyGuard restore_next_id( [ next_id ]() { Base::SetNextId( next_id ); } );
			size_t local_applies = 0, authoritative_applies = 0;
			BackendFixture client;
			BackendFixture authority;
			client.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
			authority.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
			auto* local_base = client.AddBaseWithPop( 201, 7 );
			auto* authoritative_base = authority.AddBaseWithPop( 201, 7 );
			const std::string name = "transport_remove_population";
			const auto install = [ &name ]( BackendFixture& fixture, size_t& applies, GSE_CALLABLE ) {
				auto* validate = NATIVE_CALL() {
					auto* event = VALUE_DATA( value::Object, arguments.at( 0 ) );
					if ( VALUE_GET( value::Int, event->Get( "caller" ) ) != 2 ) {
						return VALUE( value::String,, "only the owner may remove this population" );
					}
					return VALUE( value::Undefined );
				} );
				auto* apply = NATIVE_CALL( &fixture, &applies ) {
					if ( fixture.GetGame()->GetState()->GetConnection() && fixture.PendingWireEvents() ) {
						THROW( "event was queued before optimistic application succeeded" );
					}
					auto* data = VALUE_DATA( value::Object, VALUE_DATA( value::Object, arguments.at( 0 ) )->Get( "data" ) );
					auto* base = Base::Unwrap( data->Get( "base" ) );
					auto* pop = Pop::Unwrap( data->Get( "pop" ) );
					const auto id = pop->m_id;
					base->RemovePop( id );
					++applies;
					return VALUE( value::Int,, id );
				} );
				auto* rollback = NATIVE_CALL() { return VALUE( value::Undefined ); } );
				fixture.RegisterEvent( name, new EventHandler( gc_space, name, validate, nullptr, apply, rollback ) );
			};
			std::string wire;
			const auto local_error = client.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* connection = client.UseClientConnection( gc_space );
				install( client, local_applies, GSE_CALL );
				auto* pop = local_base->m_pops.at( 7 ).Wrap( GSE_CALL );
				auto* event = new Event( client.GetGame(), Event::ES_LOCAL, 2, GSE_CALL, name,
					{ { "base", local_base->Wrap( GSE_CALL ) }, { "pop", pop } }, "optimistic-removal" );
				client.GetGame()->AddEvent( event );
				client.ProcessEvents();
				GT_ASSERT( local_applies == 1 && local_base->m_pops.empty() && pop->type == VT_UNDEFINED, "optimistic removal did not invalidate its live wrapper" );
				GT_ASSERT( client.WaitingResponses() == 1 && client.PendingWireEvents() == 1, "request was not queued after optimistic application" );
				client.FlushEvents();
				GT_ASSERT( connection->sent.size() == 1, "connection did not send exactly one request" );
				wire = connection->sent.front();
				GT_OK();
			} );
			GT_ASSERT( local_error.empty(), "local transport processing failed: " + local_error );
			const auto authority_error = authority.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				install( authority, authoritative_applies, GSE_CALL );
				auto* event = Event::Deserialize( authority.GetGame(), Event::ES_CLIENT, GSE_CALL, wire );
				auto* data = VALUE_DATA( value::Object, event->GetData().at( "data" ) );
				GT_ASSERT( data->Get( "pop" )->type == VT_OBJECT, "wire request lost the population reference after optimistic removal" );
				GT_ASSERT( Pop::Unwrap( data->Get( "pop" ) ) == &authoritative_base->m_pops.at( 7 ), "request did not resolve the authority's population" );
				GT_ASSERT( event->GetId() == "optimistic-removal" && event->GetCaller() == 2 && event->GetEventName() == name, "request metadata changed" );
				authority.GetGame()->AddEvent( event );
				authority.ProcessEvents();
				GT_ASSERT( authoritative_applies == 1 && authoritative_base->m_pops.empty(), "authority did not apply the received removal" );
				GT_OK();
			} );
			GT_ASSERT( authority_error.empty(), "authoritative transport processing failed: " + authority_error );
			GT_OK();
		}
	);
	task->AddTest(
		"test local transport still defers resolved events and does not queue failed applications",
		GT() {
			using namespace game::backend::event;
			for ( const bool deferred : { false, true } ) {
				BackendFixture fixture;
				const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* connection = fixture.UseClientConnection( gc_space );
					auto* noop = NATIVE_CALL() { return VALUE( value::Undefined ); } );
					auto* fail = NATIVE_CALL() { THROW( "injected apply failure" ); } );
					fixture.RegisterEvent( "transport_test", new EventHandler( gc_space, "transport_test", noop, deferred ? noop : nullptr, fail, noop ) );
					fixture.GetGame()->AddEvent( new Event( fixture.GetGame(), Event::ES_LOCAL, 2, GSE_CALL, "transport_test", {}, "transport-order" ) );
					bool failed = false;
					try { fixture.ProcessEvents(); }
					catch ( const std::runtime_error& exception ) {
						failed = true;
						GT_ASSERT( std::string( exception.what() ).find( "injected apply failure" ) != std::string::npos, "unexpected event processing failure" );
					}
					GT_ASSERT( failed != deferred, "resolution/application timing changed" );
					GT_ASSERT( fixture.WaitingResponses() == ( deferred ? 1 : 0 ), "failed application left response bookkeeping" );
					fixture.FlushEvents();
					GT_ASSERT( connection->sent.size() == ( deferred ? 1 : 0 ), "failed application queued an outgoing request" );
					GT_OK();
				} );
				GT_ASSERT( error.empty(), "transport ordering check failed: " + error );
			}
			GT_OK();
		}
	);
}

static void AddEventResponseTests( task::gsetests::GSETests* task ) {
	for ( const bool deferred : { false, true } ) {
		for ( const bool accepted : { false, true } ) {
			task->AddTest(
				"test " + std::string( deferred ? "deferred" : "optimistic" ) + " event response is " + ( accepted ? "accepted" : "rejected" ),
				GT( deferred, accepted ) {
					using namespace game::backend::event;
					size_t applies = 0, rollbacks = 0;
					int state = 10;
					BackendFixture fixture;
					const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
						auto* connection = fixture.UseClientConnection( gc_space );
						auto* validate = NATIVE_CALL() { return VALUE( value::Undefined ); } );
						auto* resolve = NATIVE_CALL() { THROW( "client must not resolve its request" ); } );
						auto* apply = NATIVE_CALL( &applies, &state, deferred ) {
							auto* request = VALUE_DATA( value::Object, arguments.at( 0 ) );
							const auto previous = state;
							state += deferred ? VALUE_GET( value::Int, request->Get( "resolved" ) ) : 7;
							++applies;
							return VALUE( value::Int,, previous );
						} );
						auto* rollback = NATIVE_CALL( &rollbacks, &state ) {
							auto* request = VALUE_DATA( value::Object, arguments.at( 0 ) );
							state = VALUE_GET( value::Int, request->Get( "applied" ) );
							++rollbacks;
							return VALUE( value::Undefined );
						} );
						fixture.RegisterEvent( "response_test", new EventHandler( gc_space, "response_test", validate, deferred ? resolve : nullptr, apply, rollback ) );
						fixture.GetGame()->AddEvent( new Event( fixture.GetGame(), Event::ES_LOCAL, 2, GSE_CALL, "response_test", {}, "pending-request" ) );
						fixture.ProcessEvents();
						GT_ASSERT( applies == ( deferred ? 0 : 1 ) && rollbacks == 0 && state == ( deferred ? 10 : 17 ), "request application timing changed" );
						GT_ASSERT( fixture.WaitingResponses() == 1, "request was not retained for a response" );
						fixture.FlushEvents();
						GT_ASSERT( connection->sent.size() == 1, "request was not sent exactly once" );
						fixture.GetGame()->AddEventResponse( "pending-request", accepted, deferred && accepted ? VALUE( value::Int,, 37 ) : nullptr );
						fixture.GetGame()->Iterate();
						GT_ASSERT( applies == ( deferred && !accepted ? 0 : 1 ), "response applied the request the wrong number of times" );
						GT_ASSERT( rollbacks == ( !deferred && !accepted ? 1 : 0 ), "response rolled back the request the wrong number of times" );
						GT_ASSERT( state == ( accepted ? ( deferred ? 47 : 17 ) : 10 ), "response did not preserve resolved or rollback data" );
						GT_ASSERT( fixture.WaitingResponses() == 0, "completed response left the request pending" );
						fixture.GetGame()->Iterate();
						GT_ASSERT( applies == ( deferred && !accepted ? 0 : 1 ) && rollbacks == ( !deferred && !accepted ? 1 : 0 ), "response was processed again on the next iteration" );
						GT_OK();
					} );
					GT_ASSERT( error.empty(), "event acknowledgement failed: " + error );
					GT_OK();
				}
			);
		}
	}
}

static void AddPopReferenceTests( task::gsetests::GSETests* task ) {
	task->AddTest(
		"test nested event population references resolve in the destination base",
		GT() {
			using namespace game::backend::base;
			using game::backend::event::Event;
			const auto next_id = Base::GetNextId();
			util::FinallyGuard restore_next_id( [ next_id ]() { Base::SetNextId( next_id ); } );
			BackendFixture source;
			BackendFixture destination;
			source.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
			destination.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
			auto* source_base = source.AddBaseWithPop( 201, 7 );
			auto* other_base = destination.AddBaseWithPop( 202, 7, 6 );
			auto* destination_base = destination.AddBaseWithPop( 201, 7 );
			types::Buffer snapshot;
			const auto write_error = source.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* pop = source_base->m_pops.at( 7 ).Wrap( GSE_CALL );
				types::Buffer reference;
				Value::Serialize( &reference, pop );
				types::Buffer expected;
				expected.WriteInt( VT_OBJECT );
				expected.WriteString( "Pop" );
				expected.WriteInt( 201 );
				expected.WriteInt( 7 );
				GT_ASSERT( reference.ToString() == expected.ToString(), "population reference is not the base/pop ID pair" );
				auto* object = new value::Object( GSE_CALL, { { "pop", pop } } );
				auto* array = new value::Array( gc_space, { object } );
				auto* event = new Event( source.GetGame(), Event::ES_LOCAL, 2, GSE_CALL, "population_change",
					{ { "base", source_base->Wrap( GSE_CALL ) }, { "changes", array } }, "pop-reference-test" );
				event->SetResolved( new value::Object( GSE_CALL, { { "pops", new value::Array( gc_space, { pop } ) } } ) );
				snapshot = event->Serialize();
				GT_OK();
			} );
			GT_ASSERT( write_error.empty(), "population event serialization failed: " + write_error );
			const auto read_error = destination.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* event = Event::Deserialize( destination.GetGame(), Event::ES_SERVER, GSE_CALL, snapshot );
				GT_ASSERT( event->GetId() == "pop-reference-test" && event->GetEventName() == "population_change" && event->GetCaller() == 2, "event metadata changed" );
				auto* data = VALUE_DATA( value::Object, event->GetData().at( "data" ) );
				GT_ASSERT( Base::Unwrap( data->Get( "base" ) ) == destination_base, "existing base reference format changed" );
				auto* changes = VALUE_DATA( value::Array, data->Get( "changes" ) );
				auto* change = VALUE_DATA( value::Object, changes->Get( 0 ) );
				auto* pop = Pop::Unwrap( change->Get( "pop" ) );
				GT_ASSERT( pop == &destination_base->m_pops.at( 7 ), "event pop did not resolve in destination base" );
				GT_ASSERT( pop != &source_base->m_pops.at( 7 ) && pop != &other_base->m_pops.at( 7 ), "event pop resolved in the wrong world or base" );
				auto* resolved = VALUE_DATA( value::Object, event->GetResolved() );
				auto* pops = VALUE_DATA( value::Array, resolved->Get( "pops" ) );
				GT_ASSERT( Pop::Unwrap( pops->Get( 0 ) ) == pop, "resolved event result lost its population reference" );
				GT_ASSERT( event->Serialize().ToString() == snapshot.ToString(), "event bytes changed after population reference reload" );
				GT_OK();
			} );
			GT_ASSERT( read_error.empty(), "population event deserialization failed: " + read_error );
			GT_OK();
		}
	);
	task->AddTest(
		"test population reference deserialization rejects missing and removed members",
		GT() {
			using namespace game::backend::base;
			const auto next_id = Base::GetNextId();
			util::FinallyGuard restore_next_id( [ next_id ]() { Base::SetNextId( next_id ); } );
			BackendFixture fixture;
			fixture.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
			auto* base = fixture.AddBaseWithPop( 201, 7 );
			const auto make_reference = []( long long base_id, long long pop_id ) {
				types::Buffer buffer;
				buffer.WriteInt( VT_OBJECT );
				buffer.WriteString( "Pop" );
				buffer.WriteInt( base_id );
				buffer.WriteInt( pop_id );
				return buffer;
			};
			for ( const auto ids : { std::pair< long long, long long >{ 999, 7 }, { 201, 999 }, { -1, 7 }, { 201, -1 } } ) {
				auto reference = make_reference( ids.first, ids.second );
				const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					Value::Deserialize( GSE_CALL, &reference, fixture.GetGame() );
					GT_OK();
				} );
				GT_ASSERT( error.find( "pop reference" ) != std::string::npos, "invalid population reference was not rejected: " + error );
			}
			const auto removal_error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto valid = make_reference( 201, 7 );
				auto* wrapper = Value::Deserialize( GSE_CALL, &valid, fixture.GetGame() );
				GT_ASSERT( Pop::Unwrap( wrapper ) == &base->m_pops.at( 7 ), "valid population reference failed" );
				base->RemovePop( 7 );
				GT_ASSERT( wrapper->type == VT_UNDEFINED, "removed population left a live script wrapper" );
				auto removed = make_reference( 201, 7 );
				try {
					Value::Deserialize( GSE_CALL, &removed, fixture.GetGame() );
				}
				catch ( const std::runtime_error& error ) {
					GT_ASSERT( std::string( error.what() ).find( "pop reference" ) != std::string::npos, "unexpected removed population error" );
					GT_OK();
				}
				GT_FAIL( "removed population reference was accepted" );
			} );
			GT_ASSERT( removal_error.empty(), "population removal reference check failed: " + removal_error );
			GT_OK();
		}
	);
	task->AddTest(
		"test population reference serialization rejects a detached population",
		GT() {
			BackendFixture fixture;
			const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				game::backend::base::Pop detached;
				types::Buffer reference;
				Value::Serialize( &reference, detached.Wrap( GSE_CALL ) );
				GT_OK();
			} );
			GT_ASSERT( error.find( "pop reference" ) != std::string::npos, "detached population reference was not rejected: " + error );
			GT_OK();
		}
	);
}

// Expose the production packet dispatcher, without opening sockets.
class RulesetClient : public game::backend::connection::Client {
public:
	using Client::Client;
	bool HasProtocolError() const { return m_disconnect_reason == "Network protocol error"; }
	const std::string& DisconnectReason() const { return m_disconnect_reason; }
	void Receive( const types::Packet& packet ) {
		network::Event event;
		event.type = network::Event::ET_PACKET;
		event.data.packet_data = packet.Serialize().ToString();
		ProcessEvent( event );
	}
};

static void AddBackendFixtureLifecycleTests( task::gsetests::GSETests* task ) {
	task->AddTest( "test backend fixture retires map and tile wrappers under GC exclusion", GT() {
		for ( const bool already_accumulating : { false, true } ) {
			BackendFixture fixture;
			fixture.DefinePop( "WORKER", "Worker", game::backend::base::PopDef::PF_TILE_WORKER );
			auto* map = fixture.GetGame()->GetMap();
			auto* tile = map->GetTile( 2, 2 );
			const Wrappable* const map_dependency = map;
			const Wrappable* const tile_dependency = tile;
			size_t map_notifications = 0, tile_notifications = 0, unprotected = 0, still_published = 0;
			class DestructionObserver final : public Wrappable {
			public:
				std::function< void( const Wrappable* ) > on_destroy;
				Value* const Wrap( GSE_CALLABLE ) override { return nullptr; }
				void NotifyDependencyDestruction( const Wrappable* const dependency ) override { on_destroy( dependency ); }
			} observer;
			observer.on_destroy = [ & ]( const Wrappable* dependency ) {
				map_notifications += dependency == map_dependency;
				tile_notifications += dependency == tile_dependency;
				unprotected += !fixture.IsAccumulating();
				still_published += fixture.HasMap();
			};
			bool map_attached = false, tile_attached = false;
			// If setup/assertion fails before retirement, detach while the runtime
			// and dependencies are alive; the stack observer must never dangle.
			util::FinallyGuard detach( [ & ]() {
				if ( fixture.HasMap() && ( map_attached || tile_attached ) ) {
					fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
						if ( map_attached ) { map->Undepend( &observer ); }
						if ( tile_attached ) { tile->Undepend( &observer ); }
						GT_OK();
					} );
				}
			} );
			Value* map_value = nullptr;
			Value* tile_value = nullptr;
			const auto setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				map_value = map->Wrap( GSE_CALL ); tile_value = tile->Wrap( GSE_CALL );
				ctx->CreateConst( "retired_map", map_value, si, ep );
				ctx->CreateConst( "retired_tile", tile_value, si, ep );
				map->Depend( &observer ); map_attached = true;
				tile->Depend( &observer ); tile_attached = true;
				GT_OK();
			} );
			GT_ASSERT( setup.empty(), "retirement observer setup failed: " + setup );
			if ( already_accumulating ) {
				const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					fixture.DestroyMapBeforeManagers();
					GT_OK();
				} );
				GT_ASSERT( error.empty(), "nested retirement failed: " + error );
			}
			else { fixture.DestroyMapBeforeManagers(); }
			GT_ASSERT( map_notifications == 1 && tile_notifications == 1, "real map/tile destruction notifications missing or duplicated" );
			GT_ASSERT( unprotected == 0, "map or tile destruction ran without GC collection exclusion" );
			GT_ASSERT( still_published == 0 && !fixture.HasMap(), "map remained published during wrapper destruction" );
			const auto checked = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				GT_ASSERT( map_value->type == VT_UNDEFINED && tile_value->type == VT_UNDEFINED, "retired map/tile wrappers remained valid" );
				GT_ASSERT( fixture.GetGame()->GetBM()->GetPopDef( "WORKER" ), "map retirement destroyed the retained manager" );
				GT_OK();
			} );
			GT_ASSERT( checked.empty(), "retired fixture validation failed: " + checked );
			fixture.DestroyMapBeforeManagers();
			GT_ASSERT( map_notifications == 1 && tile_notifications == 1, "empty retirement destroyed dependencies twice" );
		}
		GT_OK();
	} );
}

static bool RejectsSettings( const std::function< void() >& operation ) {
	try { operation(); }
	catch ( const std::runtime_error& ) { return true; }
	return false;
}

static void AddRulesetTests( task::gsetests::GSETests* task ) {
	task->AddTest( "test production faction configure registers both rulesets", GT() {
		using game::backend::faction::Faction;
		BackendFixture fixture;
		const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* fm = fixture.GetGame()->GetState()->GetFM();
			auto* wrapped = static_cast< value::Object* >( fm->Wrap( GSE_CALL ) );
			// Only asset reads are substituted. Production configure invokes the
			// real native add callable and populates the State's FactionManager.
			auto* imports = new value::Object( GSE_CALL, {
				{ "add", wrapped->Get( "add" ) },
				{ "import_colors", NATIVE_CALL() {
					N_EXPECT_ARGS( 1 );
					N_GETVALUE( filename, 0, String );
					return new value::Object( GSE_CALL, {
						{ "text", types::Color( 1.0f, 0.0f, 0.0f ).Wrap( GSE_CALL ) },
						{ "text_shadow", types::Color( 0.0f, 1.0f, 0.0f ).Wrap( GSE_CALL ) },
						{ "border", types::Color( 0.0f, 0.0f, 1.0f ).Wrap( GSE_CALL ) },
					} );
				} ) },
				{ "import_base_names", NATIVE_CALL() {
					N_EXPECT_ARGS( 1 );
					N_GETVALUE( filename, 0, String );
					return new value::Object( GSE_CALL, {
						{ "land", VALUE( value::Array,, { VALUE( value::String,, filename ) } ) },
						{ "water", VALUE( value::Array,, {} ) },
					} );
				} ) },
			} );
			auto* module = static_cast< value::Object* >( fixture.RunScript( GSE_CALL, "./GLSMAC_data/default/factions" ) );
			auto* configure = static_cast< value::Callable* >( module->Get( "configure" ) );
			const std::vector< std::string > names = {
				"Gaians", "Hive", "University", "Morganites", "Spartans", "Believers", "Peacekeepers",
				"Consciousness", "Pirates", "Drones", "Angels", "Planetcult", "Caretakers", "Usurpers"
			};
			const std::vector< std::string > files = {
				"gaians", "hive", "univ", "morgan", "spartans", "believe", "peace",
				"cyborg", "pirates", "drone", "angels", "fungboy", "caretake", "usurper"
			};
			for ( const std::string id : { "smac", "smax", "smac" } ) {
				fm->Clear();
				configure->Run( GSE_CALL, { imports, VALUE( value::String,, id ) } );
				const auto factions = fm->GetAll();
				const size_t count = id == "smac" ? 7 : 14;
				GT_ASSERT( factions.size() == count, "production faction roster count is incorrect" );
				for ( size_t i = 0 ; i < count ; i++ ) {
					const auto* faction = factions.at( i );
					GT_ASSERT( faction->m_name == names.at( i ), "production faction order/name changed" );
					GT_ASSERT( faction->m_bases_render.file == files.at( i ) + ".pcx", "faction selected the wrong sprite file" );
					GT_ASSERT( faction->m_base_names.land == std::vector< std::string >{ files.at( i ) + ".txt" }, "faction selected the wrong names file" );
					const auto flags = i == 8 ? Faction::FF_NAVAL : i >= 12 ? Faction::FF_PROGENITOR : Faction::FF_NONE;
					GT_ASSERT( faction->m_flags == flags, "production faction flags changed" );
				}
				GT_ASSERT( fm->Get( "GAIANS" ) == factions.front(), "faction ID was not uppercased" );
				GT_ASSERT( !!fm->Get( "USURPERS" ) == ( id == "smax" ), "SMAX faction leaked across roster reset" );
			}
			GT_OK();
		} );
		GT_ASSERT( error.empty(), "production faction registration failed: " + error );
		GT_OK();
	} );
	task->AddTest( "test explicit ruleset selection and versioned settings rejection", GT() {
		using game::backend::settings::GlobalSettings;
		GlobalSettings settings;
		GT_ASSERT( settings.GetRulesetId() == "unselected", "new session silently selected a ruleset" );
		GT_ASSERT( RejectsSettings( [ & ]() { settings.Serialize(); } ), "unset settings serialized" );
		GT_ASSERT( RejectsSettings( [ & ]() { settings.LockRuleset(); } ), "unset settings initialized" );
		GT_ASSERT( RejectsSettings( [ & ]() { settings.SelectRuleset( "gog" ); } ), "installer type accepted as ruleset" );
		settings.SelectRuleset( "smac" );
		settings.InitializeHostRules( smac_rules_fixture::Source );
		settings.game_name = "unchanged";
		settings.map.size_x = 86;
		const auto before = settings.Serialize().ToString();
		std::vector< types::Buffer > invalid( 4 );
		invalid[ 0 ].WriteString( settings.map.Serialize().ToString() ); // legacy, string-first format
		invalid[ 1 ].WriteInt( 3 ); // unsupported version
		invalid[ 2 ].WriteInt( 2 );
		invalid[ 2 ].WriteString( "unselected" );
		invalid[ 3 ].WriteInt( 2 ); // truncated before identity
		for ( const auto& snapshot : invalid ) {
			GT_ASSERT( RejectsSettings( [ & ]() { settings.Deserialize( snapshot ); } ), "invalid settings accepted" );
			GT_ASSERT( settings.Serialize().ToString() == before, "identity rejection mutated settings" );
		}
		settings.LockRuleset();
		GT_ASSERT( RejectsSettings( [ & ]() { settings.SelectRuleset( "smax" ); } ), "initialized session changed ruleset" );
		GlobalSettings changed;
		changed.SelectRuleset( "smax" );
		GT_ASSERT( RejectsSettings( [ & ]() { settings.Deserialize( changed.Serialize() ); } ), "host changed a locked identity" );
		GT_ASSERT( settings.Serialize().ToString() == before, "host identity rejection mutated settings" );
		settings.Deserialize( types::Buffer( before ) ); // same-identity lobby update is allowed
		settings.ResetRuleset();
		GT_ASSERT( settings.GetRulesetId() == "unselected", "new session inherited previous identity" );
		settings.SelectRuleset( "smax" );
		GT_ASSERT( settings.GetRulesetId() == "smax", "new session could not select another identity" );
		GT_OK();
	} );

	task->AddTest( "test ruleset State and Packet roundtrip reaches production GSE", GT() {
		for ( const auto& id : { "smac", "smax" } ) {
			BackendFixture source, destination;
			auto* state = source.GetGame()->GetState();
			state->m_settings.global.SelectRuleset( id );
			state->m_settings.PrepareRuleset( smac_rules_fixture::Source );
			state->m_settings.global.game_name = "Ruleset roundtrip";
			state->m_settings.global.map.size_x = 86;
			types::Packet packet( types::Packet::PT_GLOBAL_SETTINGS );
			packet.data.str = state->Serialize().ToString();
			types::Packet decoded( types::Packet::PT_NONE );
			decoded.Deserialize( packet.Serialize() );
			auto* restored = destination.GetGame()->GetState();
			restored->Deserialize( types::Buffer( decoded.data.str ) );
			GT_ASSERT( std::string( id ) == "smac" ? smac_rules_fixture::Matches( restored->m_settings.global ) : !restored->m_settings.global.GetSmacBaseSquareRules(), "State/Packet lost host numeric rules" );
			GT_ASSERT( restored->Serialize().ToString() == state->Serialize().ToString(), "state roundtrip changed settings or factions" );
			const auto error = destination.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* script = destination.RunScript( GSE_CALL, "./GLSMAC_data/tests/_ruleset" );
				static_cast< value::Callable* >( script )->Run( GSE_CALL, {
					destination.GetGame()->Wrap( GSE_CALL ), restored->m_settings.global.Wrap( GSE_CALL ), new value::String( gc_space, id )
				} );
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "production ruleset read failed: " + error );
		}
		GT_OK();
	} );

	task->AddTest( "test joining and rejoining defer state callbacks until host ruleset arrives", GT() {
		using namespace game::backend;
		BackendFixture destination;
		for ( const auto& id : { "smax", "smac" } ) {
			for ( const auto announced : { connection::Connection::GS_LOBBY, connection::Connection::GS_RUNNING } ) {
				BackendFixture source;
				source.GetGame()->GetState()->m_settings.global.SelectRuleset( id );
				source.GetGame()->GetState()->m_settings.PrepareRuleset( smac_rules_fixture::Source );
				types::Packet settings( types::Packet::PT_GLOBAL_SETTINGS );
				settings.data.str = source.GetGame()->GetState()->Serialize().ToString();
				const auto error = destination.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* state = destination.GetGame()->GetState();
					state->m_settings.local.network_role = game::backend::settings::LocalSettings::NR_CLIENT;
					state->m_settings.PrepareRuleset( []( const std::string& ) -> std::string { throw std::runtime_error( "client tried reading local alpha.txt" ); } );
					if ( state->m_slots->GetSlot( 0 ).GetState() != slot::Slot::SS_PLAYER ) {
						NEWV( host, Player, "Host", Player::PR_HOST, nullptr, "Transcend" );
						state->AddPlayer( host );
						state->m_slots->GetSlot( 0 ).SetPlayer( host, 0, "" );
					}
					auto* client = new RulesetClient( gc_space, &state->m_settings.local );
					state->SetConnection( client );
					util::FinallyGuard cleanup( [ & ]() {
						client->ResetHandlers();
						state->DetachConnection();
					} );
					size_t callbacks = 0;
					std::string callback_ruleset;
					bool callback_rules_valid = false;
					client->m_on_game_state_change = [ & ]( const connection::Connection::game_state_t actual ) {
						callbacks++;
						callback_ruleset = actual == announced ? state->m_settings.global.GetRulesetId() : "wrong state";
						callback_rules_valid = std::string( id ) == "smac" ? smac_rules_fixture::Matches( state->m_settings.global ) : !state->m_settings.global.GetSmacBaseSquareRules();
					};
					size_t script_callbacks = 0;
					std::string script_ruleset;
					const auto listener = client->On( GSE_CALL, "game_state", NATIVE_CALL( state, &script_callbacks, &script_ruleset ) {
						script_callbacks++;
						auto* wrapped = static_cast< value::Object* >( state->GetGame()->Wrap( GSE_CALL ) );
						auto* result = static_cast< value::Callable* >( wrapped->Get( "get_ruleset" ) )->Run( GSE_CALL, {} );
						script_ruleset = VALUE_DATA( value::String, result )->value;
						return VALUE( value::Undefined );
					} ) );
					util::FinallyGuard remove_listener( [ & ]() { client->Off( GSE_CALL, "game_state", listener ); } );
					types::Packet status( types::Packet::PT_GAME_STATE );
					status.udata.game_state.state = announced;
					client->Receive( status );
					GT_ASSERT( callbacks == 0 && script_callbacks == 0 && !client->HasGlobalSettings(), "state callback ran before host identity" );
					client->Receive( settings );
					GT_ASSERT( callbacks == 1 && client->HasGlobalSettings(), "state callback not released after settings" );
					GT_ASSERT( callback_ruleset == id, "callback observed stale or unset ruleset" );
					GT_ASSERT( callback_rules_valid, "state callback ran without the host's numeric rules" );
					GT_ASSERT( script_callbacks == 1 && script_ruleset == id, "GSE state event observed stale or unset ruleset" );
					client->Receive( status );
					GT_ASSERT( callbacks == 1 && script_callbacks == 1, "duplicate state notification" );
					GT_OK();
				} );
				GT_ASSERT( error.empty(), "join packet dispatch failed: " + error );
			}
		}
		GT_OK();
	} );
}

static void AddHostRulesPacketTests( task::gsetests::GSETests* task ) {
	task->AddTest( "test real client rejects missing and tampered host rules without publishing callbacks or state", GT() {
		using namespace game::backend;
		for ( const int scenario : { 0, 1, 2 } ) {
			BackendFixture source, destination;
			auto* host_state = source.GetGame()->GetState();
			host_state->m_settings.global.SelectRuleset( "smac" );
			host_state->m_settings.PrepareRuleset( smac_rules_fixture::Source );
			types::Packet valid( types::Packet::PT_GLOBAL_SETTINGS );
			valid.data.str = host_state->Serialize().ToString();
			auto host_snapshot = host_state->Serialize();
			host_snapshot.ReadString();
			const auto factions = host_snapshot.ReadString();
			std::array< int64_t, 9 > values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 };
			if ( scenario == 1 ) { values[ 4 ] = int64_t( INT32_MAX ) + 1; }
			if ( scenario == 2 ) { values[ 7 ] = 77; }
			types::Buffer invalid_state;
			invalid_state.WriteString( smac_rules_fixture::Payload( "smac", scenario != 0, values ).ToString() );
			invalid_state.WriteString( factions );
			types::Packet invalid( types::Packet::PT_GLOBAL_SETTINGS );
			invalid.data.str = invalid_state.ToString();
			const auto error = destination.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* state = destination.GetGame()->GetState();
				state->m_settings.local.network_role = settings::LocalSettings::NR_CLIENT;
				state->m_settings.PrepareRuleset( []( const std::string& ) -> std::string { throw std::runtime_error( "client read local file" ); } );
				NEWV( host, Player, "Host", Player::PR_HOST, nullptr, "Transcend" );
				state->AddPlayer( host );
				state->m_slots->GetSlot( 0 ).SetPlayer( host, 0, "" );
				auto* client = new RulesetClient( gc_space, &state->m_settings.local );
				state->SetConnection( client );
				util::FinallyGuard cleanup( [ & ]() { client->ResetHandlers(); state->DetachConnection(); } );
				size_t callbacks = 0, settings_updates = 0;
				client->m_on_game_state_change = [ & ]( connection::Connection::game_state_t ) { ++callbacks; };
				client->m_on_global_settings_update = [ & ]() { ++settings_updates; };
				types::Packet status( types::Packet::PT_GAME_STATE );
				status.udata.game_state.state = connection::Connection::GS_LOBBY;
				client->Receive( status );
				if ( scenario == 2 ) { client->Receive( valid ); } // Lock before the tampered update.
				const auto before_map = state->m_settings.global.map.Serialize().ToString();
				const auto before_name = state->m_settings.global.game_name;
				const auto before_factions = state->GetFM()->Serialize().ToString();
				const auto expected_updates = scenario == 2 ? 1 : 0;
				destination.WithoutDisconnectIO( client, [ & ]() { client->Receive( invalid ); } );
				GT_ASSERT( client->HasProtocolError(), "invalid host payload did not reach the protocol error path" );
				GT_ASSERT( callbacks == expected_updates && settings_updates == expected_updates, "rejected host payload published callbacks" );
				GT_ASSERT( client->HasGlobalSettings() == ( scenario == 2 ), "rejected host rules were accepted as initialized settings" );
				GT_ASSERT( state->m_settings.global.map.Serialize().ToString() == before_map && state->m_settings.global.game_name == before_name, "rejected packet changed live global settings" );
				GT_ASSERT( state->GetFM()->Serialize().ToString() == before_factions, "rejected rules packet reached faction publication" );
				GT_ASSERT( scenario == 2 ? smac_rules_fixture::Matches( state->m_settings.global ) : state->m_settings.global.GetRulesetId() == "unselected" && !state->m_settings.global.GetSmacBaseSquareRules(), "rejected packet changed session identity or values" );
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "host rules packet rejection failed: " + error );
		}
		GT_OK();
	} );
}

static void AddWorldSnapshotTests( task::gsetests::GSETests* task ) {
	task->AddTest( "actual world snapshot writer prefixes frozen profile without changing body layout", GT() {
		for ( const auto* id : { "smac", "smax" } ) {
			BackendFixture fixture;
			fixture.EnableWorldSnapshot();
			auto* game = fixture.GetGame();
			auto& settings = game->GetState()->m_settings.global;
			settings.SelectRuleset( id );
			settings.InitializeHostRules( smac_rules_fixture::Source );
			types::Buffer old_body;
			game->GetMap()->SaveToBuffer( old_body );
			const auto managers = fixture.WorldManagers();
			// Append the old body frames independently of the production writer.
			types::Buffer manager_frames( managers );
			for ( size_t i = 0 ; i < 4 ; ++i ) { old_body.WriteString( manager_frames.ReadString() ); }
			old_body.WriteInt( game->GetTurnId() );
			const auto snapshot = fixture.WorldSnapshot();
			types::Buffer input( snapshot );
			game::backend::world_snapshot::ValidateHeader( input, settings );
			GT_ASSERT( snapshot.substr( input.lenr ) == old_body.ToString(), "world body order or content changed" );
			fixture.DestroyMapBeforeManagers();
			GT_ASSERT( fixture.WorldSnapshot().empty(), "not-yet-generated map no longer returns an empty download response" );
		}
		GT_OK();
	} );
	for ( const bool queued : { false, true } ) {
		task->AddTest( std::string( "real final download chunk rejects invalid world profile before map allocation " ) + ( queued ? "queued" : "immediate" ), GT( queued ) {
			using namespace game::backend;
			auto rejected = world_rules_fixture::InvalidSmacHeaders();
			// Matching headers reach the actual body's first read, still before
			// Map's asset-bearing constructor. Native errors there must also be
			// caught when completion executes later on the accumulation thread.
			rejected.emplace_back( "matching header without map frame", world_rules_fixture::Header().ToString() );
			auto wrong_body = world_rules_fixture::Header();
			wrong_body.WriteInt( 0 );
			rejected.emplace_back( "matching header with wrong map frame type", wrong_body.ToString() );
			for ( const auto& invalid : rejected ) {
				BackendFixture fixture;
				fixture.EnableWorldSnapshot();
				fixture.DefinePop( "WORKER", "Worker", base::PopDef::PF_TILE_WORKER );
				fixture.DestroyMapBeforeManagers();
				auto* game = fixture.GetGame();
				auto* state = game->GetState();
				RulesetClient* client = nullptr;
				// State is a runtime root and reaches its attached Client. Keep that
				// ownership across setup, worker dispatch, owner drain and assertions.
				util::FinallyGuard cleanup( [ & ]() {
					fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
						if ( client ) { client->ResetHandlers(); state->DetachConnection(); }
						GT_OK();
					} );
				} );
				types::Packet packet( types::Packet::PT_DOWNLOAD_NEXT_CHUNK_RESPONSE );
				packet.data.str = invalid.second;
				packet.udata.download.offset = 0;
				packet.udata.download.size = packet.data.str.size();
				const auto setup_error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					state->m_settings.global.SelectRuleset( "smac" );
					state->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
					client = new RulesetClient( gc_space, &state->m_settings.local );
					state->SetConnection( client );
					fixture.PrepareWorldDownload( client, packet.data.str.size() );
					GT_OK();
				} );
				GT_ASSERT( setup_error.empty(), "download setup failed: " + setup_error );
				const auto before_settings = state->m_settings.global.Serialize().ToString();
				const auto before_managers = fixture.WorldManagers();
				const auto before_turn = game->GetTurnId();
				const auto before_frontend = fixture.FrontendRequestCount();
				const auto dispatch = [ & ]() -> std::string {
					// Collect locks pending work before collection; an outer Accumulate
					// would hold collection while joining a producer waiting for pending.
					GT_ASSERT( !fixture.IsAccumulating(), "download fixture must dispatch outside accumulation" );
					std::exception_ptr escaped;
					std::string early_error;
					try {
						fixture.WithoutDisconnectIO( client, [ & ]() {
							if ( queued ) {
								std::thread worker( [ & ]() {
									try { client->Receive( packet ); }
									catch ( ... ) { escaped = std::current_exception(); }
								} );
								worker.join();
								early_error = client->DisconnectReason();
								fixture.ProcessAccumulations();
							}
							else { client->Receive( packet ); }
						} );
					}
					catch ( const std::exception& error ) { return error.what(); }
					GT_ASSERT( !escaped, "download dispatcher threw on worker thread" );
					GT_ASSERT( !queued || early_error.empty(), "queued validation ran before accumulation processing" );
					GT_OK();
				};
				const auto dispatch_error = dispatch();
				GT_ASSERT( dispatch_error.empty(), "download dispatch failed: " + dispatch_error );
				const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					GT_ASSERT( client->DisconnectReason() == "Snapshot format mismatch", "invalid profile did not safely disconnect" );
					GT_ASSERT( fixture.IsDownloadFinished( client ) && !client->m_on_download_complete && !client->m_on_download_progress, "completion did not retire download state and callbacks" );
					GT_ASSERT( !fixture.HasMap() && fixture.IsPreparingMap(), "invalid profile allocated or initialized a map" );
					GT_ASSERT( state->m_settings.global.Serialize().ToString() == before_settings, "world profile changed frozen settings" );
					GT_ASSERT( fixture.WorldManagers() == before_managers && game->GetTurnId() == before_turn && fixture.FrontendRequestCount() == before_frontend, "profile rejection changed managers, turn or frontend state" );
					GT_OK();
				} );
				GT_ASSERT( error.empty(), "download rejection failed: " + invalid.first + ": " + error );
			}
			GT_OK();
		} );
	}
}

namespace {

Value* InitializeWorkers( GSE_CALLABLE, BackendFixture& fixture ) {
	fixture.SetRunning( true );
	auto* game = fixture.GetGame();
	auto* script = static_cast< value::Callable* >( fixture.RunScript( GSE_CALL, "./GLSMAC_data/tests/_base_workers" ) );
	return script->Run( GSE_CALL, { game->GetBM()->Wrap( GSE_CALL ), game->GetTM()->Wrap( GSE_CALL ) } );
}

Value* CallBase( GSE_CALLABLE, game::backend::base::Base* base, const std::string& method ) {
	auto* object = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
	return static_cast< value::Callable* >( object->Get( method ) )->Run( GSE_CALL, {} );
}

void AddWorkerBases( GSE_CALLABLE, BackendFixture& fixture ) {
	using namespace game::backend::base;
	auto* game = fixture.GetGame();
	auto* bm = game->GetBM();
	auto* map = game->GetMap();
	auto* base = new Base( game, 601, &game->GetState()->m_slots->GetSlot( 2 ), game->GetFaction( "GAIANS" ), map->GetTile( 4, 4 ), "Workers", {}, {
		{ 7, Pop( nullptr, 7, bm->GetPopDef( "WORKER" ), 1, map->GetTile( 2, 4 ) ) },
		{ 8, Pop( nullptr, 8, bm->GetPopDef( "WORKER" ), 2, map->GetTile( 6, 4 ) ) },
		{ 9, Pop( nullptr, 9, bm->GetPopDef( "DOCTOR" ), 0 ) },
	}, 20, 37 );
	bm->SpawnBase( GSE_CALL, base );
	auto* other = new Base( game, 602, &game->GetState()->m_slots->GetSlot( 2 ), game->GetFaction( "GAIANS" ), map->GetTile( 0, 4 ), "Other workers", {}, {
		{ 7, Pop( nullptr, 7, bm->GetPopDef( "WORKER" ), 3, map->GetTile( 1, 1 ) ) },
	}, 10 );
	bm->SpawnBase( GSE_CALL, other );
}

struct SavedWorker {
	int64_t key, id;
	std::string type;
	bool assigned;
	int64_t x = 0, y = 0;
};

types::Buffer WorkerRecord( const std::vector< SavedWorker >& pops, const int64_t next_pop_id = 20, const int64_t base_id = 603,
	const std::string& production = game::backend::base::SerializeProductionState( std::nullopt ).ToString() ) {
	types::Buffer buf;
	buf.WriteInt( base_id ); buf.WriteInt( 2 ); buf.WriteString( "GAIANS" );
	buf.WriteInt( 2 ); buf.WriteInt( 2 ); buf.WriteString( "Invalid workers" );
	buf.WriteInt( 0 ); buf.WriteInt( pops.size() );
	for ( const auto& pop : pops ) {
		buf.WriteInt( pop.key ); buf.WriteInt( pop.id ); buf.WriteString( pop.type ); buf.WriteInt( 0 );
		buf.WriteBool( pop.assigned );
		if ( pop.assigned ) { buf.WriteInt( pop.x ); buf.WriteInt( pop.y ); }
	}
	buf.WriteInt( next_pop_id ); buf.WriteInt( 37 );
	buf.WriteString( production );
	return buf;
}

}

namespace {

value::Object* ProductionRecord( GSE_CALLABLE, const std::string& current = "unit", const std::string& baseline = "facility",
	const int64_t minerals = 9, const int64_t baseline_minerals = 30 ) {
	const auto item = [ & ]( const std::string& kind ) {
		value::object_properties_t fields = { { "kind", VALUE( value::String,, kind ) } };
		if ( kind != "stockpile" ) { fields.emplace( "id", VALUE( value::String,, "unresolved-" + kind ) ); }
		return new value::Object( GSE_CALL, fields );
	};
	return new value::Object( GSE_CALL, {
		{ "current_item", item( current ) }, { "minerals", VALUE( value::Int,, minerals ) },
		{ "baseline_item", item( baseline ) }, { "baseline_minerals", VALUE( value::Int,, baseline_minerals ) },
	} );
}

void WriteProductionRecord( GSE_CALLABLE, game::backend::base::Base* base, Value* record ) {
	auto* wrapped = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
	static_cast< value::Callable* >( wrapped->Get( "set_production_state" ) )->Run( GSE_CALL, { record } );
}

}

static void AddProductionStateTests( task::gsetests::GSETests* task ) {
	task->AddTest( "test base production setter is authoritative atomic and detached", GT() {
		using namespace game::backend::base;
		const auto next = Base::GetNextId();
		util::FinallyGuard restore( [ next ]() { Base::SetNextId( next ); } );
		size_t updates = 0;
		BackendFixture fixture;
		fixture.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
		auto* base = fixture.AddBaseWithPop( 601, 7 );
		GT_ASSERT( !base->GetProductionState(), "new base invented production state" );
		const auto error = fixture.WithMutableScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.GetGame(); auto* bm = game->GetBM();
			bm->TriggerUpdates( GSE_CALL );
			game->On( GSE_CALL, "update_base", NATIVE_CALL( &updates ) { ++updates; return VALUE( value::Undefined ); } ) );
			GT_ASSERT( CallBase( GSE_CALL, base, "get_production_state" )->type == VT_UNDEFINED, "unconfigured getter returned a target" );
			auto* input = ProductionRecord( GSE_CALL );
			WriteProductionRecord( GSE_CALL, base, input );
			bm->TriggerUpdates( GSE_CALL );
			GT_ASSERT( updates == 1, "complete setter did not emit one update" );
			GT_ASSERT( base->GetProductionState()->minerals == 9 && base->GetProductionState()->baseline_minerals == 30 &&
				base->GetProductionState()->current_item.kind == ProductionItemKind::Unit && base->GetProductionState()->baseline_item.kind == ProductionItemKind::Facility,
				"setter conflated the current record with its distinct baseline" );
			const auto before = Base::Serialize( base ).ToString();
			const auto rng = game->GetRandom()->GetStateString();
			input->Set( "minerals", VALUE( value::Int,, 99 ), GSE_CALL );
			static_cast< value::Object* >( input->Get( "current_item" ) )->Set( "id", VALUE( value::String,, "changed" ), GSE_CALL );
			auto* copy = static_cast< value::Object* >( CallBase( GSE_CALL, base, "get_production_state" ) );
			copy->Set( "baseline_minerals", VALUE( value::Int,, -1 ), GSE_CALL );
			static_cast< value::Object* >( copy->Get( "baseline_item" ) )->Set( "kind", VALUE( value::String,, "project" ), GSE_CALL );
			GT_ASSERT( Base::Serialize( base ).ToString() == before, "input or getter retained mutable native aliases" );
			for ( const auto& invalid : std::vector< ProductionItem >{ { static_cast< ProductionItemKind >( 99 ), "id" }, { ProductionItemKind::Stockpile, "id" }, { ProductionItemKind::Project, "" } } ) {
				auto replacement = *base->GetProductionState(); replacement.baseline_item = invalid;
				bool rejected = false;
				try { base->SetProductionState( GSE_CALL, replacement ); } catch ( const std::runtime_error& ) { rejected = true; }
				GT_ASSERT( rejected && Base::Serialize( base ).ToString() == before, "native setter accepted a malformed late item or partially mutated" );
			}
			WriteProductionRecord( GSE_CALL, base, ProductionRecord( GSE_CALL ) );
			bm->TriggerUpdates( GSE_CALL );
			GT_ASSERT( updates == 1, "identical setter emitted a redundant update" );
			for ( int scenario = 0; scenario < 13; ++scenario ) {
				auto* record = ProductionRecord( GSE_CALL );
				Value* bad = record;
				auto* current = static_cast< value::Object* >( record->Get( "current_item" ) );
				auto* baseline = static_cast< value::Object* >( record->Get( "baseline_item" ) );
				switch ( scenario ) {
					case 0: bad = VALUE( value::Int,, 1 ); break;
					case 1: record->Set( "minerals", VALUE( value::String,, "9" ), GSE_CALL ); break;
					case 2: record->Set( "baseline_minerals", VALUE( value::Bool,, true ), GSE_CALL ); break;
					case 3: current->Set( "kind", VALUE( value::String,, "unknown" ), GSE_CALL ); break;
					case 4: baseline->Set( "kind", VALUE( value::String,, "unknown" ), GSE_CALL ); break;
					case 5: current->Set( "id", VALUE( value::String,, "" ), GSE_CALL ); break;
					case 6: baseline->Set( "id", VALUE( value::String,, "" ), GSE_CALL ); break;
					case 7: current->Set( "kind", VALUE( value::String,, "stockpile" ), GSE_CALL ); break;
					case 8: baseline->Set( "kind", VALUE( value::String,, "stockpile" ), GSE_CALL ); break;
					case 9: record->Set( "extra", VALUE( value::Int,, 0 ), GSE_CALL ); break;
					case 10: { auto fields = record->value; fields.erase( "baseline_minerals" ); bad = new value::Object( GSE_CALL, fields ); break; }
					case 11: baseline->Set( "id", VALUE( value::Int,, 4 ), GSE_CALL ); break;
					case 12: baseline->Set( "extra", VALUE( value::Int,, 4 ), GSE_CALL ); break;
				}
				bool rejected = false;
				try { WriteProductionRecord( GSE_CALL, base, bad ); }
				catch ( const Exception& e ) { rejected = e.class_name == EC.INVALID_CALL; }
				GT_ASSERT( rejected, "invalid production setter accepted: " + std::to_string( scenario ) );
				GT_ASSERT( Base::Serialize( base ).ToString() == before, "invalid production setter partially mutated state" );
			}
			bm->TriggerUpdates( GSE_CALL );
			GT_ASSERT( updates == 1 && game->GetRandom()->GetStateString() == rng, "rejected/alias writes notified or consumed RNG" );
			// These are supplied state checkpoints from the switching contract,
			// not a switching calculation implemented by this setter.
			for ( const auto& checkpoint : std::vector< std::pair< std::string, int64_t > >{ { "unit", 30 }, { "facility", 20 }, { "project", 20 }, { "unit", 30 } } ) {
				WriteProductionRecord( GSE_CALL, base, ProductionRecord( GSE_CALL, checkpoint.first, "unit", checkpoint.second, 30 ) );
				GT_ASSERT( base->GetProductionState()->minerals == checkpoint.second && base->GetProductionState()->baseline_minerals == 30 &&
					base->GetProductionState()->baseline_item.id == "unresolved-unit", "supplied switch checkpoint rewrote its independent baseline" );
			}
			bm->TriggerUpdates( GSE_CALL );
			WriteProductionRecord( GSE_CALL, base, VALUE( value::Undefined ) );
			bm->TriggerUpdates( GSE_CALL );
			GT_ASSERT( !base->GetProductionState() && updates == 3, "explicit unconfigure did not replace the complete record" );
			GT_OK();
		} );
		GT_ASSERT( error.empty(), "production: " + error );
		const auto readonly = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			for ( auto* input : std::vector< Value* >{ ProductionRecord( GSE_CALL ), VALUE( value::Undefined ) } ) {
				bool rejected = false;
				try { WriteProductionRecord( GSE_CALL, base, input ); }
				catch ( const Exception& e ) { rejected = e.class_name == EC.GAME_ERROR; }
				GT_ASSERT( rejected && !base->GetProductionState(), "read-only production mutation accepted" );
			}
			GT_OK();
		} );
		GT_ASSERT( readonly.empty(), "production: " + readonly ); GT_OK();
	} );
	for ( const bool immediate : { true, false } ) {
		task->AddTest( std::string( "test base production " ) + ( immediate ? "immediate" : "deferred" ) + " snapshots preserve all tags and independent balances", GT( immediate ) {
			using namespace game::backend::base;
			const auto next = Base::GetNextId();
			util::FinallyGuard restore( [ next ]() { Base::SetNextId( next ); } );
			const std::vector< std::string > kinds{ "unit", "facility", "orbital", "project", "stockpile" };
			for ( size_t scenario = 0; scenario < 6; ++scenario ) {
				BackendFixture source, target;
				source.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER ); source.DefinePop( "DOCTOR", "Doctor", PopDef::PF_NONE );
				const auto setup = source.WithMutableScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					AddWorkerBases( GSE_CALL, source );
					if ( scenario < kinds.size() ) {
						WriteProductionRecord( GSE_CALL, source.GetGame()->GetBM()->GetBase( 601 ), ProductionRecord( GSE_CALL, kinds[ scenario ], kinds[ ( scenario + 1 ) % kinds.size() ],
							std::numeric_limits< int64_t >::min(), std::numeric_limits< int64_t >::max() ) );
						WriteProductionRecord( GSE_CALL, source.GetGame()->GetBM()->GetBase( 602 ), ProductionRecord( GSE_CALL, kinds[ scenario ], kinds[ scenario ], 0, -2 ) );
					}
					GT_OK();
				} );
				GT_ASSERT( setup.empty(), "production: " + setup );
				types::Buffer bytes; source.GetGame()->GetBM()->Serialize( bytes );
				target.SetRunning( immediate ); target.RestoreBaseManager( bytes );
				if ( !immediate ) {
					GT_ASSERT( target.GetGame()->GetBM()->GetBases().empty(), "deferred production record published early" );
					const auto error = target.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string { target.GetGame()->GetBM()->ProcessUnprocessed( GSE_CALL ); GT_OK(); } );
					GT_ASSERT( error.empty(), "production: " + error );
				}
				for ( const size_t id : { 601, 602 } ) {
					auto* actual = target.GetGame()->GetBM()->GetBase( id );
					GT_ASSERT( actual->GetProductionState() == source.GetGame()->GetBM()->GetBase( id )->GetProductionState(), "codec changed independent production identities or balances" );
					GT_ASSERT( Base::Serialize( actual ).ToString() == Base::Serialize( source.GetGame()->GetBM()->GetBase( id ) ).ToString(), "production/population/nutrient snapshot bytes changed" );
					GT_ASSERT( actual->GetProductionState().has_value() == ( scenario < kinds.size() ), "configured versus absent state changed" );
				}
				auto* bm = target.GetGame()->GetBM(); auto* base = bm->GetBase( 601 );
				GT_ASSERT( bm->GetWorkingPop( { 2, 4 } ) == &base->m_pops.at( 7 ) && bm->GetWorkingPop( { 6, 4 } ) == &base->m_pops.at( 8 ), "production restore changed worker links" );
				GT_ASSERT( target.GetGame()->GetMap()->GetTile( 4, 4 )->base == base && base->GetAccumulatedNutrients() == 37 && !base->m_pops.at( 9 ).GetWorkedTileCoords(), "restore changed center/nutrients/specialist" );
			}
			GT_OK();
		} );
	}
	for ( const bool immediate : { true, false } ) {
		task->AddTest( std::string( "test base production malformed " ) + ( immediate ? "immediate" : "deferred" ) + " records do not publish the failing base", GT( immediate ) {
			using namespace game::backend::base;
			const auto next = Base::GetNextId();
			util::FinallyGuard restore( [ next ]() { Base::SetNextId( next ); } );
			for ( int scenario = 0; scenario < 11; ++scenario ) {
				for ( const bool prior_record : { false, true } ) {
					BackendFixture source, target;
					source.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER ); source.DefinePop( "DOCTOR", "Doctor", PopDef::PF_NONE );
					const auto setup = source.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string { AddWorkerBases( GSE_CALL, source ); GT_OK(); } );
					GT_ASSERT( setup.empty(), "production: " + setup );
					types::Buffer production;
					if ( scenario == 0 ) { production.WriteInt( 1 ); }
					else {
						production.WriteBool( scenario != 8 );
						production.WriteInt( scenario == 1 ? 0 : scenario == 2 ? std::numeric_limits< int64_t >::max() : 1 );
						production.WriteString( scenario == 3 ? "" : "missing-unit" ); production.WriteInt( -2 );
						production.WriteInt( scenario == 4 ? -1 : scenario == 5 ? 5 : 2 );
						production.WriteString( scenario == 6 ? "" : "missing-baseline" );
						if ( scenario == 10 ) { production.WriteString( "30" ); }
						else if ( scenario != 7 ) { production.WriteInt( 30 ); }
						if ( scenario == 9 ) { production.WriteInt( 99 ); }
					}
					types::Buffer snapshot;
					snapshot.WriteString( "GLSMAC_BASES_V4" ); snapshot.WriteInt( 2 );
					for ( const auto* id : { "WORKER", "DOCTOR" } ) { snapshot.WriteString( id ); snapshot.WriteString( PopDef::Serialize( source.GetGame()->GetBM()->GetPopDef( id ) ).ToString() ); }
					snapshot.WriteInt( 0 ); snapshot.WriteInt( prior_record ? 2 : 1 );
					if ( prior_record ) { snapshot.WriteString( Base::Serialize( source.GetGame()->GetBM()->GetBase( 601 ) ).ToString() ); }
					snapshot.WriteString( WorkerRecord( { { 7, 7, "WORKER", true, 6, 2 } }, 20, 9000, production.ToString() ).ToString() ); snapshot.WriteInt( 604 );
					const auto before_id = Base::GetNextId();
					target.SetRunning( immediate ); bool rejected = false;
					try { target.RestoreBaseManager( snapshot ); } catch ( const std::runtime_error& ) { rejected = true; }
					if ( !immediate ) {
						GT_ASSERT( !rejected && target.GetGame()->GetBM()->GetBases().empty(), "deferred envelope published or parsed records early" );
						target.SetRunning( true );
						const auto error = target.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
							try { target.GetGame()->GetBM()->ProcessUnprocessed( GSE_CALL ); } catch ( const std::runtime_error& ) { rejected = true; } GT_OK();
						} );
						GT_ASSERT( error.empty(), "production: " + error );
					}
					auto* bm = target.GetGame()->GetBM();
					GT_ASSERT( rejected && !bm->GetBase( 9000 ) && !target.GetGame()->GetMap()->GetTile( 2, 2 )->base && !bm->GetWorkingPop( { 6, 2 } ), "bad production record published base/center/worker" );
					GT_ASSERT( Base::GetNextId() == ( immediate ? before_id : 604 ) && bm->GetBases().size() == ( prior_record ? 1 : 0 ), "bad record advanced IDs or changed earlier publication" );
					const auto& updates = target.PushBaseUpdates();
					GT_ASSERT( updates.size() == ( prior_record ? 2 : 0 ), "bad record emitted frontend notifications" );
				}
			}
			GT_OK();
		} );
	}
	task->AddTest( "test base production optimistic event rollback restores the complete independent record", GT() {
		using namespace game::backend::base; using namespace game::backend::event;
		const auto next = Base::GetNextId(); util::FinallyGuard restore( [ next ]() { Base::SetNextId( next ); } );
		for ( const bool configured : { false, true } ) {
			BackendFixture fixture;
			fixture.EnableYieldFixture(); fixture.SetRunning( true );
			fixture.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER ); fixture.DefinePop( "DOCTOR", "Doctor", PopDef::PF_NONE );
			const auto error = fixture.WithMutableScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				AddWorkerBases( GSE_CALL, fixture ); auto* game = fixture.GetGame(); auto* base = game->GetBM()->GetBase( 601 );
				fixture.PushBaseUpdates(); // Initialize synthetic render coordinates before actual Iterate flushes base updates.
				if ( configured ) { WriteProductionRecord( GSE_CALL, base, ProductionRecord( GSE_CALL ) ); }
				const auto before = Base::Serialize( base ).ToString(); const auto rng = game->GetRandom()->GetStateString();
				auto* connection = fixture.UseClientConnection( gc_space );
				auto* noop = NATIVE_CALL() { return VALUE( value::Undefined ); } );
				auto* apply = NATIVE_CALL( base ) {
					auto* prior = CallBase( GSE_CALL, base, "get_production_state" );
					WriteProductionRecord( GSE_CALL, base, ProductionRecord( GSE_CALL, "project", "stockpile", 20, 0 ) ); return prior;
				} );
				auto* rollback = NATIVE_CALL( base ) {
					auto* event = static_cast< value::Object* >( arguments.at( 0 ) );
					WriteProductionRecord( GSE_CALL, base, event->Get( "applied" ) ); return VALUE( value::Undefined );
				} );
				fixture.RegisterEvent( "test_production_state", new EventHandler( gc_space, "test_production_state", noop, nullptr, apply, rollback ) );
				game->AddEvent( new Event( game, Event::ES_LOCAL, 2, GSE_CALL, "test_production_state", {}, "production-response" ) );
				fixture.ProcessEvents(); fixture.FlushEvents();
				GT_ASSERT( fixture.WaitingResponses() == 1 && connection->sent.size() == 1 && base->GetProductionState()->minerals == 20, "actual optimistic event was not retained/applied" );
				game->AddEventResponse( "production-response", false, nullptr ); game->Iterate();
				GT_ASSERT( !fixture.WaitingResponses() && Base::Serialize( base ).ToString() == before && game->GetRandom()->GetStateString() == rng, "rejection did not restore full production/population state" );
				GT_ASSERT( game->GetBM()->GetWorkingPop( { 2, 4 } ) == &base->m_pops.at( 7 ), "production rollback changed worker ownership" ); GT_OK();
			} );
			GT_ASSERT( error.empty(), "production: " + error );
		}
		GT_OK();
	} );
}

static void AddWorkerTests( task::gsetests::GSETests* task ) {
	for ( const bool immediate : { true, false } ) {
		task->AddTest( immediate ? "test immediate worker snapshot preserves assignments and intake" : "test deferred worker snapshot preserves assignments and intake",
		[ immediate ]( GSE* gse, context::Context* ctx ) -> std::string {
			using namespace game::backend::base;
			const auto next = Base::GetNextId();
			util::FinallyGuard reset( [ next ]() { Base::SetNextId( next ); } );
			BackendFixture source, restored;
			source.EnableYieldFixture(); restored.EnableYieldFixture();
			source.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
			source.DefinePop( "DOCTOR", "Doctor", PopDef::PF_NONE );
			const auto setup = source.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string { AddWorkerBases( GSE_CALL, source ); GT_OK(); } );
			GT_ASSERT( setup.empty(), "worker setup: " + setup );
			types::Buffer snapshot;
			source.GetGame()->GetBM()->Serialize( snapshot );
			restored.SetRunning( immediate ); restored.RestoreBaseManager( snapshot );
			if ( !immediate ) {
				GT_ASSERT( !restored.GetGame()->GetBM()->GetWorkingPop( { 2, 4 } ), "deferred assignment published early" );
				const auto error = restored.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string { restored.GetGame()->GetBM()->ProcessUnprocessed( GSE_CALL ); GT_OK(); } );
				GT_ASSERT( error.empty(), "worker check: " + error );
			}
			std::vector< size_t > unworked_counts;
			for ( auto* fixture : { &source, &restored } ) {
				const auto error = fixture->WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					InitializeWorkers( GSE_CALL, *fixture );
					auto* bm = fixture->GetGame()->GetBM();
					auto* base = bm->GetBase( 601 );
					auto* other = bm->GetBase( 602 );
					GT_ASSERT( bm->GetWorkingPop( { 2, 4 } ) == &base->m_pops.at( 7 ), "first reverse claim missing" );
					GT_ASSERT( bm->GetWorkingPop( { 6, 4 } ) == &base->m_pops.at( 8 ), "second reverse claim missing" );
					GT_ASSERT( bm->GetWorkingPop( { 1, 1 } ) == &other->m_pops.at( 7 ), "base-local IDs collided" );
					GT_ASSERT( !base->m_pops.at( 9 ).GetWorkedTileCoords(), "specialist acquired a tile" );
					GT_ASSERT( static_cast< value::Array* >( CallBase( GSE_CALL, base, "get_worked_tiles" ) )->value.size() == 2, "worked tile set changed" );
					unworked_counts.push_back( static_cast< value::Array* >( CallBase( GSE_CALL, base, "get_unworked_tiles" ) )->value.size() );
					auto* intake = static_cast< value::Object* >( CallBase( GSE_CALL, base, "get_intake" ) );
					GT_ASSERT( static_cast< value::Int* >( intake->Get( "NUTRIENTS" ) )->value == 15, "intake lost worker tiles or double-counted center" );
					GT_ASSERT( static_cast< value::Int* >( intake->Get( "MINERALS" ) )->value == 3, "intake tile count changed" );
					GT_OK();
				} );
				GT_ASSERT( error.empty(), "worker check: " + error );
			}
			GT_ASSERT( unworked_counts[ 0 ] == unworked_counts[ 1 ], "unworked tiles changed after restore" );
			for ( const size_t id : { 601, 602 } ) {
				GT_ASSERT( Base::Serialize( source.GetGame()->GetBM()->GetBase( id ) ).ToString() == Base::Serialize( restored.GetGame()->GetBM()->GetBase( id ) ).ToString(), "population IDs, variants, or assignments changed" );
			}
			GT_OK();
		} );
	}
	for ( const bool immediate : { true, false } ) {
		task->AddTest( immediate ? "test immediate restore rejects competing worker claims" : "test deferred restore rejects competing worker claims",
		[ immediate ]( GSE* gse, context::Context* ctx ) -> std::string {
			using namespace game::backend::base;
			const auto next = Base::GetNextId();
			util::FinallyGuard reset( [ next ]() { Base::SetNextId( next ); } );
			BackendFixture source, restored;
			source.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
			source.DefinePop( "DOCTOR", "Doctor", PopDef::PF_NONE );
			const auto setup = source.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string { AddWorkerBases( GSE_CALL, source ); GT_OK(); } );
			GT_ASSERT( setup.empty(), "worker setup: " + setup );
			types::Buffer snapshot;
			snapshot.WriteString( "GLSMAC_BASES_V4" ); snapshot.WriteInt( 2 );
			for ( const auto* id : { "WORKER", "DOCTOR" } ) {
				snapshot.WriteString( id );
				snapshot.WriteString( PopDef::Serialize( source.GetGame()->GetBM()->GetPopDef( id ) ).ToString() );
			}
			snapshot.WriteInt( 0 ); snapshot.WriteInt( 2 );
			snapshot.WriteString( Base::Serialize( source.GetGame()->GetBM()->GetBase( 601 ) ).ToString() );
			snapshot.WriteString( WorkerRecord( { { 7, 7, "WORKER", true, 2, 4 } } ).ToString() );
			snapshot.WriteInt( 604 );
			restored.SetRunning( immediate );
			bool rejected = false;
			if ( immediate ) {
				try { restored.RestoreBaseManager( snapshot ); }
				catch ( const std::runtime_error& ) { rejected = true; }
			}
			else {
				restored.RestoreBaseManager( snapshot );
				restored.SetRunning( true );
				const auto error = restored.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					try { restored.GetGame()->GetBM()->ProcessUnprocessed( GSE_CALL ); }
					catch ( const std::runtime_error& ) { rejected = true; }
					GT_OK();
				} );
				GT_ASSERT( error.empty(), "deferred worker check: " + error );
			}
			GT_ASSERT( rejected, "competing snapshot claims accepted" );
			auto* bm = restored.GetGame()->GetBM();
			GT_ASSERT( bm->GetBases().size() == 1 && bm->GetBase( 601 ), "earlier valid record was not retained" );
			GT_ASSERT( !restored.GetGame()->GetMap()->GetTile( 2, 2 )->base, "invalid record claimed its center" );
			GT_ASSERT( bm->GetWorkingPop( { 2, 4 } ) == &bm->GetBase( 601 )->m_pops.at( 7 ), "competing claim stole existing worker" );
			const auto& updates = restored.PushBaseUpdates();
			GT_ASSERT( updates.size() == 2, "invalid record queued frontend updates" );
			// A failed manager load is terminal. Reusing its partially restored
			// definitions/records is not the snapshot loader's current contract.
			GT_OK();
		} );
	}
	task->AddTest( "test worker mutation and production event rollback preserve assignments", GT() {
		using namespace game::backend::base;
		const auto next = Base::GetNextId();
		util::FinallyGuard reset( [ next ]() { Base::SetNextId( next ); } );
		BackendFixture fixture, foreign;
		fixture.EnableYieldFixture();
		fixture.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
		fixture.DefinePop( "DOCTOR", "Doctor", PopDef::PF_NONE );
		const auto error = fixture.WithMutableScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			AddWorkerBases( GSE_CALL, fixture );
			auto* game = fixture.GetGame(); auto* bm = game->GetBM();
			auto* base = bm->GetBase( 601 ); auto* other = bm->GetBase( 602 );
			auto* pop = &base->m_pops.at( 7 ); auto* tile = game->GetMap()->GetTile( 2, 4 );
			const auto before = Base::Serialize( base ).ToString();
			const auto rng = game->GetRandom()->GetStateString();
			try { base->AddPop( *pop ); GT_FAIL( "duplicate population replaced its original" ); }
			catch ( const std::runtime_error& ) {}
			try { bm->SpawnBase( GSE_CALL, base ); GT_FAIL( "duplicate base registration accepted" ); }
			catch ( const Exception& e ) { GT_ASSERT( e.class_name == EC.GAME_ERROR, "wrong duplicate base error" ); }
			GT_ASSERT( Base::Serialize( base ).ToString() == before && bm->GetWorkingPop( { 2, 4 } ) == pop, "duplicate ID changed its original record" );
			types::Buffer assigned, unassigned;
			pop->Serialize( assigned );
			unassigned.WriteInt( 7 ); unassigned.WriteString( "WORKER" ); unassigned.WriteInt( 1 ); unassigned.WriteBool( false );
			Pop staged;
			staged.Deserialize( assigned, game );
			GT_ASSERT( staged.GetWorkedTileCoords().has_value(), "assigned record was not decoded" );
			staged.Deserialize( unassigned, game );
			GT_ASSERT( !staged.GetWorkedTileCoords(), "unassigned record retained a prior decoded coordinate" );
			for ( size_t invalid = 0; invalid < 5; invalid++ ) {
				try {
					base->SetPopAssignment( GSE_CALL, invalid == 0 ? &other->m_pops.at( 7 ) : pop, invalid == 1 ? "DOCTOR" : "WORKER",
						invalid == 2 ? game->GetMap()->GetTile( 6, 4 ) : invalid == 3 ? base->GetTile() : invalid == 4 ? foreign.GetGame()->GetMap()->GetTile( 2, 4 ) : tile );
					GT_FAIL( "invalid assignment accepted" );
				} catch ( const Exception& e ) { GT_ASSERT( e.class_name == EC.GAME_ERROR, "wrong assignment error" ); }
				GT_ASSERT( Base::Serialize( base ).ToString() == before && bm->GetWorkingPop( { 2, 4 } ) == pop, "failed assignment mutated state" );
			}
			auto* wrapped = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
			try {
				static_cast< value::Callable* >( wrapped->Get( "create_pop" ) )->Run( GSE_CALL, {
					new value::Object( GSE_CALL, { { "type", new value::String( gc_space, "WORKER" ) }, { "worked_tile", tile->Wrap( GSE_CALL ) } } )
				} );
				GT_FAIL( "new population stole an occupied tile" );
			} catch ( const Exception& e ) { GT_ASSERT( e.class_name == EC.GAME_ERROR, "wrong new-pop assignment error" ); }
			base->SetPopAssignment( GSE_CALL, pop, "WORKER", tile );
			GT_ASSERT( Base::Serialize( base ).ToString() == before, "failed creation or idempotent assignment changed state" );
			base->ChangePopType( GSE_CALL, 7, "DOCTOR" );
			GT_ASSERT( !bm->GetWorkingPop( { 2, 4 } ) && !pop->GetWorkedTileCoords(), "specialist transition retained tile" );
			base->SetPopAssignment( GSE_CALL, pop, "WORKER", tile );
			GT_ASSERT( game->GetRandom()->GetStateString() == rng, "assignment consumed RNG" );
			auto* facade = InitializeWorkers( GSE_CALL, fixture );
			auto* events = static_cast< value::Callable* >( fixture.RunScript( GSE_CALL, "./GLSMAC_data/tests/_base_worker_events" ) );
			events->Run( GSE_CALL, { facade, base->Wrap( GSE_CALL ), other->Wrap( GSE_CALL ), game->GetMap()->GetTile( 6, 2 )->Wrap( GSE_CALL ) } );
			// Removal and despawn free numeric claims; reused local IDs cannot resolve stale claims.
			other->RemovePop( 7 ); GT_ASSERT( !bm->GetWorkingPop( { 1, 1 } ), "removed population kept claim" );
			base->SetPopAssignment( GSE_CALL, &base->m_pops.at( 8 ), "WORKER", game->GetMap()->GetTile( 1, 1 ) );
			bm->DespawnBase( GSE_CALL, 601 );
			GT_ASSERT( !bm->GetWorkingPop( { 1, 1 } ) && !bm->GetWorkingPop( { 2, 4 } ), "despawn kept worker claims" );
			bm->Clear(); GT_ASSERT( !bm->GetWorkingPop( { 6, 4 } ), "clear kept worker claims" );
			GT_OK();
		} );
		GT_ASSERT( error.empty(), "worker check: " + error ); GT_OK();
	} );
	task->AddTest( "test worker writes require a mutable game scope", GT() {
		using namespace game::backend::base;
		const auto next = Base::GetNextId();
		util::FinallyGuard reset( [ next ]() { Base::SetNextId( next ); } );
		BackendFixture fixture;
		fixture.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
		auto* base = fixture.AddBaseWithPop( 601, 7 );
		const auto before = Base::Serialize( base ).ToString();
		const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* object = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
			try {
				static_cast< value::Callable* >( object->Get( "set_pop_assignment" ) )->Run( GSE_CALL, {
					base->m_pops.at( 7 ).Wrap( GSE_CALL ), new value::String( gc_space, "WORKER" ), fixture.GetGame()->GetMap()->GetTile( 2, 4 )->Wrap( GSE_CALL )
				} );
				GT_FAIL( "read-only worker write accepted" );
			} catch ( const Exception& e ) { GT_ASSERT( e.class_name == EC.GAME_ERROR, "wrong read-only worker error" ); }
			GT_ASSERT( Base::Serialize( base ).ToString() == before, "read-only write changed assignment" );
			GT_OK();
		} );
		GT_ASSERT( error.empty(), "worker read-only check: " + error ); GT_OK();
	} );
	task->AddTest( "test malformed worker records reject before publishing base state", GT() {
		using namespace game::backend::base;
		const auto next = Base::GetNextId();
		util::FinallyGuard reset( [ next ]() { Base::SetNextId( next ); } );
		BackendFixture fixture;
		fixture.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
		fixture.DefinePop( "DOCTOR", "Doctor", PopDef::PF_NONE );
		const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			AddWorkerBases( GSE_CALL, fixture );
			auto* game = fixture.GetGame(); auto* bm = game->GetBM();
			fixture.PushBaseUpdates();
			const auto id = Base::GetNextId(), requests = fixture.FrontendRequestCount();
			const std::vector< std::vector< SavedWorker > > records = {
				{ { 7, 7, "WORKER", true, -2, 2 } }, { { 7, 7, "WORKER", true, 8, 2 } },
				{ { 7, 7, "WORKER", true, 2, 3 } }, { { 7, 7, "DOCTOR", true, 4, 2 } },
				{ { 7, 8, "WORKER", true, 4, 2 } }, { { 7, 7, "MISSING", false } },
				{ { 7, 7, "WORKER", true, 2, 2 } }, { { 7, 7, "WORKER", true, 2, 4 } },
				{ { 7, 7, "WORKER", true, 4, 2 }, { 8, 8, "WORKER", true, 4, 2 } },
				{ { 7, 7, "WORKER", false }, { 7, 7, "WORKER", false } },
			};
			std::vector< types::Buffer > malformed;
			for ( const auto& pops : records ) { malformed.push_back( WorkerRecord( pops ) ); }
			malformed.push_back( WorkerRecord( { { 7, 7, "WORKER", false } }, 7 ) );
			malformed.push_back( WorkerRecord( { { 7, 7, "WORKER", false } }, 20, 601 ) );
			for ( auto& bytes : malformed ) {
				bool rejected = false;
				try { std::unique_ptr< Base > invalid( Base::Deserialize( bytes, game ) ); }
				catch ( const std::runtime_error& ) { rejected = true; }
				GT_ASSERT( rejected, "malformed worker record accepted" );
				GT_ASSERT( !game->GetMap()->GetTile( 2, 2 )->base && Base::GetNextId() == id, "failed restore claimed center or advanced ID" );
				GT_ASSERT( bm->GetBases().size() == 2 && !bm->GetWorkingPop( { 4, 2 } ), "failed restore published assignments" );
				GT_ASSERT( bm->GetWorkingPop( { 2, 4 } ) == &bm->GetBase( 601 )->m_pops.at( 7 ), "failed restore stole existing claim" );
				GT_ASSERT( fixture.FrontendRequestCount() == requests, "failed restore emitted frontend messages" );
			}
			GT_ASSERT( fixture.PushBaseUpdates().empty(), "failed restore queued base updates" );
			GT_OK();
		} );
		GT_ASSERT( error.empty(), "worker check: " + error ); GT_OK();
	} );
	task->AddTest( "test worker teardown tolerates map destruction before collected managers", GT() {
		using namespace game::backend::base;
		const auto next = Base::GetNextId();
		util::FinallyGuard reset( [ next ]() { Base::SetNextId( next ); } );
		BackendFixture fixture;
		fixture.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
		fixture.DefinePop( "DOCTOR", "Doctor", PopDef::PF_NONE );
		const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			AddWorkerBases( GSE_CALL, fixture );
			auto* base = fixture.GetGame()->GetBM()->GetBase( 601 );
			ctx->CreateVariable( "retained_workers", new value::Array( gc_space, { base->Wrap( GSE_CALL ), base->m_pops.at( 7 ).Wrap( GSE_CALL ), base->m_pops.at( 7 ).GetWorkedTile()->Wrap( GSE_CALL ) } ), si, ep );
			fixture.DestroyMapBeforeManagers();
			GT_OK();
		} );
		GT_ASSERT( error.empty(), "worker check: " + error ); GT_OK();
	} );
}

namespace {

using namespace game::backend::unit;

void DefineSnapshotUnit( UnitManager* manager ) {
	MoraleSet::morale_values_t values;
	for ( size_t i = 0 ; i < 7 ; ++i ) { values.emplace_back( "Rank " + std::to_string( i ) ); }
	manager->DefineMoraleSet( new MoraleSet( "TEST_MORALE", values ) );
	manager->DefineUnit( new StaticDef( "TEST_UNIT", manager->GetMoraleSet( "TEST_MORALE" ), "Snapshot unit",
		MT_LAND, 1.0f, new SpriteRender( "unused-test-asset.pcx", 2, 3, 100, 75, 53, 54, 102 ) ) );
}

struct UnitSnapshotRecord {
	int64_t id = 71, owner = 2, x = 2, y = 2, morale = 3;
	std::string def = "TEST_UNIT";
	float movement = 0.375f, health = 0.625f;
	bool moved = true;
	types::Buffer Encode() const {
		types::Buffer buf;
		buf.WriteInt( id ); buf.WriteString( def ); buf.WriteInt( owner );
		buf.WriteInt( x ); buf.WriteInt( y ); buf.WriteFloat( movement );
		buf.WriteInt( morale ); buf.WriteFloat( health ); buf.WriteBool( moved );
		return buf;
	}
};

Unit* SpawnSnapshotUnit( GSE_CALLABLE, BackendFixture& fixture, const size_t id, const size_t x = 2 ) {
	auto* game = fixture.GetGame();
	auto* unit = new Unit( GSE_CALL, game->GetUM(), id, game->GetUM()->GetUnitDef( "TEST_UNIT" ),
		&game->GetState()->m_slots->GetSlot( 2 ), game->GetMap()->GetTile( x, 2 ), 0.375f, 3, 0.625f, true );
	game->GetUM()->SpawnUnit( GSE_CALL, unit );
	return unit;
}

class CountedUnitDef : public StaticDef {
public:
	CountedUnitDef( const MoraleSet* morale, size_t& destroyed )
		: StaticDef( "COUNTED", morale, "Counted", MT_LAND, 1.0f, new SpriteRender( "unused", 0, 0, 1, 1, 0, 0, 0 ) )
		, m_destroyed( destroyed ) {}
	~CountedUnitDef() override { ++m_destroyed; }
private:
	size_t& m_destroyed;
};

}

static void AddUnitSnapshotTests( task::gsetests::GSETests* task ) {
	task->AddTest( "test unit snapshots use destination definitions and preserve state without spawn hooks", GT() {
		const auto old_next_id = Unit::GetNextId();
		util::FinallyGuard reset_id( [ = ]() { Unit::SetNextId( old_next_id ); } );
		for ( const bool immediate : { false, true } ) {
			for ( const bool client : { false, true } ) {
				BackendFixture source, restored;
				source.EnableTurns(); restored.EnableTurns();
				source.SetRunning( true ); restored.SetRunning( immediate );
				auto* source_manager = source.GetGame()->GetUM();
				auto* manager = restored.GetGame()->GetUM();
				size_t hook_calls = 0;
				const auto source_error = source.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					DefineSnapshotUnit( source_manager );
					SpawnSnapshotUnit( GSE_CALL, source, 71 );
					auto* second = SpawnSnapshotUnit( GSE_CALL, source, 88, 4 );
					second->m_movement = -0.0f; second->m_morale = 6;
					second->m_health = -0.125f; second->m_moved_this_turn = false;
					GT_OK();
				} );
				GT_ASSERT( source_error.empty(), "source unit setup failed: " + source_error );
				Unit::SetNextId( 123 );
				types::Buffer snapshot;
				source_manager->Serialize( snapshot );
				const auto rng = restored.GetGame()->GetRandom()->GetStateString();
				const auto error = restored.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					if ( client ) { restored.UseClientConnection( gc_space ); }
					manager->On( GSE_CALL, "unit_spawn", NATIVE_CALL( &restored, &hook_calls ) {
						++hook_calls;
						restored.GetGame()->GetRandom()->GetUInt( 1, 100 );
						return VALUE( value::Undefined );
					} ) );
					manager->Deserialize( GSE_CALL, snapshot );
					if ( !immediate ) {
						GT_ASSERT( manager->GetUnits().empty(), "deferred restore published units before startup" );
						GT_ASSERT( restored.GetGame()->GetMap()->GetTile( 2, 2 )->units.size() == 1, "deferred restore lost its tile registration" );
						restored.SetRunning( true );
						manager->ProcessUnprocessed( GSE_CALL );
						manager->ProcessUnprocessed( GSE_CALL );
					}
					GT_ASSERT( manager->GetUnits().size() == 2 && Unit::GetNextId() == 123, "unit count or next ID changed" );
					for ( const auto& entry : source_manager->GetUnits() ) {
						const auto* unit = manager->GetUnit( entry.first );
						GT_ASSERT( unit && unit->m_def == manager->GetUnitDef( "TEST_UNIT" ), "restored unit does not borrow its manager's definition" );
						GT_ASSERT( unit->m_def != entry.second->m_def, "restored unit borrowed the source world's definition" );
						GT_ASSERT( unit->m_def->m_moraleset == manager->GetMoraleSet( "TEST_MORALE" ), "definition borrowed another world's morale" );
						GT_ASSERT( unit->m_owner == &restored.GetGame()->GetState()->m_slots->GetSlot( 2 ), "unit owner resolved in another world" );
						GT_ASSERT( unit->GetTile()->units.at( unit->m_id ) == unit, "tile lost its unit identity" );
						GT_ASSERT( Unit::Serialize( unit ).ToString() == Unit::Serialize( entry.second ).ToString(), "unit record changed after round trip" );
					}
					GT_ASSERT( hook_calls == 0 && restored.GetGame()->GetRandom()->GetStateString() == rng, "restore invoked new-unit gameplay or consumed RNG" );
					types::Buffer again; manager->Serialize( again );
					GT_ASSERT( snapshot.lenr == snapshot.lenw && again.ToString() == snapshot.ToString(), "unit manager snapshot changed" );
					types::Buffer retry( snapshot.ToString() );
					bool rejected = false;
					try { manager->Deserialize( GSE_CALL, retry ); }
					catch ( const std::runtime_error& ) { rejected = true; }
					GT_ASSERT( rejected && retry.lenr == 0 && manager->GetUnits().size() == 2 && Unit::GetNextId() == 123, "reload into populated manager changed live state" );
					GT_OK();
				} );
				GT_ASSERT( error.empty(), "unit restore failed: " + error );
				const auto& requests = restored.PushUnitUpdates();
				std::set< size_t > spawned;
				for ( const auto& request : requests ) {
					if ( request.type == game::FrontendRequest::FR_TURN_STATUS ) { continue; }
					GT_ASSERT( request.type == game::FrontendRequest::FR_UNIT_SPAWN, "restore queued an unexpected frontend operation" );
					const auto& data = request.data.unit_spawn;
					GT_ASSERT( spawned.insert( data.unit_id ).second, "restored unit spawned twice in the frontend" );
					const auto* unit = manager->GetUnit( data.unit_id );
					GT_ASSERT( unit && *data.unitdef_id == unit->m_def->m_id && data.health == unit->m_health && data.movement == unit->m_movement, "frontend spawn lost restored state" );
				}
				GT_ASSERT( spawned == std::set< size_t >( { 71, 88 } ), "restored units need exactly one frontend spawn each" );
				const auto spawn_error = restored.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					restored.SetRunning( false );
					SpawnSnapshotUnit( GSE_CALL, restored, Unit::GetNextId(), 6 );
					GT_ASSERT( hook_calls == 0, "ordinary deferred spawn fired before startup" );
					restored.SetRunning( true ); manager->ProcessUnprocessed( GSE_CALL );
					GT_ASSERT( hook_calls == ( client ? 0 : 1 ), "ordinary deferred spawn lost master/client hook behavior" );
					GT_ASSERT( ( restored.GetGame()->GetRandom()->GetStateString() != rng ) == !client, "ordinary spawn RNG behavior changed" );
					SpawnSnapshotUnit( GSE_CALL, restored, Unit::GetNextId(), 6 );
					GT_ASSERT( hook_calls == ( client ? 0 : 2 ), "ordinary immediate spawn lost master/client hook behavior" );
					GT_OK();
				} );
				GT_ASSERT( spawn_error.empty(), "ordinary spawn after restore failed: " + spawn_error );
			}
		}
		GT_OK();
	} );

	task->AddTest( "test malformed unit records reject before tile registration or ID allocation", GT() {
		const auto old_next_id = Unit::GetNextId();
		util::FinallyGuard reset_id( [ = ]() { Unit::SetNextId( old_next_id ); } );
		for ( int scenario = 0 ; scenario < 22 ; ++scenario ) {
			BackendFixture fixture; fixture.EnableTurns();
			const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* manager = fixture.GetGame()->GetUM();
				DefineSnapshotUnit( manager );
				UnitSnapshotRecord record;
				switch ( scenario ) {
					case 0: record.id = 0; break;
					case 1: record.id = -1; break;
					case 2: record.id = std::numeric_limits< int64_t >::max(); break;
					case 3: record.def = "MISSING"; break;
					case 4: record.owner = -1; break;
					case 5: record.owner = 3; break;
					case 6: record.x = -2; break;
					case 7: record.x = 8; break;
					case 8: record.y = 8; break;
					case 9: record.x = 3; break;
					case 10: record.morale = -1; break;
					case 11: record.morale = 7; break;
					case 12: record.movement = std::numeric_limits< float >::quiet_NaN(); break;
					case 13: record.movement = std::numeric_limits< float >::infinity(); break;
					case 14: record.health = std::numeric_limits< float >::quiet_NaN(); break;
					case 15: record.health = -std::numeric_limits< float >::infinity(); break;
				}
				Unit* existing = nullptr;
				if ( scenario >= 18 ) {
					fixture.SetRunning( scenario < 20 );
					existing = SpawnSnapshotUnit( GSE_CALL, fixture, 71 );
					if ( scenario & 1 ) { record.x = 4; }
				}
				auto bytes = record.Encode().ToString();
				if ( scenario == 16 ) { bytes.pop_back(); }
				if ( scenario == 17 ) { bytes += "trailing"; }
				types::Buffer input( bytes );
				const auto next_id = Unit::GetNextId();
				bool rejected = false;
				try { std::unique_ptr< Unit > unexpected( Unit::Deserialize( GSE_CALL, input, manager ) ); }
				catch ( const std::runtime_error& ) { rejected = true; }
				GT_ASSERT( rejected, "malformed unit record accepted: " + std::to_string( scenario ) );
				GT_ASSERT( Unit::GetNextId() == next_id, "rejected unit consumed an ID" );
				const auto& units = fixture.GetGame()->GetMap()->GetTile( 2, 2 )->units;
				GT_ASSERT( units.size() == ( existing ? 1 : 0 ), "rejected unit changed tile occupancy" );
				GT_ASSERT( !existing || units.at( 71 ) == existing, "duplicate unit displaced the original" );
				GT_ASSERT( fixture.GetGame()->GetMap()->GetTile( 4, 2 )->units.empty(), "duplicate unit claimed a second tile" );
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "malformed unit validation failed: " + error );
		}
		GT_OK();
	} );

	task->AddTest( "test unit snapshot preserves finite signed movement and health boundaries", GT() {
		const auto old_next_id = Unit::GetNextId();
		util::FinallyGuard reset_id( [ = ]() { Unit::SetNextId( old_next_id ); } );
		for ( const float value : { -std::numeric_limits< float >::max(), -1.0f, -0.0f, 0.0f,
			std::numeric_limits< float >::denorm_min(), 1.0f, 2.0f, std::numeric_limits< float >::max() } ) {
			BackendFixture fixture; fixture.EnableTurns();
			const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* manager = fixture.GetGame()->GetUM(); DefineSnapshotUnit( manager );
				UnitSnapshotRecord record; record.movement = record.health = value;
				auto input = record.Encode();
				auto* unit = Unit::Deserialize( GSE_CALL, input, manager );
				manager->SpawnUnit( GSE_CALL, unit );
				GT_ASSERT( Unit::Serialize( unit ).ToString() == input.ToString(), "finite unit state changed" );
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "finite boundary snapshot failed: " + error );
		}
		GT_OK();
	} );

	task->AddTest( "test unit manager snapshot rejects unsupported formats and malformed registries", GT() {
		const auto old_next_id = Unit::GetNextId();
		util::FinallyGuard reset_id( [ = ]() { Unit::SetNextId( old_next_id ); } );
		for ( const bool running : { false, true } ) {
			for ( int scenario = 0 ; scenario < 18 ; ++scenario ) {
				BackendFixture source, restored; source.EnableTurns(); restored.EnableTurns(); restored.SetRunning( running );
				std::string morale_bytes, def_bytes;
				const auto source_error = source.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* manager = source.GetGame()->GetUM(); DefineSnapshotUnit( manager );
					morale_bytes = MoraleSet::Serialize( manager->GetMoraleSet( "TEST_MORALE" ) ).ToString();
					def_bytes = Def::Serialize( manager->GetUnitDef( "TEST_UNIT" ) ).ToString();
					GT_OK();
				} );
				GT_ASSERT( source_error.empty(), "registry fixture failed: " + source_error );
				types::Buffer input;
				if ( scenario == 0 ) { input.WriteInt( 0 ); } // legacy unversioned manager
				else if ( scenario == 1 ) { input.WriteString( "GLSMAC_UNITS_V999" ); }
				else {
					input.WriteString( "GLSMAC_UNITS_V1" );
					if ( scenario == 2 || scenario == 3 ) { input.WriteInt( scenario == 2 ? -1 : std::numeric_limits< int64_t >::max() ); }
					else {
						input.WriteInt( scenario == 5 ? 2 : 1 );
						input.WriteString( scenario == 4 ? "WRONG_MORALE_KEY" : "TEST_MORALE" ); input.WriteString( morale_bytes );
						if ( scenario == 5 ) { input.WriteString( "TEST_MORALE" ); input.WriteString( morale_bytes ); }
						input.WriteInt( scenario == 7 ? 2 : 1 );
						input.WriteString( scenario == 6 ? "WRONG_UNIT_KEY" : "TEST_UNIT" );
						if ( scenario == 8 || scenario == 9 || scenario >= 14 ) {
							types::Buffer invalid;
							invalid.WriteString( "TEST_UNIT" ); invalid.WriteString( scenario == 8 ? "MISSING" : "TEST_MORALE" );
							invalid.WriteString( "Broken definition" ); invalid.WriteInt( scenario < 14 ? INT64_C( 4294967296 ) : DT_STATIC );
							if ( scenario >= 14 ) {
								invalid.WriteInt( scenario == 14 ? -1 : ( scenario == 15 ? INT64_C( 4294967296 ) : MT_LAND ) );
								invalid.WriteFloat( scenario == 16 ? std::numeric_limits< float >::infinity() : 1.0f );
								invalid.WriteInt( INT64_C( 4294967296 ) ); // invalid render tag must not narrow to RT_SPRITE
							}
							input.WriteString( invalid.ToString() );
						}
						else { input.WriteString( def_bytes ); }
						if ( scenario == 7 ) { input.WriteString( "TEST_UNIT" ); input.WriteString( def_bytes ); }
						input.WriteInt( scenario == 11 ? 2 : 1 );
						UnitSnapshotRecord record; if ( scenario == 10 ) { record.def = "MISSING"; }
						input.WriteString( record.Encode().ToString() );
						if ( scenario == 11 ) { record.x = 4; input.WriteString( record.Encode().ToString() ); }
						input.WriteInt( scenario == 12 ? 71 : 123 );
						if ( scenario == 13 ) { input.WriteInt( 999 ); }
					}
				}
				const auto next_id = Unit::GetNextId();
				const auto rng = restored.GetGame()->GetRandom()->GetStateString();
				size_t hooks = 0;
				const auto error = restored.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* manager = restored.GetGame()->GetUM();
					manager->On( GSE_CALL, "unit_spawn", NATIVE_CALL( &hooks ) { ++hooks; return VALUE( value::Undefined ); } ) );
					bool rejected = false;
					try { manager->Deserialize( GSE_CALL, input ); }
					catch ( const std::runtime_error& ) { rejected = true; }
					GT_ASSERT( rejected, "malformed unit manager accepted: " + std::to_string( scenario ) );
					GT_ASSERT( hooks == 0 && restored.GetGame()->GetRandom()->GetStateString() == rng, "malformed restore ran gameplay" );
					if ( scenario < 4 ) {
						GT_ASSERT( !manager->GetMoraleSet( "TEST_MORALE" ) && !manager->GetUnitDef( "TEST_UNIT" ) && manager->GetUnits().empty(), "unsupported header published state" );
						GT_ASSERT( next_id == Unit::GetNextId() && restored.FrontendRequestCount() == 0, "unsupported header changed IDs or frontend" );
					}
					if ( scenario == 11 ) {
						GT_ASSERT( restored.GetGame()->GetMap()->GetTile( 2, 2 )->units.size() == 1, "duplicate record displaced accepted unit" );
						GT_ASSERT( restored.GetGame()->GetMap()->GetTile( 4, 2 )->units.empty(), "duplicate record claimed another tile" );
					}
					GT_OK();
				} );
				GT_ASSERT( error.empty(), "manager rejection failed: " + error );
			}
		}
		GT_OK();
	} );

	task->AddTest( "test deferred unit callback failure transfers ownership exactly once", GT() {
		const auto old_next_id = Unit::GetNextId();
		util::FinallyGuard reset_id( [ = ]() { Unit::SetNextId( old_next_id ); } );
		for ( const bool resume : { false, true } ) {
			BackendFixture fixture; fixture.EnableTurns();
			const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* manager = fixture.GetGame()->GetUM(); DefineSnapshotUnit( manager );
				size_t hooks = 0;
				manager->On( GSE_CALL, "unit_spawn", NATIVE_CALL( &hooks ) {
					if ( ++hooks == 1 ) { GSE_ERROR( EC.GAME_ERROR, "deferred unit hook failure" ); }
					return VALUE( value::Undefined );
				} ) );
				auto* first = SpawnSnapshotUnit( GSE_CALL, fixture, 71 );
				auto* second = SpawnSnapshotUnit( GSE_CALL, fixture, 88, 4 );
				const auto next_id = Unit::GetNextId();
				for ( size_t phase = 0 ; phase < 2 ; ++phase ) {
					types::Buffer output; output.WriteString( "preserve" );
					const auto before = output.ToString();
					bool rejected = false;
					try { manager->Serialize( output ); }
					catch ( const std::runtime_error& ) { rejected = true; }
					GT_ASSERT( rejected && output.ToString() == before, "snapshot omitted pending units or changed output before rejecting" );
					if ( !phase ) {
						fixture.SetRunning( true );
						bool threw = false;
						try { manager->ProcessUnprocessed( GSE_CALL ); }
						catch ( const Exception& error ) { threw = error.class_name == EC.GAME_ERROR; }
						GT_ASSERT( threw && hooks == 1, "deferred spawn did not propagate its callback failure" );
						GT_ASSERT( manager->GetUnits().size() == 1 && manager->GetUnit( 71 ) == first && !manager->GetUnit( 88 ), "callback failure lost ownership of registered or pending units" );
					}
				}
				if ( resume ) {
					manager->ProcessUnprocessed( GSE_CALL ); manager->ProcessUnprocessed( GSE_CALL );
					GT_ASSERT( hooks == 2 && manager->GetUnit( 71 ) == first && manager->GetUnit( 88 ) == second, "resuming pending startup replayed an already published unit" );
					types::Buffer completed; manager->Serialize( completed );
				}
				GT_ASSERT( Unit::GetNextId() == next_id, "startup registration changed IDs" );
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "deferred ownership failure: " + error );
			fixture.DestroyMapBeforeManagers(); // also exercises teardown with an untouched pending entry
		}
		GT_OK();
	} );

	task->AddTest( "test unit registry removal preserves live and pending borrowers", GT() {
		const auto old_next_id = Unit::GetNextId();
		util::FinallyGuard reset_id( [ = ]() { Unit::SetNextId( old_next_id ); } );
		for ( const bool running : { false, true } ) {
			size_t destroyed = 0;
			BackendFixture fixture; fixture.EnableTurns(); fixture.SetRunning( running );
			const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.GetGame(); auto* manager = game->GetUM(); DefineSnapshotUnit( manager );
				auto* morale = manager->GetMoraleSet( "TEST_MORALE" );
				auto* def = new CountedUnitDef( morale, destroyed ); manager->DefineUnit( def );
				manager->SpawnUnit( GSE_CALL, new Unit( GSE_CALL, manager, 71, def, &game->GetState()->m_slots->GetSlot( 2 ),
					game->GetMap()->GetTile( 2, 2 ), 1, 0, 1, false ) );
				const auto requests = fixture.FrontendRequestCount();
				for ( const bool remove_morale : { false, true } ) {
					bool rejected = false;
					try { if ( remove_morale ) { manager->UndefineMoraleSet( "TEST_MORALE" ); } else { manager->UndefineUnit( "COUNTED" ); } }
					catch ( const std::runtime_error& ) { rejected = true; }
					GT_ASSERT( rejected && destroyed == 0 && manager->GetUnitDef( "COUNTED" ) == def && manager->GetMoraleSet( "TEST_MORALE" ) == morale, "registry removal invalidated a borrower" );
					GT_ASSERT( fixture.FrontendRequestCount() == requests, "rejected undefine sent a frontend change" );
				}
				fixture.SetRunning( true ); manager->ProcessUnprocessed( GSE_CALL );
				manager->DespawnUnit( GSE_CALL, 71 );
				manager->UndefineUnit( "COUNTED" );
				GT_ASSERT( destroyed == 1 && !manager->GetUnitDef( "COUNTED" ), "unused definition was not deleted exactly once" );
				// Reverse-order definition rollback remains valid once no units borrow it.
				manager->UndefineUnit( "TEST_UNIT" ); manager->UndefineMoraleSet( "TEST_MORALE" );
				GT_ASSERT( !manager->GetUnitDef( "TEST_UNIT" ) && !manager->GetMoraleSet( "TEST_MORALE" ), "unused registries could not be removed in reverse order" );
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "registry ownership failure: " + error );
		}
		GT_OK();
	} );

	task->AddTest( "test manager owns shared unit definitions through map-first teardown", GT() {
		const auto old_next_id = Unit::GetNextId();
		util::FinallyGuard reset_id( [ = ]() { Unit::SetNextId( old_next_id ); } );
		for ( const bool running : { false, true } ) {
			size_t destroyed = 0;
			{
				BackendFixture fixture; fixture.EnableTurns(); fixture.SetRunning( running );
				const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* game = fixture.GetGame(); auto* manager = game->GetUM(); DefineSnapshotUnit( manager );
					auto* def = new CountedUnitDef( manager->GetMoraleSet( "TEST_MORALE" ), destroyed ); manager->DefineUnit( def );
					for ( size_t id : { 71, 88 } ) {
						manager->SpawnUnit( GSE_CALL, new Unit( GSE_CALL, manager, id, def, &game->GetState()->m_slots->GetSlot( 2 ),
							game->GetMap()->GetTile( 2, 2 ), 1, 0, 1, false ) );
					}
					GT_ASSERT( destroyed == 0, "live units destroyed their shared definition" );
					GT_OK();
				} );
				GT_ASSERT( error.empty(), "unit ownership setup failed: " + error );
				fixture.DestroyMapBeforeManagers();
			}
			GT_ASSERT( destroyed == 1, "shared unit definition was not destroyed exactly once" );
		}
		GT_OK();
	} );
}

void AddBackendTests( task::gsetests::GSETests* task ) {
	AddUnitSnapshotTests( task );
	AddEventTransportTests( task );
	AddEventResponseTests( task );
	AddRulesetTests( task );
	AddBackendFixtureLifecycleTests( task );
	AddWorldSnapshotTests( task );
	AddHostRulesPacketTests( task );
	AddPopReferenceTests( task );
	task->AddTest(
		"test backend mutation scopes restore read-only access after nested exceptions",
		GT() {
			BackendFixture fixture;
			const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.GetGame();
				const auto check_read_only = [ & ]() {
					try { game->CheckRW( GSE_CALL ); }
					catch ( const Exception& e ) { return e.class_name == EC.GAME_ERROR; }
					return false;
				};
				GT_ASSERT( check_read_only(), "fixture starts with mutation enabled" );
				for ( const auto kind : { 0, 1, 2 } ) {
					const auto fail = [ & ]() {
						if ( kind == 0 ) { throw std::runtime_error( "native callback failure" ); }
						if ( kind == 1 ) { GSE_ERROR( EC.GAME_ERROR, "script callback failure" ); }
						throw 42;
					};
					bool inner_caught = false, outer_caught = false, outer_still_writable = false;
					try {
						fixture.WithRW( [ & ]() {
							game->CheckRW( GSE_CALL );
							try {
								fixture.WithRW( [ & ]() { game->CheckRW( GSE_CALL ); fail(); } );
							}
							catch ( ... ) { inner_caught = true; }
							game->CheckRW( GSE_CALL );
							outer_still_writable = true;
							fail();
						} );
					}
					catch ( ... ) { outer_caught = true; }
					GT_ASSERT( inner_caught && outer_caught, "callback exception did not propagate" );
					GT_ASSERT( outer_still_writable, "nested failure disabled its enclosing mutation scope" );
					GT_ASSERT( check_read_only(), "callback exception left mutation enabled" );
					fixture.WithRW( [ & ]() { game->CheckRW( GSE_CALL ); } );
					GT_ASSERT( check_read_only(), "later successful scope left mutation enabled" );
				}
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "mutation scope check failed: " + error );
			GT_OK();
		}
	);
	AddWorkerTests( task );
	AddProductionStateTests( task );
	for ( const bool immediate : { true, false } ) {
		task->AddTest(
			immediate
				? "test immediate base manager restore skips new-base hooks and preserves frontend updates"
				: "test deferred base manager restore skips new-base hooks and preserves frontend updates",
			[ immediate ]( GSE* gse, context::Context* ctx ) -> std::string {
				using namespace game::backend::base;
				const auto previous_next_id = Base::GetNextId();
				util::FinallyGuard restore_next_id( [ previous_next_id ]() { Base::SetNextId( previous_next_id ); } );
				size_t spawn_calls[ 2 ] = {};
				BackendFixture source;
				BackendFixture restored;
				BackendFixture* fixtures[] = { &source, &restored };
				for ( size_t i = 0 ; i < 2 ; i++ ) {
					auto* fixture = fixtures[ i ];
					auto* game = fixture->GetGame();
					const auto error = fixtures[ i ]->WithScriptValues( [ &, i, game ]( GSE_CALLABLE ) -> std::string {
						GT_ASSERT( game->GetState()->IsMaster(), "fixture must exercise master gameplay hooks" );
						game->GetBM()->On( GSE_CALL, "base_spawn", NATIVE_CALL( fixture, game, &spawn_calls, i ) {
							spawn_calls[ i ]++;
							N_EXPECT_ARGS( 1 );
							N_GETVALUE( event, 0, Object );
							N_GETPROP_UNWRAP( base, event, "base", Base );
							// Like the default test-world hook, initializing a new base
							// adds random citizens and consumes the gameplay RNG.
							auto* wrapped = static_cast< value::Object* >( base->Wrap( GSE_CALL ) );
							auto* create_pop = static_cast< value::Callable* >( wrapped->Get( "create_pop" ) );
							auto* info = new value::Object( GSE_CALL, { { "type", new value::String( gc_space, "WORKER" ) } } );
							const auto extra_pops = game->GetRandom()->GetUInt( 1, 3 );
							fixture->WithRW( [ & ]() {
								for ( size_t n = 0 ; n < extra_pops ; n++ ) {
									create_pop->Run( GSE_CALL, { info } );
								}
							} );
							return VALUE( value::Undefined );
						} ) );
						GT_OK();
					} );
					GT_ASSERT( error.empty(), "spawn hook setup failed: " + error );
				}
				source.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
				auto* source_game = source.GetGame();
				auto* restored_game = restored.GetGame();
				const auto original_rng = source_game->GetRandom()->GetStateString();
				const auto source_error = source.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					for ( size_t i = 0 ; i < 2 ; i++ ) {
						auto* base = new Base(
							source_game, 301 + i, &source_game->GetState()->m_slots->GetSlot( 2 ),
							source_game->GetFaction( "GAIANS" ), source_game->GetMap()->GetTile( 2 + i * 2, 2 ),
							i ? "Forest Primeval" : "Gaia's Landing", {},
							{ { 7, Pop( nullptr, 7, source_game->GetBM()->GetPopDef( "WORKER" ), 1 ) } }, 23
						);
						source_game->GetBM()->SpawnBase( GSE_CALL, base );
					}
					GT_OK();
				} );
				GT_ASSERT( source_error.empty(), "new-base spawn failed: " + source_error );
				GT_ASSERT( spawn_calls[ 0 ] == 2, "new bases did not invoke their gameplay hooks" );
				GT_ASSERT( source_game->GetRandom()->GetStateString() != original_rng, "new-base hook did not consume gameplay RNG" );
				types::Buffer snapshot;
				source_game->GetBM()->Serialize( snapshot );
				restored_game->GetRandom()->SetState( source_game->GetRandom()->GetState() );
				const auto saved_rng = restored_game->GetRandom()->GetStateString();
				restored.SetRunning( immediate );
				restored.RestoreBaseManager( snapshot );
				if ( !immediate ) {
					GT_ASSERT( restored_game->GetBM()->GetBases().empty(), "deferred bases registered before startup" );
					GT_ASSERT( spawn_calls[ 1 ] == 0, "deferred snapshot invoked gameplay hooks before startup" );
					restored.SetRunning( true );
					const auto error = restored.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
						restored_game->GetBM()->ProcessUnprocessed( GSE_CALL );
						GT_OK();
					} );
					GT_ASSERT( error.empty(), "deferred base processing failed: " + error );
				}
				GT_ASSERT( snapshot.lenr == snapshot.lenw, "base manager snapshot has unread fields" );
				GT_ASSERT( restored_game->GetBM()->GetBases().size() == 2, "restored base count changed" );
				for ( const auto& entry : source_game->GetBM()->GetBases() ) {
					const auto* expected = entry.second;
					const auto* actual = restored_game->GetBM()->GetBase( entry.first );
					GT_ASSERT( actual, "restored base ID disappeared" );
					GT_ASSERT( actual->m_pops.size() == expected->m_pops.size(), "restore reran new-base initialization and changed population count" );
					for ( const auto& pop : expected->m_pops ) {
						const auto it = actual->m_pops.find( pop.first );
						GT_ASSERT( it != actual->m_pops.end(), "restored population ID disappeared" );
						GT_ASSERT( it->second.m_id == pop.second.m_id && it->second.m_variant == pop.second.m_variant, "restored population identity changed" );
						GT_ASSERT( it->second.m_base == actual, "restored citizen points to the wrong base" );
						GT_ASSERT( it->second.m_def == restored_game->GetBM()->GetPopDef( pop.second.m_def->m_id ), "restored citizen points to the wrong definition" );
					}
					GT_ASSERT( Base::Serialize( actual ).ToString() == Base::Serialize( expected ).ToString(), "restored base bytes changed" );
				}
				GT_ASSERT( spawn_calls[ 1 ] == 0, "restore invoked a new-base gameplay hook" );
				GT_ASSERT( restored_game->GetRandom()->GetStateString() == saved_rng, "restore consumed gameplay RNG" );
				const auto& requests = restored.PushBaseUpdates();
				GT_ASSERT( requests.size() == 4, "restored bases need one frontend spawn and refresh each" );
				std::unordered_set< size_t > spawned, refreshed;
				for ( const auto& request : requests ) {
					if ( request.type == game::FrontendRequest::FR_BASE_SPAWN ) {
						const auto& data = request.data.base_spawn;
						const auto* base = restored_game->GetBM()->GetBase( data.base_id );
						GT_ASSERT( base && *data.name == base->m_name, "frontend spawn lost the restored base name" );
						GT_ASSERT( data.tile_coords.x == base->GetTile()->coord.x && data.tile_coords.y == base->GetTile()->coord.y, "frontend spawn lost the restored base location" );
						spawned.insert( data.base_id );
					}
					else if ( request.type == game::FrontendRequest::FR_BASE_UPDATE ) {
						const auto& data = request.data.base_update;
						const auto* base = restored_game->GetBM()->GetBase( data.base_id );
						GT_ASSERT( base && data.pops->size() == base->m_pops.size(), "frontend refresh lost the restored population" );
						refreshed.insert( data.base_id );
					}
				}
				GT_ASSERT( spawned.size() == 2 && refreshed == spawned, "frontend did not spawn and refresh every restored base" );
				const auto new_base_error = restored.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* base = new Base( restored_game, Base::GetNextId(), &restored_game->GetState()->m_slots->GetSlot( 2 ),
						restored_game->GetFaction( "GAIANS" ), restored_game->GetMap()->GetTile( 6, 2 ), "Gaia's Landing", {}, {} );
					restored_game->GetBM()->SpawnBase( GSE_CALL, base );
					GT_ASSERT( base->m_name == "Gaia's Landing 2", "restored names were not registered for future new bases" );
					GT_ASSERT( !base->m_pops.empty(), "new base after restore did not initialize population" );
					GT_OK();
				} );
				GT_ASSERT( new_base_error.empty(), "new-base spawn after restore failed: " + new_base_error );
				GT_ASSERT( spawn_calls[ 1 ] == 1, "new-base gameplay hook no longer runs after restore" );
				GT_ASSERT( restored_game->GetRandom()->GetStateString() != saved_rng, "new-base gameplay RNG no longer advances after restore" );
				GT_OK();
			}
		);
	}
	task->AddTest(
		"test backend nutrient state survives snapshots and validates script writes",
		GT() {
			using namespace game::backend::base;
			const auto previous_next_id = Base::GetNextId();
			util::FinallyGuard restore_next_id( [ previous_next_id ]() { Base::SetNextId( previous_next_id ); } );
			for ( const int64_t nutrients : { std::numeric_limits< int64_t >::min(), int64_t( -2 ), int64_t( 0 ), int64_t( 37 ), std::numeric_limits< int64_t >::max() } ) {
				BackendFixture source, restored;
				auto* game = source.GetGame();
				Base base( game, 501, &game->GetState()->m_slots->GetSlot( 2 ), game->GetFaction( "GAIANS" ), game->GetMap()->GetTile( 2, 2 ), "Nutrient test", {}, {} );
				GT_ASSERT( base.GetAccumulatedNutrients() == 0, "new base nutrients must default to zero" );
				const auto read_only_error = source.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* object = static_cast< value::Object* >( base.Wrap( GSE_CALL ) );
					try {
						static_cast< value::Callable* >( object->Get( "set_accumulated_nutrients" ) )->Run( GSE_CALL, { new value::Int( gc_space, nutrients ) } );
						GT_FAIL( "read-only nutrient write was accepted" );
					}
					catch ( const Exception& e ) { GT_ASSERT( e.class_name == EC.GAME_ERROR, "wrong read-only error" ); }
					GT_ASSERT( base.GetAccumulatedNutrients() == 0, "read-only write changed nutrients" );
					GT_OK();
				} );
				GT_ASSERT( read_only_error.empty(), "read-only binding check failed: " + read_only_error );
				const auto write_error = source.WithMutableScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* object = static_cast< value::Object* >( base.Wrap( GSE_CALL ) );
					auto* setter = static_cast< value::Callable* >( object->Get( "set_accumulated_nutrients" ) );
					setter->Run( GSE_CALL, { new value::Int( gc_space, nutrients ) } );
					auto* value = static_cast< value::Callable* >( object->Get( "get_accumulated_nutrients" ) )->Run( GSE_CALL, {} );
					GT_ASSERT( value->type == VT_INT && static_cast< value::Int* >( value )->value == nutrients, "binding did not return the current nutrient value" );
					try { setter->Run( GSE_CALL, { new value::String( gc_space, "37" ) } ); GT_FAIL( "non-integer nutrient write was accepted" ); }
					catch ( const Exception& ) {}
					GT_ASSERT( base.GetAccumulatedNutrients() == nutrients, "rejected write changed nutrients" );
					GT_OK();
				} );
				GT_ASSERT( write_error.empty(), "nutrient binding check failed: " + write_error );
				auto snapshot = Base::Serialize( &base );
				std::unique_ptr< Base > actual( Base::Deserialize( snapshot, restored.GetGame() ) );
				GT_ASSERT( actual->GetAccumulatedNutrients() == nutrients, "nutrients changed after reload" );
				GT_ASSERT( snapshot.lenr == snapshot.lenw, "nutrient snapshot has unread fields" );
				GT_ASSERT( Base::Serialize( actual.get() ).ToString() == snapshot.ToString(), "snapshot bytes changed after nutrient reload" );
			}
			GT_OK();
		}
	);
	task->AddTest(
		"test negative starvation balances and signed minimum survive nutrient snapshots",
		GT() {
			using namespace game::backend::base;
			const auto previous_next_id = Base::GetNextId();
			util::FinallyGuard restore_next_id( [ previous_next_id ]() { Base::SetNextId( previous_next_id ); } );
			// RE-SMAC-GROW-001 identifies negative stored food as starvation
			// state (static-derived SMAC evidence, not a runtime observation).
			for ( const int64_t nutrients : { int64_t( -2 ), std::numeric_limits< int64_t >::min() } ) {
				BackendFixture source, restored;
				auto* game = source.GetGame();
				Base base( game, 503, &game->GetState()->m_slots->GetSlot( 2 ), game->GetFaction( "GAIANS" ), game->GetMap()->GetTile( 2, 2 ), "Negative nutrient balance", {}, {}, 1, nutrients );
				GT_ASSERT( base.GetAccumulatedNutrients() == nutrients, "constructor changed the signed nutrient balance" );
				auto snapshot = Base::Serialize( &base );
				std::unique_ptr< Base > actual( Base::Deserialize( snapshot, restored.GetGame() ) );
				GT_ASSERT( actual->GetAccumulatedNutrients() == nutrients, "snapshot changed the negative nutrient balance" );
				GT_ASSERT( Base::Serialize( actual.get() ).ToString() == snapshot.ToString(), "negative nutrient snapshot bytes changed" );
			}
			GT_OK();
		}
	);
	task->AddTest(
		"test population event apply and rollback preserve authoritative nutrients",
		GT() {
			using namespace game::backend::base;
			const auto previous_next_id = Base::GetNextId();
			util::FinallyGuard restore_next_id( [ previous_next_id ]() { Base::SetNextId( previous_next_id ); } );
			BackendFixture fixture;
			fixture.EnableRandom();
			fixture.DefinePop( "DOCTOR", "Doctor", PopDef::PF_NONE );
			auto* game = fixture.GetGame();
			Base base( game, 502, &game->GetState()->m_slots->GetSlot( 2 ), game->GetFaction( "GAIANS" ), game->GetMap()->GetTile( 2, 2 ), "Nutrient events", {}, { { 1, Pop( nullptr, 1, game->GetBM()->GetPopDef( "DOCTOR" ), 0 ) } }, 2 );
			const auto error = fixture.WithMutableScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* script = fixture.RunScript( GSE_CALL, "./GLSMAC_data/tests/_base_nutrients" );
				GT_ASSERT( script->type == VT_CALLABLE, "nutrient event test did not return a callable" );
				static_cast< value::Callable* >( script )->Run( GSE_CALL, { base.Wrap( GSE_CALL ) } );
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "nutrient event regression failed: " + error );
			GT_OK();
		}
	);
	task->AddTest(
		"test nested script snapshots resolve tiles in the destination world",
		GT() {
			BackendFixture source;
			BackendFixture restored;
			for ( const bool array_first : { true, false } ) {
				types::Buffer snapshot;
				const auto write_error = source.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* tile = source.GetGame()->GetMap()->GetTile( 4, 4 )->Wrap( GSE_CALL );
					Value* nested;
					if ( array_first ) {
						auto* object = new value::Object( GSE_CALL, { { "tile", tile } } );
						nested = new value::Array( gc_space, { object } );
					}
					else {
						auto* array = new value::Array( gc_space, { tile } );
						nested = new value::Object( GSE_CALL, { { "tiles", array } } );
					}
					Value::Serialize( &snapshot, nested );
					GT_OK();
				} );
				GT_ASSERT( write_error.empty(), "nested snapshot write failed: " + write_error );
				const auto read_error = restored.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* nested = Value::Deserialize( GSE_CALL, &snapshot, restored.GetGame() );
					Value* wrapped_tile;
					if ( array_first ) {
						GT_ASSERT( nested->type == VT_ARRAY, "outer array type changed" );
						auto* object = static_cast< value::Array* >( nested )->Get( 0 );
						GT_ASSERT( object->type == VT_OBJECT, "inner object type changed" );
						wrapped_tile = static_cast< value::Object* >( object )->Get( "tile" );
					}
					else {
						GT_ASSERT( nested->type == VT_OBJECT, "outer object type changed" );
						auto* array = static_cast< value::Object* >( nested )->Get( "tiles" );
						GT_ASSERT( array->type == VT_ARRAY, "inner array type changed" );
						wrapped_tile = static_cast< value::Array* >( array )->Get( 0 );
					}
					auto* tile = Tile::Unwrap( wrapped_tile );
					GT_ASSERT( tile == restored.GetGame()->GetMap()->GetTile( 4, 4 ), "tile did not resolve in the destination world" );
					GT_ASSERT( tile != source.GetGame()->GetMap()->GetTile( 4, 4 ), "tile still points into the source world" );
					GT_ASSERT( snapshot.lenr == snapshot.lenw, "nested snapshot has unread fields" );
					types::Buffer repeated;
					Value::Serialize( &repeated, nested );
					GT_ASSERT( repeated.ToString() == snapshot.ToString(), "nested snapshot bytes changed after reload" );
					GT_OK();
				} );
				GT_ASSERT( read_error.empty(), "nested snapshot read failed: " + read_error );
			}
			GT_OK();
		}
	);
	task->AddTest(
		"test backend base manager snapshots restore facility definitions before base instances",
		GT() {
			using namespace game::backend::base;
			const auto previous_next_id = Base::GetNextId();
			util::FinallyGuard restore_next_id( [ previous_next_id ]() { Base::SetNextId( previous_next_id ); } );
			BackendFixture source;
			BackendFixture restored;
			source.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
			source.DefineFacility( new FacilityDef( "HEADQUARTERS", "Headquarters", "Administration", 0, { "facilities.pcx", 1, 2, 24, 32 } ) );
			source.DefineFacility( new FacilityDef( "RECYCLING_TANKS", "Recycling Tanks", "Recycles base resources", 40, { "facilities.pcx", 37, 44, 48, 64 } ) );
			auto* source_game = source.GetGame();
			auto* restored_game = restored.GetGame();
			Base expected(
				source_game, 201, &source_game->GetState()->m_slots->GetSlot( 2 ),
				source_game->GetFaction( "GAIANS" ), source_game->GetMap()->GetTile( 4, 4 ), "Gaia's Landing",
				{
					{ "HEADQUARTERS", source_game->GetBM()->GetFacilityDef( "HEADQUARTERS" ) },
					{ "RECYCLING_TANKS", source_game->GetBM()->GetFacilityDef( "RECYCLING_TANKS" ) },
				},
				{ { 7, Pop( nullptr, 7, source_game->GetBM()->GetPopDef( "WORKER" ), 1 ) } },
				8
			);
			types::Buffer definitions;
			source_game->GetBM()->Serialize( definitions );
			auto snapshot = Base::Serialize( &expected );
			std::unique_ptr< Base > actual;
			try {
				restored.RestoreBaseManager( definitions );
				for ( const auto& entry : expected.m_facilities ) {
					const auto* def = restored_game->GetBM()->GetFacilityDef( entry.first );
					GT_ASSERT( def, "facility definition missing after manager reload: " + entry.first );
					GT_ASSERT( def != entry.second, "facility definition still points into source world" );
					GT_ASSERT( FacilityDef::Serialize( def ).ToString() == FacilityDef::Serialize( entry.second ).ToString(), "facility metadata changed: " + entry.first );
				}
				actual.reset( Base::Deserialize( snapshot, restored_game ) );
			}
			catch ( const std::runtime_error& error ) {
				GT_FAIL( "facility snapshot reload failed: " + std::string( error.what() ) );
			}
			GT_ASSERT( definitions.lenr == definitions.lenw, "manager snapshot has unread fields" );
			GT_ASSERT( actual->m_facilities.size() == 2, "base facility count changed" );
			for ( const auto& entry : actual->m_facilities ) {
				GT_ASSERT( entry.second == restored_game->GetBM()->GetFacilityDef( entry.first ), "base facility does not use restored definition" );
			}
			GT_ASSERT( Base::Serialize( actual.get() ).ToString() == snapshot.ToString(), "base snapshot changed after restoring facility definitions" );
			GT_OK();
		}
	);
	task->AddTest(
		"test backend base manager rejects legacy and unknown snapshot formats before mutation",
		GT() {
			using namespace game::backend::base;
			const auto previous_next_id = Base::GetNextId();
			util::FinallyGuard restore_next_id( [ previous_next_id ]() { Base::SetNextId( previous_next_id ); } );
			for ( const std::string format : { "", "GLSMAC_BASES_V1", "GLSMAC_BASES_V2", "GLSMAC_BASES_V3", "GLSMAC_BASES_UNSUPPORTED" } ) {
				BackendFixture restored;
				types::Buffer snapshot;
				if ( !format.empty() ) {
					snapshot.WriteString( format );
				}
				// The complete old empty-manager format is valid under the old
				// reader; an added format marker must reject it before changing IDs.
				snapshot.WriteInt( 0 );
				snapshot.WriteInt( 0 );
				snapshot.WriteInt( previous_next_id + 123 );
				bool rejected = false;
				try {
					restored.RestoreBaseManager( snapshot );
				}
				catch ( const std::runtime_error& ) {
					rejected = true;
				}
				GT_ASSERT( rejected, "unsupported snapshot format was accepted" );
				GT_ASSERT( restored.GetGame()->GetBM()->GetBasePopDefs().empty(), "unsupported snapshot created definitions" );
				GT_ASSERT( restored.GetGame()->GetBM()->GetBases().empty(), "unsupported snapshot created bases" );
				GT_ASSERT( Base::GetNextId() == previous_next_id, "unsupported snapshot changed the next base ID" );
			}
			BackendFixture source;
			types::Buffer current;
			source.GetGame()->GetBM()->Serialize( current );
			bool old_reader_rejected = false;
			try {
				current.ReadInt(); // first operation performed by the legacy reader
			}
			catch ( const std::runtime_error& ) {
				old_reader_rejected = true;
			}
			GT_ASSERT( old_reader_rejected, "new snapshot could be mistaken for a legacy population count" );
			GT_OK();
		}
	);
	task->AddTest(
		"test backend base manager owns and clears facility definitions",
		GT() {
			size_t destructions = 0;
			BackendFixture fixture;
			fixture.DefineFacility( new CountedFacilityDef( destructions, "HEADQUARTERS" ) );
			fixture.DefineFacility( new CountedFacilityDef( destructions, "RECYCLING_TANKS" ) );
			auto* manager = fixture.GetGame()->GetBM();
			manager->Clear();
			GT_ASSERT( destructions == 2, "Clear did not destroy both owned facility definitions" );
			GT_ASSERT( !manager->GetFacilityDef( "HEADQUARTERS" ), "Clear retained headquarters definition" );
			GT_ASSERT( !manager->GetFacilityDef( "RECYCLING_TANKS" ), "Clear retained recycling tanks definition" );
			manager->Clear();
			GT_ASSERT( destructions == 2, "repeated Clear destroyed facility definitions twice" );
			GT_OK();
		}
	);
	task->AddTest(
		"test backend base snapshots resolve population by definition id",
		GT() {
			using namespace game::backend::base;
			const auto previous_next_id = Base::GetNextId();
			util::FinallyGuard restore_next_id( [ previous_next_id ]() { Base::SetNextId( previous_next_id ); } );
			BackendFixture source;
			BackendFixture restored;
			// Display names may coincide or change without changing definition IDs.
			source.DefinePop( "WORKER", "Citizen", PopDef::PF_TILE_WORKER );
			source.DefinePop( "DOCTOR", "Citizen", PopDef::PF_NONE );
			restored.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
			restored.DefinePop( "DOCTOR", "Doctor", PopDef::PF_NONE );
			auto* source_game = source.GetGame();
			auto* restored_game = restored.GetGame();
			Base expected(
				source_game, 101, &source_game->GetState()->m_slots->GetSlot( 2 ),
				source_game->GetFaction( "GAIANS" ), source_game->GetMap()->GetTile( 2, 2 ), "Gaia's Landing", {},
				{
					{ 7, Pop( nullptr, 7, source_game->GetBM()->GetPopDef( "WORKER" ), 0 ) },
					{ 19, Pop( nullptr, 19, source_game->GetBM()->GetPopDef( "DOCTOR" ), 1 ) },
				},
				23
			);
			auto snapshot = Base::Serialize( &expected );
			std::unique_ptr< Base > actual;
			try {
				actual.reset( Base::Deserialize( snapshot, restored_game ) );
			}
			catch ( const std::runtime_error& error ) {
				GT_FAIL( "base snapshot reload failed: " + std::string( error.what() ) );
			}
			GT_ASSERT( actual->m_pops.size() == 2, "population size changed" );
			for ( const auto& entry : expected.m_pops ) {
				const auto it = actual->m_pops.find( entry.first );
				GT_ASSERT( it != actual->m_pops.end(), "population ID disappeared" );
				const auto& pop = it->second;
				GT_ASSERT( pop.m_id == entry.second.m_id, "population ID changed" );
				GT_ASSERT( pop.m_variant == entry.second.m_variant, "population variant changed" );
				GT_ASSERT( pop.m_base == actual.get(), "population points to the wrong base" );
				GT_ASSERT( pop.m_def == restored_game->GetBM()->GetPopDef( entry.second.m_def->m_id ), "population resolved the wrong definition" );
				GT_ASSERT( pop.m_def != entry.second.m_def, "population still points into the source world" );
			}
			GT_ASSERT( actual->m_owner == &restored_game->GetState()->m_slots->GetSlot( 2 ), "base owner was not restored" );
			GT_ASSERT( actual->m_faction == restored_game->GetFaction( "GAIANS" ), "base faction was not restored" );
			GT_ASSERT( actual->GetTile() == restored_game->GetMap()->GetTile( 2, 2 ), "base tile was not restored" );
			GT_ASSERT( actual->GetTile()->base == actual.get(), "restored tile does not point to the base" );
			GT_ASSERT( Base::Serialize( actual.get() ).ToString() == snapshot.ToString(), "base snapshot bytes changed after reload" );
			GT_OK();
		}
	);
	task->AddTest(
		"test backend tile snapshots preserve asymmetric elevations and terrain",
		GT() {
			Tiles source( nullptr, 8, 8 );
			Tiles restored( nullptr, 8, 8 );
			auto& expected = source.At( 2, 2 );
			auto& actual = restored.At( 2, 2 );
			for ( const bool water : { false, true } ) {
				*expected.elevation.left = water ? -900 : -400;
				*expected.elevation.top = water ? -500 : 800;
				*expected.elevation.right = water ? 800 : 1400;
				*expected.elevation.bottom = water ? -600 : 2200;
				expected.moisture = water ? MOISTURE_NONE : MOISTURE_RAINY;
				expected.rockiness = water ? ROCKINESS_FLAT : ROCKINESS_ROCKY;
				expected.bonus = water ? BONUS_ENERGY : BONUS_MINERALS;
				expected.features = water ? FEATURE_GEOTHERMAL | FEATURE_XENOFUNGUS : FEATURE_RIVER | FEATURE_DUNES;
				expected.terraforming = water ? TERRAFORMING_FARM | TERRAFORMING_SOLAR : TERRAFORMING_ROAD | TERRAFORMING_SENSOR | TERRAFORMING_AIRBASE;
				expected.Update();
				GT_ASSERT( expected.is_water_tile == water, "fixture must exercise land and water" );
				GT_ASSERT( *expected.elevation.center != *expected.elevation.bottom, "fixture must distinguish center and bottom" );

				// Start with different terrain so a missed corner write cannot pass by accident.
				for ( auto* corner : actual.elevation.corners ) {
					*corner = water ? 1700 : -1300;
				}
				actual.Update();
				const auto snapshot = expected.Serialize();
				actual.Deserialize( snapshot );
				const auto error = CheckTileSnapshot( expected, actual );
				GT_ASSERT( error.empty(), "tile snapshot mismatch: " + error );
				GT_ASSERT( actual.Serialize().ToString() == snapshot.ToString(), "tile snapshot bytes changed after reload" );
			}
			GT_OK();
		}
	);
	task->AddTest(
		"test backend tile grid snapshots preserve shared vertices and map edges",
		GT() {
			Tiles source( nullptr, 8, 8 );
			for ( size_t y = 0; y < source.GetHeight(); y++ ) {
				for ( size_t x = y & 1; x < source.GetWidth(); x += 2 ) {
					auto& tile = source.At( x, y );
					// Each bottom vertex owns storage shared with the following rows.
					*tile.elevation.bottom = -2700 + static_cast< elevation_t >( y * 730 + x * 17 );
					if ( y == 0 ) {
						*tile.elevation.top = -3300 + static_cast< elevation_t >( x * 23 );
						*tile.elevation.right = -3000 + static_cast< elevation_t >( x * 19 );
					}
					tile.moisture = ( x + y ) % ( MOISTURE_RAINY + 1 );
					tile.rockiness = ( x / 2 + y ) % ( ROCKINESS_ROCKY + 1 );
					tile.bonus = ( x / 2 + y * 3 ) % ( BONUS_MINERALS + 1 );
					tile.features = ( 1 << ( ( x + y ) % 15 ) ) | FEATURE_RIVER;
					tile.terraforming = ( 1 << ( ( x + y * 2 ) % 13 ) ) | TERRAFORMING_ROAD;
				}
			}
			// Recompute only after all shared vertices have their final values.
			for ( size_t y = 0; y < source.GetHeight(); y++ ) {
				for ( size_t x = y & 1; x < source.GetWidth(); x += 2 ) {
					source.At( x, y ).Update();
				}
			}
			GT_ASSERT( source.AtConst( 0, 0 ).is_water_tile, "fixture needs water tiles" );
			GT_ASSERT( !source.AtConst( 7, 7 ).is_water_tile, "fixture needs land tiles" );

			Tiles restored( nullptr, 8, 8 );
			restored.Clear();
			const auto snapshot = source.Serialize();
			restored.Deserialize( snapshot );
			GT_ASSERT( restored.GetWidth() == source.GetWidth(), "grid width changed" );
			GT_ASSERT( restored.GetHeight() == source.GetHeight(), "grid height changed" );
			for ( size_t y = 0; y < source.GetHeight(); y++ ) {
				for ( size_t x = y & 1; x < source.GetWidth(); x += 2 ) {
					const auto& actual = restored.AtConst( x, y );
					const auto error = CheckTileSnapshot( source.AtConst( x, y ), actual );
					GT_ASSERT( error.empty(), "tile snapshot mismatch: " + error );
					GT_ASSERT( actual.tiles == &restored, "restored tile has the wrong owner" );
					GT_ASSERT( actual.elevation.left == actual.W->elevation.right, "east/west vertex sharing lost" );
					if ( y >= 2 ) {
						GT_ASSERT( actual.elevation.top == actual.N->elevation.bottom, "north/south vertex sharing lost" );
					}
				}
			}
			GT_ASSERT( restored.Serialize().ToString() == snapshot.ToString(), "grid snapshot bytes changed after reload" );
			GT_OK();
		}
	);
}

}
}
