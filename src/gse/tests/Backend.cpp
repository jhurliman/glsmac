#include "Backend.h"
#include "fixtures/SmacRulesSource.h"
#include "fixtures/WorldSnapshotRules.h"
#include "GLSMAC.h"
#include "ui/UI.h"
#include "ui/dom/Object.h"

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
#include "gc/GC.h"
#include "engine/Engine.h"
#include "resource/ResourceManager.h"
#include "game/backend/Game.h"
#include "game/backend/Random.h"
#include "game/backend/resource/Resource.h"
#include "game/backend/Player.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/SpriteRender.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/animation/AnimationManager.h"
#include "game/backend/resource/ResourceManager.h"
#include "game/frontend/Game.h"
#include "graphics/Null.h"
#include "game/backend/State.h"
#include "game/backend/connection/Client.h"
#include "game/backend/connection/Server.h"
#include "types/Packet.h"
#include "types/texture/Texture.h"
#include "types/mesh/Render.h"
#include "types/mesh/Data.h"
#include "network/Network.h"
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
#include "game/backend/map/tile/TileManager.h"
#include "game/backend/map/module/Module.h"
#include "game/backend/map/tile/Tiles.h"
#include "util/FinallyGuard.h"
#include "util/UUID.h"

#include <algorithm>
#include <memory>
#include <exception>
#include <limits>
#include <stdexcept>
#include <set>
#include <thread>
#include <filesystem>
#include <atomic>
#include <new>
#include <chrono>
#include <condition_variable>

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
			m_state = state;
			m_gse->AddRootObject( state );
			state->SetGame( &m_game );
			m_game.SetState( state );
			state->m_slots->Resize( 3 );
			state->GetFM()->Add( new game::backend::faction::Faction( "GAIANS", "Gaia's Stepdaughters" ) );
			NEWV( map, game::backend::map::Map, &m_game, 8, 8 );
			m_map.reset( map );
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
			m_state->DetachConnection();
			m_game.MTModule::Stop();
			m_game.m_admissions.clear();
			m_game.m_active_admission.reset();
			m_game.m_failed_admission.reset();
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
	static std::string CheckPolledReset( const int scenario, size_t& owner_destroyed ) {
		class SizedNull final : public graphics::Null {
		public:
			SizedNull() { OnWindowResize(); }
			const unsigned short GetWindowWidth() const override { return 64; }
			const unsigned short GetWindowHeight() const override { return 64; }
			const unsigned short GetViewportWidth() const override { return 64; }
			const unsigned short GetViewportHeight() const override { return 64; }
		} graphics;
		class Frontend final : public game::frontend::Game {
		public:
			Frontend( GLSMAC* app, game::backend::State* state, ::ui::UI* ui, size_t& iterations )
				: game::frontend::Game( app, state, ui ), iterations( iterations ) {}
			void Iterate() override { ++iterations; game::frontend::Game::Iterate(); }
			size_t& iterations;
		};
		class Iterable final : public ::ui::dom::Object {
		public:
			Iterable( GSE_CALLABLE, ::ui::UI* ui, size_t& iterations )
				: Object( GSE_CALL, ui, nullptr, {}, "reset_probe" ) {
				Object::Iterable( [ &iterations ]() { ++iterations; } );
			}
		};
		class PendingOwner final : public gc::Object {
		public:
			PendingOwner( gc::Space* space, size_t& destroyed ) : gc::Object( space ), destroyed( destroyed ) {}
			~PendingOwner() override { ++destroyed; }
			size_t& destroyed;
		};
		size_t frontend_iterations = 0, ui_iterations = 0, script_iterations = 0, load_completions = 0, old_bodies = 0, mains = 0;
		bool main_after_reset = false;
		auto* previous_graphics = g_engine->m_graphics;
		auto* previous_game = g_engine->m_game;
		{
			std::lock_guard guard( g_engine->GetGC()->m_spaces_mutex ); g_engine->m_graphics = &graphics;
		}
		util::FinallyGuard restore_services( [ & ]() {
			std::lock_guard guard( g_engine->GetGC()->m_spaces_mutex );
			g_engine->m_game = previous_game; g_engine->m_graphics = previous_graphics;
		} );
		// The rooted GLSMAC uses Engine's backend during traversal, so both
		// services remain installed until this fixture's runtime is destroyed.
		BackendFixture fixture;
		{
			std::lock_guard guard( g_engine->GetGC()->m_spaces_mutex ); g_engine->m_game = fixture.GetGame();
		}
		auto* app = fixture.m_glsmac.get();
		auto* space = fixture.m_gse->GetGCSpace();
		const auto install_load_completion = [ & ]() {
			app->m_f_after_load = [ & ]() { ++load_completions; };
			app->m_load_thread = new std::thread( []() {} );
		};
		Frontend* frontend = nullptr;
		Iterable* iterable = nullptr;
		util::FinallyGuard destroy_iterable( [ & ]() {
			if ( iterable ) { fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				iterable->Destroy( GSE_CALL ); fixture.m_gse->RemoveRootObject( iterable ); GT_OK();
			} ); }
		} );
		std::vector< common::mt_id_t > retired_requests;
		const auto setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			fixture.m_gse->AddRootObject( app );
			app->m_ui = new ::ui::UI( GSE_CALL );
			// Root this native iterable separately so S_Reset's DOM clear does
			// not erase the observation of whether UI::Iterate is entered.
			iterable = new Iterable( GSE_CALL, app->m_ui, ui_iterations );
			fixture.m_gse->AddRootObject( iterable );
			app->m_state = fixture.m_state;
			app->m_is_game_running = true;
			frontend = new Frontend( app, fixture.m_state, app->m_ui, frontend_iterations );
			app->m_game = frontend;
			frontend->m_world_ticket = fixture.FixtureWorldTicket();
			// RunMain uses an already cached argument in a running application.
			// The test callback observes scheduling, not Config's global wrapper.
			app->m_wrapobj = VALUE( value::Object,, GSE_CALL_NOGC, value::object_properties_t{} );
			app->m_main_callables.push_back( NATIVE_CALL( app, &fixture, &mains, &main_after_reset ) {
				++mains;
				main_after_reset = !app->m_reset_needed && !app->m_is_game_running && !fixture.HasMap();
				return VALUE( value::Undefined );
			} ) );
			// Both completed and pending IDs belong to an unstarted frontend.
			frontend->m_mt_ids.ping = fixture.m_game.MT_Ping();
			frontend->m_mt_ids.save_map = fixture.m_game.MT_SaveMap( frontend->m_world_ticket, "unused-fixture-map" );
			fixture.ProcessRequests(); // save returns real R_ABORTED before readiness
			// Remaining fields own unprocessed, real MT requests. Cancellation
			// treats IDs uniformly regardless of the request's operation.
#define GAME_MT_ID_X( name ) \
			if ( !frontend->m_mt_ids.name ) { frontend->m_mt_ids.name = fixture.m_game.MT_Ping(); } \
			retired_requests.push_back( frontend->m_mt_ids.name );
			GAME_MT_IDS_X
#ifdef DEBUG
			GAME_MT_ID_X( save_dump )
#endif
#undef GAME_MT_ID_X
			if ( scenario != 1 ) { app->S_Reset( GSE_CALL ); }
			GT_OK();
		} );
		GT_ASSERT( setup.empty(), "reset setup: " + setup );
		if ( scenario != 1 ) { install_load_completion(); }
		if ( scenario == 1 ) {
			// Real owner work requests reset during GSE::Iterate. Do not join a
			// producer while holding the accumulation/collection mutexes.
			std::thread producer( [ & ]() { space->Accumulate( app, [ & ]() {
				ExecutionPointer ep; app->S_Reset( space, app->m_ctx, {}, ep );
			} ); } ); producer.join();
		}
		app->Iterate(); // must return before this same thread services backend
		GT_ASSERT( app->m_reset_request && app->m_reset_barrier && app->m_reset_needed && app->m_is_game_running,
			"reset preflight did not retain its waiting request and lifecycle guards" );
		GT_ASSERT( !frontend_iterations && !ui_iterations && !load_completions && !mains, "reset crossed into an old callback stage" );
		if ( scenario == 1 ) { install_load_completion(); }
		GT_ASSERT( frontend->m_world_retired, "pre-initialization frontend was not retired" );
#define GAME_MT_ID_X( name ) GT_ASSERT( !frontend->m_mt_ids.name, "frontend ID was not cleared: " #name );
		GAME_MT_IDS_X
#ifdef DEBUG
		GAME_MT_ID_X( save_dump )
#endif
#undef GAME_MT_ID_X
		for ( const auto id : retired_requests ) {
			GT_ASSERT( fixture.m_game.MT_GetResponse( id ).result == game::backend::R_NONE, "frontend drain left an owned response" );
		}
		const auto request = app->m_reset_request;
		const auto barrier = app->m_reset_barrier;
		PendingOwner* pending_owner = nullptr;
		const auto waiting_setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			app->S_Reset( GSE_CALL ); // repeated public request keeps the same barrier
			pending_owner = new PendingOwner( space, owner_destroyed );
			fixture.m_gse->AddRootObject( pending_owner );
			GT_OK();
		} );
		GT_ASSERT( waiting_setup.empty(), "waiting setup: " + waiting_setup );
		std::thread producer( [ & ]() {
			space->Accumulate( fixture.m_state, [ & ]() { ++script_iterations; } );
			fixture.m_state->WithGame( pending_owner, [ & ]( game::backend::Game*, GSE_CALLABLE ) { ++old_bodies; } );
		} ); producer.join();
		fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string { fixture.m_gse->RemoveRootObject( pending_owner ); GT_OK(); } );
		g_engine->GetGC()->Iterate();
		GT_ASSERT( !owner_destroyed, "waiting callback owner lost its pending root" );
		for ( size_t i = 0; i < 3; ++i ) { app->Iterate(); }
		GT_ASSERT( app->m_reset_request == request && app->m_reset_barrier == barrier && fixture.AdmissionCount() == 1,
			"waiting reset resubmitted or changed its cutoff" );
		GT_ASSERT( !script_iterations && !old_bodies && !ui_iterations && !frontend_iterations && !load_completions && !mains,
			"waiting reset executed an old GSE/UI/frontend/main callback" );
		if ( scenario == 2 ) {
			fixture.m_game.MTModule::Stop(); // owns an unprocessed reset payload
			GT_ASSERT( fixture.m_game.MT_GetResponse( request ).result == game::backend::R_NONE, "shutdown left an unprocessed response" );
			GT_OK();
		}
		const auto process = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			if ( scenario == 4 ) {
				// The original cutoff must not reset a replacement binding.
				fixture.m_state->UnsetGame(); fixture.m_state->SetGame( &fixture.m_game );
				fixture.FixtureWorldTicket();
			}
			else { fixture.ReleaseMapOwnership(); }
			// Isolate the still-separate native reset/collector exclusion issue.
			fixture.ProcessRequests();
			GT_OK();
		} );
		GT_ASSERT( process.empty(), "backend reset: " + process );
		if ( scenario == 3 ) {
			fixture.m_game.MTModule::Stop(); // owns the produced, unconsumed response
			GT_ASSERT( fixture.m_game.MT_GetResponse( request ).result == game::backend::R_NONE, "shutdown left a completed response" );
			GT_OK();
		}
		app->Iterate();
		GT_ASSERT( !app->m_reset_request && !app->m_reset_barrier && !app->m_reset_needed && !app->m_is_game_running && mains == 1,
			"terminal reset did not consume its exact response and restart once" );
		GT_ASSERT( fixture.AdmissionCount() == 0 && fixture.m_game.MT_GetResponse( request ).result == game::backend::R_NONE,
			"terminal controller record or response survived consumption" );
		GT_ASSERT( scenario == 4 ? fixture.HasMap() && !main_after_reset : !fixture.HasMap() && main_after_reset,
			"reset completion did not preserve its original cutoff semantics" );
		GT_ASSERT( !script_iterations && !old_bodies && !ui_iterations && !frontend_iterations && !load_completions, "terminal tick resumed old callback stages" );
		app->Iterate();
		g_engine->GetGC()->Iterate();
		GT_ASSERT( mains == 1 && script_iterations == 1 && !old_bodies && owner_destroyed == 1 && ui_iterations == 1 && frontend_iterations == 1 && load_completions == 1,
			"post-reset iteration did not drain canceled roots and resume stages exactly once" );
		GT_OK();
	}
	static std::string CheckFrontendQuitAdmission() {
		class SizedNull final : public graphics::Null {
		public:
			SizedNull() { OnWindowResize(); }
			const unsigned short GetWindowWidth() const override { return 64; }
			const unsigned short GetWindowHeight() const override { return 64; }
			const unsigned short GetViewportWidth() const override { return 64; }
			const unsigned short GetViewportHeight() const override { return 64; }
		} graphics;
		auto* previous_graphics = g_engine->m_graphics;
		{
			std::lock_guard guard( g_engine->GetGC()->m_spaces_mutex ); g_engine->m_graphics = &graphics;
		}
		util::FinallyGuard restore_graphics( [ & ]() {
			std::lock_guard guard( g_engine->GetGC()->m_spaces_mutex ); g_engine->m_graphics = previous_graphics;
		} );
		BackendFixture fixture; // local runtime is destroyed before graphics restoration
		auto* previous_game = g_engine->m_game;
		{
			std::lock_guard guard( g_engine->GetGC()->m_spaces_mutex ); g_engine->m_game = fixture.GetGame();
		}
		util::FinallyGuard restore_game( [ & ]() {
			std::lock_guard guard( g_engine->GetGC()->m_spaces_mutex ); g_engine->m_game = previous_game;
		} );
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* ui = new ::ui::UI( GSE_CALL );
			fixture.m_glsmac->m_ui = ui;
			fixture.m_gse->AddRootObject( ui );
			game::frontend::Game frontend( fixture.m_glsmac.get(), fixture.m_state, ui, {}, {} );
			// Only the quit/request path is initialized; returning on retirement
			// must avoid accessing asset-bearing presentation managers.
			frontend.m_is_initialized = true;
			frontend.m_world_ticket = fixture.FixtureWorldTicket();
			util::FinallyGuard release_frontend( [ & ]() { frontend.m_is_initialized = false; frontend.RetireBackendRequests(); } );
			return fixture.WithTransferableFrontendQueue( [ & ]() -> std::string {
				fixture.m_game.Quit( "fixture quit" );
				frontend.m_mt_ids.get_frontend_requests = fixture.m_game.MT_GetFrontendRequests( frontend.m_world_ticket );
				fixture.ProcessRequests();
				frontend.Iterate(); // actual FR_QUIT -> ExitGame -> targeted reset
				GT_ASSERT( frontend.m_world_retired && frontend.m_on_game_exit && frontend.m_mt_ids.reset, "quit notification canceled its newly owned exit reset" );
				size_t exits = 0;
				frontend.m_on_game_exit = [ & ]() { ++exits; }; // observe terminal navigation without exiting the test application
				fixture.ReleaseMapOwnership();
				fixture.ProcessRequests();
				frontend.Iterate();
				frontend.Iterate();
				GT_ASSERT( exits == 1 && !frontend.m_mt_ids.reset && !frontend.m_on_game_exit, "exit reset did not deliver exactly one terminal frontend completion" );
				GT_OK();
			} );
		} );
	}

	std::string CheckScriptRunEntry( const bool active, const bool reset_pending, const bool retained_prompt ) {
		auto* const state = m_game.GetState();
		// Test mode omits the asset manager; install the real resolver locally.
		resource::ResourceManager resources( "" );
		auto* previous_resources = g_engine->m_resource_manager;
		g_engine->m_resource_manager = &resources;
		util::FinallyGuard restore_resources( [ & ]() { g_engine->m_resource_manager = previous_resources; } );
		return WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			// An empty directory reaches the real missing-assets prompt without
			// loading proprietary files or constructing a UI.
			const auto directory = std::filesystem::temp_directory_path() / ( "glsmac-run-entry-" + util::UUID::Generate() );
			GT_ASSERT( std::filesystem::create_directory( directory ), "could not create empty path fixture" );
			util::FinallyGuard remove_directory( [ & ]() { std::filesystem::remove( directory ); } );
			GT_ASSERT( g_engine->GetResourceManager()->GetDetectedSMACType() == config::ST_UNKNOWN, "entry fixture requires asset-free test mode" );
			size_t prompts = 0;
			value::Callable* set_path = nullptr;
			m_glsmac->On( GSE_CALL, "smacpath_prompt", NATIVE_CALL( &prompts, &set_path ) {
				++prompts;
				set_path = static_cast< value::Callable* >( static_cast< value::Object* >( arguments.at( 0 ) )->Get( "set_smacpath" ) );
				return VALUE( value::Undefined );
			} ) );
			util::FinallyGuard restore( [ & ]() {
				m_glsmac->Off( GSE_CALL, "smacpath_prompt", 0 );
				m_glsmac->m_is_game_running = false;
				m_glsmac->m_reset_needed = false;
				m_glsmac->m_state = nullptr;
			} );
			if ( retained_prompt ) {
				m_glsmac->S_Init( GSE_CALL, directory.string() );
				GT_ASSERT( prompts == 1 && set_path, "first run did not reach the real path prompt" );
			}
			m_glsmac->m_state = state;
			m_glsmac->m_is_game_running = active;
			m_glsmac->m_reset_needed = reset_pending;
			const auto previous_prompts = prompts;
			bool rejected = false;
			try {
				if ( retained_prompt ) { set_path->Run( GSE_CALL, { VALUE( value::String,, directory.string() ) } ); }
				else { m_glsmac->S_Init( GSE_CALL, directory.string() ); }
			}
			catch ( const Exception& error ) { rejected = error.class_name == EC.GAME_ERROR; }
			GT_ASSERT( rejected == ( active || reset_pending ), "run entry did not enforce the active/reset boundary" );
			GT_ASSERT( prompts == previous_prompts + ( active || reset_pending ? 0 : 1 ), "blocked run reached resource loading/prompt" );
			GT_ASSERT( m_glsmac->m_is_game_running == active && m_glsmac->m_reset_needed == reset_pending,
				"run rejection/path error changed lifecycle flags" );
			GT_ASSERT( m_glsmac->m_state == state && state->GetGame() == &m_game && m_game.GetMap() == m_map.get(),
				"run entry replaced the bound state or map" );
			GT_OK();
		} );
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

	size_t PendingResponses() const { return m_game.m_pending_event_responses.size(); }
	const std::vector< game::backend::event::Event* >& PendingEvents() const { return m_game.m_pending_events; }
	void RebindSession() { m_state->UnsetGame(); m_state->SetGame( &m_game ); }
	void ResetRequest() {
		// This test owns the GC exclusion. The independent OP_RESET/collector
		// diagnostic documents why production's missing exclusion is separate.
		m_map.release();
		const auto id = m_game.MT_ResetController();
		m_game.MTModule::Iterate();
		const auto response = m_game.MT_GetResponse( id );
		if ( response.result != game::backend::R_SUCCESS ) { throw std::runtime_error( "reset request failed" ); }
	}
	void ProcessRequests() { m_game.MTModule::Iterate(); }
	void RequireInitializationReset() { m_game.m_game_state = game::backend::Game::GS_PREPARING_MAP; }
	std::string CheckStartupCoordinator( const int mode ) {
		using namespace game::backend;
		EnableTurnPlayers();
		pending_initialization_t pending;
		common::mt_id_t ping = 0, reset = 0;
		size_t initialized = 0, configured = 0;
		bool order = true, cancellation_parked = false;
		const auto setup = WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			ResetRequest();
			m_state->SetGame( &m_game );
			m_game.On( GSE_CALL, "initialize", NATIVE_CALL( this, &initialized, &order, &reset, &cancellation_parked, mode ) {
				++initialized;
				order = order && !m_game.m_tm && !m_game.m_rm && !m_game.m_um && !m_game.m_bm && !m_game.m_am &&
					m_game.m_game_state == Game::GS_PREPARING_MAP && m_game.m_slot_num == 0;
				if ( mode == 4 ) { throw std::runtime_error( "coordinated initialize failure" ); }
				if ( mode == 2 ) {
					const auto session = m_state->CaptureGameSession();
					// The worker performs only the actual native admission/reset and
					// parked Iterate paths; it never requests Space accumulation.
					std::thread native( [ & ]() {
						reset = m_game.MT_Reset( m_game.m_active_admission->ticket );
						m_game.Iterate();
						cancellation_parked = m_game.MT_GetResponse( reset ).result == R_NONE && session->IsActive() && m_state->GetGame() == &m_game;
					} );
					native.join();
				}
				return VALUE( value::Undefined );
			} ) );
			m_game.On( GSE_CALL, "configure", NATIVE_CALL( this, &initialized, &configured, &order ) {
				++configured;
				order = order && initialized == 1 && m_game.m_tm && m_game.m_rm && m_game.m_um && m_game.m_bm && m_game.m_am;
				m_game.ShowLoader( "configure output" );
				return VALUE( value::Undefined );
			} ) );
			pending = m_game.MT_Init( m_state );
			ping = m_game.MT_Ping();
			ProcessRequests();
			GT_ASSERT( initialized == 0 && configured == 0 && !m_game.m_map && !m_game.m_um, "OP_INIT ran owner hooks or native preparation before terminal admission" );
			GT_ASSERT( m_game.MT_GetResponse( pending.request ).result == R_SUCCESS && m_game.MT_GetResponse( ping ).result == R_SUCCESS,
				"init and following extracted request did not finish before owner work" );
			GT_OK();
		} );
		if ( !setup.empty() ) { return setup; }
		if ( mode == 1 || mode == 3 || mode == 5 ) {
			std::exception_ptr error;
			std::thread producer( [ & ]() { try { m_game.Iterate(); } catch ( ... ) { error = std::current_exception(); } } );
			producer.join();
			if ( error ) { std::rethrow_exception( error ); }
			GT_ASSERT( initialized == 0 && m_game.OwnerBlocksNative(), "foreign backend did not publish Pending without running owner" );
			const auto previous_transition = m_game.m_owner_transition;
			ping = m_game.MT_Ping();
			m_game.Iterate();
			GT_ASSERT( m_game.MT_GetResponse( ping ).result == R_NONE, "Pending admitted a native Ping" );
			if ( mode == 1 || mode == 5 ) {
				reset = m_game.MT_Reset( pending.ticket );
				const auto result = WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					m_game.Iterate();
					GT_ASSERT( m_game.MT_GetResponse( reset ).result == R_SUCCESS && !m_state->GetGame(), "canceled Pending required owner drain before reset" );
					GT_OK();
				} );
				if ( !result.empty() ) { return result; }
			}
			if ( mode == 5 ) {
				const auto replace = WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					m_state->SetGame( &m_game );
					m_game.On( GSE_CALL, "initialize", NATIVE_CALL( &initialized ) { ++initialized; return VALUE( value::Undefined ); } ) );
					const auto fresh = m_game.MT_Init( m_state ); ProcessRequests();
					GT_ASSERT( m_game.MT_GetResponse( fresh.request ).result == R_SUCCESS, "replacement admission failed" ); GT_OK();
				} );
				if ( !replace.empty() ) { return replace; }
				std::thread replacement( [ & ]() { try { m_game.Iterate(); } catch ( ... ) { error = std::current_exception(); } } );
				replacement.join(); if ( error ) { std::rethrow_exception( error ); }
				GT_ASSERT( m_game.OwnerBlocksNative(), "replacement did not publish its own Pending owner operation" );
				// Space does not promise owner-key iteration order. Invoke the real
				// old cleanup explicitly while the replacement is still Pending;
				// its eventual queued cleanup must remain harmless as well.
				m_game.FinishOwnerTransition( previous_transition );
				GT_ASSERT( m_game.OwnerBlocksNative(), "old finalizer cleared the replacement's Pending operation" );
				g_engine->GetGC()->Iterate();
			}
			ProcessAccumulations();
			if ( mode == 1 ) { GT_ASSERT( initialized == 0 && configured == 0 && !m_game.OwnerBlocksNative(), "canceled owner callback executed or retained parking" ); GT_OK(); }
			if ( mode == 5 ) { GT_ASSERT( initialized == 1 && configured == 0 && m_game.m_um && !m_game.OwnerBlocksNative(), "stale owner finalizer cleared or executed the replacement transition" ); GT_OK(); }
		}
		else {
			const auto result = WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string { m_game.Iterate(); GT_OK(); } );
			if ( !result.empty() ) { return result; }
		}
		return WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			GT_ASSERT( order && initialized == 1 && configured == ( mode == 4 ? 0u : 1u ) && !m_game.OwnerBlocksNative(), "owner hook order/managers or terminal parking is wrong" );
			if ( mode == 4 ) {
				const auto request = m_game.MT_GetMapData( pending.ticket ); ProcessRequests();
				const auto response = m_game.MT_GetResponse( request );
				util::FinallyGuard release( [ & ]() { m_game.MT_DestroyResponse( response ); } );
				GT_ASSERT( response.result == R_ERROR && *response.data.error.error_text == "coordinated initialize failure" && !m_game.m_map, "owner failure lost ticket error or early cleanup" );
			}
			else if ( mode == 2 ) {
				GT_ASSERT( cancellation_parked, "Running reset destroyed state or completed before owner exit" );
				m_game.Iterate();
				GT_ASSERT( m_game.MT_GetResponse( reset ).result == R_SUCCESS && !m_state->GetGame(), "Running reset did not execute after owner exit" );
			}
			else {
				GT_ASSERT( m_game.m_active_admission->stage == Game::StartupStage::PrepareBindings && !m_game.m_map && m_frontend_requests.size() == 1,
					"inline phase fell through into native preparation or cleared configure output" );
			}
			GT_OK();
		} );
	}
	std::string CheckConfigureCheckpoint( const bool client_side ) {
		using namespace game::backend;
		EnableTurnPlayers();
		size_t validated = 0, applied = 0, caller = 99;
		bool bindings_ready = false;
		const auto setup = WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			ResetRequest(); m_state->SetGame( &m_game );
			if ( client_side ) {
				auto* client = new connection::Client( gc_space, &m_state->m_settings.local );
				client->m_slot = 2; m_state->SetConnection( client );
			}
			else { m_state->SetConnection( new connection::Server( gc_space, &m_state->m_settings.local ) ); }
			auto* validate = NATIVE_CALL( &validated, &caller ) {
					++validated;
					caller = VALUE_GET( value::Int, VALUE_DATA( value::Object, arguments.at( 0 ) )->Get( "caller" ) );
					if ( caller ) { return VALUE( value::String,, "host only" ); }
					return VALUE( value::Undefined );
				} );
				auto* apply = NATIVE_CALL( this, &applied, &bindings_ready, client_side ) {
					++applied;
					bindings_ready = !client_side && m_state->GetConnection()->m_game_state == connection::Connection::GS_INITIALIZING;
					m_game.Event( GSE_CALL, "bootstrap", {} );
					return VALUE( value::Undefined );
				} );
				auto* noop = NATIVE_CALL() { return VALUE( value::Undefined ); } );
			RegisterEvent( "bootstrap", new event::EventHandler( gc_space, "bootstrap", validate, nullptr, apply, noop ) );
			m_game.On( GSE_CALL, "configure", NATIVE_CALL( this ) {
				m_game.Event( GSE_CALL, "bootstrap", {} );
				return VALUE( value::Undefined );
			} ) );
			const auto pending = m_game.MT_Init( m_state ); ProcessRequests();
			GT_ASSERT( m_game.MT_GetResponse( pending.request ).result == R_SUCCESS, "bootstrap admission failed" );
			m_game.Iterate(); m_game.Iterate();
			GT_ASSERT( validated == 0 && m_game.m_pending_events.size() == 1 && m_game.m_active_admission->stage == Game::StartupStage::ConfigureEvents,
				"configure events ran before native connection preparation" );
			GT_OK();
		} );
		if ( !setup.empty() ) { return setup; }
		std::exception_ptr escaped;
		std::thread producer( [ & ]() { try { m_game.Iterate(); } catch ( ... ) { escaped = std::current_exception(); } } );
		producer.join(); if ( escaped ) { std::rethrow_exception( escaped ); }
		GT_ASSERT( m_game.OwnerBlocksNative() && m_game.m_pending_events.empty() && !validated, "configure batch was not captured before owner publication" );
		const auto late = WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string { m_game.Event( GSE_CALL, "bootstrap", {} ); GT_OK(); } );
		if ( !late.empty() ) { return late; }
		g_engine->GetGC()->Iterate();
		ProcessAccumulations();
		GT_ASSERT( validated == 1 && caller == ( client_side ? 2u : 0u ) && applied == ( client_side ? 0u : 1u ),
			"bootstrap checkpoint changed event authority or processed more than its captured batch" );
		GT_ASSERT( client_side || bindings_ready, "host bootstrap apply preceded GS_INITIALIZING" );
		GT_ASSERT( m_game.m_pending_events.size() == ( client_side ? 1u : 2u ) && m_game.m_active_admission->stage == Game::StartupStage::AcquireMap,
			"generated/late events were recursively drained or map acquisition ran inline" );
		GT_OK();
	}
	std::string CheckStartupFinish( const bool restored, const bool cancel ) {
		using namespace game::backend;
		EnableWorldSnapshot(); EnableTurnPlayers();
		BackendFixture source;
		source.EnableWorldSnapshot(); source.EnableTurnPlayers(); source.SetSnapshotTurn( 37 );
		for ( auto* state : { m_state, source.m_state } ) {
			state->m_settings.global.SelectRuleset( "smax" );
			state->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
		}
		const auto saved_random = source.m_game.GetRandom()->GetStateString();
		size_t starts = 0, worlds = 0, advances = 0, statuses = 0, parked_checks = 0;
		bool parked = true;
		common::mt_id_t reset = 0;
		std::vector< common::mt_id_t > pings;
		return WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			if ( restored ) {
				m_state->SetConnection( new RecordingConnection( gc_space ) );
				RestoreWorldBody( GSE_CALL, source.WorldSnapshot() );
			}
			const auto ticket = FixtureWorldTicket();
			m_game.m_active_admission->connection = m_state->GetConnection();
			m_game.m_active_admission->slot = 0;
			m_game.m_active_admission->stage = Game::StartupStage::Start;
			m_game.m_game_state = Game::GS_STARTING;
			const auto session = m_state->CaptureGameSession();
			const auto probe = [ & ]() {
				++parked_checks;
				std::thread native( [ & ]() {
					if ( cancel && !reset ) { reset = m_game.MT_Reset( ticket ); }
					const auto ping = m_game.MT_Ping(); pings.push_back( ping );
					m_game.Iterate();
					parked = parked && m_game.MT_GetResponse( ping ).result == R_NONE &&
						( !reset || m_game.MT_GetResponse( reset ).result == R_NONE ) && session->IsActive();
				} );
				native.join();
			};
			m_game.On( GSE_CALL, "create_world", NATIVE_CALL( &worlds, &probe ) { ++worlds; probe(); return VALUE( value::Undefined ); } ) );
			m_game.On( GSE_CALL, "start", NATIVE_CALL( this, &starts, &probe ) { ++starts; m_game.m_random->GetUInt( 0, 99 ); probe(); return VALUE( value::Undefined ); } ) );
			m_game.On( GSE_CALL, "turn_status", NATIVE_CALL( &statuses, &probe ) { ++statuses; probe(); return VALUE( value::Undefined ); } ) );
			auto* noop = NATIVE_CALL() { return VALUE( value::Undefined ); } );
			auto* apply = NATIVE_CALL( this, &advances, &probe ) {
				++advances; probe(); m_game.AdvanceTurn( 1 ); return VALUE( value::Undefined );
			} );
			RegisterEvent( "advance_turn", new event::EventHandler( gc_space, "advance_turn", noop, nullptr, apply, noop ) );
			m_map.release();
			util::FinallyGuard recover( [ this ]() { m_map.reset( m_game.m_map ); } );
			m_game.Iterate();
			GT_ASSERT( parked && parked_checks >= 2 && starts == 1 && worlds == ( restored ? 0u : 1u ) && advances == ( restored ? 0u : 1u ) && statuses,
				"Start/FirstTurn/status released native work before the complete owner operation" );
			GT_ASSERT( m_game.IsRunning() && !m_game.OwnerBlocksNative() && m_game.m_current_turn.GetId() == ( restored ? 37u : 1u ), "owner Start did not finish its full continuation" );
			GT_ASSERT( !restored || ( !HasPendingSnapshotTurn() && m_game.GetRandom()->GetStateString() == saved_random ), "Start released before saved turn/RNG installation" );
			GT_ASSERT( session->IsActive() != cancel, "cancellation retired the binding before owner exit or failed to retire it afterward" );
			m_game.Iterate();
			for ( const auto ping : pings ) { GT_ASSERT( m_game.MT_GetResponse( ping ).result == R_SUCCESS, "parked request did not finish after owner exit" ); }
			if ( cancel ) { GT_ASSERT( m_game.MT_GetResponse( reset ).result == R_SUCCESS && !m_game.m_map, "deferred reset did not retire the finished owner world" ); }
			GT_OK();
		} );
	}
	std::string CheckStartupDownloadBatch( const bool queued, const bool cancel ) {
		using namespace game::backend;
		class Transport final : public network::Network {
		public:
			~Transport() override { Stop(); }
			void Queue( const types::Packet& packet ) {
				network::Event event; event.type = network::Event::ET_PACKET;
				event.data.packet_data = packet.Serialize().ToString(); AddEvent( event );
			}
		protected:
			network::MT_Response ListenStart() override { return Success(); }
			network::MT_Response ListenStop() override { return Success(); }
			network::MT_Response Connect( const std::string&, MT_CANCELABLE ) override { return Success(); }
			network::MT_Response Disconnect() override { return Success(); }
			network::MT_Response DisconnectClient( network::cid_t ) override { return Success(); }
			void ProcessEvents() override { GetEvents(); }
		} transport;
		class Client final : public connection::Client {
		public:
			Client( gc::Space* space, settings::LocalSettings* settings, network::Network* network ) : connection::Client( space, settings, network ) {}
			void ServerReady() { m_game_state = GS_RUNNING; }
			const std::string& Reason() const { return m_disconnect_reason; }
		};
		EnableTurnPlayers();
		Client* client = nullptr;
		pending_initialization_t pending{};
		size_t messages = 0, errors = 0, roster_updates = 0;
		bool full_batch_before_owner = false, message_saw_roster = false;
		const auto setup = WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			ResetRequest(); m_state->SetGame( &m_game );
			m_state->m_settings.global.SelectRuleset( "smax" );
			m_state->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
			m_state->m_settings.local.remote_address = "fixture";
			client = new Client( gc_space, &m_state->m_settings.local, &transport );
			m_state->SetConnection( client ); client->Connect(); transport.Iterate(); m_state->Iterate(); client->ServerReady();
			m_game.On( GSE_CALL, "message", NATIVE_CALL( this, &messages, &message_saw_roster ) {
				++messages; message_saw_roster = m_state->m_slots->GetSlot( 0 ).GetPlayer()->GetPlayerName() == "Replacement";
				return VALUE( value::Undefined );
			} ) );
			m_game.On( GSE_CALL, "error", NATIVE_CALL( &errors ) { ++errors; return VALUE( value::Undefined ); } ) );
			pending = m_game.MT_Init( m_state ); ProcessRequests();
			GT_ASSERT( m_game.MT_GetResponse( pending.request ).result == R_SUCCESS, "client admission failed" );
			// Run the actual initialize/configure, native binding, captured event,
			// and map-acquisition phases. Only the socket transport is synthetic.
			for ( size_t i = 0; i < 4; ++i ) { transport.Iterate(); m_game.Iterate(); }
			GT_ASSERT( m_game.m_active_admission && m_game.m_active_admission->stage == Game::StartupStage::AwaitDownload && !m_game.m_map,
				"client did not reach download through actual startup stages" );
			client->m_on_players_list_update = [ & ]( size_t ) {
				++roster_updates; full_batch_before_owner = messages == 0 && m_game.m_pending_events.empty() && client->Reason().empty();
			};
			types::Buffer invalid; invalid.WriteString( "legacy untyped snapshot" );
			PrepareWorldDownload( client, invalid.ToString().size() );
			// Ensure the next real GETEVENTS response owns this entire packet batch.
			if ( client->m_mt_ids.events ) { transport.MT_Cancel( client->m_mt_ids.events ); transport.MT_GetResult( client->m_mt_ids.events ); client->m_mt_ids.events = 0; }
			m_state->Iterate();
			types::Packet message( types::Packet::PT_MESSAGE ); message.data.str = "before download"; transport.Queue( message );
			auto* original = new event::Event( &m_game, event::Event::ES_SERVER, 0, GSE_CALL, "captured_server_event", {}, "staged-event" );
			types::Buffer event_data; event_data.WriteString( original->Serialize().ToString() );
			types::Packet event_packet( types::Packet::PT_GAME_EVENT ); event_packet.data.str = event_data.ToString(); transport.Queue( event_packet );
			types::Packet chunk( types::Packet::PT_DOWNLOAD_NEXT_CHUNK_RESPONSE );
			chunk.data.str = invalid.ToString(); chunk.udata.download.offset = 0; chunk.udata.download.size = chunk.data.str.size(); transport.Queue( chunk );
			message.data.str = "after rejected download"; transport.Queue( message );
			Player replacement( "Replacement", Player::PR_HOST, nullptr, "" );
			slot::Slots slots( m_state ); slots.Resize( 3 ); slots.GetSlot( 0 ).SetPlayer( &replacement, 0, "" );
			types::Packet roster( types::Packet::PT_PLAYERS ); roster.data.num = 0; roster.data.str = slots.Serialize().ToString(); transport.Queue( roster );
			GT_OK();
		} );
		util::FinallyGuard cleanup( [ & ]() {
			WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				if ( m_state->GetConnection() ) { client->Disconnect(); transport.Iterate(); m_state->Iterate(); }
				GT_OK();
			} );
		} );
		if ( !setup.empty() ) { return setup; }
		transport.Iterate();
		std::exception_ptr escaped;
		const auto iterate = [ & ]() { try { m_game.Iterate(); } catch ( ... ) { escaped = std::current_exception(); } };
		if ( queued ) { std::thread producer( iterate ); producer.join(); } else { iterate(); }
		if ( escaped ) { std::rethrow_exception( escaped ); }
		GT_ASSERT( roster_updates == 1 && full_batch_before_owner, "download/event/message owner work ran before the complete native packet batch" );
		if ( queued ) {
			GT_ASSERT( messages == 0 && m_game.m_pending_events.empty() && m_game.OwnerBlocksNative(), "queued ingress decoded before owner claim" );
			g_engine->GetGC()->Iterate(); // actual collection with pending owned payloads
			if ( cancel ) {
				const auto reset = m_game.MT_Reset( pending.ticket );
				const auto result = WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					m_game.Iterate(); GT_ASSERT( m_game.MT_GetResponse( reset ).result == R_SUCCESS, "pending download reset required owner drain" ); GT_OK();
				} );
				if ( !result.empty() ) { return result; }
			}
			ProcessAccumulations();
		}
		if ( cancel ) { GT_ASSERT( !messages && !errors && !m_game.m_map && !m_game.OwnerBlocksNative(), "retired download ingress affected the world" ); GT_OK(); }
		GT_ASSERT( messages == 1 && message_saw_roster && m_game.m_pending_events.size() == 1 && !m_game.m_map && client->Reason() == "Snapshot format mismatch",
			"ingress order, exact owned event, current roster, or terminal decoder rejection changed" );
		GT_ASSERT( !errors, "decoder failure executed another owner phase inline" );
		const auto result = WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			m_game.Iterate();
			GT_ASSERT( errors == 1 && !m_game.m_map && m_game.m_pending_events.empty(), "failed download did not reach exact owner cleanup" );
			const auto id = m_game.MT_GetMapData( pending.ticket ); ProcessRequests();
			const auto response = m_game.MT_GetResponse( id ); util::FinallyGuard release( [ & ]() { m_game.MT_DestroyResponse( response ); } );
			GT_ASSERT( response.result == R_ERROR, "download failure lost its accepted ticket" ); GT_OK();
		} );
		return result;
	}
	// Older backend fixtures directly construct a running world. Give that
	// fixture world an explicit ticket; admission tests below use real MT_Init.
	game::backend::world_ticket_t FixtureWorldTicket() {
		std::lock_guard guard( m_game.m_admission_mutex );
		const auto session = m_state->CaptureGameSession();
		if ( !m_game.m_active_admission || m_game.m_active_admission->admitted_binding != session ) {
			const auto ticket = game::backend::Game::ReserveWorldTicket();
			m_game.m_active_admission = std::make_shared< game::backend::Game::WorldAdmission >(
				ticket, game::backend::Game::WorldAdmission::INITIALIZATION, m_state, session );
			m_game.m_active_admission->admitted_binding = session;
		}
		return m_game.m_active_admission->ticket;
	}
	size_t AdmissionCount() const { std::lock_guard guard( m_game.m_admission_mutex ); return m_game.m_admissions.size(); }
	bool AdmissionCanceled( game::backend::world_ticket_t ticket ) const {
		std::lock_guard guard( m_game.m_admission_mutex );
		const auto found = m_game.m_admissions.find( ticket );
		return found != m_game.m_admissions.end() && found->second->cancelled;
	}
	void ReleaseMapOwnership() { m_map.release(); }
	void ResetWorld() { m_map.release(); m_game.ResetGame(); }
	void CancelWorldInitialization() { m_map.release(); m_game.InitFailed( "Operation aborted" ); }
	bool HasActiveAdmission( game::backend::world_ticket_t ticket ) const {
		std::lock_guard guard( m_game.m_admission_mutex ); return m_game.MatchesActiveAdmission( ticket );
	}
	bool CollectAdmissionRoot( game::backend::world_ticket_t ticket ) {
		bool reachable = false;
		m_gse->GetGCSpace()->Accumulate( m_gse, [ & ]() {
			std::unordered_set< gc::Object* > objects;
			m_game.GetReachableObjects( objects );
			reachable = objects.count( m_state ) != 0 && m_game.m_admissions.count( ticket ) != 0;
			if ( reachable ) { m_gse->AddRootObject( &m_game ); m_gse->RemoveRootObject( m_state ); }
		} );
		if ( !reachable ) { return false; } // do not destroy a missing production root
		g_engine->GetGC()->Iterate();
		m_gse->GetGCSpace()->Accumulate( m_gse, [ & ]() {
			m_gse->AddRootObject( m_state ); m_gse->RemoveRootObject( &m_game );
		} );
		return true;
	}
	bool CollectDuringEventHandoff( gc::Object* expected, const std::function< void() >& after_collection ) {
		// Split the two actual production stages deterministically: collection
		// and reset/rebind can run after extraction but before queue registration.
		const auto session = m_state->CaptureGameSession();
		auto handoff = m_game.ExtractEventBatch( m_state, session );
		bool rooted = false;
		m_gse->GetGCSpace()->Accumulate( m_gse, [ & ]() {
			std::unordered_set< gc::Object* > reachable;
			m_game.GetReachableObjects( reachable );
			rooted = handoff && reachable.count( expected ) != 0;
		} );
		// A broken handoff must fail before destructive collection of an Event
		// still held by the native batch. Handoff destruction always unpublishes.
		if ( !rooted ) { return false; }
		g_engine->GetGC()->Iterate();
		after_collection();
		std::exception_ptr producer_error;
		std::thread producer( [ & ]() {
			try { m_game.DispatchEventBatch( std::move( handoff ) ); }
			catch ( ... ) { producer_error = std::current_exception(); }
		} );
		producer.join();
		if ( producer_error ) { std::rethrow_exception( producer_error ); }
		return true;
	}
	bool ExtractWithSession( const game::backend::State::game_session_t& session ) {
		return m_game.ExtractEventBatch( m_state, session ) != nullptr;
	}
	void SplitTileLockDispatch( const std::function< void() >& after_extraction ) {
		auto* const manager = m_game.GetTM();
		const auto session = m_state->CaptureGameSession();
		auto batch = manager->ExtractTileLockBatch( m_state, session );
		after_extraction();
		std::exception_ptr producer_error;
		std::thread producer( [ & ]() {
			try { game::backend::map::tile::TileManager::DispatchTileLockBatch( std::move( batch ) ); }
			catch ( ... ) { producer_error = std::current_exception(); }
		} );
		producer.join();
		if ( producer_error ) { std::rethrow_exception( producer_error ); }
	}
	size_t ExtractTileLocksWithSession( const game::backend::State::game_session_t& session ) {
		return m_game.GetTM()->ExtractTileLockBatch( m_state, session ).requests.size();
	}

	size_t EventHandoffs() const { return m_game.m_event_batch_handoffs.size(); }

	void QueueTileLock() {
		m_game.GetTM()->m_tile_lock_requests.push_back( { true, 2, { m_game.GetMap()->GetTile( 2, 2 ) } } );
	}
	size_t TileLockCount() const { return m_game.GetTM()->m_tile_locks.size(); }

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
	void EnableTurnPlayers() {
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this ]() {
			for ( const size_t id : { 0, 2 } ) {
				NEWV( player, game::backend::Player, "Turn player", game::backend::Player::PR_SINGLE, m_game.GetFaction( "GAIANS" ), "" );
				m_game.GetState()->AddPlayer( player );
				m_game.GetState()->m_slots->GetSlot( id ).SetPlayer( player, 0, "" );
			}
			m_game.m_slot_num = 2;
			m_game.m_slot = &m_game.GetState()->m_slots->GetSlot( 2 );
			m_game.m_player = m_game.m_slot->GetPlayer();
		} );
	}
	void EnableTileLockFixture() {
		EnableTurnPlayers();
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this ]() {
			m_game.m_tm = new game::backend::map::tile::TileManager( &m_game );
		} );
	}

	std::string WorldSnapshot() const { return m_game.SerializeWorldSnapshot(); }
	void SetSnapshotTurn( const size_t turn ) { m_game.m_current_turn.AdvanceTurn( turn ); }
	const std::vector< game::FrontendRequest >& FrontendRequests() const { return m_frontend_requests; }
	void ClearFrontendRequests() { m_frontend_requests.clear(); }
	void CheckTurnComplete() { m_game.CheckTurnComplete(); }
	void RestoreWorldBody( GSE_CALLABLE, const std::string& snapshot ) {
		m_game.m_game_state = game::backend::Game::GS_PREPARING_MAP;
		types::Buffer buffer( snapshot );
		game::backend::world_snapshot::ValidateHeader( buffer, m_game.GetState()->m_settings.global );
		const auto random = game::backend::world_snapshot::ReadRandomState( buffer );
		const auto turn = game::backend::world_snapshot::ReadTurnState( buffer, m_game.GetState()->m_slots->GetSlots() );
		const auto map = buffer.ReadString();
		m_game.DeserializeWorldSnapshotBody( GSE_CALL, buffer, map, random, turn );
	}
	void CompleteWorldRestore( GSE_CALLABLE ) {
		// Real InitComplete can call InitFailed/ResetGame, which owns deletion.
		m_map.release();
		util::FinallyGuard recover( [ this ]() { m_map.reset( m_game.m_map ); } );
		m_game.InitComplete( GSE_CALL );
	}
	void IterateWorldRestore() {
		m_map.release();
		util::FinallyGuard recover( [ this ]() { m_map.reset( m_game.m_map ); } );
		m_game.Iterate();
	}
	void ValidateSnapshotBeforeMapInitialization() {
		// These decoder fixtures deliberately bypass admission and asset-bearing
		// map initialization. Exercise its real validation helper directly; the
		// coordinator tests separately cover production Iterate admission.
		m_map.release();
		util::FinallyGuard recover( [ this ]() { m_map.reset( m_game.m_map ); } );
		m_game.CheckPendingSnapshotTurn();
	}
	std::string WithTransferableFrontendQueue( const std::function< std::string() >& check ) {
		// The production request transfers this queue to the response recipient.
		// Most fixture tests instead borrow the stack-owned m_frontend_requests.
		auto* const previous = m_game.m_pending_frontend_requests;
		NEW( m_game.m_pending_frontend_requests, std::vector< game::FrontendRequest > );
		util::FinallyGuard restore( [ & ]() {
			using Requests = common::MTModule< game::backend::MT_Request, game::backend::MT_Response >;
			static_cast< Requests& >( m_game ).Requests::Stop();
			DELETE( m_game.m_pending_frontend_requests );
			m_game.m_pending_frontend_requests = previous;
		} );
		return check();
	}
	void ProcessMapProgress( game::backend::map::module::Module& module, const size_t tile_count = 1 ) {
		// Invoke the actual progress/cancellation loop without render assets.
		auto* const map = m_game.m_map;
		if ( !map->m_map_state ) {
			NEW( map->m_map_state, game::backend::map::MapState );
			map->m_map_state->dimensions = { 8, 8 };
			const common::mt_flag_t canceled = false;
			map->m_map_state->LinkTileStates( canceled );
		}
		game::backend::map::Map::module_passes_t passes{ { &module } };
		game::backend::map::Map::tiles_t tiles( tile_count, map->GetTile( 0, 0 ) );
		// The old progress pump can execute ResetGame and delete this Map.
		m_map.release();
		util::FinallyGuard recover( [ this ]() { m_map.reset( m_game.m_map ); } );
		map->ProcessTiles( passes, tiles, m_game.m_init_cancel );
	}
	bool IsInitializationCanceled() const { return m_game.m_init_cancel; }
	std::string CheckInitializationFailure( const bool with_map, const int hook_failure, const bool retire_in_hook = false ) {
		return WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			// Exercise the real helper under owner accumulation; native reset's
			// independent collector-exclusion gap is outside this regression.
			struct Texture final : types::texture::Texture {
				size_t& destroyed;
				explicit Texture( size_t& count ) : types::texture::Texture( 1, 1 ), destroyed( count ) {}
				~Texture() override { ++destroyed; }
			};
			struct Render final : types::mesh::Render {
				size_t& destroyed;
				explicit Render( size_t& count ) : types::mesh::Render( 3, 1 ), destroyed( count ) {}
				~Render() override { ++destroyed; }
			};
			struct Data final : types::mesh::Data {
				size_t& destroyed;
				explicit Data( size_t& count ) : types::mesh::Data( 3, 1 ), destroyed( count ) {}
				~Data() override { ++destroyed; }
			};
			size_t destroyed = 0;
			if ( with_map ) {
				NEW( m_game.m_map->m_textures.terrain, Texture, destroyed );
				NEW( m_game.m_map->m_meshes.terrain, Render, destroyed );
				NEW( m_game.m_map->m_meshes.terrain_data, Data, destroyed );
			}
			else {
				DestroyMapBeforeManagers();
				m_game.m_bm = nullptr;
			}
			const auto ticket = FixtureWorldTicket();
			const auto binding = m_state->CaptureGameSession();
			m_game.m_game_state = game::backend::Game::GS_PREPARING_MAP;
			const std::string original = "original initialization failure";
			std::string input = original, received;
			size_t calls = 0;
			bool live_context = false, assets_released = false;
			m_game.On( GSE_CALL, "error", NATIVE_CALL( this, &calls, &received, &input, &live_context, &assets_released, &destroyed, with_map, hook_failure, retire_in_hook, ticket ) {
				++calls;
				received = VALUE_GET( value::String, VALUE_DATA( value::Object, arguments.at( 0 ) )->Get( "error" ) );
				live_context = m_game.GetState() == m_state && m_state->GetGame() == &m_game && bool( m_game.m_map ) == with_map;
				assets_released = destroyed == ( with_map ? 3u : 0u );
				input = "handler changed caller-owned message";
				if ( retire_in_hook ) { m_game.MT_RetireWorld( ticket ); }
				if ( hook_failure == 1 ) { GSE_ERROR( EC.GAME_ERROR, "secondary script error hook failure" ); }
				if ( hook_failure == 2 ) { throw std::runtime_error( "secondary native error hook failure" ); }
				return VALUE( value::Undefined );
			} ) );
			m_map.release();
			util::FinallyGuard recover( [ this ]() { m_map.reset( m_game.m_map ); } );
			std::exception_ptr escaped;
			try { m_game.InitFailed( input ); } catch ( ... ) { escaped = std::current_exception(); }
			GT_ASSERT( !escaped, "error hook escaped initialization failure cleanup" );
			GT_ASSERT( calls == 1 && received == original && live_context && assets_released, "failure notification lost original error, context, or presentation cleanup order" );
			GT_ASSERT( !m_game.m_map && !m_game.m_state && !m_state->GetGame() && !m_state->IsGameSessionActive( binding ) &&
				!m_game.m_tm && !m_game.m_rm && !m_game.m_um && !m_game.m_bm && !m_game.m_am &&
				m_game.m_game_state == game::backend::Game::GS_NONE && !m_game.HasHandlers( "error" ), "failure retained world state, managers, session, or handlers" );
			GT_ASSERT( m_game.m_initialization_error == original, "secondary hook failure or caller mutation replaced the original initialization error" );
			GT_ASSERT( m_frontend_requests.size() == 1 && m_frontend_requests.front().type == game::FrontendRequest::FR_LOADER_HIDE,
				"loader hide was cleared by failure cleanup" );
			const auto own = m_game.MT_GetMapData( ticket );
			const auto unrelated = m_game.MT_GetMapData( ticket + 1 );
			ProcessRequests();
			const auto response = m_game.MT_GetResponse( own );
			util::FinallyGuard release_response( [ & ]() { m_game.MT_DestroyResponse( response ); } );
			GT_ASSERT( retire_in_hook ? response.result == game::backend::R_ABORTED :
				response.result == game::backend::R_ERROR && response.data.error.error_text && *response.data.error.error_text == original,
				"failure was not bound to its ticket or hook retirement revived the failed attempt" );
			GT_ASSERT( m_game.MT_GetResponse( unrelated ).result == game::backend::R_ABORTED, "unrelated ticket received the failure" );
			const auto reset = m_game.MT_Reset( ticket );
			const auto late = m_game.MT_GetMapData( ticket );
			ProcessRequests();
			GT_ASSERT( m_game.MT_GetResponse( reset ).result == game::backend::R_SUCCESS && m_game.MT_GetResponse( late ).result == game::backend::R_ABORTED,
				"reset retained the failed ticket error" );
			GT_OK();
		} );
	}
	std::string CheckStartupPayloadOwnership( const int scenario ) {
		return WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			struct Texture final : types::texture::Texture {
				size_t& destroyed;
				explicit Texture( size_t& count ) : types::texture::Texture( 1, 1 ), destroyed( count ) {}
				~Texture() override { ++destroyed; }
			};
			struct Render final : types::mesh::Render {
				size_t& destroyed;
				explicit Render( size_t& count ) : types::mesh::Render( 3, 1 ), destroyed( count ) {}
				~Render() override { ++destroyed; }
			};
			struct Data final : types::mesh::Data {
				size_t& destroyed;
				explicit Data( size_t& count ) : types::mesh::Data( 3, 1 ), destroyed( count ) {}
				~Data() override { ++destroyed; }
			};
			size_t texture_deletes = 0, render_deletes = 0, data_deletes = 0;
			NEWV( texture, Texture, texture_deletes );
			NEWV( render, Render, render_deletes );
			NEWV( data, Data, data_deletes );
			m_game.m_map->m_textures.terrain = texture;
			m_game.m_map->m_meshes.terrain = render;
			m_game.m_map->m_meshes.terrain_data = data;
			const auto ticket = FixtureWorldTicket();
			if ( scenario != 0 ) {
				// Match the real native initialization envelope: pointers alias
				// Map ownership until OP_GET_MAP_DATA transfers both together.
				NEW( m_game.m_response_map_data, game::backend::response_map_data_t );
				m_game.m_response_map_data->terrain_texture = texture;
				if ( scenario != 5 ) {
					m_game.m_response_map_data->terrain_mesh = render;
					m_game.m_response_map_data->terrain_data_mesh = data;
				}
			}
			util::FinallyGuard cleanup( [ & ]() {
				if ( m_game.m_response_map_data ) {
					m_game.m_response_map_data->terrain_texture = nullptr;
					m_game.m_response_map_data->terrain_mesh = nullptr;
					m_game.m_response_map_data->terrain_data_mesh = nullptr;
					DELETE( m_game.m_response_map_data ); m_game.m_response_map_data = nullptr;
				}
				if ( m_game.m_map ) {
					m_game.m_map->m_textures.terrain = nullptr;
					m_game.m_map->m_meshes.terrain = nullptr;
					m_game.m_map->m_meshes.terrain_data = nullptr;
				}
				if ( !texture_deletes ) { DELETE( texture ); }
				if ( !render_deletes ) { DELETE( render ); }
				if ( !data_deletes ) { DELETE( data ); }
			} );
			if ( scenario < 3 || scenario == 5 ) {
				if ( scenario == 2 ) { CancelWorldInitialization(); }
				else { ResetWorld(); }
				GT_ASSERT( !m_game.m_response_map_data && !m_game.m_map && texture_deletes == 1 && render_deletes == 1 && data_deletes == 1,
					"startup reset/failure retained the response envelope or untransferred presentation" );
			}
			else {
				SetRunning( true );
				const auto request = m_game.MT_GetMapData( ticket );
				ProcessRequests();
				GT_ASSERT( !m_game.m_response_map_data && !m_game.m_map->m_textures.terrain && !m_game.m_map->m_meshes.terrain && !m_game.m_map->m_meshes.terrain_data,
					"map response did not relinquish backend presentation ownership" );
				if ( scenario == 4 ) {
					m_game.MT_Cancel( request ); // real completed response disposal owns all three assets
					GT_ASSERT( m_game.MT_GetResponse( request ).result == game::backend::R_NONE, "completed map response was not consumed by cancellation" );
					ResetWorld();
				}
				else {
					const auto response = m_game.MT_GetResponse( request );
					util::FinallyGuard release( [ & ]() { m_game.MT_DestroyResponse( response ); } );
					GT_ASSERT( response.result == game::backend::R_SUCCESS && response.data.get_map_data->terrain_texture == texture &&
						response.data.get_map_data->terrain_mesh == render && response.data.get_map_data->terrain_data_mesh == data,
						"successful response lost the original presentation payload" );
					ResetWorld();
					GT_ASSERT( texture_deletes == 0 && render_deletes == 0 && data_deletes == 0, "backend reset reclaimed transferred frontend assets" );
					// Mirror the frontend's consuming handoff before response disposal.
					response.data.get_map_data->terrain_texture = nullptr;
					response.data.get_map_data->terrain_mesh = nullptr;
					response.data.get_map_data->terrain_data_mesh = nullptr;
					DELETE( texture ); DELETE( render ); DELETE( data );
				}
				GT_ASSERT( texture_deletes == 1 && render_deletes == 1 && data_deletes == 1, "transferred assets were not released exactly once by their recipient" );
			}
			GT_OK();
		} );
	}
	std::string CheckFailedOldMapOwnership( const bool stop ) {
		return WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			struct RetainedMap final : game::backend::map::Map {
				size_t& destroyed;
				RetainedMap( game::backend::Game* game, size_t& count ) : Map( game, 2, 2 ), destroyed( count ) {}
				~RetainedMap() override { ++destroyed; }
			};
			size_t destroyed = 0;
			NEWV( old_map, RetainedMap, &m_game, destroyed );
			m_game.m_old_map = old_map;
			m_map.release();
			util::FinallyGuard cleanup( [ & ]() {
				if ( m_game.m_old_map && m_game.m_old_map != m_game.m_map ) { DELETE( m_game.m_old_map ); }
				m_game.m_old_map = nullptr;
				if ( m_game.m_map ) { DELETE( m_game.m_map ); m_game.m_map = nullptr; }
			} );
			m_game.InitFailed( "old map ownership fixture" );
			GT_ASSERT( m_game.m_map == old_map && !m_game.m_old_map && destroyed == 0,
				"failed initialization duplicated or destroyed retained map ownership" );
			if ( stop ) {
				// Stop owns these allocations in production; this fixture normally
				// supplies a stack queue and a separately owned Random instead.
				m_random.reset();
				NEW( m_game.m_random, game::backend::Random, &m_game, 12345 );
				NEW( m_game.m_pending_frontend_requests, std::vector< game::FrontendRequest > );
				util::FinallyGuard restore( [ & ]() {
					if ( m_game.m_random ) { DELETE( m_game.m_random ); m_game.m_random = nullptr; }
					if ( m_game.m_pending_frontend_requests ) { DELETE( m_game.m_pending_frontend_requests ); }
					m_game.m_pending_frontend_requests = &m_frontend_requests;
				} );
				m_game.Stop();
			}
			else { m_game.ResetGame(); m_game.ResetGame(); }
			GT_ASSERT( destroyed == 1 && !m_game.m_map && !m_game.m_old_map,
				"reset/stop did not release the transferred map exactly once" );
			GT_OK();
		} );
	}
	void DrainWorldRestore() {
		m_map.release();
		util::FinallyGuard recover( [ this ]() { m_map.reset( m_game.m_map ); } );
		ProcessAccumulations();
	}
	std::vector< value::Object* > PendingSnapshotTokens() const {
		std::lock_guard guard( m_game.m_snapshot_turn_mutex );
		std::vector< value::Object* > result;
		if ( m_game.m_pending_snapshot_turn ) {
			for ( const auto& binding : m_game.m_pending_snapshot_turn->bindings ) { result.push_back( binding.player ); }
		}
		return result;
	}
	bool HasPendingSnapshotRandom() const { return m_game.m_pending_snapshot_random.has_value(); }
	bool HasPendingSnapshotTurn() const { std::lock_guard guard( m_game.m_snapshot_turn_mutex ); return m_game.m_pending_snapshot_turn.has_value(); }
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
	bool IsRunning() const { return m_game.IsRunning(); }
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

	std::string WithEngineGameScriptValues( const std::function< std::string( GSE_CALLABLE ) >& check ) {
		// Player's native ready callback resolves its backend from Engine when wrapped.
		// The headless fixture owns that backend; synchronize the temporary service binding
		// with the collector, as in the existing actual network/startup fixtures.
		auto* const previous_game = g_engine->m_game;
		{ std::lock_guard lock( g_engine->GetGC()->m_spaces_mutex ); g_engine->m_game = &m_game; }
		util::FinallyGuard restore( [ & ]() {
			std::lock_guard lock( g_engine->GetGC()->m_spaces_mutex ); g_engine->m_game = previous_game;
		} );
		return WithMutableScriptValues( check );
	}

	std::string WithScriptValues( const std::function< std::string( GSE_CALLABLE ) >& check ) {
		std::string result;
		m_gse->GetGCSpace()->Accumulate( m_gse, [ this, &check, &result ]() {
			try {
				gse::ExecutionPointer ep;
				result = check( m_gse->GetGCSpace(), m_state->m_ctx, {}, ep );
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
			m_game.m_rm = new game::backend::resource::ResourceManager( &m_game );
			m_game.m_tm = new map::tile::TileManager( &m_game );
			for ( const auto* id : { "NUTRIENTS", "MINERALS", "ENERGY" } ) {
				m_game.m_rm->DefineResource( new game::backend::resource::Resource( id, id, {} ) );
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
	struct MapDeleter { void operator()( game::backend::map::Map* map ) const { DELETE( map ); } };
	static GSE* CreateRuntime() {
		// GLSMAC owns this runtime and destroys it through tracked DELETE.
		NEWV( runtime, GSE );
		return runtime;
	}

	game::backend::Game m_game;
	std::vector< game::FrontendRequest > m_frontend_requests;
	std::unique_ptr< game::backend::map::Map, MapDeleter > m_map;
	std::unique_ptr< game::backend::Random > m_random;
	GSE* m_gse; // owned by m_glsmac
	game::backend::State* m_state = nullptr; // retained by the runtime even after InitFailed clears Game's pointer
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

static void AddLoaderProgressTests( task::gsetests::GSETests* task ) {
	for ( const bool disconnect : { false, true } ) {
		task->AddTest( disconnect
			? "map processing defers queued disconnect until ordinary state iteration"
			: "map processing defers queued roster replacement until ordinary state iteration", GT( disconnect ) {
			using namespace game::backend;
			// Keep the actual Network request/response queue and Client packet
			// dispatcher; replace only socket I/O with a fixture-owned transport.
			class FixtureNetwork final : public network::Network {
			public:
				~FixtureNetwork() override { Stop(); }
				void Queue( const network::Event& event ) { AddEvent( event ); }
			protected:
				network::MT_Response ListenStart() override { return Success(); }
				network::MT_Response ListenStop() override { return Success(); }
				network::MT_Response Connect( const std::string&, MT_CANCELABLE ) override { return Success(); }
				network::MT_Response Disconnect() override { return Success(); }
				network::MT_Response DisconnectClient( const network::cid_t ) override { return Success(); }
				void ProcessEvents() override { GetEvents(); }
			} network;
			class FixtureClient final : public connection::Client {
			public:
				FixtureClient( gc::Space* space, settings::LocalSettings* settings, network::Network* network )
					: Client( space, settings, network ) {}
				bool IsDisconnectRequested() const { return !m_disconnect_reason.empty(); }
			};
			BackendFixture fixture;
			fixture.EnableTurnPlayers();
			auto* const game = fixture.GetGame();
			auto* const state = game->GetState();
			const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				state->m_settings.local.remote_address = "fixture";
				auto* client = new FixtureClient( gc_space, &state->m_settings.local, &network );
				state->SetConnection( client );
				client->Connect();
				network.Iterate();
				state->Iterate();
				size_t roster_updates = 0, disconnects = 0;
				util::FinallyGuard finish_connection( [ & ]() {
					if ( state->GetConnection() ) {
						client->Disconnect();
						network.Iterate();
						state->Iterate();
					}
				} );
				GT_ASSERT( client->IsConnected(), "fixture did not use the real connection completion path" );
				auto* const original = state->m_slots->GetSlot( 0 ).GetPlayer();
				client->m_on_players_list_update = [ & ]( size_t ) { ++roster_updates; };
				client->m_on_disconnect = [ & ]() { ++disconnects; };
				// First request the next event batch, then fill its real response.
				state->Iterate();
				network::Event event;
				if ( disconnect ) {
					event.type = network::Event::ET_DISCONNECT;
				}
				else {
					Player replacement( "Replacement", Player::PR_HOST, nullptr, "" );
					slot::Slots slots( state );
					slots.Resize( 3 );
					slots.GetSlot( 0 ).SetPlayer( &replacement, 0, "" );
					types::Packet packet( types::Packet::PT_PLAYERS );
					packet.data.num = 0; // full roster update, not initial slot assignment
					packet.data.str = slots.Serialize().ToString();
					event.type = network::Event::ET_PACKET;
					event.data.packet_data = packet.Serialize().ToString();
				}
				network.Queue( event );
				network.Iterate();
				return fixture.WithTransferableFrontendQueue( [ & ]() -> std::string {
					class ProgressModule final : public map::module::Module {
					public:
						explicit ProgressModule( map::Map* map ) : Module( map ) {}
						std::function< void() > visit;
						void GenerateTile( const map::tile::Tile*, map::tile::TileState*, map::MapState* ) override { visit(); }
					} module( game->GetMap() );
					size_t visits = 0;
					bool changed_inside_map = false;
					common::mt_id_t progress = 0, ping = 0;
					module.visit = [ & ]() {
						if ( !visits++ ) {
							progress = game->MT_GetFrontendRequests( fixture.FixtureWorldTicket() );
							ping = game->MT_Ping();
						}
						changed_inside_map |= state->m_slots->GetSlot( 0 ).GetPlayer() != original || client->IsDisconnectRequested();
					};
					// Cross the former 64-tile State::Iterate threshold. Repeated
					// tile references avoid needing a larger rendered map fixture.
					fixture.ProcessMapProgress( module, 65 );
					GT_ASSERT( visits == 65, "map did not cross the former packet-pump boundary" );
					GT_ASSERT( !changed_inside_map && !roster_updates && !disconnects, "map processing applied queued roster/disconnect events" );
					GT_ASSERT( state->m_slots->GetSlot( 0 ).GetPlayer() == original && client->IsConnected() && !client->IsDisconnectRequested(), "map processing changed the live connection or roster" );
					const auto response = game->MT_GetResponse( progress );
					util::FinallyGuard free_response( [ & ]() { game->MT_DestroyResponse( response ); } );
					GT_ASSERT( response.result == R_SUCCESS && response.data.get_frontend_requests.requests && !response.data.get_frontend_requests.requests->empty(), "map progress polling stopped" );
					GT_ASSERT( game->MT_GetResponse( ping ).result == R_SUCCESS, "map progress ping stopped" );
					state->Iterate();
					if ( disconnect ) {
						network.Iterate();
						state->Iterate();
						GT_ASSERT( disconnects == 1 && !state->GetConnection() && state->m_slots->GetCount() == 0, "ordinary state iteration did not apply disconnect cleanup" );
					}
					else {
						GT_ASSERT( roster_updates == 1 && state->m_slots->GetSlot( 0 ).GetPlayer() != original && state->m_slots->GetSlot( 0 ).GetPlayer()->GetPlayerName() == "Replacement", "ordinary state iteration did not apply the queued roster" );
						state->Iterate();
						GT_ASSERT( roster_updates == 1, "queued roster was applied more than once" );
					}
					GT_OK();
				} );
			} );
			GT_ASSERT( error.empty(), "map packet deferral check: " + error );
			GT_OK();
		} );
	}
	task->AddTest( "loader progress keeps reset queued until actual map processing returns", GT() {
		using namespace game::backend;
		BackendFixture fixture;
		auto* const game = fixture.GetGame();
		bool processing = false, destroyed_during_processing = false;
		size_t destructions = 0;
		class Observer final : public Wrappable {
		public:
			std::function< void() > on_destroy;
			Value* const Wrap( GSE_CALLABLE ) override { return nullptr; }
			void NotifyDependencyDestruction( const Wrappable* ) override { on_destroy(); }
		} observer;
		observer.on_destroy = [ & ]() { ++destructions; destroyed_during_processing |= processing; };
		class ProgressModule final : public map::module::Module {
		public:
			explicit ProgressModule( map::Map* map ) : Module( map ) {}
			std::function< void() > enqueue;
			void GenerateTile( const map::tile::Tile*, map::tile::TileState*, map::MapState* ) override { enqueue(); }
		} module( game->GetMap() );
		const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* const map = game->GetMap();
			map->Depend( &observer );
			util::FinallyGuard detach( [ & ]() { if ( !destructions ) { map->Undepend( &observer ); } } );
			return fixture.WithTransferableFrontendQueue( [ & ]() -> std::string {
				common::mt_id_t reset = 0, progress = 0, ping = 0;
				module.enqueue = [ & ]() {
					reset = game->MT_Reset( fixture.FixtureWorldTicket() );
					progress = game->MT_GetFrontendRequests( fixture.FixtureWorldTicket() );
					ping = game->MT_Ping();
				};
				processing = true;
				fixture.ProcessMapProgress( module );
				processing = false;
				GT_ASSERT( !destroyed_during_processing && !destructions && fixture.HasMap(), "progress pump destroyed the executing Map" );
				GT_ASSERT( fixture.IsInitializationCanceled(), "progress pump cleared cancellation before Map returned" );
				GT_ASSERT( game->MT_GetResponse( reset ).result == R_NONE, "reset completed inside Map progress" );
				const auto progress_response = game->MT_GetResponse( progress );
				util::FinallyGuard release_progress( [ & ]() { game->MT_DestroyResponse( progress_response ); } );
				GT_ASSERT( progress_response.result == R_ABORTED, "retired world published loader progress" );
				GT_ASSERT( game->MT_GetResponse( progress ).result == R_NONE, "frontend response was delivered twice" );
				GT_ASSERT( game->MT_GetResponse( ping ).result == R_SUCCESS, "progress pump did not answer ping" );
				fixture.IterateWorldRestore();
				GT_ASSERT( destructions == 1 && !destroyed_during_processing && !fixture.HasMap(), "ordinary Iterate did not retire Map after processing" );
				GT_ASSERT( !fixture.IsInitializationCanceled(), "ordinary reset did not clear cancellation" );
				GT_ASSERT( game->MT_GetResponse( reset ).result == R_SUCCESS, "ordinary Iterate did not complete queued reset" );
				fixture.IterateWorldRestore();
				GT_ASSERT( destructions == 1 && game->MT_GetResponse( reset ).result == R_NONE, "reset ran more than once" );
				GT_OK();
			} );
		} );
		GT_ASSERT( error.empty(), "loader progress check: " + error );
		GT_OK();
	} );
	task->AddTest( "loader progress leaves every non-progress operation queued and cancellable", GT() {
		using namespace game::backend;
		BackendFixture fixture;
		auto* const game = fixture.GetGame();
		const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			return fixture.WithTransferableFrontendQueue( [ & ]() -> std::string {
				const std::vector< op_t > blocked{
					OP_NONE, OP_INIT, OP_GET_MAP_DATA, OP_RESET, OP_SAVE_MAP, OP_SEND_BACKEND_REQUESTS, OP_ADD_EVENT,
#ifdef DEBUG
					OP_SAVE_DUMP, OP_LOAD_DUMP,
#endif
				};
				std::vector< common::mt_id_t > ids;
				for ( const auto op : blocked ) {
					MT_Request request{};
					request.op = op;
					if ( op == OP_SAVE_MAP ) { NEW( request.data.save_map.path, std::string, "must-not-be-written.map" ); }
					if ( op == OP_SEND_BACKEND_REQUESTS ) { NEW( request.data.send_backend_requests.requests, std::vector< game::BackendRequest > ); }
					if ( op == OP_ADD_EVENT ) { NEW( request.data.add_event.serialized_event, std::string, "must-not-be-executed" ); }
					ids.push_back( game->MT_CreateRequest( request ) );
				}
				const auto progress = game->MT_GetFrontendRequests( fixture.FixtureWorldTicket() );
				const auto ping = game->MT_Ping();
				game->SetLoaderText( "Still processing" );
				for ( const auto id : ids ) {
					GT_ASSERT( game->MT_GetResponse( id ).result == R_NONE, "progress pump claimed a non-progress operation" );
					game->MT_Cancel( id );
				}
				const auto response = game->MT_GetResponse( progress );
				util::FinallyGuard release( [ & ]() { game->MT_DestroyResponse( response ); } );
				GT_ASSERT( response.result == R_SUCCESS && response.data.get_frontend_requests.requests, "blocked operations starved frontend polling" );
				GT_ASSERT( game->MT_GetResponse( ping ).result == R_SUCCESS, "blocked operations starved ping" );
				fixture.IterateWorldRestore();
				GT_ASSERT( fixture.HasMap(), "canceled reset was dispatched by normal Iterate" );
				for ( const auto id : ids ) { GT_ASSERT( game->MT_GetResponse( id ).result == R_NONE, "canceled operation published a response" ); }
				GT_OK();
			} );
		} );
		GT_ASSERT( error.empty(), "operation filtering check: " + error );
		GT_OK();
	} );
	task->AddTest( "filtered MT extraction preserves payload ownership and unrestricted iteration", GT() {
		struct Payload {
			explicit Payload( size_t& destroyed ) : destroyed( destroyed ) {}
			~Payload() { ++destroyed; }
			size_t& destroyed;
		};
		struct Request { bool allowed; Payload* payload; };
		struct Response { bool ready; Payload* payload; };
		size_t request_destructions = 0, response_destructions = 0, executions = 0;
		class CountingModule final : public common::MTModule< Request, Response > {
		public:
			const std::string GetClassName() const override { return "CountingModule"; }
			CountingModule( size_t& executions, size_t& response_destructions )
				: executions( executions ), response_destructions( response_destructions ) {}
			~CountingModule() override { Stop(); }
			void Pump() { ProcessRequests( []( const Request& request ) noexcept { return request.allowed; } ); }
		protected:
			const Response ProcessRequest( const Request&, MT_CANCELABLE ) override {
				++executions;
				return { true, new Payload( response_destructions ) };
			}
			void DestroyRequest( const Request& request ) override { delete request.payload; }
			void DestroyResponse( const Response& response ) override { delete response.payload; }
		private:
			size_t& executions;
			size_t& response_destructions;
		} module( executions, response_destructions );
		const auto blocked = module.MT_CreateRequest( { false, new Payload( request_destructions ) } );
		const auto retrieved = module.MT_CreateRequest( { true, new Payload( request_destructions ) } );
		module.MT_CreateRequest( { true, new Payload( request_destructions ) } );
		module.Pump();
		GT_ASSERT( executions == 2 && request_destructions == 0 && response_destructions == 0, "filtered dispatch changed payload ownership" );
		module.MT_Cancel( blocked );
		GT_ASSERT( request_destructions == 1 && !module.MT_GetResponse( blocked ).ready, "skipped request was not cancellable before execution" );
		const auto response = module.MT_GetResponse( retrieved );
		GT_ASSERT( response.ready && request_destructions == 2, "response retrieval did not release exactly one request" );
		module.MT_DestroyResponse( response );
		GT_ASSERT( response_destructions == 1 && !module.MT_GetResponse( retrieved ).ready, "response ownership was retained after delivery" );
		module.Stop();
		GT_ASSERT( request_destructions == 3 && response_destructions == 2, "Stop did not release the unclaimed response exactly once" );
		module.Stop();
		const auto normal = module.MT_CreateRequest( { false, new Payload( request_destructions ) } );
		module.Iterate();
		const auto normal_response = module.MT_GetResponse( normal );
		GT_ASSERT( executions == 3 && normal_response.ready && request_destructions == 4, "normal Iterate inherited the progress filter" );
		module.MT_DestroyResponse( normal_response );
		GT_ASSERT( response_destructions == 3, "normal response cleanup changed" );
		GT_OK();
	} );
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

static void AddWorldAdmissionTests( task::gsetests::GSETests* task ) {
	for ( int scenario = 0; scenario < 5; ++scenario ) {
		const char* names[] = { "same-thread pending and success", "request during script stage", "shutdown before processing", "shutdown after processing", "stale cutoff no-op" };
		task->AddTest( std::string( "public reset polling " ) + names[ scenario ], GT( scenario ) {
			size_t destroyed = 0;
			const auto error = BackendFixture::CheckPolledReset( scenario, destroyed );
			GT_ASSERT( error.empty(), "reset polling: " + error );
			GT_ASSERT( destroyed == 1, "pending callback owner was not destroyed exactly once after drain/shutdown" );
			GT_OK();
		} );
	}
	using namespace game::backend;
	for ( const bool client : { false, true } ) {
		task->AddTest( "startup coordinator captured configure batch client " + std::to_string( client ), GT( client ) {
			BackendFixture fixture; return fixture.CheckConfigureCheckpoint( client );
		} );
	}
	for ( const bool queued : { false, true } ) {
		for ( const bool cancel : { false, true } ) {
			if ( cancel && !queued ) { continue; }
			task->AddTest( "startup coordinator complete packet batch queued " + std::to_string( queued ) + " cancellation " + std::to_string( cancel ), GT( queued, cancel ) {
				BackendFixture fixture; return fixture.CheckStartupDownloadBatch( queued, cancel );
			} );
		}
	}
	for ( const bool restored : { false, true } ) {
		for ( const bool cancel : { false, true } ) {
			task->AddTest( "startup coordinator complete Start " + std::to_string( restored ) + " cancellation " + std::to_string( cancel ), GT( restored, cancel ) {
				BackendFixture fixture; return fixture.CheckStartupFinish( restored, cancel );
			} );
		}
	}
	for ( const auto& scenario : std::vector< std::pair< int, std::string > >{
		{ 0, "terminal admission then inline initialize configure" }, { 1, "Pending cancellation resets without owner drain" },
		{ 2, "Running cancellation parks reset until owner exit" }, { 3, "queued owner uses identical startup order" },
		{ 4, "early owner exception retains ticket error" },
		{ 5, "stale Pending finalizer preserves replacement transition" },
	} ) {
		const auto mode = scenario.first;
		task->AddTest( "startup coordinator " + scenario.second, GT( mode ) { BackendFixture fixture; return fixture.CheckStartupCoordinator( mode ); } );
	}
	for ( const auto& scenario : std::vector< std::pair< int, std::string > >{
		{ 0, "reset before response envelope" }, { 1, "reset with aliased response envelope" }, { 2, "failure with aliased response envelope" },
		{ 3, "frontend transfer survives backend reset" }, { 4, "completed response cancellation owns transferred assets" },
		{ 5, "reset with partially populated envelope" },
	} ) {
		const int mode = scenario.first;
		task->AddTest( "startup payload ownership " + scenario.second, GT( mode ) { BackendFixture fixture; return fixture.CheckStartupPayloadOwnership( mode ); } );
	}
	task->AddTest( "initialization failure before map and managers retains original error", GT() {
		BackendFixture fixture; return fixture.CheckInitializationFailure( false, 0 );
	} );
	task->AddTest( "initialization failure releases unpublished map presentation and retains loader hide", GT() {
		BackendFixture fixture; return fixture.CheckInitializationFailure( true, 0 );
	} );
	task->AddTest( "initialization failure survives a throwing script error hook", GT() {
		BackendFixture fixture; return fixture.CheckInitializationFailure( true, 1 );
	} );
	task->AddTest( "initialization failure before map survives a throwing native error hook", GT() {
		BackendFixture fixture; return fixture.CheckInitializationFailure( false, 2 );
	} );
	task->AddTest( "initialization failure cannot revive a ticket retired by its error hook", GT() {
		BackendFixture fixture; return fixture.CheckInitializationFailure( true, 1, true );
	} );
	task->AddTest( "initialization failure transfers retained map ownership through repeated reset", GT() {
		BackendFixture fixture; return fixture.CheckFailedOldMapOwnership( false );
	} );
	task->AddTest( "initialization failure transfers retained map ownership through stop", GT() {
		BackendFixture fixture; return fixture.CheckFailedOldMapOwnership( true );
	} );
	task->AddTest( "world admission frontend quit retains its newly enqueued exit reset", GT() { return BackendFixture::CheckFrontendQuitAdmission(); } );
	const auto prepare = []( BackendFixture& fixture, gc::Space* space ) {
		fixture.EnableTurnPlayers();
		fixture.ResetRequest();
		auto* game = fixture.GetGame();
		auto* state = game->GetState();
		state->SetGame( game );
		state->SetConnection( new RulesetClient( space, &state->m_settings.local ) );
	};
	task->AddTest( "world admission cancels queued init without retaining request or changing binding", GT( prepare ) {
		BackendFixture fixture;
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			prepare( fixture, gc_space );
			auto* game = fixture.GetGame();
			const auto binding = game->GetState()->CaptureGameSession();
			const auto pending = game->MT_Init( game->GetState() );
			GT_ASSERT( pending.request && pending.ticket && fixture.AdmissionCount() == 1, "init did not publish an owned admission" );
			game->MT_Cancel( pending.request );
			fixture.ProcessRequests();
			GT_ASSERT( fixture.AdmissionCount() == 0 && game->MT_GetResponse( pending.request ).result == R_NONE, "queued cancellation retained an admission or response" );
			GT_ASSERT( game->GetState()->CaptureGameSession() == binding && !game->GetUM(), "queued cancellation initialized or rebound the world" );
			GT_OK();
		} );
	} );
	task->AddTest( "world admission cancellation retires an already completed init response", GT( prepare ) {
		BackendFixture fixture;
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			prepare( fixture, gc_space );
			auto* game = fixture.GetGame();
			const auto pending = game->MT_Init( game->GetState() );
			fixture.ProcessRequests();
			GT_ASSERT( fixture.HasActiveAdmission( pending.ticket ), "real client init did not publish its accepted binding" );
			game->MT_Cancel( pending.request );
			GT_ASSERT( game->MT_GetResponse( pending.request ).result == R_NONE && fixture.AdmissionCount() == 0, "completed cancellation left MT response or pending ownership" );
			GT_ASSERT( !fixture.HasActiveAdmission( pending.ticket ), "completed cancellation left its session active" );
			const auto map = game->MT_GetMapData( pending.ticket );
			const auto reset = game->MT_Reset( 0, pending.ticket );
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( map ).result == R_ABORTED, "canceled initialization accepted map polling" );
			GT_ASSERT( game->MT_GetResponse( reset ).result == R_SUCCESS && !game->GetState()->GetGame(), "pending handle could not retire its admitted world" );
			GT_OK();
		} );
	} );
	task->AddTest( "world admission validates canceled init inside an already extracted request batch", GT( prepare ) {
		BackendFixture fixture;
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			prepare( fixture, gc_space );
			auto* game = fixture.GetGame();
			const auto pending = game->MT_Init( game->GetState() );
			const auto reset = game->MT_Reset( 0, pending.ticket );
			// MTModule extracts both requests before executing the first one.
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( pending.request ).result == R_ABORTED && !game->GetUM(), "canceled extracted init ran callbacks or remained pending" );
			GT_ASSERT( game->MT_GetResponse( reset ).result == R_SUCCESS && fixture.AdmissionCount() == 0, "targeted reset failed to finish its request bookkeeping" );
			GT_OK();
		} );
	} );
	task->AddTest( "world admission rejects same-address State rebinding before init teardown", GT( prepare ) {
		BackendFixture fixture;
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			prepare( fixture, gc_space );
			auto* game = fixture.GetGame();
			const auto pending = game->MT_Init( game->GetState() );
			fixture.RebindSession();
			const auto replacement = game->GetState()->CaptureGameSession();
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( pending.request ).result == R_ABORTED, "old configured binding was admitted into replacement State session" );
			GT_ASSERT( game->GetState()->CaptureGameSession() == replacement && game->GetState()->IsGameSessionActive( replacement ) && !game->GetUM(), "stale init touched replacement binding or managers" );
			GT_OK();
		} );
	} );
	task->AddTest( "world admission reports owner initialization failure on its accepted map ticket", GT( prepare ) {
		BackendFixture fixture;
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			prepare( fixture, gc_space );
			auto* game = fixture.GetGame();
			game->On( GSE_CALL, "initialize", NATIVE_CALL() { throw std::runtime_error( "intentional admission initialization error" ); } ) );
			const auto pending = game->MT_Init( game->GetState() );
			fixture.ProcessRequests();
			const auto admitted = game->MT_GetResponse( pending.request );
			GT_ASSERT( admitted.result == R_SUCCESS && admitted.world_ticket == pending.ticket, "initialization did not complete terminal admission before owner work" );
			fixture.WithoutDisconnectIO( game->GetState()->GetConnection(), [ & ]() { game->Iterate(); } );
			const auto poll = game->MT_GetMapData( pending.ticket );
			fixture.ProcessRequests();
			const auto response = game->MT_GetResponse( poll );
			util::FinallyGuard destroy( [ & ]() { game->MT_DestroyResponse( response ); } );
			GT_ASSERT( response.result == R_ERROR && response.world_ticket == pending.ticket && response.data.error.error_text &&
				*response.data.error.error_text == "intentional admission initialization error", "initialization failed without a terminal owned error" );
			GT_ASSERT( fixture.AdmissionCount() == 0 && !fixture.HasActiveAdmission( pending.ticket ), "failed init kept request ownership or an executable session" );
			const auto reset = game->MT_Reset( 0, pending.ticket );
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( reset ).result == R_SUCCESS, "failed init could not be reset by its pending handle" );
			GT_OK();
		} );
	} );
	task->AddTest( "world admission valid preparing ticket cannot save absent map or finish absent animations", GT( prepare ) {
		BackendFixture fixture;
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			prepare( fixture, gc_space );
			auto* game = fixture.GetGame();
			const auto pending = game->MT_Init( game->GetState() );
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( pending.request ).result == R_SUCCESS, "initial admission failed" );
			const auto save = game->MT_SaveMap( pending.ticket, "must-not-exist.map" );
			const auto animation = game->MT_SendBackendRequests( pending.ticket, {} );
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( save ).result == R_ABORTED && game->MT_GetResponse( animation ).result == R_ABORTED, "ticket was mistaken for map/manager readiness" );
			GT_OK();
		} );
	} );
	task->AddTest( "world admission reload retains accepted session after init response retirement", GT( prepare ) {
		BackendFixture fixture;
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			prepare( fixture, gc_space );
			auto* game = fixture.GetGame();
			const auto first = game->MT_Init( game->GetState() );
			fixture.ProcessRequests();
			const auto first_response = game->MT_GetResponse( first.request );
			GT_ASSERT( first_response.result == R_SUCCESS && first_response.world_ticket == first.ticket && fixture.AdmissionCount() == 0, "accepted init did not return its own ticket and release pending record" );
			const auto old_binding = game->GetState()->CaptureGameSession();
			const auto reload = game->MT_LoadMap( first.ticket, "synthetic-reload.map" );
			fixture.ProcessRequests();
			const auto response = game->MT_GetResponse( reload.request );
			GT_ASSERT( response.result == R_SUCCESS && response.world_ticket == reload.ticket && reload.ticket > first.ticket, "reload admission failed or reused a ticket" );
			GT_ASSERT( game->GetState()->CaptureGameSession() != old_binding && fixture.HasActiveAdmission( reload.ticket ), "reload did not retain its exact post-reset binding" );
			GT_ASSERT( game->GetState()->m_settings.global.map.filename == "synthetic-reload.map", "reload admission lost its owned path" );
			const auto stale_reset = game->MT_Reset( first.ticket );
			const auto stale_poll = game->MT_GetMapData( first.ticket );
			const auto fresh_poll = game->MT_GetMapData( reload.ticket );
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( stale_reset ).result == R_SUCCESS && fixture.HasActiveAdmission( reload.ticket ), "old reset retired fresh reload" );
			GT_ASSERT( game->MT_GetResponse( stale_poll ).result == R_ABORTED && game->MT_GetResponse( fresh_poll ).result == R_PENDING, "map polling crossed world tickets" );
			bool rejected = false;
			try { game->MT_LoadMap( first.ticket, "old.map" ); } catch ( const std::runtime_error& ) { rejected = true; }
			GT_ASSERT( rejected && game->GetState()->m_settings.global.map.filename == "synthetic-reload.map", "stale reload changed replacement settings" );
			GT_OK();
		} );
	} );
	task->AddTest( "world admission aborts old operations extracted behind reset with terminal responses", GT( prepare ) {
		BackendFixture fixture;
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			prepare( fixture, gc_space );
			auto* game = fixture.GetGame();
			const auto pending = game->MT_Init( game->GetState() );
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( pending.request ).result == R_SUCCESS, "initial admission failed" );
			const auto reset = game->MT_Reset( pending.ticket );
			const std::vector< common::mt_id_t > old{
				game->MT_GetMapData( pending.ticket ), game->MT_SaveMap( pending.ticket, "must-not-be-written.map" ),
				game->MT_GetFrontendRequests( pending.ticket ), game->MT_SendBackendRequests( pending.ticket, {} ),
			};
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( reset ).result == R_SUCCESS, "reset did not finish" );
			for ( const auto id : old ) {
				const auto response = game->MT_GetResponse( id );
				util::FinallyGuard destroy( [ & ]() { game->MT_DestroyResponse( response ); } );
				GT_ASSERT( response.result == R_ABORTED && response.world_ticket == pending.ticket, "extracted old operation ran or lacked terminal identity" );
				GT_ASSERT( game->MT_GetResponse( id ).result == R_NONE, "aborted response was not retired" );
			}
			GT_OK();
		} );
	} );
	task->AddTest( "world admission controller cutoff cancels prior init and ignores replacement binding", GT( prepare ) {
		BackendFixture fixture;
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			prepare( fixture, gc_space );
			auto* game = fixture.GetGame();
			const auto pending = game->MT_Init( game->GetState() );
			const auto reset = game->MT_ResetController();
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( pending.request ).result == R_ABORTED && game->MT_GetResponse( reset ).result == R_SUCCESS, "controller cutoff admitted an older queued init" );
			game->GetState()->SetGame( game );
			const auto stale_reset = game->MT_ResetController();
			fixture.RebindSession();
			const auto replacement = game->GetState()->CaptureGameSession();
			const auto fresh = game->MT_Init( game->GetState() );
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( stale_reset ).result == R_SUCCESS && game->MT_GetResponse( fresh.request ).result == R_SUCCESS, "stale controller reset prevented replacement admission" );
			GT_ASSERT( game->GetState()->CaptureGameSession() == replacement && fixture.HasActiveAdmission( fresh.ticket ), "stale controller reset retired replacement binding" );
			GT_OK();
		} );
	} );
	task->AddTest( "world admission pending State is a collector root until request cancellation", GT( prepare ) {
		BackendFixture fixture;
		pending_initialization_t pending;
		const auto setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			prepare( fixture, gc_space );
			pending = fixture.GetGame()->MT_Init( fixture.GetGame()->GetState() );
			GT_OK();
		} );
		GT_ASSERT( setup.empty(), "admission setup: " + setup );
		GT_ASSERT( fixture.CollectAdmissionRoot( pending.ticket ), "pending admission omitted the intended State from actual Game reachability" );
		fixture.GetGame()->MT_Cancel( pending.request );
		GT_ASSERT( fixture.AdmissionCount() == 0, "canceled admission retained pending State root" );
		GT_OK();
	} );
	task->AddTest( "world admission controller reserves cancellation before drain and submits its original cutoff once", GT( prepare ) {
		BackendFixture fixture;
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			prepare( fixture, gc_space );
			auto* game = fixture.GetGame();
			const auto old = game->MT_Init( game->GetState() );
			const auto barrier = game->MT_BeginResetController();
			GT_ASSERT( fixture.AdmissionCanceled( old.ticket ), "reset reservation did not revoke pending admission before drain" );
			game->MT_Cancel( old.request ); // actual frontend-style drain, before reset submission
			GT_ASSERT( fixture.AdmissionCount() == 1, "drain lost the unsubmitted controller root" );
			fixture.RebindSession();
			const auto fresh = game->MT_Init( game->GetState() );
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( fresh.request ).result == R_SUCCESS, "fresh admission after cutoff failed" );
			const auto request = game->MT_SubmitResetController( barrier );
			GT_ASSERT( game->MT_SubmitResetController( barrier ) == request, "repeated submit enqueued another reset" );
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( request ).result == R_SUCCESS && fixture.HasActiveAdmission( fresh.ticket ), "late controller submission resampled and reset a newer world" );
			bool rejected = false;
			try { game->MT_SubmitResetController( barrier ); } catch ( const std::runtime_error& ) { rejected = true; }
			GT_ASSERT( rejected && fixture.AdmissionCount() == 0, "consumed barrier was reused or retained" );
			const auto abandoned = game->MT_BeginResetController();
			game->MT_AbandonResetController( abandoned );
			game->MT_AbandonResetController( abandoned );
			GT_ASSERT( fixture.AdmissionCount() == 0 && !fixture.HasActiveAdmission( fresh.ticket ), "abandonment leaked a barrier or revived retired world" );
			const auto completed_barrier = game->MT_BeginResetController();
			const auto completed_reset = game->MT_SubmitResetController( completed_barrier );
			fixture.ProcessRequests();
			game->MT_AbandonResetController( completed_barrier );
			GT_ASSERT( fixture.AdmissionCount() == 0 && game->MT_GetResponse( completed_reset ).result == R_NONE, "abandoning a completed barrier retained its MT response" );
			GT_OK();
		} );
	} );
	task->AddTest( "world admission concurrent reservation and reset have one scalar cutoff order", GT( prepare ) {
		BackendFixture fixture;
		const auto setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string { prepare( fixture, gc_space ); GT_OK(); } );
		GT_ASSERT( setup.empty(), "admission setup: " + setup );
		auto* game = fixture.GetGame();
		pending_initialization_t pending;
		world_ticket_t barrier = 0;
		std::mutex mutex;
		std::condition_variable changed;
		size_t ready = 0;
		bool start = false;
		std::exception_ptr init_error, reset_error;
		const auto rendezvous = [ & ]() {
			std::unique_lock lock( mutex ); ++ready; changed.notify_all(); changed.wait( lock, [ & ]() { return start; } );
		};
		std::thread initializer( [ & ]() { rendezvous(); try { pending = game->MT_Init( game->GetState() ); } catch ( ... ) { init_error = std::current_exception(); } } );
		std::thread resetter( [ & ]() { rendezvous(); try { barrier = game->MT_BeginResetController(); } catch ( ... ) { reset_error = std::current_exception(); } } );
		{
			std::unique_lock lock( mutex ); changed.wait( lock, [ & ]() { return ready == 2; } ); start = true; changed.notify_all();
		}
		initializer.join(); resetter.join();
		if ( init_error ) { std::rethrow_exception( init_error ); }
		if ( reset_error ) { std::rethrow_exception( reset_error ); }
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			const auto reset = game->MT_SubmitResetController( barrier );
			fixture.ProcessRequests();
			const auto response = game->MT_GetResponse( pending.request );
			const bool before_cutoff = pending.ticket < barrier;
			GT_ASSERT( response.result == ( before_cutoff ? R_ABORTED : R_SUCCESS ), "ticket reservation and request admission crossed reset cutoff order" );
			GT_ASSERT( game->MT_GetResponse( reset ).result == R_SUCCESS && fixture.HasActiveAdmission( pending.ticket ) == !before_cutoff, "controller reset applied outside its reserved cutoff" );
			GT_ASSERT( fixture.AdmissionCount() == 0, "concurrent requests retained pending bookkeeping" );
			GT_OK();
		} );
	} );
	task->AddTest( "world admission failed-map errors are owned by their ticket and retired by reset", GT() {
		BackendFixture fixture;
		return fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.GetGame();
			const auto ticket = fixture.FixtureWorldTicket();
			fixture.CancelWorldInitialization(); // real InitFailed with an asset-free map
			const auto own = game->MT_GetMapData( ticket );
			const auto unrelated = game->MT_GetMapData( ticket + 1 );
			fixture.ProcessRequests();
			const auto error = game->MT_GetResponse( own );
			util::FinallyGuard destroy( [ & ]() { game->MT_DestroyResponse( error ); } );
			GT_ASSERT( error.result == R_ERROR && error.data.error.error_text && *error.data.error.error_text == "Operation aborted", "own failure was lost or mislabeled" );
			GT_ASSERT( game->MT_GetResponse( unrelated ).result == R_ABORTED, "another ticket observed failed-world error" );
			const auto reset = game->MT_Reset( ticket );
			const auto late = game->MT_GetMapData( ticket );
			fixture.ProcessRequests();
			GT_ASSERT( game->MT_GetResponse( reset ).result == R_SUCCESS && game->MT_GetResponse( late ).result == R_ABORTED, "reset retained its failed admission/error" );
			GT_OK();
		} );
	} );
	task->AddTest( "world admission cancels an in-flight init before owner callbacks drain", GT( prepare ) {
		class BlockingClient final : public RulesetClient {
		public:
			using RulesetClient::RulesetClient;
			std::mutex mutex;
			std::condition_variable changed;
			bool entered = false, released = false;
			void ResetHandlers() override {
				RulesetClient::ResetHandlers();
				std::unique_lock lock( mutex );
				entered = true; changed.notify_all();
				changed.wait( lock, [ & ]() { return released; } );
			}
		};
		BackendFixture fixture;
		BlockingClient* client = nullptr;
		pending_initialization_t pending;
		size_t callbacks = 0;
		const auto setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			prepare( fixture, gc_space );
			auto* game = fixture.GetGame();
			game->GetState()->DetachConnection();
			client = new BlockingClient( gc_space, &game->GetState()->m_settings.local );
			game->GetState()->SetConnection( client );
			game->On( GSE_CALL, "initialize", NATIVE_CALL( &callbacks ) { ++callbacks; return VALUE( value::Undefined ); } ) );
			// A reinitialization still performs native teardown inside OP_INIT;
			// only its owner hooks moved behind the terminal admission response.
			fixture.RequireInitializationReset();
			pending = game->MT_Init( game->GetState() );
			GT_OK();
		} );
		GT_ASSERT( setup.empty(), "admission setup: " + setup );
		std::exception_ptr producer_error;
		std::thread producer( [ & ]() { try { fixture.ProcessRequests(); } catch ( ... ) { producer_error = std::current_exception(); } } );
		bool entered;
		{
			std::unique_lock lock( client->mutex );
			entered = client->changed.wait_for( lock, std::chrono::seconds( 10 ), [ & ]() { return client->entered; } );
		}
		std::thread canceler;
		bool canceled = false;
		if ( entered ) {
			canceler = std::thread( [ & ]() { fixture.GetGame()->MT_Cancel( pending.request ); } );
			const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
			while ( !( canceled = fixture.AdmissionCanceled( pending.ticket ) ) && std::chrono::steady_clock::now() < limit ) { std::this_thread::yield(); }
		}
		{
			std::lock_guard guard( client->mutex ); client->released = true; client->changed.notify_all();
		}
		producer.join();
		if ( canceler.joinable() ) { canceler.join(); }
		if ( producer_error ) { std::rethrow_exception( producer_error ); }
		GT_ASSERT( entered && canceled, "actual in-flight init/cancel handoff did not occur" );
		GT_ASSERT( fixture.GetGame()->MT_GetResponse( pending.request ).result == R_NONE && fixture.AdmissionCount() == 0, "in-flight cancellation left a response or request owner" );
		fixture.ProcessAccumulations();
		GT_ASSERT( callbacks == 0 && !fixture.GetGame()->GetUM(), "canceled init's queued owner work ran" );
		GT_OK();
	} );
}

// Session tests use the real packet dispatcher and event queue, without sockets.
class SessionServer : public game::backend::connection::Server {
public:
	using Server::Server;
	void Receive( const types::Packet& packet ) {
		network::Event event;
		event.type = network::Event::ET_PACKET;
		event.cid = 23;
		event.data.packet_data = packet.Serialize().ToString();
		ProcessEvent( event );
	}
};

static void AddSessionCallbackTests( task::gsetests::GSETests* task ) {
	task->AddTest( "test session callbacks keep their State and supplied owner rooted until cancellation drains", GT() {
		using namespace game::backend;
		std::atomic< size_t > state_destroyed{ 0 }, owner_destroyed{ 0 };
		class CountedState final : public State {
		public:
			CountedState( gc::Space* space, context::Context* ctx, std::atomic< size_t >& count ) : State( space, ctx, nullptr ), count( count ) {}
			~CountedState() override { ++count; }
		private:
			std::atomic< size_t >& count;
		};
		class CountedOwner final : public gc::Object {
		public:
			CountedOwner( gc::Space* space, std::atomic< size_t >& count ) : gc::Object( space ), count( count ) {}
			~CountedOwner() override { ++count; }
		private:
			std::atomic< size_t >& count;
		};
		Game game;
		GSE runtime;
		auto* space = runtime.GetGCSpace();
		CountedState* state = nullptr;
		CountedOwner* owner = nullptr;
		size_t calls = 0, cleanups = 0;
		space->Accumulate( &runtime, [ & ]() {
			state = new CountedState( space, runtime.CreateGlobalContext(), state_destroyed );
			owner = new CountedOwner( space, owner_destroyed );
			state->SetGame( &game );
			runtime.AddRootObject( state ); runtime.AddRootObject( owner );
		} );
		std::thread producer( [ & ]() { state->WithGame( owner, [ & ]( Game* const bound, GSE_CALLABLE ) { if ( bound == &game ) { ++calls; } }, [ & ]() { ++cleanups; } ); } );
		producer.join();
		space->Accumulate( &runtime, [ & ]() { runtime.RemoveRootObject( state ); runtime.RemoveRootObject( owner ); } );
		g_engine->GetGC()->Iterate();
		GT_ASSERT( state_destroyed == 0 && owner_destroyed == 0, "queued callback lost State or owner before retirement" );
		space->Accumulate( &runtime, [ & ]() { state->Reset(); } );
		g_engine->GetGC()->Iterate();
		GT_ASSERT( state_destroyed == 0 && owner_destroyed == 0, "canceled callback lost captures before its queue entry was retired" );
		space->ProcessAccumulations();
		g_engine->GetGC()->Iterate();
		GT_ASSERT( calls == 0 && cleanups == 1, "canceled session executed or cleanup count changed" );
		GT_ASSERT( state_destroyed == 1 && owner_destroyed == 1, "retired callback retained State or owner" );
		GT_OK();
	} );
	task->AddTest( "test session callbacks preserve immediate nesting exceptions cleanup and fresh binding order", GT() {
		BackendFixture fixture;
		auto* state = fixture.GetGame()->GetState();
		std::vector< int > order;
		const auto immediate = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* other_state = new game::backend::State( gc_space, ctx, nullptr );
			other_state->SetGame( fixture.GetGame() );
			const auto invalid = [ & ]( game::backend::Game* const, GSE_CALLABLE ) { order.push_back( -1 ); };
			state->WithGameSession( {}, state, invalid );
			state->WithGameSession( other_state->CaptureGameSession(), state, invalid );
			state->WithGame( state, [ & ]( game::backend::Game* const bound, GSE_CALLABLE ) {
				if ( bound != fixture.GetGame() ) { throw std::runtime_error( "wrong game binding" ); }
				order.push_back( 1 );
				state->WithGame( state, [ & ]( game::backend::Game* const, GSE_CALLABLE ) { order.push_back( 2 ); } );
				order.push_back( 3 );
			} );
			GT_OK();
		} );
		GT_ASSERT( immediate.empty() && order == std::vector< int >( { 1, 2, 3 } ), "nested session order changed: " + immediate );
		size_t attempts = 0, cleanups = 0;
		bool caught = false;
		try { state->WithGame( state, []( game::backend::Game* const, GSE_CALLABLE ) { throw 41; } ); }
		catch ( int value ) { caught = value == 41; }
		GT_ASSERT( caught && !fixture.IsAccumulating(), "immediate exception lost identity or accumulation state" );
		std::thread producer( [ & ]() {
			state->WithGame( state, [ & ]( game::backend::Game* const, GSE_CALLABLE ) { if ( ++attempts == 1 ) { throw 42; } }, [ & ]() { ++cleanups; } );
		} ); producer.join();
		caught = false;
		try { fixture.ProcessAccumulations(); } catch ( int value ) { caught = value == 42; }
		GT_ASSERT( caught && cleanups == 0, "queued exception changed existing queue cleanup semantics" );
		fixture.RebindSession();
		state->WithGame( state, [ & ]( game::backend::Game* const, GSE_CALLABLE ) { order.push_back( 4 ); } );
		fixture.ProcessAccumulations();
		GT_ASSERT( attempts == 1 && cleanups == 1 && order.back() == 4, "retired throwing callback ran again in replacement session" );
		GT_OK();
	} );
	task->AddTest( "test session reset retires queued Game messages and pending event responses", GT() {
		BackendFixture fixture;
		auto* game = fixture.GetGame();
		size_t messages = 0;
		std::thread producer( [ & ]() { game->Message( "old world" ); } ); producer.join();
		const auto reset = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			game->AddEventResponse( "reused-id", true, VALUE( value::Int,, 7 ) );
			GT_ASSERT( fixture.PendingResponses() == 1, "response setup failed" );
			fixture.ResetRequest();
			GT_ASSERT( fixture.PendingResponses() == 0, "reset kept a response for retired event IDs" );
			game->GetState()->SetGame( game );
			game->On( GSE_CALL, "message", NATIVE_CALL( &messages ) { ++messages; return VALUE( value::Undefined ); } ) );
			GT_OK();
		} );
		GT_ASSERT( reset.empty(), "reset/rebind setup failed: " + reset );
		fixture.ProcessAccumulations();
		GT_ASSERT( messages == 0, "old message reached replacement handlers" );
		game->Message( "fresh world" );
		GT_ASSERT( messages == 1, "fresh message was canceled" );
		GT_OK();
	} );
	task->AddTest( "test session initialize after reset binds before initialize and configure callbacks", GT() {
		BackendFixture fixture;
		fixture.EnableTurnPlayers();
		auto* game = fixture.GetGame();
		std::vector< std::string > order;
		std::thread producer( [ & ]() { game->Message( "old world" ); } ); producer.join();
		const auto result = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			fixture.ResetRequest();
			auto* state = game->GetState();
			GT_ASSERT( !state->GetGame(), "reset did not unbind the old game" );
			state->SetGame( game ); // production GLSMAC configures the binding before MT_Init
			auto* client = new RulesetClient( gc_space, &state->m_settings.local );
			state->SetConnection( client );
			game->On( GSE_CALL, "initialize", NATIVE_CALL( &order, state, game ) {
				if ( state->GetGame() != game || game->GetUM() ) { throw std::runtime_error( "initialize binding/order is wrong" ); }
				order.push_back( "initialize" ); return VALUE( value::Undefined );
			} ) );
			game->On( GSE_CALL, "configure", NATIVE_CALL( &order, state, game ) {
				if ( state->GetGame() != game || !game->GetUM() ) { throw std::runtime_error( "configure binding/order is wrong" ); }
				order.push_back( "configure" ); return VALUE( value::Undefined );
			} ) );
			game->On( GSE_CALL, "message", NATIVE_CALL( &order ) { order.push_back( "stale" ); return VALUE( value::Undefined ); } ) );
			const auto id = game->MT_Init( state );
			fixture.ProcessRequests();
			const auto response = game->MT_GetResponse( id.request );
			GT_ASSERT( response.result == game::backend::R_SUCCESS && fixture.IsPreparingMap(), "asset-free client initialization failed" );
			game->Iterate(); // explicit post-response startup owner checkpoint
			GT_OK();
		} );
		GT_ASSERT( result.empty(), "initialization ordering failed: " + result );
		fixture.ProcessAccumulations();
		GT_ASSERT( order == std::vector< std::string >( { "initialize", "configure" } ), "new initialization was canceled or old message reached it" );
		GT_OK();
	} );

	for ( const bool cancel : { false, true } ) {
		task->AddTest( std::string( "test session extracted event batch survives collection then " ) + ( cancel ? "retires" : "executes" ), GT( cancel ) {
			using namespace game::backend::event;
			std::atomic< size_t > destroyed{ 0 };
			class CountedEvent final : public Event {
			public:
				CountedEvent( game::backend::Game* game, GSE_CALLABLE, std::atomic< size_t >& count ) : Event( game, ES_LOCAL, 0, GSE_CALL, "batch", {}, "batch-id" ), count( count ) {}
				~CountedEvent() override { ++count; }
			private: std::atomic< size_t >& count;
			};
			BackendFixture fixture;
			CountedEvent* pending_event = nullptr;
			size_t applied = 0;
			const auto setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* noop = NATIVE_CALL() { return VALUE( value::Undefined ); } );
				auto* apply = NATIVE_CALL( &applied ) { ++applied; return VALUE( value::Undefined ); } );
				fixture.RegisterEvent( "batch", new EventHandler( gc_space, "batch", noop, nullptr, apply, noop ) );
				pending_event = new CountedEvent( fixture.GetGame(), GSE_CALL, destroyed );
				fixture.GetGame()->AddEvent( pending_event );
				GT_OK();
			} );
			GT_ASSERT( setup.empty(), "batch setup failed: " + setup );
			bool alive_during_handoff = false;
			const auto handed_off = fixture.CollectDuringEventHandoff( pending_event, [ & ]() { alive_during_handoff = destroyed == 0; } );
			GT_ASSERT( handed_off && alive_during_handoff, "event lost its root between extraction and queue registration" );
			GT_ASSERT( fixture.EventHandoffs() == 0, "temporary batch root outlived queue registration" );
			GT_ASSERT( fixture.PendingEvents().empty(), "batch was not extracted before queuing" );
			g_engine->GetGC()->Iterate();
			GT_ASSERT( destroyed == 0, "extracted event was collected before dispatch" );
			if ( cancel ) { fixture.RebindSession(); }
			fixture.ProcessAccumulations();
			g_engine->GetGC()->Iterate();
			GT_ASSERT( applied == ( cancel ? 0 : 1 ), "batch crossed a retired session or fresh delivery failed" );
			GT_ASSERT( destroyed == 1, "completed/canceled batch still roots its event" );
			const auto throwing = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				// Same extraction path on the owner: a throwing immediate callback
				// must release the handoff even though Space skips f_cleanup.
				fixture.GetGame()->ClearEvents();
				auto* noop = NATIVE_CALL() { return VALUE( value::Undefined ); } );
				auto* fail = NATIVE_CALL() { throw 73; return VALUE( value::Undefined ); } );
				fixture.RegisterEvent( "batch", new EventHandler( gc_space, "batch", noop, nullptr, fail, noop ) );
				fixture.GetGame()->AddEvent( new CountedEvent( fixture.GetGame(), GSE_CALL, destroyed ) );
				bool caught = false;
				try { fixture.ProcessEvents(); } catch ( int value ) { caught = value == 73; }
				GT_ASSERT( caught && fixture.EventHandoffs() == 0, "throwing immediate batch leaked its native handoff" );
				GT_OK();
			} );
			GT_ASSERT( throwing.empty(), "throwing batch check failed: " + throwing );
			g_engine->GetGC()->Iterate();
			GT_ASSERT( destroyed == 2, "throwing immediate batch retained its event" );
			const auto abandoned = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				pending_event = new CountedEvent( fixture.GetGame(), GSE_CALL, destroyed );
				fixture.GetGame()->AddEvent( pending_event );
				GT_OK();
			} );
			GT_ASSERT( abandoned.empty(), "abandoned batch setup failed: " + abandoned );
			bool caught = false;
			try { fixture.CollectDuringEventHandoff( pending_event, []() { throw 74; } ); }
			catch ( int value ) { caught = value == 74; }
			GT_ASSERT( caught && fixture.EventHandoffs() == 0, "abandoned extracted batch retained its handoff" );
			g_engine->GetGC()->Iterate();
			GT_ASSERT( destroyed == 3, "abandoned extracted batch retained its event" );
			GT_OK();
		} );
	}
	task->AddTest( "test session extracted event retains its identity across reset before dispatch", GT() {
		using namespace game::backend::event;
		BackendFixture fixture;
		auto* state = fixture.GetGame()->GetState();
		const auto old_session = state->CaptureGameSession();
		Event* old_event = nullptr;
		size_t applied = 0;
		const auto add = [ & ]( GSE_CALLABLE ) {
			auto* noop = NATIVE_CALL() { return VALUE( value::Undefined ); } );
			auto* apply = NATIVE_CALL( &applied ) { ++applied; return VALUE( value::Undefined ); } );
			fixture.RegisterEvent( "handoff", new EventHandler( gc_space, "handoff", noop, nullptr, apply, noop ) );
			auto* event = new Event( fixture.GetGame(), Event::ES_LOCAL, 0, GSE_CALL, "handoff", {}, "reused-id" );
			fixture.GetGame()->AddEvent( event );
			return event;
		};
		const auto setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string { old_event = add( GSE_CALL ); GT_OK(); } );
		GT_ASSERT( setup.empty(), "handoff setup failed: " + setup );
		std::string reset_error;
		bool consumed_fresh = true;
		const auto handed_off = fixture.CollectDuringEventHandoff( old_event, [ & ]() {
			reset_error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				fixture.ResetRequest();
				state->SetGame( fixture.GetGame() );
				add( GSE_CALL );
				consumed_fresh = fixture.ExtractWithSession( old_session );
				GT_OK();
			} );
			g_engine->GetGC()->Iterate(); // Extracted old batch is still a real root after reset.
		} );
		GT_ASSERT( handed_off && reset_error.empty(), "handoff/reset failed: " + reset_error );
		GT_ASSERT( !consumed_fresh && fixture.PendingEvents().size() == 1, "retired producer consumed the new-session queue" );
		GT_ASSERT( fixture.EventHandoffs() == 0, "dispatched handoff was not released" );
		fixture.ProcessAccumulations();
		GT_ASSERT( applied == 0, "old extracted batch sampled the replacement session at dispatch" );
		fixture.ProcessEvents(); // Normal unsplit production entry still processes the fresh batch.
		GT_ASSERT( applied == 1 && fixture.PendingEvents().empty(), "fresh event failed after stale dispatch cancellation" );
		GT_OK();
	} );

	for ( const bool server : { false, true } ) {
		task->AddTest( std::string( "test session " ) + ( server ? "Server" : "Client" ) + " ingress rejects retired packets and targets its bound world", GT( server ) {
			using namespace game::backend;
			BackendFixture fixture;
			fixture.EnableTurnPlayers();
			auto* state = fixture.GetGame()->GetState();
			RulesetClient* client = nullptr;
			SessionServer* host = nullptr;
			types::Packet packet( types::Packet::PT_GAME_EVENT );
			types::Packet response( types::Packet::PT_GAME_EVENT_RESPONSE );
			const auto setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				if ( server ) {
					host = new SessionServer( gc_space, &state->m_settings.local ); state->SetConnection( host );
					auto& slot = state->m_slots->GetSlot( 2 ); slot.SetPlayer( slot.GetPlayer(), 23, "" );
				} else { client = new RulesetClient( gc_space, &state->m_settings.local ); state->SetConnection( client ); }
				auto* event = new event::Event( fixture.GetGame(), event::Event::ES_LOCAL, 2, GSE_CALL, "bound-event", { { "tile", fixture.GetGame()->GetMap()->GetTile( 2, 2 )->Wrap( GSE_CALL ) } }, "wire-id" );
				types::Buffer bytes; bytes.WriteString( event->Serialize().ToString() ); packet.data.str = bytes.ToString();
				response.data.str = "wire-id"; response.data.boolean = true;
				types::Buffer result; Value::Serialize( &result, fixture.GetGame()->GetMap()->GetTile( 2, 2 )->Wrap( GSE_CALL ) ); response.data.str2 = result.ToString();
				GT_OK();
			} );
			GT_ASSERT( setup.empty(), "ingress setup failed: " + setup );
			const auto receive = [ & ]() { if ( server ) { host->Receive( packet ); } else { client->Receive( packet ); client->Receive( response ); } };
			std::thread old( receive ); old.join();
			fixture.RebindSession();
			fixture.ProcessAccumulations();
			GT_ASSERT( fixture.PendingEvents().empty() && fixture.PendingResponses() == 0, "retired packet reached replacement session" );
			std::thread fresh( receive ); fresh.join();
			fixture.ProcessAccumulations();
			GT_ASSERT( fixture.PendingEvents().size() == 1 && fixture.PendingResponses() == ( server ? 0 : 1 ), "fresh packet did not target fixture Game" );
			const auto check = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* data = VALUE_DATA( value::Object, fixture.PendingEvents().front()->GetData().at( "data" ) );
				GT_ASSERT( Tile::Unwrap( data->Get( "tile" ) ) == fixture.GetGame()->GetMap()->GetTile( 2, 2 ), "packet reference resolved in another world" );
				GT_OK();
			} );
			GT_ASSERT( check.empty(), "bound packet reference failed: " + check );
			GT_OK();
		} );
	}
	task->AddTest( "test session queued world completion cannot resume after reset and rebind", GT() {
		BackendFixture fixture;
		auto* state = fixture.GetGame()->GetState();
		RulesetClient* client = nullptr;
		types::Packet packet( types::Packet::PT_DOWNLOAD_NEXT_CHUNK_RESPONSE );
		packet.data.str = "invalid world header"; packet.udata.download.offset = 0; packet.udata.download.size = packet.data.str.size();
		const auto setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			state->m_settings.global.SelectRuleset( "smac" );
			state->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
			client = new RulesetClient( gc_space, &state->m_settings.local ); state->SetConnection( client );
			fixture.PrepareWorldDownload( client, packet.data.str.size() );
			GT_OK();
		} );
		GT_ASSERT( setup.empty(), "completion setup failed: " + setup );
		const auto retained_complete = client->m_on_download_complete;
		const auto retained_progress = client->m_on_download_progress;
		std::thread producer( [ & ]() { client->Receive( packet ); } ); producer.join();
		const auto reset = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			fixture.WithoutDisconnectIO( client, [ & ]() { fixture.ResetRequest(); } );
			state->SetGame( fixture.GetGame() );
			GT_OK();
		} );
		GT_ASSERT( reset.empty(), "completion reset failed: " + reset );
		fixture.WithoutDisconnectIO( client, [ & ]() { fixture.ProcessAccumulations(); } );
		GT_ASSERT( client->DisconnectReason().empty() && !fixture.HasMap(), "old completion resumed in replacement session" );
		const auto fresh = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string { fixture.PrepareWorldDownload( client, packet.data.str.size() ); GT_OK(); } );
		GT_ASSERT( fresh.empty(), "fresh completion setup failed: " + fresh );
		const auto frontend_count = fixture.FrontendRequestCount();
		// These are actual installed handlers retained before reset, invoked only
		// after the same Client/State/Game have a new session and fresh handlers.
		retained_progress( 0.5f );
		fixture.WithoutDisconnectIO( client, [ & ]() { retained_complete( packet.data.str ); } );
		GT_ASSERT( client->m_on_download_complete && client->m_on_download_progress, "retired completion erased replacement download handlers" );
		GT_ASSERT( fixture.FrontendRequestCount() == frontend_count && client->DisconnectReason().empty(), "retired download handler published progress or decoded a replacement world" );
		std::thread fresh_producer( [ & ]() { client->Receive( packet ); } ); fresh_producer.join();
		fixture.WithoutDisconnectIO( client, [ & ]() { fixture.ProcessAccumulations(); } );
		GT_ASSERT( client->DisconnectReason() == "Snapshot format mismatch" && !fixture.HasMap(), "fresh completion no longer validates its header" );
		GT_OK();
	} );

	task->AddTest( "test session animation completion copies request IDs and cancels retired callbacks", GT() {
		BackendFixture fixture;
		fixture.EnableWorldSnapshot();
		fixture.SetRunning( true );
		std::vector< size_t > completed, requested;
		const auto send = [ & ]() {
			std::vector< game::BackendRequest > requests;
			for ( size_t i = 0; i < 2; ++i ) {
				const auto id = fixture.GetGame()->GetAM()->AddAnimationCallback( [ & ]( const size_t& id ) { completed.push_back( id ); } );
				requested.push_back( id );
				game::BackendRequest request( game::BackendRequest::BR_ANIMATION_FINISHED ); request.data.animation_finished.animation_id = id; requests.push_back( request );
			}
			const auto id = fixture.GetGame()->MT_SendBackendRequests( fixture.FixtureWorldTicket(), requests );
			std::thread producer( [ & ]() { fixture.ProcessRequests(); } ); producer.join();
			const auto result = fixture.GetGame()->MT_GetResponse( id );
			return result.result == game::backend::R_SUCCESS;
		};
		GT_ASSERT( send(), "animation request failed" );
		fixture.RebindSession(); fixture.ProcessAccumulations();
		GT_ASSERT( completed.empty(), "retired animation callback ran" );
		GT_ASSERT( send(), "fresh animation request failed" );
		fixture.ProcessAccumulations();
		GT_ASSERT( completed == std::vector< size_t >( requested.begin() + 2, requested.end() ), "animation completion retained a destroyed request-loop reference" );
		GT_OK();
	} );
	task->AddTest( "test session tile lock batches retain values and retire without consuming randomness", GT() {
		BackendFixture fixture;
		fixture.EnableTileLockFixture(); fixture.SetRunning( true );
		auto* const state = fixture.GetGame()->GetState();
		const auto old_session = state->CaptureGameSession();
		const auto random = fixture.GetGame()->GetRandom()->GetStateString();
		fixture.QueueTileLock();
		size_t stolen = 1;
		fixture.SplitTileLockDispatch( [ & ]() {
			fixture.RebindSession();
			fixture.QueueTileLock();
			stolen = fixture.ExtractTileLocksWithSession( old_session );
		} );
		fixture.ProcessAccumulations();
		GT_ASSERT( stolen == 0, "retired tile producer consumed replacement requests" );
		GT_ASSERT( fixture.TileLockCount() == 0, "retired tile-lock batch executed" );
		std::thread fresh( [ & ]() { fixture.GetGame()->GetTM()->ProcessTileLockRequests(); } ); fresh.join();
		fixture.ProcessAccumulations();
		GT_ASSERT( fixture.TileLockCount() == 1, "fresh tile-lock batch lost its copied requests" );
		GT_ASSERT( fixture.GetGame()->GetRandom()->GetStateString() == random, "tile-lock cancellation consumed random state" );
		GT_OK();
	} );
	task->AddTest( "test session extracted tile batch cancels after reset collects its manager", GT() {
		BackendFixture fixture;
		fixture.EnableTileLockFixture(); fixture.SetRunning( true );
		auto* const state = fixture.GetGame()->GetState();
		Value* manager_wrapper = nullptr;
		const auto setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			manager_wrapper = fixture.GetGame()->GetTM()->Wrap( GSE_CALL );
			ctx->CreateConst( "retired_tile_manager", manager_wrapper, si, ep );
			GT_OK();
		} );
		GT_ASSERT( setup.empty(), "tile lifetime setup failed: " + setup );
		fixture.QueueTileLock();
		std::string reset_error;
		bool collected = false;
		fixture.SplitTileLockDispatch( [ & ]() {
			reset_error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				fixture.ResetRequest();
				state->SetGame( fixture.GetGame() );
				GT_OK();
			} );
			g_engine->GetGC()->Iterate();
			collected = manager_wrapper->type == VT_UNDEFINED;
		} );
		GT_ASSERT( reset_error.empty() && collected, "reset did not collect the extracted batch's manager: " + reset_error );
		g_engine->GetGC()->Iterate(); // No pending queue owner may reference the retired manager.
		fixture.ProcessAccumulations();
		GT_ASSERT( !fixture.HasMap() && fixture.GetGame()->GetTM() == nullptr, "retired tile batch recreated world state" );
		GT_OK();
	} );
}

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
	task->AddTest( "decoded roster players follow State teardown allocation ownership", GT() {
		using namespace game::backend;
		BackendFixture fixture;
		const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* const state = fixture.GetGame()->GetState();
			auto* client = new RulesetClient( gc_space, &state->m_settings.local );
			state->SetConnection( client );
			// A null faction keeps this allocation test independent of the
			// existing detached Faction ownership in Player::Deserialize.
			Player source( "Decoded player", Player::PR_HOST, nullptr, "" );
			slot::Slots slots( state );
			slots.Resize( 1 );
			slots.GetSlot( 0 ).SetPlayer( &source, 0, "" );
			types::Packet packet( types::Packet::PT_PLAYERS );
			packet.data.num = 0;
			packet.data.str = slots.Serialize().ToString();
			client->Receive( packet );
			GT_ASSERT( state->m_slots->GetCount() == 1 && state->m_slots->GetSlot( 0 ).GetPlayer()->GetPlayerName() == "Decoded player", "actual Client roster decode did not publish player" );
			state->Reset();
			GT_ASSERT( state->m_slots->GetCount() == 0 && !state->GetConnection(), "actual State reset did not retire decoded roster" );
			GT_OK();
		} );
		GT_ASSERT( error.empty(), "decoded roster ownership check: " + error );
		GT_OK();
	} );
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
		invalid[ 1 ].WriteInt( 5 ); // unsupported version
		invalid[ 2 ].WriteInt( 4 );
		invalid[ 2 ].WriteString( "unselected" );
		invalid[ 3 ].WriteInt( 4 ); // truncated before identity
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
				smac_rules_fixture::AcceptWorldSize( source.GetGame()->GetState()->m_settings.global );
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
						callback_rules_valid = callback_rules_valid && ( std::string( id ) == "smac"
							? state->m_settings.global.map.GetSmacWorldSize() == std::optional< settings::SmacWorldSizeSelection >( smac_rules_fixture::WorldSizes().SelectPreset( 0 ) )
							: !state->m_settings.global.map.GetSmacWorldSize() && !state->m_settings.global.GetSmacWorldSizeRules() );
						using Prerequisite = settings::SmacProductionPrerequisite;
						const auto reference = state->m_settings.global.GetSmacFacilityPrerequisite( 3 );
						callback_rules_valid = callback_rules_valid && ( std::string( id ) == "smac"
							? reference.status == Prerequisite::Status::TECHNOLOGY_ID && reference.technology_id == 2
							: reference.status == Prerequisite::Status::UNAVAILABLE && !state->m_settings.global.GetSmacTechnologyRules() );
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
		for ( const int scenario : { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 } ) {
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
			const bool locked = ( scenario >= 2 && scenario <= 4 ) || scenario == 8 || scenario == 9 || scenario == 14 || scenario == 15;
			std::optional< std::string > production;
			std::optional< std::string > technologies;
			std::optional< std::string > sizes;
			if ( scenario == 3 || scenario == 4 ) {
				auto changed = smac_rules_fixture::Production();
				if ( scenario == 3 ) { changed.facilities.front().cost = 123; }
				else { changed.facilities.back().prerequisite = "ChangedTechnology"; }
				production = changed.Serialize().ToString();
			}
			if ( scenario == 5 ) { types::Buffer wrong_version; wrong_version.WriteInt( 2 ); production = wrong_version.ToString(); }
			if ( scenario == 6 ) { production = smac_rules_fixture::Production().Serialize().ToString(); production->pop_back(); }
			if ( scenario == 7 ) {
				types::Buffer wrong_range; wrong_range.WriteInt( 1 ); wrong_range.WriteInt( int64_t( INT32_MAX ) + 1 ); production = wrong_range.ToString();
			}
			if ( scenario == 8 || scenario == 9 ) {
				auto changed = smac_rules_fixture::Technologies();
				if ( scenario == 8 ) { changed.technologies.back().key = "Changed"; }
				else { changed.technologies.front().flags ^= 1; }
				technologies = changed.Serialize().ToString();
			}
			if ( scenario == 10 ) { types::Buffer wrong_version; wrong_version.WriteInt( 2 ); technologies = wrong_version.ToString(); }
			if ( scenario == 11 ) { technologies = ""; }
			if ( scenario == 12 ) { technologies = smac_rules_fixture::Technologies().Serialize().ToString(); technologies->pop_back(); }
			if ( scenario == 13 ) {
				types::Buffer wrong_range; wrong_range.WriteInt( 1 ); wrong_range.WriteInt( 89 ); wrong_range.WriteInt( int64_t( INT32_MAX ) + 1 ); technologies = wrong_range.ToString();
			}
			if ( scenario == 14 || scenario == 15 ) {
				auto changed = smac_rules_fixture::WorldSizes();
				if ( scenario == 14 ) { changed.presets.back().name += " changed"; }
				else { ++changed.presets.back().source_y; }
				sizes = changed.Serialize().ToString();
			}
			if ( scenario == 16 ) { types::Buffer bad; bad.WriteInt( 2 ); sizes = bad.ToString(); }
			if ( scenario == 17 ) { sizes = smac_rules_fixture::WorldSizes().Serialize().ToString(); sizes->pop_back(); }
			if ( scenario == 18 ) {
				types::Buffer bad; bad.WriteInt( 1 ); bad.WriteInt( 5 );
				const auto rows = smac_rules_fixture::WorldSizes();
				for ( size_t index = 0; index < 5; ++index ) {
					const auto& row = rows.presets[ index ];
					bad.WriteString( row.name ); bad.WriteInt( row.source_x ); bad.WriteInt( index == 4 ? INT64_MAX : row.source_y );
				}
				sizes = bad.ToString();
			}
			if ( scenario == 19 ) { auto bad = smac_rules_fixture::WorldSizes().Serialize(); bad.WriteInt( 42 ); sizes = bad.ToString(); }
			types::Buffer invalid_state;
			invalid_state.WriteString( smac_rules_fixture::Payload( "smac", scenario != 0, values, production, technologies, sizes ).ToString() );
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
				if ( locked ) { client->Receive( valid ); } // Lock before the tampered update.
				const auto before_map = state->m_settings.global.map.Serialize().ToString();
				const auto before_name = state->m_settings.global.game_name;
				const auto before_factions = state->GetFM()->Serialize().ToString();
				const auto expected_updates = locked ? 1 : 0;
				destination.WithoutDisconnectIO( client, [ & ]() { client->Receive( invalid ); } );
				GT_ASSERT( client->HasProtocolError(), "invalid host payload did not reach the protocol error path" );
				GT_ASSERT( callbacks == expected_updates && settings_updates == expected_updates, "rejected host payload published callbacks" );
				GT_ASSERT( client->HasGlobalSettings() == locked, "rejected host rules were accepted as initialized settings" );
				GT_ASSERT( state->m_settings.global.map.Serialize().ToString() == before_map && state->m_settings.global.game_name == before_name, "rejected packet changed live global settings" );
				GT_ASSERT( state->GetFM()->Serialize().ToString() == before_factions, "rejected rules packet reached faction publication" );
				GT_ASSERT( locked ? smac_rules_fixture::Matches( state->m_settings.global ) : state->m_settings.global.GetRulesetId() == "unselected"
					&& !state->m_settings.global.GetSmacBaseSquareRules() && !state->m_settings.global.GetSmacProductionRules()
					&& !state->m_settings.global.GetSmacTechnologyRules(), "rejected packet changed session identity or values" );
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "host rules packet rejection failed: " + error );
		}
		GT_OK();
	} );
}

static void AddWorldSnapshotTests( task::gsetests::GSETests* task ) {
	for ( const auto* id : { "smac", "smax" } ) {
		task->AddTest( std::string( "actual world-size menu custom cancellation and lobby rollback " ) + id, GT( id ) {
			BackendFixture fixture; fixture.EnableTurnPlayers();
			auto* state = fixture.GetGame()->GetState();
			state->m_settings.global.SelectRuleset( id ); state->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
			const auto error = fixture.WithEngineGameScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* script = static_cast< value::Callable* >( fixture.RunScript( GSE_CALL, "./GLSMAC_data/tests/_world_size_setup" ) );
				script->Run( GSE_CALL, { fixture.GetGame()->Wrap( GSE_CALL ), state->m_settings.global.Wrap( GSE_CALL ), VALUE( value::Bool,, std::string( id ) == "smac" ) } );
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "setup route: " + error ); GT_OK();
		} );
	}
	task->AddTest( "actual SMAC world-size event and State Packet preserve accepted category", GT() {
		using namespace game::backend;
		using namespace game::backend::event;
		BackendFixture fixture, destination; fixture.EnableTurnPlayers();
		auto* game = fixture.GetGame(); auto* state = game->GetState();
		state->m_settings.global.SelectRuleset( "smac" ); state->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
		state->m_settings.global.SelectSmacCustomSize( 48, 48 );
		const auto error = fixture.WithEngineGameScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* handlers = static_cast< value::Object* >( fixture.RunScript( GSE_CALL, "./GLSMAC_data/default/game/event/game_settings" ) );
			fixture.RegisterEvent( "game_settings", new EventHandler( gc_space, "game_settings", static_cast< value::Callable* >( handlers->Get( "validate" ) ), nullptr,
				static_cast< value::Callable* >( handlers->Get( "apply" ) ), static_cast< value::Callable* >( handlers->Get( "rollback" ) ) ) );
			auto* change = VALUE( value::Array,, ( value::array_elements_t{ VALUE( value::String,, "planet_size" ), VALUE( value::String,, "1" ) } ) );
			game->AddEvent( new Event( game, Event::ES_LOCAL, 0, GSE_CALL, "game_settings", {
				{ "changes", VALUE( value::Array,, ( value::array_elements_t{ change } ) ) } } ) );
			fixture.ProcessEvents();
			GT_ASSERT( state->m_settings.global.map.GetSmacWorldSize()->category == 1 && state->m_settings.global.map.size_x == 48, "actual accepted event lost preset identity" );
			GT_OK();
		} );
		GT_ASSERT( error.empty(), "event route: " + error );
		types::Packet packet( types::Packet::PT_GLOBAL_SETTINGS ); packet.data.str = state->Serialize().ToString();
		types::Packet decoded( types::Packet::PT_NONE ); decoded.Deserialize( packet.Serialize() );
		auto* restored = destination.GetGame()->GetState(); restored->Deserialize( types::Buffer( decoded.data.str ) );
		GT_ASSERT( restored->m_settings.global.map.GetSmacWorldSize() == state->m_settings.global.map.GetSmacWorldSize()
			&& *restored->m_settings.global.GetSmacWorldSizeRules() == *state->m_settings.global.GetSmacWorldSizeRules(), "State Packet lost accepted category or frozen source" );
		GT_OK();
	} );
	task->AddTest( "actual world snapshot writer captures RNG without drawing or changing body layout", GT() {
		for ( const auto* id : { "smac", "smax" } ) {
			BackendFixture fixture;
			fixture.EnableWorldSnapshot();
			auto* game = fixture.GetGame();
			auto& settings = game->GetState()->m_settings.global;
			settings.SelectRuleset( id );
			settings.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( settings );
			types::Buffer old_body;
			game->GetMap()->SaveToBuffer( old_body );
			const auto managers = fixture.WorldManagers();
			// Append the old body frames independently of the production writer.
			types::Buffer manager_frames( managers );
			for ( size_t i = 0 ; i < 4 ; ++i ) { old_body.WriteString( manager_frames.ReadString() ); }
			game->GetRandom()->GetUInt();
			const auto before_rng = game->GetRandom()->GetStateString();
			const auto snapshot = fixture.WorldSnapshot();
			GT_ASSERT( game->GetRandom()->GetStateString() == before_rng, "snapshot writing advanced RNG" );
			types::Buffer input( snapshot );
			game::backend::world_snapshot::ValidateHeader( input, settings );
			util::random::Random captured( 1 ); captured.SetState( game::backend::world_snapshot::ReadRandomState( input ) );
			GT_ASSERT( captured.GetStateString() == before_rng, "writer captured another RNG boundary" );
			const auto turn = game::backend::world_snapshot::ReadTurnState( input, game->GetState()->m_slots->GetSlots() );
			GT_ASSERT( turn.id == game->GetTurnId() && turn.players.empty(), "writer changed empty-roster turn state" );
			GT_ASSERT( snapshot.substr( input.lenr ) == old_body.ToString(), "world body order or content changed" );
			fixture.DestroyMapBeforeManagers();
			GT_ASSERT( fixture.WorldSnapshot().empty(), "not-yet-generated map no longer returns an empty download response" );
		}
		GT_OK();
	} );
	for ( const auto parameters : { std::pair< bool, const char* >{ false, "smac" }, { true, "smac" }, { false, "smax" }, { true, "smax" } } ) {
		const bool queued = parameters.first;
		const auto* ruleset = parameters.second;
		task->AddTest( std::string( "real final download chunk rejects invalid world profile before map allocation " ) + ( queued ? "queued" : "immediate" ) + ( std::string( ruleset ) == "smax" ? " SMAX" : "" ), GT( queued, ruleset ) {
			using namespace game::backend;
			auto rejected = world_rules_fixture::InvalidProductionHeaders( ruleset );
			for ( const auto& invalid : world_rules_fixture::InvalidTechnologyHeaders( ruleset ) ) {
				rejected.emplace_back( "technology: " + invalid.first, invalid.second );
			}
			for ( const auto& invalid : world_rules_fixture::InvalidWorldSizeHeaders( ruleset ) ) { rejected.emplace_back( "world-size: " + invalid.first, invalid.second ); }
			if ( std::string( ruleset ) == "smac" ) {
				const auto profile_rejected = world_rules_fixture::InvalidSmacHeaders();
				rejected.insert( rejected.end(), profile_rejected.begin(), profile_rejected.end() );
				const auto rng_rejected = world_rules_fixture::InvalidRandomHeaders();
				rejected.insert( rejected.end(), rng_rejected.begin(), rng_rejected.end() );
				const auto turn_rejected = world_rules_fixture::InvalidTurnHeaders();
				rejected.insert( rejected.end(), turn_rejected.begin(), turn_rejected.end() );
				// Matching headers reach the actual body's first read, still before
				// Map's asset-bearing constructor. Native errors there must also be
				// caught when completion executes later on the accumulation thread.
				const world_snapshot::TurnState turn{ 37, { { 0, true }, { 2, false } } };
				rejected.emplace_back( "matching header without map frame", world_rules_fixture::WorldHeader( turn ).ToString() );
				auto wrong_body = world_rules_fixture::WorldHeader( turn );
				wrong_body.WriteInt( 0 );
				rejected.emplace_back( "matching header with wrong map frame type", wrong_body.ToString() );
				for ( const auto xy : { std::pair< int64_t, int64_t >{ 10, 8 }, { 8, 9 }, { -1, 8 }, { INT64_MAX, 8 } } ) {
					auto wrong_map = world_rules_fixture::WorldHeader( turn ); types::Buffer map;
					map.WriteInt( xy.first ); map.WriteInt( xy.second ); wrong_map.WriteString( map.ToString() );
					rejected.emplace_back( "world-size: mismatched actual tile prefix", wrong_map.ToString() );
				}
			}
			for ( const auto& invalid : rejected ) {
				const bool retain_map = invalid.first.compare( 0, 12, "technology: " ) == 0 || invalid.first.compare( 0, 12, "world-size: " ) == 0;
				size_t callbacks = 0;
				BackendFixture fixture;
				fixture.EnableWorldSnapshot();
				fixture.EnableTurnPlayers();
				fixture.DefinePop( "WORKER", "Worker", base::PopDef::PF_TILE_WORKER );
				if ( !retain_map ) { fixture.DestroyMapBeforeManagers(); }
				auto* game = fixture.GetGame();
				auto* const prior_map = retain_map ? game->GetMap() : nullptr;
				types::Buffer map_before;
				if ( prior_map ) { prior_map->SaveToBuffer( map_before ); }
				auto* state = game->GetState();
				state->m_slots->GetSlot( 2 ).GetPlayer()->CompleteTurn();
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
					state->m_settings.global.SelectRuleset( ruleset );
					state->m_settings.global.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( state->m_settings.global );
					const auto on_callback = [ & ]() { return NATIVE_CALL( &callbacks ) { ++callbacks; return VALUE( value::Undefined ); } ); };
					game->On( GSE_CALL, "start", on_callback() ); game->On( GSE_CALL, "turn", on_callback() );
					game->GetUM()->On( GSE_CALL, "unit_spawn", on_callback() ); game->GetBM()->On( GSE_CALL, "base_spawn", on_callback() );
					client = new RulesetClient( gc_space, &state->m_settings.local );
					state->SetConnection( client );
					fixture.PrepareWorldDownload( client, packet.data.str.size() );
					GT_OK();
				} );
				GT_ASSERT( setup_error.empty(), "download setup failed: " + setup_error );
				const auto before_settings = state->m_settings.global.Serialize().ToString();
				const auto before_managers = fixture.WorldManagers();
				const auto before_turn = game->GetTurnId();
				const auto before_rng = game->GetRandom()->GetStateString();
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
					GT_ASSERT( fixture.HasMap() == retain_map && fixture.IsPreparingMap(), "invalid profile allocated or retired a map" );
					if ( retain_map ) {
						GT_ASSERT( game->GetMap() == prior_map, "late technology rejection replaced the existing map" );
						types::Buffer after; game->GetMap()->SaveToBuffer( after );
						GT_ASSERT( after.ToString() == map_before.ToString(), "late technology rejection changed existing map bytes" );
					}
					GT_ASSERT( game->GetRandom()->GetStateString() == before_rng && !fixture.HasPendingSnapshotRandom(), "rejected header changed or staged RNG" );
					GT_ASSERT( !fixture.HasPendingSnapshotTurn() && !state->m_slots->GetSlot( 0 ).GetPlayer()->IsTurnCompleted() && state->m_slots->GetSlot( 2 ).GetPlayer()->IsTurnCompleted(), "rejected header changed or staged completion flags" );
					GT_ASSERT( state->m_settings.global.Serialize().ToString() == before_settings, "world profile changed frozen settings" );
					GT_ASSERT( fixture.WorldManagers() == before_managers && game->GetTurnId() == before_turn && fixture.FrontendRequestCount() == before_frontend, "profile rejection changed managers, turn or frontend state" );
					GT_ASSERT( !callbacks, "profile rejection ran world setup or gameplay callbacks" );
					GT_OK();
				} );
				GT_ASSERT( error.empty(), "download rejection failed: " + invalid.first + ": " + error );
			}
			GT_OK();
		} );
	}
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

static void AddWorldCompletionTests( task::gsetests::GSETests* task ) {
	using namespace game::backend;
	task->AddTest( "world completion parked failure blocks queued ordinary event work", GT() {
		size_t applies = 0, errors = 0;
		BackendFixture source, fixture;
		for ( auto* current : { &source, &fixture } ) {
			current->EnableWorldSnapshot(); current->EnableTurnPlayers();
			current->GetGame()->GetState()->m_settings.global.SelectRuleset( "smax" ); current->GetGame()->GetState()->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
		}
		const auto setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* noop = NATIVE_CALL() { return VALUE( value::Undefined ); } );
			auto* apply = NATIVE_CALL( &applies ) { ++applies; return VALUE( value::Undefined ); } );
			fixture.RegisterEvent( "queued_before_failure", new event::EventHandler( gc_space, "queued_before_failure", noop, nullptr, apply, noop ) );
			fixture.GetGame()->AddEvent( new event::Event( fixture.GetGame(), event::Event::ES_LOCAL, 0, GSE_CALL, "queued_before_failure", {}, "queued-before-failure" ) );
			fixture.GetGame()->On( GSE_CALL, "error", NATIVE_CALL( &errors ) { ++errors; return VALUE( value::Undefined ); } ) ); GT_OK();
		} );
		GT_ASSERT( setup.empty(), "parked event setup: " + setup );
		std::exception_ptr escaped;
		const auto iterate = [ & ]() { try { fixture.ValidateSnapshotBeforeMapInitialization(); } catch ( ... ) { escaped = std::current_exception(); } };
		// Queue real ordinary event/base work before the download enters its
		// failure state. It must not run stale work ahead of the error callback.
		std::thread before_download( iterate ); before_download.join();
		GT_ASSERT( !escaped && applies == 0, "foreign ordinary work did not defer" );
		const auto stage = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			fixture.RestoreWorldBody( GSE_CALL, source.WorldSnapshot() );
			fixture.GetGame()->GetState()->m_slots->GetSlot( 2 ).GetPlayerAndClose(); GT_OK();
		} );
		GT_ASSERT( stage.empty(), "parked event restore: " + stage );
		std::thread failure( iterate ); failure.join(); std::thread repeat( iterate ); repeat.join();
		GT_ASSERT( !escaped && errors == 0 && applies == 0, "foreign failure ran script work" );
		fixture.DrainWorldRestore();
		GT_ASSERT( errors == 1 && applies == 0 && !fixture.HasMap() && !fixture.HasPendingSnapshotTurn(), "parked failure allowed earlier queued event work" ); GT_OK();
	} );
	task->AddTest( "world completion early roster rejection runs on Space owner and retires stale queued errors", GT() {
		BackendFixture source; source.EnableWorldSnapshot(); source.EnableTurnPlayers(); source.SetSnapshotTurn( 37 );
		source.GetGame()->GetState()->m_settings.global.SelectRuleset( "smax" ); source.GetGame()->GetState()->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
		source.GetGame()->GetState()->m_slots->GetSlot( 0 ).GetPlayer()->CompleteTurn();
		const auto snapshot = source.WorldSnapshot();
		for ( const int scenario : { 0, 1, 2, 3, 4 } ) {
			size_t errors = 0, starts = 0; bool error_on_owner = false;
			std::string error_message;
			Player* detached = nullptr;
			BackendFixture fixture; fixture.EnableWorldSnapshot(); fixture.EnableTurnPlayers();
			auto* game = fixture.GetGame(); auto* state = game->GetState();
			state->m_settings.global.SelectRuleset( "smax" ); state->m_settings.global.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( state->m_settings.global );
			const auto setup = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				fixture.RestoreWorldBody( GSE_CALL, snapshot );
				const auto owner = std::this_thread::get_id();
				game->On( GSE_CALL, "error", NATIVE_CALL( &errors, &error_on_owner, &error_message, owner ) {
					++errors; error_on_owner = std::this_thread::get_id() == owner;
					error_message = VALUE_GET( value::String, VALUE_DATA( value::Object, arguments.at( 0 ) )->Get( "error" ) );
					return VALUE( value::Undefined );
				} ) );
				game->On( GSE_CALL, "start", NATIVE_CALL( &starts ) { ++starts; return VALUE( value::Undefined ); } ) );
				detached = state->m_slots->GetSlot( 2 ).GetPlayerAndClose(); GT_OK();
			} );
			GT_ASSERT( setup.empty(), "early rejection setup: " + setup );
			GT_ASSERT( !fixture.IsAccumulating(), "early rejection must exercise real outside-accumulation Iterate" );
			std::exception_ptr escaped;
			const auto iterate = [ & ]() { try { fixture.ValidateSnapshotBeforeMapInitialization(); } catch ( ... ) { escaped = std::current_exception(); } };
			if ( scenario == 0 ) { iterate(); }
			else { std::thread backend( iterate ); backend.join(); }
			if ( escaped ) { try { std::rethrow_exception( escaped ); } catch ( const std::exception& error ) { GT_FAIL( "early rejection escaped: " + std::string( error.what() ) ); } }
			if ( scenario == 0 ) {
				GT_ASSERT( errors == 1 && error_on_owner && !fixture.HasMap() && !fixture.HasPendingSnapshotTurn(), "same-owner early failure did not retire initialization" );
				continue;
			}
			GT_ASSERT( errors == 0 && fixture.HasMap() && fixture.HasPendingSnapshotTurn(), "foreign Iterate executed error callback before owner drain" );
			std::thread repeat( iterate ); repeat.join();
			GT_ASSERT( !escaped && errors == 0, "waiting state repeated unsafe initialization" );
			const auto before_drain = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				if ( scenario == 2 ) { fixture.ResetWorld(); }
				if ( scenario >= 3 ) {
					state->m_slots->GetSlot( 2 ).SetPlayer( detached, 0, "" );
					fixture.RestoreWorldBody( GSE_CALL, snapshot );
					if ( scenario == 4 ) { detached->SetSlot( nullptr ); }
				}
				GT_OK();
			} );
			GT_ASSERT( before_drain.empty(), "queued error reset/replacement failed: " + before_drain );
			if ( scenario == 4 ) { std::thread replacement_failure( iterate ); replacement_failure.join(); GT_ASSERT( !escaped, "replacement failure escaped backend" ); }
			fixture.DrainWorldRestore();
			GT_ASSERT( errors == ( scenario == 1 || scenario == 4 ? 1u : 0u ) && starts == 0, "queued stale error affected reset/replacement or published twice" );
			if ( scenario == 1 || scenario == 4 ) { GT_ASSERT( error_on_owner && !fixture.HasMap() && !fixture.HasPendingSnapshotTurn(), "foreign failure did not execute terminal cleanup on owner" ); }
			if ( scenario == 4 ) { GT_ASSERT( error_message == "Noncanonical world snapshot Player link", "old queued generation replaced the new snapshot's failure" ); }
			if ( scenario == 2 ) { GT_ASSERT( !fixture.HasMap() && !fixture.HasPendingSnapshotTurn(), "discarded error retained world state" ); }
			if ( scenario == 3 ) {
				GT_ASSERT( fixture.HasMap() && fixture.HasPendingSnapshotTurn(), "stale queued error consumed replacement snapshot" );
				const auto finish = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string { fixture.CompleteWorldRestore( GSE_CALL ); GT_OK(); } );
				GT_ASSERT( finish.empty() && fixture.IsRunning() && starts == 1 && !errors && state->m_slots->GetSlot( 0 ).GetPlayer()->IsTurnCompleted(), "replacement snapshot failed after stale error retirement" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "world completion decoder rejects malformed records without a usable turn state", GT() {
		BackendFixture fixture; fixture.EnableTurnPlayers();
		auto* state = fixture.GetGame()->GetState();
		state->m_settings.global.SelectRuleset( "smac" ); state->m_settings.global.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( state->m_settings.global );
		for ( const auto& invalid : world_rules_fixture::InvalidTurnHeaders() ) {
			types::Buffer input( invalid.second ); world_snapshot::ValidateHeader( input, state->m_settings.global ); world_snapshot::ReadRandomState( input );
			bool rejected = false;
			try { world_snapshot::ReadTurnState( input, state->m_slots->GetSlots() ); } catch ( const std::runtime_error& ) { rejected = true; }
			GT_ASSERT( rejected, "completion decoder accepted " + invalid.first );
		}
		GT_ASSERT( !state->m_slots->GetSlot( 0 ).GetPlayer()->IsTurnCompleted() && !state->m_slots->GetSlot( 2 ).GetPlayer()->IsTurnCompleted(), "malformed decode changed live flags" ); GT_OK();
	} );
	task->AddTest( "world completion capture owns exact occupied roster including disconnected players", GT() {
		BackendFixture fixture; fixture.EnableWorldSnapshot(); fixture.EnableTurnPlayers(); fixture.SetSnapshotTurn( 37 );
		auto* game = fixture.GetGame(); auto* state = game->GetState();
		state->m_settings.global.SelectRuleset( "smax" ); state->m_settings.global.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( state->m_settings.global );
		auto* first = state->m_slots->GetSlot( 0 ).GetPlayer(); auto* last = state->m_slots->GetSlot( 2 ).GetPlayer();
		first->CompleteTurn(); last->Connect(); last->Disconnect();
		const auto random = game->GetRandom()->GetStateString();
		const auto snapshot = fixture.WorldSnapshot();
		first->UncompleteTurn(); last->CompleteTurn(); fixture.SetSnapshotTurn( 99 );
		types::Buffer input( snapshot ); world_snapshot::ValidateHeader( input, state->m_settings.global );
		world_snapshot::ReadRandomState( input ); const auto captured = world_snapshot::ReadTurnState( input, state->m_slots->GetSlots() );
		GT_ASSERT( captured.id == 37 && captured.players.size() == 2 && captured.players[ 0 ].id == 0 && captured.players[ 0 ].completed && captured.players[ 1 ].id == 2 && !captured.players[ 1 ].completed, "writer used later flags or omitted disconnected player" );
		GT_ASSERT( game->GetRandom()->GetStateString() == random, "capturing turn state consumed randomness" );
		// Writer and reader both reject broken canonical reverse links; open and
		// closed slots are omitted, not encoded as players with default flags.
		state->m_slots->GetSlot( 1 ).Close();
		world_snapshot::ValidateTurnRoster( captured, state->m_slots->GetSlots() );
		last->SetSlot( nullptr );
		bool writer_rejected = false, reader_rejected = false;
		try { fixture.WorldSnapshot(); } catch ( const std::runtime_error& ) { writer_rejected = true; }
		types::Buffer block; world_snapshot::WriteTurnState( block, captured );
		try { world_snapshot::ReadTurnState( block, state->m_slots->GetSlots() ); } catch ( const std::runtime_error& ) { reader_rejected = true; }
		last->SetSlot( &state->m_slots->GetSlot( 2 ) );
		GT_ASSERT( writer_rejected && reader_rejected, "noncanonical Player reverse link was accepted" );
		for ( const auto records : { std::vector< world_snapshot::SlotCompletion >{ { 0, true }, { 0, false } }, { { 2, true }, { 0, false } } } ) {
			types::Buffer output; output.WriteString( "unchanged" ); const auto before = output.ToString(); bool rejected = false;
			try { world_snapshot::WriteTurnState( output, { 37, records } ); } catch ( const std::runtime_error& ) { rejected = true; }
			GT_ASSERT( rejected && output.ToString() == before, "writer published noncanonical completion records" );
		}
		GT_OK();
	} );

	task->AddTest( "world completion pending Player tokens survive collection and are released after commit or discard", GT() {
		class CountedValue : public value::Int {
		public:
			CountedValue( gc::Space* space, std::atomic< size_t >& destroyed ) : value::Int( space, 0 ), destroyed( destroyed ) {}
			~CountedValue() override { ++destroyed; }
		private:
			std::atomic< size_t >& destroyed;
		};
		BackendFixture source; source.EnableWorldSnapshot(); source.EnableTurnPlayers(); source.SetSnapshotTurn( 37 );
		source.GetGame()->GetState()->m_settings.global.SelectRuleset( "smax" ); source.GetGame()->GetState()->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
		source.GetGame()->GetState()->m_slots->GetSlot( 0 ).GetPlayer()->CompleteTurn();
		source.GetGame()->GetState()->m_slots->GetSlot( 2 ).GetPlayer()->CompleteTurn();
		const auto snapshot = source.WorldSnapshot();
		for ( const bool cancel : { false, true } ) {
			std::atomic< size_t > destroyed = 0;
			BackendFixture fixture; fixture.EnableWorldSnapshot(); fixture.EnableTurnPlayers();
			auto* game = fixture.GetGame(); game->GetState()->m_settings.global.SelectRuleset( "smax" ); game->GetState()->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
			const auto staged = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				fixture.RestoreWorldBody( GSE_CALL, snapshot );
				const auto tokens = fixture.PendingSnapshotTokens(); GT_ASSERT( tokens.size() == 2, "missing pending lifetime tokens" );
				for ( auto* token : tokens ) { token->Assign( "fixture_lifetime", new CountedValue( gc_space, destroyed ) ); }
				GT_OK();
			} );
			GT_ASSERT( staged.empty(), "token staging failed: " + staged );
			// Run the actual collector outside accumulation. No test-owned root
			// retains these values: only Game's pending token traversal does so.
			g_engine->GetGC()->Iterate();
			GT_ASSERT( destroyed == 0, "collector reclaimed a pending Player lifetime token" );
			const auto finished = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				if ( cancel ) { fixture.CancelWorldInitialization(); }
				else {
					fixture.CompleteWorldRestore( GSE_CALL );
					fixture.CheckTurnComplete();
					GT_ASSERT( game->GetState()->m_slots->GetSlot( 0 ).GetPlayer()->IsTurnCompleted() && game->GetState()->m_slots->GetSlot( 2 ).GetPlayer()->IsTurnCompleted(), "collected token prevented valid completion restore" );
					GT_ASSERT( game->GetTurnId() == 37, "restoring all-complete flags auto-advanced the turn" );
					for ( const auto& request : fixture.FrontendRequests() ) {
						if ( request.type == game::FrontendRequest::FR_TURN_STATUS ) { GT_ASSERT( request.data.turn_status.status == turn::TS_WAITING_FOR_PLAYERS, "all-complete restore published active status" ); }
					}
				}
				GT_ASSERT( !fixture.HasPendingSnapshotTurn(), "completion retained token roots" ); GT_OK();
			} );
			GT_ASSERT( finished.empty(), "token completion failed: " + finished );
			g_engine->GetGC()->Iterate();
			GT_ASSERT( destroyed == 2, "commit/discard retained pending Player lifetime tokens" );
		}
		GT_OK();
	} );

	for ( const int scenario : { 0, 1, 2, 3, 4, 5 } ) {
		task->AddTest( "world completion rejects roster or Player replacement before installing any flags " + std::to_string( scenario ), GT( scenario ) {
			const auto next_id = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ]() { Unit::SetNextId( next_id ); } );
			BackendFixture source, fixture;
			for ( auto* current : { &source, &fixture } ) {
				current->EnableWorldSnapshot(); current->EnableTurnPlayers(); current->SetSnapshotTurn( 37 );
				current->GetGame()->GetState()->m_settings.global.SelectRuleset( "smax" ); current->GetGame()->GetState()->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
			}
			source.GetGame()->GetState()->m_slots->GetSlot( 0 ).GetPlayer()->CompleteTurn();
			source.GetGame()->GetRandom()->SetState( { 7, 8, 9, 10 } );
			const auto snapshot = source.WorldSnapshot();
			const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.GetGame(); auto* state = game->GetState();
				auto* first = state->m_slots->GetSlot( 0 ).GetPlayer(); auto* current = state->m_slots->GetSlot( 2 ).GetPlayer();
				current->CompleteTurn();
				const auto before_random = game->GetRandom()->GetStateString();
				fixture.RestoreWorldBody( GSE_CALL, snapshot );
				size_t starts = 0, errors = 0, turns = 0;
				game->On( GSE_CALL, "error", NATIVE_CALL( &errors ) { ++errors; return VALUE( value::Undefined ); } ) );
				game->On( GSE_CALL, "turn", NATIVE_CALL( &turns ) { ++turns; return VALUE( value::Undefined ); } ) );
				const auto replace = [ & ]() {
					auto& slot = state->m_slots->GetSlot( 2 ); auto* previous = slot.GetPlayerAndClose();
					if ( scenario == 2 ) {
						// Deterministically end and restart a Player lifetime at the exact
						// same tracked allocation. State still owns that address; no
						// allocator-reuse assumption or second allocation record is needed.
						previous->~Player();
						current = ::new ( previous ) Player( "Replacement", Player::PR_SINGLE, game->GetFaction( "GAIANS" ), "" );
					}
					else { NEWV( replacement, Player, "Replacement", Player::PR_SINGLE, game->GetFaction( "GAIANS" ), "" ); current = replacement; state->AddPlayer( current ); }
					slot.SetPlayer( current, 0, "" ); current->CompleteTurn();
				};
				game->On( GSE_CALL, "start", NATIVE_CALL( &starts, &replace, scenario ) { ++starts; if ( scenario == 2 ) { replace(); } return VALUE( value::Undefined ); } ) );
				if ( scenario == 0 ) { state->m_slots->GetSlot( 2 ).GetPlayerAndClose(); }
				if ( scenario == 1 ) { replace(); }
				if ( scenario == 3 ) {
					DefineSnapshotUnit( game->GetUM() ); SpawnSnapshotUnit( GSE_CALL, fixture, 88 );
					game->GetUM()->On( GSE_CALL, "unit_spawn", NATIVE_CALL( &replace ) { replace(); return VALUE( value::Undefined ); } ) );
				}
				if ( scenario == 4 ) { current->SetSlot( nullptr ); }
				if ( scenario == 5 ) {
					NEWV( extra, Player, "Extra", Player::PR_SINGLE, game->GetFaction( "GAIANS" ), "" ); state->AddPlayer( extra ); state->m_slots->GetSlot( 1 ).SetPlayer( extra, 0, "" );
				}
				fixture.ClearFrontendRequests();
				if ( scenario == 0 || scenario == 4 || scenario == 5 ) { fixture.ValidateSnapshotBeforeMapInitialization(); }
				else { fixture.CompleteWorldRestore( GSE_CALL ); }
				GT_ASSERT( errors == 1 && starts == ( scenario == 2 || scenario == 3 ? 1u : 0u ) && !turns, "identity rejection did not use terminal initialization failure at the correct boundary" );
				GT_ASSERT( !first->IsTurnCompleted() && current->IsTurnCompleted(), "identity rejection partially installed completion flags" );
				GT_ASSERT( !fixture.IsRunning() && !fixture.HasMap() && !fixture.HasPendingSnapshotTurn() && !fixture.HasPendingSnapshotRandom(), "failed identity retained world or staged continuation" );
				GT_ASSERT( game->GetRandom()->GetStateString() == before_random, "failed identity installed saved RNG" );
				for ( const auto& request : fixture.FrontendRequests() ) {
					GT_ASSERT( request.type != game::FrontendRequest::FR_TURN_ADVANCE && request.type != game::FrontendRequest::FR_TURN_STATUS, "failed identity published restored turn/status" );
				}
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "roster identity scenario " + std::to_string( scenario ) + ": " + error ); GT_OK();
		} );
	}
}

static void AddWorldTurnTests( task::gsetests::GSETests* task ) {
	using game::backend::base::Base;
	using game::backend::base::PopDef;
	using namespace game::backend::turn;
	for ( const int scenario : { 0, 1, 2, 3 } ) {
		task->AddTest( "world turn restore preserves simulation and publishes after registration " + std::to_string( scenario ), GT( scenario ) {
			const auto old_unit_id = Unit::GetNextId(), old_base_id = Base::GetNextId();
			util::FinallyGuard restore_ids( [ = ]() { Unit::SetNextId( old_unit_id ); Base::SetNextId( old_base_id ); } );
			BackendFixture source, destination;
			for ( auto* fixture : { &source, &destination } ) {
				fixture->EnableWorldSnapshot(); fixture->EnableTurnPlayers();
				auto& settings = fixture->GetGame()->GetState()->m_settings.global;
				settings.SelectRuleset( scenario == 3 ? "smac" : "smax" ); settings.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( settings );
			}
			source.SetRunning( true ); source.SetSnapshotTurn( 37 );
			source.GetGame()->GetState()->m_slots->GetSlot( 0 ).GetPlayer()->CompleteTurn();
			if ( scenario == 2 ) { source.GetGame()->GetState()->m_slots->GetSlot( 2 ).GetPlayer()->CompleteTurn(); }
			source.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
			source.AddBaseWithPop( 601, 9 )->SetAccumulatedNutrients( -123 );
			const auto source_error = source.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				DefineSnapshotUnit( source.GetGame()->GetUM() );
				auto* unit = SpawnSnapshotUnit( GSE_CALL, source, 71 );
				unit->m_movement = scenario == 1 ? -0.25f : 0.375f;
				unit->m_health = 0.625f; unit->m_moved_this_turn = true;
				GT_OK();
			} );
			GT_ASSERT( source_error.empty(), "source setup: " + source_error );
			const auto snapshot = source.WorldSnapshot();
			const auto error = destination.WithMutableScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = destination.GetGame();
				auto* local = game->GetState()->m_slots->GetSlot( 2 ).GetPlayer();
				auto* remote = game->GetState()->m_slots->GetSlot( 0 ).GetPlayer();
				if ( scenario != 2 ) { local->CompleteTurn(); } // Deliberately opposite earlier lobby state.
				if ( scenario == 2 ) { destination.UseClientConnection( gc_space ); }
				size_t turns = 0, unit_turns = 0, base_turns = 0, starts = 0, statuses = 0;
				bool start_saw_metadata = false, status_saw_instances = false;
				game->On( GSE_CALL, "turn", NATIVE_CALL( &turns ) { ++turns; return VALUE( value::Undefined ); } ) );
				game->GetUM()->On( GSE_CALL, "unit_turn", NATIVE_CALL( &unit_turns ) { ++unit_turns; return VALUE( value::Undefined ); } ) );
				game->GetBM()->On( GSE_CALL, "base_turn", NATIVE_CALL( &base_turns ) { ++base_turns; return VALUE( value::Undefined ); } ) );
				game->On( GSE_CALL, "start", NATIVE_CALL( &starts, &start_saw_metadata, game, local, remote, scenario ) {
					++starts; start_saw_metadata = game->GetTurnId() == 37 && !remote->IsTurnCompleted() && local->IsTurnCompleted() == ( scenario != 2 );
					// Setup changes must not replace the downloaded continuation.
					remote->UncompleteTurn(); local->UncompleteTurn();
					return VALUE( value::Undefined );
				} ) );
				game->On( GSE_CALL, "turn_status", NATIVE_CALL( &statuses, &status_saw_instances, game ) {
					++statuses;
					status_saw_instances = game->GetUM()->GetUnit( 71 ) && game->GetBM()->GetBase( 601 );
					return VALUE( value::Undefined );
				} ) );
				destination.ClearFrontendRequests();
				destination.RestoreWorldBody( GSE_CALL, snapshot );
				GT_ASSERT( !turns && !unit_turns && !base_turns, "download invoked gameplay turn hooks" );
				GT_ASSERT( !remote->IsTurnCompleted() && local->IsTurnCompleted() == ( scenario != 2 ), "download committed flags before setup" );
				for ( const auto& request : destination.FrontendRequests() ) {
					GT_ASSERT( request.type != game::FrontendRequest::FR_TURN_ADVANCE && request.type != game::FrontendRequest::FR_TURN_STATUS, "turn published before instance registration" );
				}
				destination.CompleteWorldRestore( GSE_CALL );
				GT_ASSERT( !destination.HasPendingSnapshotTurn(), "completed restore retained pending turn publication" );
				GT_ASSERT( starts == 1 && start_saw_metadata && statuses == 1 && status_saw_instances, "restore publication ordering changed" );
				GT_ASSERT( !turns && !unit_turns && !base_turns, "initialization advanced gameplay turn" );
				const auto* unit = game->GetUM()->GetUnit( 71 ); const auto* base = game->GetBM()->GetBase( 601 );
				GT_ASSERT( unit->m_movement == ( scenario == 1 ? -0.25f : 0.375f ) && unit->m_health == 0.625f && unit->m_moved_this_turn, "restore reset movement, health, or moved state" );
				GT_ASSERT( base->GetAccumulatedNutrients() == -123 && base->m_pops.size() == 1 && base->m_pops.count( 9 ), "restore changed nutrients or population" );
				GT_ASSERT( remote->IsTurnCompleted() && local->IsTurnCompleted() == ( scenario == 2 ), "initialization cleared player completion flags" );
				GT_ASSERT( game->GetRandom()->GetStateString() == source.GetGame()->GetRandom()->GetStateString(), "turn restore consumed saved RNG" );
				GT_ASSERT( destination.WorldSnapshot() == snapshot, "complete restored snapshot changed frozen profile, map, managers, RNG or turn state" );
				const auto expected_status = scenario == 2 ? TS_WAITING_FOR_PLAYERS : scenario == 1 ? TS_TURN_COMPLETE : TS_TURN_ACTIVE;
				size_t advances = 0, updates = 0;
				for ( const auto& request : destination.FrontendRequests() ) {
					if ( request.type == game::FrontendRequest::FR_TURN_ADVANCE ) { ++advances; GT_ASSERT( request.data.turn_advance.turn_id == 37, "frontend turn ID differs" ); }
					if ( request.type == game::FrontendRequest::FR_TURN_STATUS ) { ++updates; GT_ASSERT( request.data.turn_status.status == expected_status, "frontend turn status differs" ); }
				}
				GT_ASSERT( advances == 1 && updates == 1, "restored turn/status not published exactly once" );
				destination.CheckTurnComplete();
				GT_ASSERT( statuses == 1, "next iteration repeated or replaced restored status" );
				// UI is constructed after backend start. Use the actual minimap script
				// with the real wrapped Game, replacing only asset-dependent factories.
				auto* wrapper = static_cast< value::Object* >( game->Wrap( GSE_CALL ) );
				auto* get_status = static_cast< value::Callable* >( wrapper->Get( "get_turn_status" ) );
				const auto expected_name = scenario == 2 ? "waiting_for_players" : scenario == 1 ? "turn_complete" : "active";
				GT_ASSERT( static_cast< value::String* >( get_status->Run( GSE_CALL, {} ) )->value == expected_name, "cached script status differs" );
				auto* factory = static_cast< value::Callable* >( destination.RunScript( GSE_CALL, "./GLSMAC_data/tests/_restored_minimap" ) );
				auto* read_ui = static_cast< value::Callable* >( factory->Run( GSE_CALL, { wrapper } ) );
				auto* ui = static_cast< value::Object* >( read_ui->Run( GSE_CALL, {} ) );
				GT_ASSERT( static_cast< value::String* >( ui->Get( "year" ) )->value == "Mission Year 2137", "late minimap lost restored year" );
				GT_ASSERT( static_cast< value::String* >( ui->Get( "text" ) )->value == ( scenario == 2 ? "WAITING FOR PLAYERS" : "TURN COMPLETE" ), "late minimap lost restored status" );
				GT_ASSERT( static_cast< value::Int* >( ui->Get( "plays" ) )->value == ( scenario == 1 ? 1 : 0 ) && static_cast< value::Int* >( ui->Get( "timers" ) )->value == ( scenario == 1 ? 1 : 0 ), "late minimap readiness presentation differs" );
				// Ordinary advancement must still invoke real unit-turn logic and all
				// gameplay hooks, clear completion, reset movement and publish normally.
				auto* turns_module = static_cast< value::Object* >( destination.RunScript( GSE_CALL, "./GLSMAC_data/default/units/turns" ) );
				static_cast< value::Callable* >( turns_module->Get( "configure" ) )->Run( GSE_CALL, { wrapper } );
				if ( scenario == 2 ) { game->GetState()->DetachConnection(); } // ordinary host-turn positive control
				game->AdvanceTurn( 38 );
				GT_ASSERT( turns == 1 && unit_turns == 1 && base_turns == 1 && game->GetTurnId() == 38, "ordinary advancement hooks changed" );
				GT_ASSERT( !local->IsTurnCompleted() && !remote->IsTurnCompleted() && !unit->m_moved_this_turn && unit->m_movement == 1.0f && unit->m_health == 0.625f, "ordinary advancement no longer resets gameplay state" );
				ui = static_cast< value::Object* >( read_ui->Run( GSE_CALL, {} ) );
				GT_ASSERT( static_cast< value::String* >( ui->Get( "year" ) )->value == "Mission Year 2138" && static_cast< value::String* >( ui->Get( "text" ) )->value == "TURN COMPLETE" && static_cast< value::Int* >( ui->Get( "timers" ) )->value == 0, "minimap stopped processing subsequent turn/status events" );
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "turn restoration: " + error ); GT_OK();
		} );
	}
	task->AddTest( "world turn restore validates signed ID and mission year boundaries", GT() {
		BackendFixture source; source.EnableWorldSnapshot(); source.EnableTurnPlayers(); source.SetSnapshotTurn( 37 );
		auto& settings = source.GetGame()->GetState()->m_settings.global;
		settings.SelectRuleset( "smax" ); settings.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( settings );
		const auto snapshot = source.WorldSnapshot();
		types::Buffer input( snapshot ); game::backend::world_snapshot::ValidateHeader( input, settings );
		game::backend::world_snapshot::ReadRandomState( input );
		const auto prefix = snapshot.substr( 0, input.lenr );
		game::backend::world_snapshot::ReadTurnState( input, source.GetGame()->GetState()->m_slots->GetSlots() );
		const auto body = snapshot.substr( input.lenr );
		const auto maximum = static_cast< int64_t >( std::min( static_cast< uint64_t >( std::numeric_limits< size_t >::max() ), static_cast< uint64_t >( INT64_MAX ) ) - 2100 );
		std::vector< std::pair< int64_t, std::string > > cases;
		for ( const auto value : { int64_t( 0 ), int64_t( 1 ), maximum, maximum + 1, int64_t( -1 ), INT64_MIN, INT64_MAX } ) {
			types::Buffer turn; turn.WriteString( "turn-state-v1" ); turn.WriteInt( value ); turn.WriteInt( 2 );
			turn.WriteInt( 0 ); turn.WriteBool( false ); turn.WriteInt( 2 ); turn.WriteBool( false );
			cases.emplace_back( value, prefix + turn.ToString() + body );
		}
		types::Buffer wrong_type; wrong_type.WriteString( "turn-state-v1" ); wrong_type.WriteString( "37" );
		cases.emplace_back( -1, prefix + wrong_type.ToString() );
		cases.emplace_back( -1, prefix ); cases.emplace_back( -1, snapshot.substr( 0, snapshot.size() - 1 ) );
		for ( const auto& test_case : cases ) {
			const auto turn = test_case.first;
			const auto& payload = test_case.second;
			BackendFixture fixture; fixture.EnableWorldSnapshot(); fixture.EnableTurnPlayers(); fixture.SetSnapshotTurn( 99 );
			auto* game = fixture.GetGame();
			game->GetState()->m_settings.global.SelectRuleset( "smax" ); game->GetState()->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
			const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				const auto before = game->GetRandom()->GetStateString();
				size_t turns = 0, starts = 0;
				game->On( GSE_CALL, "turn", NATIVE_CALL( &turns ) { ++turns; return VALUE( value::Undefined ); } ) );
				game->On( GSE_CALL, "start", NATIVE_CALL( &starts, game, turn ) {
					++starts; if ( game->GetTurnId() != static_cast< size_t >( turn ) ) { throw std::runtime_error( "start saw wrong turn" ); }
					return VALUE( value::Undefined );
				} ) );
				bool rejected = false;
				try { fixture.RestoreWorldBody( GSE_CALL, payload ); } catch ( const std::runtime_error& ) { rejected = true; }
				const bool valid = turn >= 0 && turn <= maximum;
				GT_ASSERT( rejected != valid && !turns, "invalid turn accepted or turn hook invoked" );
				if ( valid ) {
					GT_ASSERT( game->GetTurnId() == static_cast< size_t >( turn ) && fixture.HasPendingSnapshotTurn(), "valid boundary not staged" );
					fixture.CompleteWorldRestore( GSE_CALL );
					auto* wrapper = static_cast< value::Object* >( game->Wrap( GSE_CALL ) );
					auto* year = static_cast< value::Callable* >( wrapper->Get( "get_year" ) )->Run( GSE_CALL, {} );
					GT_ASSERT( starts == 1 && static_cast< value::Int* >( year )->value == turn + 2100, "mission year narrowed or overflowed" );
				}
				else {
					GT_ASSERT( game->GetTurnId() == 99 && !fixture.HasPendingSnapshotTurn() && !fixture.HasPendingSnapshotRandom() && !starts, "rejection mutated turn or staged continuation" );
					GT_ASSERT( game->GetRandom()->GetStateString() == before && fixture.IsPreparingMap(), "rejection completed restore or changed RNG" );
					for ( const auto& request : fixture.FrontendRequests() ) {
						GT_ASSERT( request.type != game::FrontendRequest::FR_TURN_ADVANCE && request.type != game::FrontendRequest::FR_TURN_STATUS, "rejected turn published to frontend" );
					}
				}
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "turn boundary " + std::to_string( turn ) + ": " + error );
		}
		GT_OK();
	} );
}

static void AddWorldRandomTests( task::gsetests::GSETests* task ) {
	using game::backend::base::Base;
	using game::backend::base::PopDef;
	for ( const auto* ruleset : { "smac", "smax" } ) {
		task->AddTest( std::string( "world RNG continuation installs after actual restore setup " ) + ruleset, GT( ruleset ) {
			const auto old_unit_id = Unit::GetNextId(), old_base_id = Base::GetNextId();
			util::FinallyGuard restore_ids( [ = ]() { Unit::SetNextId( old_unit_id ); Base::SetNextId( old_base_id ); } );
			for ( const auto state : { util::random::state_t{ 0, 0, 0, 0 }, util::random::state_t{ UINT32_MAX, 0, 2147483648u, 7 } } ) {
				BackendFixture source, destination;
				for ( auto* fixture : { &source, &destination } ) {
					fixture->EnableWorldSnapshot();
					auto& settings = fixture->GetGame()->GetState()->m_settings.global;
					settings.SelectRuleset( ruleset ); settings.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( settings );
				}
				source.SetRunning( true ); source.SetSnapshotTurn( 37 );
				source.DefinePop( "WORKER", "Worker", PopDef::PF_TILE_WORKER );
				source.AddBaseWithPop( 601, 9 );
				const auto source_error = source.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					DefineSnapshotUnit( source.GetGame()->GetUM() ); SpawnSnapshotUnit( GSE_CALL, source, 71 ); GT_OK();
				} );
				GT_ASSERT( source_error.empty(), "source setup failed: " + source_error );
				source.GetGame()->GetRandom()->SetState( state );
				const auto snapshot = source.WorldSnapshot();
				const auto error = destination.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
					auto* game = destination.GetGame(); auto* random = game->GetRandom();
					const auto before = random->GetStateString();
					destination.RestoreWorldBody( GSE_CALL, snapshot );
					GT_ASSERT( destination.HasPendingSnapshotRandom() && random->GetStateString() == before, "decode installed RNG before reconstruction" );
					GT_ASSERT( game->GetUM()->GetUnits().empty() && game->GetBM()->GetBases().empty(), "decoded instances bypassed deferred registration" );
					// The asset-free map skips renderer initialization. Model its draws
					// before exercising the real start and deferred instance sequence.
					for ( size_t i = 0 ; i < 11 ; ++i ) { random->GetUInt(); }
					size_t starts = 0, setup_spawns = 0;
					game->On( GSE_CALL, "start", NATIVE_CALL( &starts, random ) {
						++starts; random->GetUInt(); return VALUE( value::Undefined );
					} ) );
					game->GetUM()->On( GSE_CALL, "unit_spawn", NATIVE_CALL( &setup_spawns, random ) {
						++setup_spawns; random->GetUInt(); return VALUE( value::Undefined );
					} ) );
					// An extra setup-only unit forces a real pending-registration hook
					// draw after start, catching installation between these two phases.
					SpawnSnapshotUnit( GSE_CALL, destination, 88, 6 );
					destination.CompleteWorldRestore( GSE_CALL );
					GT_ASSERT( starts == 1 && setup_spawns == 1, "setup sequence or restored-unit hook suppression changed" );
					GT_ASSERT( !destination.HasPendingSnapshotRandom() && destination.IsRunning() && game->GetTurnId() == 37, "restore did not complete" );
					const auto* unit = game->GetUM()->GetUnit( 71 ); const auto* base = game->GetBM()->GetBase( 601 );
					GT_ASSERT( unit && unit->m_def == game->GetUM()->GetUnitDef( "TEST_UNIT" ) && unit->m_moved_this_turn && unit->m_movement == 0.375f, "actual unit reconstruction changed state or definition ownership" );
					GT_ASSERT( base && base->m_pops.size() == 1 && base->m_pops.count( 9 ), "actual base/pop reconstruction failed" );
					GT_ASSERT( random->GetStateString() == source.GetGame()->GetRandom()->GetStateString(), "setup draws changed saved continuation" );
					for ( size_t i = 0 ; i < 24 ; ++i ) {
						auto* expected = source.GetGame()->GetRandom();
						GT_ASSERT( random->GetUInt() == expected->GetUInt() && random->GetInt64( -1000, 1000 ) == expected->GetInt64( -1000, 1000 ) && random->GetFloat() == expected->GetFloat(), "resumed RNG sequence diverged" );
					}
					GT_OK();
				} );
				GT_ASSERT( error.empty(), "world RNG restore failed: " + error );
			}
			GT_OK();
		} );
	}
	task->AddTest( "world RNG ordinary initialization preserves its existing draw ordering", GT() {
		const auto old_id = Unit::GetNextId(); util::FinallyGuard restore_id( [ = ]() { Unit::SetNextId( old_id ); } );
		BackendFixture fixture; fixture.EnableWorldSnapshot();
		const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.GetGame(); auto* random = game->GetRandom();
			DefineSnapshotUnit( game->GetUM() ); SpawnSnapshotUnit( GSE_CALL, fixture, 71 );
			util::random::Random expected( 1 ); expected.SetState( random->GetState() );
			std::vector< uint32_t > draws;
			game->On( GSE_CALL, "start", NATIVE_CALL( &draws, random ) { draws.push_back( random->GetUInt() ); return VALUE( value::Undefined ); } ) );
			game->GetUM()->On( GSE_CALL, "unit_spawn", NATIVE_CALL( &draws, random ) { draws.push_back( random->GetUInt() ); return VALUE( value::Undefined ); } ) );
			fixture.CompleteWorldRestore( GSE_CALL );
			GT_ASSERT( draws.size() == 2 && draws[ 0 ] == expected.GetUInt() && draws[ 1 ] == expected.GetUInt(), "ordinary setup draw ordering changed" );
			for ( const auto& request : fixture.FrontendRequests() ) {
				GT_ASSERT( request.type != game::FrontendRequest::FR_TURN_ADVANCE && request.type != game::FrontendRequest::FR_TURN_STATUS, "ordinary initialization published a restored turn" );
			}
			GT_ASSERT( !fixture.HasPendingSnapshotRandom() && random->GetStateString() == expected.GetStateString(), "ordinary initialization reset RNG" );
			GT_OK();
		} );
		GT_ASSERT( error.empty(), "ordinary initialization failed: " + error ); GT_OK();
	} );
	task->AddTest( "world RNG reset cancellation and setup failure discard staged continuation", GT() {
		BackendFixture source; source.EnableWorldSnapshot();
		source.GetGame()->GetState()->m_settings.global.SelectRuleset( "smax" );
		source.GetGame()->GetState()->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
		source.GetGame()->GetRandom()->SetState( { UINT32_MAX, 1, 2, 3 } );
		const auto snapshot = source.WorldSnapshot();
		for ( const int scenario : { 0, 1, 2, 3, 4, 5 } ) {
			BackendFixture fixture; fixture.EnableWorldSnapshot();
			auto* game = fixture.GetGame();
			game->GetState()->m_settings.global.SelectRuleset( "smax" );
			game->GetState()->m_settings.global.InitializeHostRules( smac_rules_fixture::Source );
			const auto error = fixture.WithScriptValues( [ & ]( GSE_CALLABLE ) -> std::string {
				const auto before = game->GetRandom()->GetStateString();
				if ( scenario == 4 ) {
					bool rejected = false;
					try { fixture.RestoreWorldBody( GSE_CALL, snapshot.substr( 0, snapshot.size() - 1 ) ); }
					catch ( const std::runtime_error& ) { rejected = true; }
					GT_ASSERT( rejected, "truncated body was accepted" );
				}
				else {
					fixture.RestoreWorldBody( GSE_CALL, snapshot );
					GT_ASSERT( fixture.HasPendingSnapshotRandom(), "valid body did not stage continuation" );
					GT_ASSERT( fixture.HasPendingSnapshotTurn(), "valid body did not stage turn publication" );
					if ( scenario == 0 ) { fixture.ResetWorld(); }
					if ( scenario == 1 ) { fixture.CancelWorldInitialization(); }
					if ( scenario == 5 ) {
						auto* state = game->GetState();
						auto* client = new RulesetClient( gc_space, &state->m_settings.local );
						state->SetConnection( client );
						fixture.PrepareWorldDownload( client, 0 );
						client->ResetHandlers(); state->DetachConnection();
					}
					if ( scenario == 2 || scenario == 3 ) {
						game->On( GSE_CALL, "start", NATIVE_CALL( scenario ) {
							if ( scenario == 2 ) { throw std::runtime_error( "RNG setup failure" ); }
							GSE_ERROR( EC.GAME_ERROR, "RNG setup failure" );
						} ) );
						bool threw = false;
						try { fixture.CompleteWorldRestore( GSE_CALL ); }
						catch ( const std::runtime_error& ) { threw = true; }
						GT_ASSERT( threw == ( scenario == 2 ), "setup error path changed" );
					}
				}
				GT_ASSERT( !fixture.HasPendingSnapshotRandom() && game->GetRandom()->GetStateString() == before, "failure/reset installed or retained saved RNG" );
				GT_ASSERT( !fixture.HasPendingSnapshotTurn(), "failure/reset retained turn publication" );
				for ( const auto& request : fixture.FrontendRequests() ) {
					GT_ASSERT( request.type != game::FrontendRequest::FR_TURN_ADVANCE && request.type != game::FrontendRequest::FR_TURN_STATUS, "failure/reset published restored turn" );
				}
				if ( scenario == 0 || scenario == 1 || scenario == 3 ) { GT_ASSERT( !fixture.HasMap(), "real reset/cancel path retained map" ); }
				GT_OK();
			} );
			GT_ASSERT( error.empty(), "RNG discard scenario " + std::to_string( scenario ) + " failed: " + error );
		}
		GT_OK();
	} );
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

types::Buffer WorkerRecord( const std::vector< SavedWorker >& pops, const int64_t next_pop_id = 20, const int64_t base_id = 603 ) {
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
	return buf;
}

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
			snapshot.WriteString( "GLSMAC_BASES_V3" ); snapshot.WriteInt( 2 );
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

void AddBackendTests( task::gsetests::GSETests* task ) {
	for ( const bool active : { false, true } ) {
		for ( const bool reset_pending : { false, true } ) {
			for ( const bool retained_prompt : { false, true } ) {
				task->AddTest( std::string( "script run entry " ) + ( active ? "active" : "inactive" ) +
					( reset_pending ? " reset-pending" : " no-reset" ) + ( retained_prompt ? " retained-prompt" : " direct" ), GT( active, reset_pending, retained_prompt ) {
					BackendFixture fixture;
					const auto error = fixture.CheckScriptRunEntry( active, reset_pending, retained_prompt );
					GT_ASSERT( error.empty(), "run entry check: " + error );
					GT_OK();
				} );
			}
		}
	}
	AddLoaderProgressTests( task );
	AddSessionCallbackTests( task );
	AddWorldAdmissionTests( task );
	AddWorldTurnTests( task );
	AddWorldCompletionTests( task );
	AddWorldRandomTests( task );
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
			for ( const std::string format : { "", "GLSMAC_BASES_V1", "GLSMAC_BASES_V2", "GLSMAC_BASES_UNSUPPORTED" } ) {
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
