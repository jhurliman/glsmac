#include "Game.h"

#include "engine/Engine.h"
#include "types/Exception.h"
#include "types/texture/Texture.h"
#include "types/mesh/Render.h"
#include "types/mesh/Data.h"
#include "util/FS.h"
#include "util/FinallyGuard.h"
#include "types/Buffer.h"
#include "State.h"
#include "WorldSnapshotRules.h"
#include "WorldSnapshotWorkerKnowledge.h"
#include "WorldSnapshotRandom.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/faction/FactionManager.h"
#include "Random.h"
#include "config/Config.h"
#include "slot/Slots.h"
#include "Player.h"
#include "connection/Connection.h"
#include "connection/Server.h"
#include "connection/Client.h"
#include "map/Map.h"
#include "map/Consts.h"
#include "gse/value/String.h"
#include "gse/value/Int.h"
#include "gse/value/Undefined.h"
#include "gse/value/Array.h"
#include "map/tile/TileManager.h"
#include "map/tile/Tiles.h"
#include "map/MapState.h"
#include "resource/ResourceManager.h"
#include "Bindings.h"
#include "graphics/Graphics.h"
#include "animation/Def.h"
#include "unit/Def.h"
#include "unit/UnitManager.h"
#include "unit/Unit.h"
#include "unit/MoraleSet.h"
#include "base/BaseManager.h"
#include "base/PopDef.h"
#include "base/Base.h"
#include "animation/AnimationManager.h"
#include "gc/Space.h"
#include "game/backend/event/Event.h"
#include "game/backend/event/EventHandler.h"
#include "gse/value/Bool.h"
#include <limits>
#include <exception>
#include <utility>
#include <algorithm>

namespace game {
namespace backend {

class Game::EventBatchRoots final : public gc::Object {
public:
	EventBatchRoots( State* state, std::vector< event::Event* > events ) : gc::Object( nullptr ), state( state ), events( std::move( events ) ) {}
	void GetReachableObjects( std::unordered_set< gc::Object* >& reachable_objects ) override {
		gc::Object::GetReachableObjects( reachable_objects );
		GC_REACHABLE( state );
		for ( auto* event : events ) { GC_REACHABLE( event ); }
	}
	State* const state;
	const std::vector< event::Event* > events;
};

Game::EventBatchHandoff::EventBatchHandoff( Game* game, State* state, State::game_session_t session, std::shared_ptr< EventBatchRoots > roots )
	: game( game ), state( state ), session( std::move( session ) ), roots( std::move( roots ) ) {}

Game::EventBatchHandoff::~EventBatchHandoff() {
	if ( published ) {
		std::lock_guard guard( game->m_pending_events_mutex );
		const auto it = std::find( game->m_event_batch_handoffs.begin(), game->m_event_batch_handoffs.end(), roots );
		if ( it != game->m_event_batch_handoffs.end() ) { game->m_event_batch_handoffs.erase( it ); }
	}
}

response_map_data_t::~response_map_data_t() {
	if ( terrain_texture ) {
		DELETE( terrain_texture );
	}
	if ( terrain_mesh ) {
		DELETE( terrain_mesh );
	}
	if ( terrain_data_mesh ) {
		DELETE( terrain_data_mesh );
	}
};

// These owners may notify the renderer when destroyed. Detach them while
// native roots are excluded from collection, then release them after the guard.
struct Game::DetachedMapPresentation {
	response_map_data_t* response = nullptr;
	types::texture::Texture* texture = nullptr;
	types::mesh::Render* mesh = nullptr;
	types::mesh::Data* data = nullptr;
	map::MapState* state = nullptr;
	DetachedMapPresentation() = default;
	DetachedMapPresentation( const DetachedMapPresentation& ) = delete;
	DetachedMapPresentation& operator=( const DetachedMapPresentation& ) = delete;
	~DetachedMapPresentation() {
		if ( response ) { DELETE( response ); }
		if ( texture ) { DELETE( texture ); }
		if ( mesh ) { DELETE( mesh ); }
		if ( data ) { DELETE( data ); }
		if ( state ) { DELETE( state ); }
	}
};

Game::Game()
	: gse::GCWrappable( nullptr ), m_gameplay_entropy_word_source( util::random::Random::NewSeed ) {}

common::mt_id_t Game::MT_Ping() {
	MT_Request request = {};
	request.op = OP_PING;
	return MT_CreateRequest( request );
}

world_ticket_t Game::ReserveWorldTicket() {
	static std::atomic< world_ticket_t > next{ 0 };
	auto value = next.load();
	do {
		if ( value == std::numeric_limits< world_ticket_t >::max() ) { THROW( "world admission tickets exhausted" ); }
	} while ( !next.compare_exchange_weak( value, value + 1 ) );
	return value + 1;
}

pending_initialization_t Game::MT_Init( State* state, const world_ticket_t previous_world ) {
	if ( !state ) { THROW( "initialization requires a configured State" ); }
	std::lock_guard guard( m_admission_mutex );
	return EnqueueInitialization( state, previous_world );
}

pending_initialization_t Game::MT_LoadMap( const world_ticket_t world, const std::string& path ) {
	if ( path.empty() ) { THROW( "map path is empty" ); }
	std::lock_guard guard( m_admission_mutex );
	if ( !MatchesActiveAdmission( world ) ) { THROW( "map reload requires the active world ticket" ); }
	return EnqueueInitialization( m_active_admission->state, world, path );
}

pending_initialization_t Game::EnqueueInitialization( State* state, const world_ticket_t previous_world, std::optional< std::string > map_path ) {
	const auto ticket = ReserveWorldTicket();
	const auto admission = std::make_shared< WorldAdmission >( ticket, WorldAdmission::INITIALIZATION, state, state->CaptureGameSession(), previous_world, std::move( map_path ) );
	// Publish the real State root before the request can be consumed. The
	// immutable session token identifies a binding, but is not itself a GC root.
	m_admissions.emplace( ticket, admission );
	MT_Request request = {};
	request.op = OP_INIT;
	request.world_ticket = ticket;
	try {
		const auto id = MT_CreateRequest( request );
		admission->request = id;
		return { id, ticket };
	}
	catch ( ... ) { m_admissions.erase( ticket ); throw; }
}

void Game::ReleaseAdmissionRequest( const common::mt_id_t request ) {
	std::lock_guard guard( m_admission_mutex );
	for ( auto it = m_admissions.begin(); it != m_admissions.end(); ++it ) {
		if ( it->second->request == request ) { m_admissions.erase( it ); break; }
	}
}

MT_Response Game::MT_GetResponse( const common::mt_id_t request ) {
	const auto response = MTModule::MT_GetResponse( request );
	if ( response.result != R_NONE ) { ReleaseAdmissionRequest( request ); }
	return response;
}

MT_Response Game::MT_WaitResponse( const common::mt_id_t request ) {
	const auto response = MTModule::MT_WaitResponse( request );
	ReleaseAdmissionRequest( request );
	return response;
}

void Game::CancelAdmission( const std::shared_ptr< WorldAdmission >& admission ) {
	admission->cancelled = true;
	if ( m_active_admission == admission ) {
		m_init_cancel = true;
		if ( m_owner_transition && m_owner_transition->admission == admission ) {
			if ( m_owner_transition->phase == OwnerTransition::Running ) { return; }
			if ( m_owner_transition->phase == OwnerTransition::Pending ) { m_owner_transition->phase = OwnerTransition::Cancelled; }
		}
		admission->state->RetireGameCallbacks( admission->admitted_binding );
	}
}

void Game::MT_Cancel( const common::mt_id_t request ) {
	if ( !request ) { return; }
	{
		std::lock_guard guard( m_admission_mutex );
		for ( const auto& entry : m_admissions ) {
			if ( entry.second->request == request ) { CancelAdmission( entry.second ); break; }
		}
	}
	// Neither wait nor MT payload destruction may hold the admission mutex.
	MTModule::MT_Cancel( request );
	// MTModule leaves an already completed response for its caller to consume.
	const auto response = MTModule::MT_GetResponse( request );
	MTModule::MT_DestroyResponse( response );
	ReleaseAdmissionRequest( request );
}

bool Game::MatchesActiveAdmission( const world_ticket_t ticket ) const {
	return ticket && m_active_admission && m_active_admission->ticket == ticket
		&& !m_active_admission->cancelled && m_state == m_active_admission->state
		&& m_state->CaptureGameSession() == m_active_admission->admitted_binding
		&& m_state->IsGameSessionActive( m_active_admission->admitted_binding );
}

common::mt_id_t Game::MT_GetMapData( const world_ticket_t world ) {
	MT_Request request = {};
	request.op = OP_GET_MAP_DATA;
	request.world_ticket = world;
	return MT_CreateRequest( request );
}

common::mt_id_t Game::MT_Reset( const world_ticket_t world, const world_ticket_t pending_initialization ) {
	std::lock_guard guard( m_admission_mutex );
	MT_Request request = {};
	request.op = OP_RESET;
	request.world_ticket = world;
	request.data.reset.pending_initialization = pending_initialization;
	const auto id = MT_CreateRequest( request );
	RetireWorld( world, pending_initialization );
	return id;
}

void Game::MT_RetireWorld( const world_ticket_t world, const world_ticket_t pending_initialization ) {
	std::lock_guard guard( m_admission_mutex );
	RetireWorld( world, pending_initialization );
}

void Game::RetireWorld( const world_ticket_t world, const world_ticket_t pending_initialization ) {
	for ( const auto& entry : m_admissions ) {
		if ( entry.first == world || entry.first == pending_initialization ) { CancelAdmission( entry.second ); }
	}
	if ( m_active_admission && ( m_active_admission->ticket == world || m_active_admission->ticket == pending_initialization ) ) { CancelAdmission( m_active_admission ); }
	if ( m_failed_admission && ( m_failed_admission->ticket == world || m_failed_admission->ticket == pending_initialization ) ) {
		CancelAdmission( m_failed_admission );
		m_failed_admission.reset();
		m_failed_admission_error.clear();
	}
}

common::mt_id_t Game::MT_ResetController() {
	const auto barrier = MT_BeginResetController();
	try { return MT_SubmitResetController( barrier ); }
	catch ( ... ) { MT_AbandonResetController( barrier ); throw; }
}

world_ticket_t Game::MT_BeginResetController() {
	std::lock_guard guard( m_admission_mutex );
	const auto cutoff = ReserveWorldTicket();
	const auto admission = std::make_shared< WorldAdmission >( cutoff, WorldAdmission::CONTROLLER_RESET, m_state, m_state ? m_state->CaptureGameSession() : State::game_session_t{} );
	m_admissions.emplace( cutoff, admission );
	for ( const auto& entry : m_admissions ) {
		if ( entry.second->kind == WorldAdmission::INITIALIZATION ) { CancelAdmission( entry.second ); }
	}
	if ( m_active_admission ) { CancelAdmission( m_active_admission ); }
	m_failed_admission.reset();
	m_failed_admission_error.clear();
	return cutoff;
}

common::mt_id_t Game::MT_SubmitResetController( const world_ticket_t barrier ) {
	std::lock_guard guard( m_admission_mutex );
	const auto found = m_admissions.find( barrier );
	if ( found == m_admissions.end() || found->second->kind != WorldAdmission::CONTROLLER_RESET || found->second->cancelled ) {
		THROW( "reset controller barrier is retired or unknown" );
	}
	const auto& admission = found->second;
	if ( admission->request ) { return admission->request; }
	MT_Request request = {};
	request.op = OP_RESET;
	request.world_ticket = barrier;
	request.data.reset.controller = true;
	const auto id = MT_CreateRequest( request );
	admission->request = id;
	return id;
}

void Game::MT_AbandonResetController( const world_ticket_t barrier ) {
	common::mt_id_t request = 0;
	{
		std::lock_guard guard( m_admission_mutex );
		const auto found = m_admissions.find( barrier );
		if ( found == m_admissions.end() ) { return; }
		if ( found->second->kind != WorldAdmission::CONTROLLER_RESET ) { THROW( "not a reset controller barrier" ); }
		found->second->cancelled = true;
		request = found->second->request;
		if ( !request ) { m_admissions.erase( found ); }
	}
	if ( request ) { MT_Cancel( request ); }
}

common::mt_id_t Game::MT_SaveMap( const world_ticket_t world, const std::string& path ) {
	ASSERT( !path.empty(), "savemap path is empty" );
	MT_Request request = {};
	request.op = OP_SAVE_MAP;
	request.world_ticket = world;
	NEW( request.data.save_map.path, std::string, path );
	try { return MT_CreateRequest( request ); }
	catch ( ... ) { DestroyRequest( request ); throw; }
}

common::mt_id_t Game::MT_GetFrontendRequests( const world_ticket_t world ) {
	MT_Request request = {};
	request.op = OP_GET_FRONTEND_REQUESTS;
	request.world_ticket = world;
	return MT_CreateRequest( request );
}

common::mt_id_t Game::MT_SendBackendRequests( const world_ticket_t world, const std::vector< BackendRequest >& requests ) {
	MT_Request request = {};
	request.op = OP_SEND_BACKEND_REQUESTS;
	request.world_ticket = world;
	NEW( request.data.send_backend_requests.requests, std::vector< BackendRequest >, requests );
	try { return MT_CreateRequest( request ); }
	catch ( ... ) { DestroyRequest( request ); throw; }
}

#ifdef DEBUG
#define x( _method, _op ) \
    common::mt_id_t Game::_method( const std::string& path ) { \
        ASSERT( !path.empty(), "dump path is empty" ); \
        MT_Request request = {}; \
        request.op = _op; \
        NEW( request.data.dump.path, std::string ); \
        *request.data.dump.path = path; \
        return MT_CreateRequest( request ); \
    }

x( MT_SaveDump, OP_SAVE_DUMP )

x( MT_LoadDump, OP_LOAD_DUMP )

#undef x
#endif

void Game::Start() {
	MTModule::Start();

	MTModule::Log( "Starting thread" );

	m_game_state = GS_NONE;
	m_init_cancel = false;

	ASSERT( !m_pending_frontend_requests, "frontend requests already set" );
	NEW( m_pending_frontend_requests, std::vector< FrontendRequest > );

	NEW( m_random, Random, this );

	const auto* config = g_engine->GetConfig();
	if ( config->HasLaunchFlag( config::Config::LF_QUICKSTART_SEED ) ) {
		m_random->SetState( config->GetQuickstartSeed() );
	}

}

void Game::Stop() {
	MTModule::Log( "Stopping thread" );
	{
		std::lock_guard guard( m_admission_mutex );
		ASSERT( !m_owner_transition || m_owner_transition->phase != OwnerTransition::Running, "owner initialization still running at backend Stop" );
		if ( m_active_admission ) { CancelAdmission( m_active_admission ); }
	}

	// Preserve Stop's original no-transport-work contract: Network may already
	// be stopping. The borrowed Space survives State unpublication below.
	WithNativeRetirement( [ this ]() {
		if ( m_state ) { m_state->RetireGameCallbacks(); m_state = nullptr; }
	} );
	ResetGame();
	RetireOldMap();
	WithNativeRetirement( [ this ]() {
		DELETE( m_random );
		m_random = nullptr;
	} );
	DELETE( m_pending_frontend_requests );
	m_pending_frontend_requests = nullptr;

	MTModule::Stop();
	WithNativeRetirement( [ this ]() {
		std::lock_guard guard( m_admission_mutex );
		m_admissions.clear();
		m_failed_admission.reset();
		m_failed_admission_error.clear();
	} );
	m_world_space = nullptr;
}

void Game::Iterate() {
	if ( OwnerBlocksNative() ) { return; }
	// A completed native/ingress checkpoint may already have rejected this
	// world. Publish its owner failure before extracting any further requests.
	// A canceled admission is excluded here so its real MT reset can progress.
	if ( PublishInitializationFailure() ) { return; }
	MTModule::Iterate();
	{
		std::lock_guard guard( m_admission_mutex );
		if ( m_active_admission && m_active_admission->cancelled ) { return; }
	}

	if ( m_state ) {
		m_state->Iterate();
	}

	if ( AdvanceInitialization() ) { return; }

	if ( m_state ) {

		m_state->WithGame( this, [ this ]( Game* const, GSE_CALLABLE ) {
			if ( HasInitializationFailure() ) { return; }

			{
				std::lock_guard guard( m_pending_event_responses_mutex );
				for ( const auto& it : m_pending_event_responses ) {
					std::lock_guard guard2( m_events_waiting_for_responses_mutex );
					ASSERT( m_events_waiting_for_responses.find( it.event_id ) != m_events_waiting_for_responses.end(), "event for response not found" );
					const auto& event_data = m_events_waiting_for_responses.at( it.event_id );
					const auto* const event = event_data.event;
					const bool was_event_applied = event_data.was_applied;
					if ( !it.is_accepted ) {
						if ( was_event_applied ) {
							// event was rejected, rollback
							m_state->WithGame(
								this, [ this, event_data, event ]( Game* const, GSE_CALLABLE ) {
									auto* obj = VALUE( gse::value::Object, , GSE_CALL_NOGC, event->GetData() );
									if ( event_data.rollback_data ) {
										obj->Set( "applied", event_data.rollback_data, GSE_CALL );
									}
									const auto fargs = gse::value::function_arguments_t{ obj };
									event::EventHandler* h = nullptr;
									{
										std::lock_guard guard( m_event_handlers_mutex );
										const auto& it = m_event_handlers.find( event->GetEventName() );
										if ( it != m_event_handlers.end() ) {
											h = it->second;
										}
										else {
											MTModule::Log( "WARNING: tried to rollback event '" + event->GetEventName() + ", but found no handler" );
										}
									}
									if ( h ) {
										WithRW( [ &h, &ctx, &gc_space, &si, &ep, &fargs ]() {
											h->Rollback( GSE_CALL, fargs );
										});
									}
								}
							);
						}
					}
					else {
						// event was accepted (and maybe resolved)
						if ( !was_event_applied ) {
							m_state->WithGame(
								this, [ this, it, event ]( Game* const, GSE_CALLABLE ) {
									auto* obj = VALUE( gse::value::Object, , GSE_CALL_NOGC, event->GetData() );
									if ( it.resolved ) {
										obj->Set( "resolved", it.resolved, GSE_CALL );
									}
									const auto fargs = gse::value::function_arguments_t{ obj };
									event::EventHandler* h = nullptr;
									{
										std::lock_guard guard( m_event_handlers_mutex );
										const auto& it2 = m_event_handlers.find( event->GetEventName() );
										if ( it2 != m_event_handlers.end() ) {
											h = it2->second;
										}
										else {
											MTModule::Log( "WARNING: tried to apply event '" + event->GetEventName() + ", but found no handler" );
										}
									}
									if ( h ) {
										WithRW( [ &h, &ctx, &gc_space, &si, &ep, &fargs ]() {
											h->Apply( GSE_CALL, fargs );
										});
									}
								}
							);
						}
					}
					m_events_waiting_for_responses.erase( it.event_id );
				}
				m_pending_event_responses.clear();
			}
			ProcessEvents();
		});

		m_state->WithGame(
			this, [ this ]( Game* const, GSE_CALLABLE ) {
				if ( HasInitializationFailure() ) { return; }
				if ( m_bm ) {
					m_bm->TriggerUpdates( GSE_CALL );
				}
			}
		);

	}
	if ( m_um ) {
		m_um->PushUpdates();
	}
	if ( m_bm ) {
		m_bm->PushUpdates();
	}
	if ( m_tm ) {
		m_tm->ProcessTileLockRequests();
	}
}

const bool Game::IsStarted() const {
	return m_game_state != GS_NONE;
}

Random* Game::GetRandom() const {
	return m_random;
}

map::Map* Game::GetMap() const {
	ASSERT( m_map, "backend game map is null" );
	return m_map;
}

State* Game::GetState() const {
	ASSERT( m_state, "state not set" );
	return m_state;
}

const Player* Game::GetPlayer() const {
	ASSERT( m_state, "state not set" );
	if ( m_state->m_connection ) {
		return m_state->m_connection->GetPlayer();
	}
	else {
		return m_state->m_slots->GetSlot( 0 ).GetPlayer();
	}
}

const size_t Game::GetSlotNum() const {
	return m_slot_num;
}

void Game::ShowLoader( const std::string& text ) {
	auto fr = FrontendRequest( FrontendRequest::FR_LOADER_SHOW );
	NEW( fr.data.loader.text, std::string, text );
	AddFrontendRequest( fr );
}

void Game::SetLoaderText( const std::string& text ) {
	auto fr = FrontendRequest( FrontendRequest::FR_LOADER_TEXT );
	NEW( fr.data.loader.text, std::string, text );
	AddFrontendRequest( fr );
	if ( OwnerBlocksNative() ) { return; }
	// Map methods can still be on the stack. Deliver progress without running
	// reset or other operations that mutate/consume the world being prepared.
	ProcessRequests( []( const MT_Request& request ) noexcept {
		return request.op == OP_GET_FRONTEND_REQUESTS || request.op == OP_PING;
	} );
}

void Game::HideLoader() {
	auto fr = FrontendRequest( FrontendRequest::FR_LOADER_HIDE );
	AddFrontendRequest( fr );
}

void Game::AddEvent( event::Event* const event ) {
	std::lock_guard guard( m_pending_events_mutex );
	m_pending_events.push_back( event );
}

void Game::AddEventResponse( const std::string& event_id, const bool result, gse::Value* const resolved ) {
	std::lock_guard guard( m_pending_event_responses_mutex );
	m_pending_event_responses.push_back({
		event_id,
		result,
		resolved
	});
}

void Game::ClearEvents() {
	std::lock_guard guard( m_event_handlers_mutex );
/*#if defined( DEBUG ) || defined( FASTDEBUG )
	for ( const auto& it : m_event_handlers ) {
		g_engine->Log( "Removing event: \"" + it.first + "\"" );
	}
#endif*/
	m_event_handlers.clear();
}

void Game::Event( GSE_CALLABLE, const std::string& name, const gse::value::object_properties_t& args ) {
	if ( HandleSmacWorkerEvent( GSE_CALL, name, args, m_slot_num, false ) ) {
		AddEvent( new event::Event( this, event::Event::ES_LOCAL, m_slot_num, GSE_CALL, name, args ) );
		return;
	}
	RequireWorkerKnowledgeMutation();
	{
		std::lock_guard guard( m_event_handlers_mutex );
		const auto& it = m_event_handlers.find( name );
		if ( it == m_event_handlers.end() ) {
			return;
		}
	}
	AddEvent( new event::Event( this, event::Event::ES_LOCAL, m_slot_num, GSE_CALL, name, args ) );
}

WRAPIMPL_BEGIN( Game )
	WRAPIMPL_PROPS
	WRAPIMPL_TRIGGERS
	WRAPIMPL_CUSTOM_SETTERS
		{
			"is_master",
			NATIVE_CALL( this ) {
				return VALUE( gse::value::Bool, , m_state->IsMaster() );
			} ),
		},
		{
			"is_slave",
			NATIVE_CALL( this ) {
			return VALUE( gse::value::Bool, , m_state->IsSlave() );
		} ),
		},
		{
			"random",
			m_random->Wrap( GSE_CALL )
		},
		{
			"message",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE( text, 0, String );
				Message( text );
				return VALUE( gse::value::Undefined );
			})
		},
		{
			"get_player",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS_MIN_MAX( 0, 1 );
				size_t slot_id = m_slot_num;
				if ( arguments.size() > 0 ) {
					N_GETVALUE( id, 0, Int );
					slot_id = id;
				}
				auto& slots = m_state->m_slots->GetSlots();
				if ( slot_id < slots.size() ) {
					auto& slot = slots.at( slot_id );
					return slot.Wrap( GSE_CALL );
				}
				GSE_ERROR( gse::EC.GAME_ERROR, "Player id " + std::to_string( slot_id ) + " not found" );
			}),
		},
		{
			"get_players",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				auto& slots = m_state->m_slots->GetSlots();
				gse::value::array_elements_t elements = {};
				for ( auto& slot : slots ) {
					const auto state = slot.GetState();
					if ( state == slot::Slot::SS_OPEN || state == slot::Slot::SS_CLOSED ) {
						continue; // skip
					}
					elements.push_back( slot.Wrap( GSE_CALL ) );
				}
				return VALUE( gse::value::Array,, elements );
			} )
		},
		{
			"get_simulation_faction_id",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE_UNWRAP( player, 0, Player );
				try { return VALUE( gse::value::Int,, RequireSimulationParticipant( player ).simulation_id ); }
				catch ( const std::runtime_error& error ) { GSE_ERROR( gse::EC.GAME_ERROR, error.what() ); }
			} )
		},
		{
			"get_technology_state",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE_UNWRAP( player, 0, Player );
				try { return CopyTechnologyState( GSE_CALL, player ); }
				catch ( const std::runtime_error& error ) { GSE_ERROR( gse::EC.GAME_ERROR, error.what() ); }
			} )
		},
		{
			"get_social_state",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE_UNWRAP( player, 0, Player );
				try { return CopySocialState( GSE_CALL, player ); }
				catch ( const std::runtime_error& error ) { GSE_ERROR( gse::EC.GAME_ERROR, error.what() ); }
			} )
		},
		{
			"get_startup_designs",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE_UNWRAP( player, 0, Player );
				try { return CopyStartupDesigns( GSE_CALL, player ); }
				catch ( const std::runtime_error& error ) { GSE_ERROR( gse::EC.GAME_ERROR, error.what() ); }
			} )
		},
		{
			"get_turn",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				return VALUE( gse::value::Int,, m_current_turn.GetId() );
			} )
		},
		{
			"get_turn_status",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				return VALUE( gse::value::String,, GetTurnStatusString() );
			} )
		},
		{
			"get_year",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				return VALUE( gse::value::Int,, m_current_turn.GetId() + 2100 /* TODO: better way to define starting year? */ );
			} )
		},
		{
			"is_turn_complete",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE( slot_id, 0, Int );

				const auto& slots = m_state->m_slots->GetSlots();
				if ( slot_id >= slots.size() ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Player id " + std::to_string( slot_id ) + " not found" );
				}

				const auto& slot = slots.at( slot_id );
				ASSERT( slot.GetState() != slot::Slot::SS_PLAYER || slot.GetPlayer(), "player is null" );
				return VALUE( gse::value::Bool,,
					slot.GetState() == slot::Slot::SS_PLAYER
						? slot.GetPlayer()->IsTurnCompleted()
						: true // ai has always turn completed during turns of players
				);
			} )
		},
		{
			"complete_turn",
			NATIVE_CALL( this ) {

				CheckRW( GSE_CALL );

				N_EXPECT_ARGS( 1 );
				N_GETVALUE( slot_id, 0, Int );

				const auto& slots = m_state->m_slots->GetSlots();
				if ( slot_id >= slots.size() ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Player id " + std::to_string( slot_id ) + " not found" );
				}

				const auto& slot = slots.at( slot_id );
				if ( slot.GetState() != slot::Slot::SS_PLAYER ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Player id " + std::to_string( slot_id ) + " is not player" );
				}
				ASSERT( slot.GetPlayer(), "player is null" );
				ASSERT( !slot.GetPlayer()->IsTurnCompleted(), "player turn already completed" );
				CompleteTurn( GSE_CALL, slot_id );

				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"uncomplete_turn",
			NATIVE_CALL( this ) {

				CheckRW( GSE_CALL );

				N_EXPECT_ARGS( 1 );
				N_GETVALUE( slot_id, 0, Int );

				const auto& slots = m_state->m_slots->GetSlots();
				if ( slot_id >= slots.size() ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Player id " + std::to_string( slot_id ) + " not found" );
				}

				const auto& slot = slots.at( slot_id );
				if ( slot.GetState() != slot::Slot::SS_PLAYER ) {
					GSE_ERROR( gse:: EC.GAME_ERROR, "Player id " + std::to_string( slot_id ) + " is not player" );
				}
				ASSERT( slot.GetPlayer(), "player is null" );
				ASSERT( slot.GetPlayer()->IsTurnCompleted(), "player turn not completed" );
				UncompleteTurn( slot_id );

				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"advance_turn",
			NATIVE_CALL( this ) {

				CheckRW( GSE_CALL );

				N_EXPECT_ARGS( 1 );
				N_GETVALUE( turn_id, 0, Int );

				AdvanceTurn( turn_id );

				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"get_ruleset",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				if ( !m_state ) { GSE_ERROR( gse::EC.GAME_ERROR, "Game not initialized" ); }
				return VALUE( gse::value::String,, m_state->m_settings.global.GetRulesetId() );
			} )
		},
		{
			"get_settings", // deprecated
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				if ( !m_state ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Game not initialized" );
				}
				return m_state->m_settings.Wrap( GSE_CALL );
			} )
		},
		{
			"get_map",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				if ( !m_map ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Map not initialized" );
				}
				return m_map->Wrap( GSE_CALL );
			} )
		},
		{
			"register_event",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 2 );
				N_GETVALUE( name, 0, String );
				{
					std::lock_guard guard( m_event_handlers_mutex );
					if ( m_event_handlers.find( name ) != m_event_handlers.end() ) {
						GSE_ERROR( gse::EC.INVALID_HANDLER, "Event already registered: " + name );
					}
				}
				N_GETVALUE( def, 1, Object );
				auto validate_it = def.find( "validate" );
				if ( validate_it == def.end() || validate_it->second->type != gse::VT_CALLABLE ) {
					GSE_ERROR( gse::EC.INVALID_HANDLER, "Event handler does not provide validate method" );
				}
				gse::value::Callable* resolve = nullptr;
				auto resolve_it = def.find( "resolve" );
				if ( resolve_it != def.end() ) {
					if ( resolve_it->second->type != gse::VT_CALLABLE ) {
						GSE_ERROR( gse::EC.INVALID_HANDLER, "Event handler does not provide resolve method" );
					}
					resolve = (gse::value::Callable*)resolve_it->second;
				}
				auto apply_it = def.find( "apply" );
				if ( apply_it == def.end() || apply_it->second->type != gse::VT_CALLABLE ) {
					GSE_ERROR( gse::EC.INVALID_HANDLER, "Event handler does not provide apply method" );
				}
				auto rollback_it = def.find( "rollback" );
				if ( rollback_it == def.end() || rollback_it->second->type != gse::VT_CALLABLE ) {
					GSE_ERROR( gse::EC.INVALID_HANDLER, "Event handler does not provide rollback method" );
				}
				{
					std::lock_guard guard( m_event_handlers_mutex );
					m_event_handlers.insert(
						{ name, new event::EventHandler(
							gc_space, name,
							(gse::value::Callable*)validate_it->second,
							resolve,
							(gse::value::Callable*)apply_it->second,
							(gse::value::Callable*)rollback_it->second
						) }
					);
				}
				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"event",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 2 );
				N_GETVALUE( name, 0, String );
				N_GET( args, 1, Object );
				if ( !args->object_class.empty() ) {
					GSE_ERROR( gse::EC.GAME_ERROR, "Invalid event data - expected: primitive object, found: " + args->object_class );
				}
				if ( !HandleSmacWorkerEvent( GSE_CALL, name, args->value, m_slot_num, false ) ) {
					RequireWorkerKnowledgeMutation();
					std::lock_guard guard( m_event_handlers_mutex );
					if ( m_event_handlers.find( name ) == m_event_handlers.end() ) {
						GSE_ERROR( gse::EC.INVALID_HANDLER, "Unknown event: " + name );
					}
				}
				AddEvent( new event::Event( this, event::Event::ES_LOCAL, m_slot_num, GSE_CALL, name, args->value ) );
				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"get_fm",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				return m_state
					? m_state->GetFM()->Wrap( GSE_CALL )
					: VALUE( gse::value::Undefined )
					;
			} )
		},
#define X( _x ) \
		{ \
			"get_" # _x, \
			NATIVE_CALL( this ) { \
				N_EXPECT_ARGS( 0 ); \
				return m_##_x \
					? m_##_x->Wrap( GSE_CALL ) \
					: VALUE( gse::value::Undefined ) \
					; \
			} ) \
		},
		X( um )
		X( bm )
		X( rm )
		X( tm )
#undef X
		{
			"is_started",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 0 );
				return VALUE( gse::value::Bool,, m_game_state != GS_NONE );
			} )
		},
		{
			"select_tile",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE_UNWRAP( tile, 0, map::tile::Tile );
				auto fr = FrontendRequest( FrontendRequest::FR_TILE_SELECT );
				fr.data.tile_select.x = tile->coord.x;
				fr.data.tile_select.y = tile->coord.y;
				AddFrontendRequest( fr );
				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"select_unit",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE_UNWRAP( unit, 0, unit::Unit );
				unit->RequireConventionalReady();
				auto fr = FrontendRequest( FrontendRequest::FR_UNIT_SELECT );
				fr.data.unit_select.unit_id = unit->m_id;
				AddFrontendRequest( fr );
				return VALUE( gse::value::Undefined );
			} )
		},
		{
			"select_base",
			NATIVE_CALL( this ) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE_UNWRAP( base, 0, base::Base );
				auto fr = FrontendRequest( FrontendRequest::FR_BASE_SELECT );
				fr.data.base_select.base_id = base->m_id;
				AddFrontendRequest( fr );
				return VALUE( gse::value::Undefined );
			} )
		},
	};
	if ( m_tm ) {
		properties.insert(
			{
				"tm",
				m_tm->Wrap( GSE_CALL )
			}
		);
	}
	if ( m_rm ) {
		properties.insert(
			{
				"rm",
				m_rm->Wrap( GSE_CALL )
			}
		);
	}
	if ( m_um ) {
		properties.insert(
			{
				"um",
				m_um->Wrap( GSE_CALL )
			}
		);
	}
	if ( m_bm ) {
		properties.insert(

			{
				"bm",
				m_bm->Wrap( GSE_CALL ),
			}
		);
	}
	if ( m_am ) {
		properties.insert(
			{
				"am",
				m_am->Wrap( GSE_CALL )
			}
		);
	}
WRAPIMPL_END_PTR()

UNWRAPIMPL_PTR( Game )

void Game::GetReachableObjects( std::unordered_set< Object* >& reachable_objects ) {
	gse::GCWrappable::GetReachableObjects( reachable_objects );

	GC_DEBUG_BEGIN( "Game" );
	{
		std::lock_guard guard( m_admission_mutex );
		for ( const auto& entry : m_admissions ) {
			if ( entry.second->state ) { GC_REACHABLE( entry.second->state ); }
		}
		if ( m_active_admission ) { GC_REACHABLE( m_active_admission->state ); }
		if ( m_failed_admission ) { GC_REACHABLE( m_failed_admission->state ); }
		if ( m_active_admission ) {
			if ( m_active_admission->connection ) { GC_REACHABLE( m_active_admission->connection ); }
			for ( const auto& entry : m_active_admission->ingress ) { if ( entry.owner ) { GC_REACHABLE( entry.owner ); } }
		}
		if ( m_owner_transition ) { GC_REACHABLE( m_owner_transition.get() ); }
		for ( const auto& transition : m_owner_transition_handoffs ) { GC_REACHABLE( transition.get() ); }
	}
	{
		std::lock_guard guard( m_snapshot_turn_mutex );
		if ( m_pending_snapshot_turn ) {
			for ( const auto& binding : m_pending_snapshot_turn->bindings ) { GC_REACHABLE( binding.player ); }
		}
	}

	if ( m_tm ) {
		GC_REACHABLE( m_tm );
	}
	if ( m_rm ) {
		GC_REACHABLE( m_rm );
	}
	if ( m_um ) {
		GC_REACHABLE( m_um );
	}
	if ( m_bm ) {
		GC_REACHABLE( m_bm );
	}
	if ( m_am ) {
		GC_REACHABLE( m_am );
	}

	if ( m_map ) {
		GC_REACHABLE( m_map );
	}
	if ( m_old_map ) {
		GC_REACHABLE( m_old_map );
	}

	GC_DEBUG_BEGIN( "events" );
	{
		std::lock_guard guard( m_pending_events_mutex );
		for ( const auto& event : m_pending_events ) {
			GC_REACHABLE( event );
		}
		for ( const auto& batch : m_event_batch_handoffs ) {
			GC_REACHABLE( batch.get() );
		}
	}
	GC_DEBUG_END();

	GC_DEBUG_BEGIN( "event_handlers" );
	{
		std::lock_guard guard( m_event_handlers_mutex );
		for ( const auto& it : m_event_handlers ) {
			GC_REACHABLE( it.second );
		}
	}
	GC_DEBUG_END();

	if ( !m_events_waiting_for_responses.empty() ) {
		std::lock_guard guard( m_events_waiting_for_responses_mutex );
		GC_DEBUG_BEGIN( "events_waiting_for_responses" );
		for ( const auto& it : m_events_waiting_for_responses ) {
			GC_REACHABLE( it.second.event );
			if ( it.second.rollback_data ) {
				GC_REACHABLE( it.second.rollback_data );
			}
		}
		GC_DEBUG_END();
	}

	if ( !m_pending_event_responses.empty() ) {
		std::lock_guard guard( m_pending_event_responses_mutex );
		GC_DEBUG_BEGIN( "pending_event_responses" );
		for ( const auto& it : m_pending_event_responses ) {
			if ( it.resolved ) {
				GC_REACHABLE( it.resolved );
			}
		}
		GC_DEBUG_END();
	}

	GC_DEBUG_END();
}

const MT_Response Game::ProcessRequest( const MT_Request& request, MT_CANCELABLE ) {
	MT_Response response = {};
	response.op = request.op;
	response.world_ticket = request.world_ticket;
	if ( request.op == OP_GET_MAP_DATA || request.op == OP_SAVE_MAP ||
		request.op == OP_GET_FRONTEND_REQUESTS || request.op == OP_SEND_BACKEND_REQUESTS ) {
		std::lock_guard guard( m_admission_mutex );
		if ( !MatchesActiveAdmission( request.world_ticket ) ) {
			if ( request.op == OP_GET_MAP_DATA && m_failed_admission &&
				m_failed_admission->ticket == request.world_ticket && !m_failed_admission->cancelled ) {
				response.result = R_ERROR;
				NEW( response.data.error.error_text, std::string, m_failed_admission_error );
			}
			else { response.result = R_ABORTED; }
			return response;
		}
	}

	switch ( request.op ) {
		case OP_INIT: {
			//MTModule::Log( "Got init request" );

			std::shared_ptr< WorldAdmission > admission;
			{
				std::lock_guard guard( m_admission_mutex );
				const auto found = m_admissions.find( request.world_ticket );
				if ( found != m_admissions.end() ) { admission = found->second; }
				if ( !admission || admission->kind != WorldAdmission::INITIALIZATION || admission->cancelled ||
					m_state != admission->state || m_state->CaptureGameSession() != admission->binding ||
					!m_state->IsGameSessionActive( admission->binding ) ||
					( admission->previous ? !MatchesActiveAdmission( admission->previous ) : !!m_active_admission ) ) {
					response.result = R_ABORTED;
					break;
				}
			}
			try { InitGame( admission, response, MT_C ); }
			catch ( const std::exception& error ) {
				// A native/immediate initialization failure must still finish the
				// MT request. Deferred owner failures are handled by their own path.
				{
					std::lock_guard guard( m_admission_mutex );
					CancelAdmission( admission );
				}
				response.result = R_ERROR;
				NEW( response.data.error.error_text, std::string, error.what() );
			}
			if ( response.result == R_SUCCESS ) { response.data.init.slot_index = m_slot_num; }
			break;
		}
		case OP_GET_MAP_DATA: {
			//MTModule::Log( "Got get-map-data request" );
			if ( m_game_state != GS_RUNNING ) {
				if ( m_initialization_error.empty() ) {
					response.result = R_PENDING;
				}
				else {
					response.result = R_ERROR;
					NEW( response.data.error.error_text, std::string, m_initialization_error );
				}
			}
			else if ( m_response_map_data ) {
				response.result = R_SUCCESS;
				response.data.get_map_data = m_response_map_data;
				m_response_map_data = nullptr;
				// ownership transferred to frontend
				m_map->m_textures.terrain = nullptr;
				m_map->m_meshes.terrain = nullptr;
				m_map->m_meshes.terrain_data = nullptr;
			}
			else if ( m_init_cancel ) {
				response.result = R_ABORTED;
			}
			else {
				response.result = R_ERROR;
				NEW( response.data.error.error_text, std::string, m_initialization_error );
			}
			break;
		}
		case OP_RESET: {
			//MTModule::Log( "Got reset request" );
			bool reset = false;
			{
				std::lock_guard guard( m_admission_mutex );
				if ( request.data.reset.controller ) {
					const auto found = m_admissions.find( request.world_ticket );
					if ( found != m_admissions.end() && found->second->kind == WorldAdmission::CONTROLLER_RESET && !found->second->cancelled ) {
						const auto& controller = found->second;
						if ( m_active_admission ) {
							reset = m_active_admission->ticket < controller->ticket && m_state == m_active_admission->state &&
								m_state->CaptureGameSession() == m_active_admission->admitted_binding;
						}
						else {
							reset = m_state == controller->state && ( !m_state || m_state->CaptureGameSession() == controller->binding );
						}
					}
				}
				else if ( m_active_admission ) {
					reset = ( m_active_admission->ticket == request.world_ticket ||
						m_active_admission->ticket == request.data.reset.pending_initialization ) &&
						m_state == m_active_admission->state && m_state->CaptureGameSession() == m_active_admission->admitted_binding;
				}
			}
			if ( reset ) { ResetGame(); }
			response.result = R_SUCCESS;
			break;
		}
		case OP_PING: {
			//MTModule::Log( "Got ping request" );
			response.result = R_SUCCESS;
			break;
		}
		case OP_SAVE_MAP: {
			//MTModule::Log( "got save map request" );
			if ( m_game_state != GS_RUNNING || !m_map ) { response.result = R_ABORTED; break; }
			const auto ec = m_map->SaveToFile( *request.data.save_map.path );
			if ( ec ) {
				response.result = R_ERROR;
				NEW( response.data.error.error_text, std::string, map::Map::GetErrorString( ec ) );
			}
			else {
				response.result = R_SUCCESS;
				NEW( response.data.save_map.path, std::string );
				*response.data.save_map.path = *request.data.save_map.path;
			}
			break;
		}
		case OP_GET_FRONTEND_REQUESTS: {
			//MTModule::Log( "got events request" );
			if ( !m_pending_frontend_requests->empty() ) {
				//MTModule::Log( "Sending " + std::to_string( m_pending_frontend_requests->size() ) + " events to frontend" );
				response.data.get_frontend_requests.requests = m_pending_frontend_requests; // will be destroyed in DestroyResponse
				NEW( m_pending_frontend_requests, std::vector< FrontendRequest > ); // reset
			}
			else {
				response.data.get_frontend_requests.requests = nullptr;
			}
			response.result = R_SUCCESS;
			break;
		}
		case OP_SEND_BACKEND_REQUESTS: {
			if ( m_game_state != GS_RUNNING || !m_am ) { response.result = R_ABORTED; break; }
			try {
				for ( const auto& r : *request.data.send_backend_requests.requests ) {
					switch ( r.type ) {
						case BackendRequest::BR_ANIMATION_FINISHED: {
							m_state->WithGame( this, [ this, id = r.data.animation_finished.animation_id ]( Game* const, GSE_CALLABLE ) {
								m_am->FinishAnimation( id );
							});
							break;
						}
						default:
							THROW( "unknown backend request type: " + std::to_string( r.type ) );
					}
				}
				response.result = R_SUCCESS;
			}
			catch ( const gse::Exception& e ) {
				OnGSEError( e );
				response.result = R_ERROR;
			}
			catch ( std::runtime_error& e ) {
				OnError( e );
				response.result = R_ERROR;
			}
			break;
		}
		case OP_ADD_EVENT: {
			THROW( "deprecated: OP_ADD_EVENT" );
			/*m_state->WithGSE( [ this, &event, &errmsg, &buf ]( GSE_CALLABLE ) {
				event = event::Event::Deserialize( GSE_CALL, buf );
				errmsg = event->Validate( GSE_CALL, this );
				if ( errmsg ) {
					// log and do nothing
					MTModule::Log( "Event declined: " + *errmsg );
					delete errmsg;
					delete event;
				}
				else {
					AddEvent( GSE_CALL, event );
				}
			});*/
			break;
		}
		default: {
			THROW( "unknown request op " + std::to_string( request.op ) );
		}
	}

	return response;
}

void Game::DestroyRequest( const MT_Request& request ) {
	switch ( request.op ) {
		case OP_SAVE_MAP: {
			if ( request.data.save_map.path ) {
				DELETE( request.data.save_map.path );
			}
			break;
		}
		case OP_ADD_EVENT: {
			DELETE( request.data.add_event.serialized_event );
			break;
		}
		case OP_SEND_BACKEND_REQUESTS: {
			if ( request.data.send_backend_requests.requests ) {
				DELETE( request.data.send_backend_requests.requests );
			}
		}
		default: {
			// nothing to delete
		}
	}
}

void Game::DestroyResponse( const MT_Response& response ) {
	if ( response.result == R_ERROR && response.data.error.error_text ) {
		DELETE( response.data.error.error_text );
	}
	if ( response.result == R_SUCCESS ) {
		switch ( response.op ) {
			case OP_GET_MAP_DATA: {
				if ( response.data.get_map_data ) {
					DELETE( response.data.get_map_data );
				}
				break;
			}
			case OP_SAVE_MAP: {
				if ( response.data.save_map.path ) {
					DELETE( response.data.save_map.path );
				}
				break;
			}
			case OP_GET_FRONTEND_REQUESTS: {
				if ( response.data.get_frontend_requests.requests ) {
					DELETE( response.data.get_frontend_requests.requests );
				}
				break;
			}
			default: {
				// nothing to delete
			}
		}
	}
}

void Game::Message( const std::string& text ) {
	WithWorldIngress( m_state, this, [ this, text ]( Game* const, GSE_CALLABLE ){
		m_state->TriggerObject(
			this, "message", ARGS_F( text ) {
				{ "text", VALUE( gse::value::String,, text ) }
			}; }
		);
	});
}

void Game::Quit( const std::string& reason ) {
	auto fr = FrontendRequest( FrontendRequest::FR_QUIT );
	NEW( fr.data.quit.reason, std::string, "Lost connection to server" );
	AddFrontendRequest( fr );
}

void Game::OnError( std::runtime_error& err ) {
	auto fr = FrontendRequest( FrontendRequest::FR_ERROR );
	NEW( fr.data.error.what, std::string, (std::string)"Script error: " + err.what() );
	fr.data.error.stacktrace = nullptr;
	AddFrontendRequest( fr );
}

void Game::OnGSEError( const gse::Exception& err ) {
	auto fr = FrontendRequest( FrontendRequest::FR_ERROR );
	NEW( fr.data.error.what, std::string, (std::string)"Script error: " + err.what() );
	NEW( fr.data.error.stacktrace, std::string, err.ToString() );
	AddFrontendRequest( fr );
}

const size_t Game::GetTurnId() const {
	return m_current_turn.GetId();
}

const bool Game::IsTurnCompleted( const size_t slot_num ) const {
	const auto& slot = m_state->m_slots->GetSlot( slot_num );
	ASSERT( slot.GetState() == slot::Slot::SS_PLAYER, "slot is not player" );
	const auto* player = slot.GetPlayer();
	ASSERT( player, "slot player not set" );
	return player->IsTurnCompleted();
}

const bool Game::IsTurnChecksumValid( const util::crc32::crc_t checksum ) const {
	return m_turn_checksum == checksum;
}

void Game::CompleteTurn( GSE_CALLABLE, const size_t slot_num ) {
	RequireWorkerKnowledgeMutation();
	const auto& slot = m_state->m_slots->GetSlot( slot_num );
	ASSERT( slot.GetState() == slot::Slot::SS_PLAYER, "slot is not player" );
	auto* player = slot.GetPlayer();
	ASSERT( player, "slot player not set" );
	player->CompleteTurn();
	SetTurnStatus( turn::TS_WAITING_FOR_PLAYERS );
}

void Game::UncompleteTurn( const size_t slot_num ) {
	const auto& slot = m_state->m_slots->GetSlot( slot_num );
	ASSERT( slot.GetState() == slot::Slot::SS_PLAYER, "slot is not player" );
	auto* player = slot.GetPlayer();
	ASSERT( player, "slot player not set" );
	player->UncompleteTurn();
	if ( slot_num == m_slot_num ) {
		m_is_turn_complete = false;
		CheckTurnComplete();
		if ( !m_is_turn_complete ) {
			SetTurnStatus( turn::TS_TURN_ACTIVE );
		}
	}
}

void Game::AdvanceTurn( const size_t turn_id ) {
	RequireWorkerKnowledgeMutation();
	// Stage the first-command social refresh before publishing the new turn or
	// invoking any upkeep consumer. A malformed context leaves both unchanged.
	RefreshSimulationForTurn( turn_id );
	m_current_turn.AdvanceTurn( turn_id );
	m_is_turn_complete = false;
	MTModule::Log( "Turn started: " + std::to_string( turn_id ) );

	{
		auto fr = FrontendRequest( FrontendRequest::FR_TURN_ADVANCE );
		fr.data.turn_advance.turn_id = turn_id;
		AddFrontendRequest( fr );
	}

	m_state->WithGame( this, [ this ]( Game* const, GSE_CALLABLE ) {
		for ( auto& it : m_um->GetUnits() ) {
			auto* unit = it.second;
			if ( unit->IsConventional() ) {
				// Captured spent reset only: no healing or failure-marker clearing.
				m_um->ResetConventionalSpent( GSE_CALL, unit );
				continue;
			}
			m_state->TriggerObject(
				m_um, "unit_turn", ARGS_F( &unit ) {
					{
						"unit",
						unit->Wrap( GSE_CALL )
					},
				}; }
			);
			unit->Legacy().moved_this_turn = false;
			m_um->RefreshUnit( GSE_CALL, unit );
		}

		for ( auto& it : m_bm->GetBases() ) {
			auto* base = it.second;
			m_state->TriggerObject(
				m_bm, "base_turn", ARGS_F( &base ) {
					{
						"base",
						base->Wrap( GSE_CALL )
					},
				}; }
			);
			m_bm->RefreshBase( base );
		}

		if ( m_state->IsMaster() ) {
			m_state->TriggerObject( this, "turn", ARGS_F( this ) {
				{
					"year",
					VALUE( gse::value::Int,, m_current_turn.GetId() + 2100 /* TODO: better way to define starting year? */ ),
				},
			}; } );
		}
	});

	for ( const auto& slot : m_state->m_slots->GetSlots() ) {
		if ( slot.GetState() == slot::Slot::SS_PLAYER ) {
			slot.GetPlayer()->UncompleteTurn();
		}
	}

	m_is_turn_complete = false;
	CheckTurnComplete();
	if ( !m_is_turn_complete ) {
		SetTurnStatus( turn::TS_TURN_ACTIVE );
	}

}

void Game::GlobalFinalizeTurn( GSE_CALLABLE ) {
	RequireWorkerKnowledgeMutation();
	ASSERT( m_state->IsMaster(), "not master" );
	ASSERT( m_verified_turn_checksum_slots.empty(), "turn finalization slots not empty" );
	MTModule::Log( "Finalizing turn ( checksum = " + std::to_string( m_turn_checksum ) + " )" );
	THROW( "TODO: GLOBAL FINALIZE TURN");
	//AddEvent( GSE_CALL, new event::FinalizeTurn( m_slot_num ) );
}

void Game::FirstTurn( GSE_CALLABLE ) {
	RequireWorkerKnowledgeMutation();
	ASSERT( m_state->IsMaster(), "not master" );

	// Startup events already queued by create_world/start still belong to turn 0.
	// Let the advance_turn event change the live ID when that event is applied.
	m_verified_turn_checksum_slots.clear();
	m_turn_checksum = 0;

	MTModule::Log( "Advancing turn ( id = 1 )" );
	m_state->WithGame( this, [ this ]( Game* const, GSE_CALLABLE ){
		Event( GSE_CALL, "advance_turn", {
			{ "turn_id", VALUE( gse::value::Int,, 1 ) }
		} );
	});
}

faction::Faction* Game::GetFaction( const std::string& id ) const {
	auto* faction = m_state->GetFM()->Get( id ); // TODO: store factions in Game itself?
	ASSERT( faction, "faction not found: " + id );
	return faction;
}

map::tile::TileManager* Game::GetTM() const {
	return m_tm;
}

resource::ResourceManager* Game::GetRM() const {
	return m_rm;
}

unit::UnitManager* Game::GetUM() const {
	return m_um;
}

base::BaseManager* Game::GetBM() const {
	return m_bm;
}

animation::AnimationManager* Game::GetAM() const {
	return m_am;
}

gc::Space* const Game::GetGCSpace() const {
	ASSERT( m_state, "state not set" );
	ASSERT( m_state->m_gc_space, "state gc space not set" );
	return m_state->m_gc_space;
}

const types::Vec3 Game::GetTileRenderCoords( const map::tile::Tile* tile ) {
	const auto* ts = m_map->GetTileState( tile );
	ASSERT( ts, "ts not set" );
	const auto l = tile->is_water_tile
		? map::tile::LAYER_WATER
		: map::tile::LAYER_LAND;
	const auto& layer = ts->layers[ l ];
	const auto& c = layer.coords.center;
	return {
		c.x,
		-c.y,
		c.z
	};
}

void Game::WithRW( const std::function< void() >& f ) {
	util::FinallyGuard restore( [ this ]() { m_rw_counter--; } );
	m_rw_counter++;
	f();
}

void Game::InitComplete( GSE_CALLABLE ) {
	ASSERT( m_game_state != GS_RUNNING, "game already initialized" );
	// A failed setup must never leave a saved continuation for a later start.
	bool completed = false;
	util::FinallyGuard discard_snapshot_state( [ this, &completed ]() {
		m_pending_snapshot_random.reset();
		DiscardSnapshotTurn();
		m_pending_simulation.reset();
		if ( !completed ) { m_simulation.reset(); }
	} );
	ValidatePendingSnapshotTurn();
	PublishPendingSimulation();

	try {
		m_state->TriggerObject(
			this, "start", ARGS_F( this ) {
				{
					"game",
					Wrap( GSE_CALL )
				},
			}; }
		);
	}
	catch ( const gse::Exception& e ) {
		MTModule::Log( (std::string)"Initialization failed: " + e.ToString() );
		InitFailed( e.what() );
		return;
	}
	ValidatePendingSnapshotTurn();
	ValidateCurrentSimulation();

	m_game_state = GS_RUNNING;
	m_um->ProcessUnprocessed( GSE_CALL );
	m_bm->ProcessUnprocessed( GSE_CALL );
	m_um->CompleteConventionalHomeRestore();
	ValidateCurrentSimulation();
	CompleteSmacWorkerKnowledgeRestore();
	std::optional< size_t > restored_turn;
	try { restored_turn = ApplySnapshotTurn(); }
	catch ( const std::runtime_error& error ) {
		InitFailed( error.what() );
		return;
	}
	if ( m_pending_snapshot_random ) {
		// Map construction/Initialize and instance setup share this RNG. Their
		// restore-only draws must not advance the downloaded continuation.
		m_random->SetState( *m_pending_snapshot_random );
		m_pending_snapshot_random.reset();
	}
	if ( restored_turn ) {
		// Restore presentation only after all instances are registered. This is
		// not a new gameplay turn and must not reset units, bases or players.
		auto fr = FrontendRequest( FrontendRequest::FR_TURN_ADVANCE );
		fr.data.turn_advance.turn_id = *restored_turn;
		AddFrontendRequest( fr );
		m_is_turn_complete = IsLocalTurnComplete();
		SetTurnStatus( m_slot && m_slot->GetState() == slot::Slot::SS_PLAYER && m_slot->GetPlayer()->IsTurnCompleted()
			? turn::TS_WAITING_FOR_PLAYERS
			: m_is_turn_complete ? turn::TS_TURN_COMPLETE : turn::TS_TURN_ACTIVE );
	}
	if ( m_state->m_connection ) {
		m_state->m_connection->IfServer(
			[]( connection::Server* connection ) -> void {
				connection->SetGameState( connection::Connection::GS_RUNNING ); // allow clients to download map
			}
		);
	}
	completed = true;
}

void Game::InitFailed( const std::string& error_text ) {
	// The caller's string and the error hook may both belong to the world
	// being retired. Own the primary error before invoking any handler.
	const std::string original_error = error_text;
	{
		std::lock_guard guard( m_admission_mutex );
		m_failed_admission = m_active_admission;
		m_failed_admission_error = original_error;
	}

	ClearUntransferredMapData();

	ASSERT( m_state, "state not set" );
	if ( m_state->m_connection ) {
		m_state->m_connection->Disconnect( "Failed to initialize game" );
	}
	std::exception_ptr handler_error;
	try {
		m_state->TriggerObject(
			this, "error", ARGS_F( this, &original_error ) {
				{
					"game",
					Wrap( GSE_CALL )
				},
				{
					"error",
					VALUE( gse::value::String, , original_error ),
				},
			}; }
		);
	}
	catch ( ... ) { handler_error = std::current_exception(); }

	ResetGame();
	m_state = nullptr;
	m_initialization_error = original_error;
	HideLoader(); // ResetGame clears the old world's frontend queue.

	if ( m_old_map ) {
		MTModule::Log( "Restoring old map state" );
		// Transfer ownership only. Restoring the old lifecycle remains TODO.
		m_map = std::exchange( m_old_map, nullptr );
	}
	if ( handler_error ) {
		try { std::rethrow_exception( handler_error ); }
		catch ( const gse::Exception& error ) {
			MTModule::Log( "Initialization error handler failed: " + error.ToString() );
		}
		catch ( const std::exception& error ) {
			MTModule::Log( std::string( "Initialization error handler failed: " ) + error.what() );
		}
		catch ( ... ) { MTModule::Log( "Initialization error handler failed with an unknown native exception" ); }
	}
}

static const std::unordered_map< backend::turn::turn_status_t, std::string > s_turn_status_str = {
	{ turn::TS_PLEASE_WAIT, "please_wait" },
	{ turn::TS_TURN_ACTIVE, "active" },
	{ turn::TS_TURN_COMPLETE, "turn_complete" },
	{ turn::TS_WAITING_FOR_PLAYERS, "waiting_for_players" },
};

void Game::SetTurnStatus( const backend::turn::turn_status_t status ) {
	m_turn_status = status;
	auto fr = FrontendRequest( FrontendRequest::FR_TURN_STATUS );
	fr.data.turn_status.status = status;
	AddFrontendRequest( fr );
	ASSERT( s_turn_status_str.find( status ) != s_turn_status_str.end(), "unknown status: " + std::to_string( status ) );
	m_state->WithGame( this, [ this, status ]( Game* const, GSE_CALLABLE ) {
		m_state->TriggerObject( this, "turn_status", ARGS_F( &status ) {
			{
				"status",
				VALUE( gse::value::String,, s_turn_status_str.at( status ) ),
			},
		}; } );
	});
}

const std::string& Game::GetTurnStatusString() const {
	return s_turn_status_str.at( m_turn_status );
}

void Game::ProcessEvents() {
	auto* const state = m_state;
	const auto session = state->CaptureGameSession();
	DispatchEventBatch( ExtractEventBatch( state, session ) );
}

std::unique_ptr< Game::EventBatchHandoff > Game::ExtractEventBatch( State* state, const State::game_session_t& session ) {
	std::unique_ptr< EventBatchHandoff > handoff;
	{
		std::lock_guard guard( m_pending_events_mutex );
		// A producer delayed before extraction must not consume events that a
		// replacement session has since published into this same Game.
		if ( !state->IsGameSessionActive( session ) || m_pending_events.empty() ) { return nullptr; }
		handoff = std::make_unique< EventBatchHandoff >( this, state, session, std::make_shared< EventBatchRoots >( state, m_pending_events ) );
		m_event_batch_handoffs.push_back( handoff->roots );
		handoff->published = true;
		m_pending_events.clear();
	}
	return handoff;
}

void Game::DispatchEventBatch( std::unique_ptr< EventBatchHandoff > handoff ) {
	if ( !handoff ) { return; }
	const auto& batch = handoff->roots;
	// Queue registration takes Space's pending mutex. Release the events mutex
	// first: Collect takes pending before traversing Game's roots. The handoff
	// root keeps this extracted batch reachable until the queue owns it.
	{
		handoff->state->WithGameSession( handoff->session, batch.get(), [ this, batch ]( Game* const, GSE_CALLABLE ) {
			const std::string* errptr = nullptr;
			for ( const auto& event : batch->events ) {
				const auto& data = event->GetData();
				const auto* args = static_cast< gse::value::Object* >( data.at( "data" ) );
				if ( event->GetSource() == event::Event::ES_LOCAL
					&& HandleSmacWorkerEvent( GSE_CALL, event->GetEventName(), args->value, event->GetCaller(), false ) ) {
					WithRW( [ & ] { HandleSmacWorkerEvent( GSE_CALL, event->GetEventName(), args->value, event->GetCaller(), true ); } );
					continue;
				}
				// Also fence commands queued before explicit knowledge admission.
				RequireWorkerKnowledgeMutation();
#if defined(DEBUG) || defined(FASTDEBUG)
				MTModule::Log( "Event begin: " + event->ToString() );
#endif
				auto* obj = VALUE( gse::value::Object, , GSE_CALL_NOGC, event->GetData() );
				const auto fargs = gse::value::function_arguments_t{ obj };
				event::EventHandler* handler = nullptr;
				{
					std::lock_guard guard( m_event_handlers_mutex );
					const auto& it = m_event_handlers.find( event->GetEventName() );
					handler = it != m_event_handlers.end()
						? it->second
						: nullptr;
				}
				if ( handler ) {
					errptr = handler->Validate( GSE_CALL, fargs );
				}
				else {
					errptr = new std::string( "Unknown event: " + event->GetEventName() );
				}
				if ( !errptr ) {

					const auto f_process = [ &handler, &fargs, &gc_space, &ctx, &si, &ep, &obj ] () {
						auto* applied = handler->Apply( GSE_CALL, fargs );
						if ( applied ) {
							obj->Set( "applied", applied, GSE_CALL );
						}
						return applied;
					};

					if ( m_state->IsMaster() ) { // either singleplayer or multiplayer host
						ASSERT( event->GetSource() != event::Event::ES_SERVER, "got event from server to server" );
						// process event
						gse::Value* resolved = nullptr;
						if ( handler->HasResolve() ) {
							resolved = handler->Resolve( GSE_CALL, fargs );
							if ( resolved ) {
								event->SetResolved( resolved );
								obj->Set( "resolved", resolved, GSE_CALL );
							}
						}
						if ( m_state->m_connection ) {
							if ( event->GetCaller() != 0 ) {
								// notify caller of acceptance
								m_state->m_connection->AsServer()->SendGameEventResponse( event->GetCaller(), event->GetId(), true, resolved );
							}
							// broadcast to clients
							ASSERT( m_state->m_connection->IsServer(), "master but not server" );
							m_state->m_connection->SendGameEvent( event );
						}
						WithRW( f_process );
					}
					else { // multiplayer non-host
						ASSERT( event->GetSource() != event::Event::ES_CLIENT, "got event from client to client" );
						ASSERT( m_state->m_connection, "not master but no connection" );
						bool process_now = false;
						if ( handler->HasResolve() ) {
							if ( event->GetSource() == event::Event::ES_LOCAL ) {
								// just send to server and wait for resolution
								ASSERT( !event->GetResolved(), "client event already resolved" );
								process_now = false;
							}
							else {
								// resolution came from server
								auto* const resolved = event->GetResolved();
								ASSERT( resolved, "server sent unresolved event" );
								obj->Set( "resolved", resolved, GSE_CALL );
								process_now = true;
							}
						}
						else {
							process_now = true;
						}
						// Optimistic application can destroy objects referenced by the
						// request, invalidating their script wrappers. Preserve the wire
						// data now, but queue it only after application succeeds.
						std::string serialized_request;
						if ( event->GetSource() == event::Event::ES_LOCAL ) {
							serialized_request = event->Serialize().ToString();
						}
						gse::Value* rollback_data = nullptr;
						if ( process_now ) {
							WithRW( [ &f_process, &rollback_data ](){
								rollback_data = f_process();
							} );
						}
						if ( event->GetSource() == event::Event::ES_LOCAL ) {
							std::lock_guard guard( m_events_waiting_for_responses_mutex );
							// send to server and wait for response asynchonously
							ASSERT( m_events_waiting_for_responses.find( event->GetId() ) == m_events_waiting_for_responses.end(), "event already waiting for response" );
							m_events_waiting_for_responses.insert(
								{
									event->GetId(),
									{
										event,
										process_now,
										rollback_data
									}
								}
							);
							m_state->m_connection->SendGameEvent( event, serialized_request );
						}
					}
				}
				else {
					MTModule::Log( "Event rejected: " + *errptr );
					if ( m_state->m_connection && m_state->IsMaster() ) {
						if ( event->GetCaller() != 0 ) {
							// notify caller of rejection
							m_state->m_connection->AsServer()->SendGameEventResponse( event->GetCaller(), event->GetId(), false, nullptr );
						}
					}
					delete ( errptr );
				}
#if defined(DEBUG) || defined(FASTDEBUG)
				MTModule::Log( "Event end: " + event->ToString() );
#endif
			}
		});
	}
}

void Game::CheckRW( GSE_CALLABLE ) const {
	if ( !m_rw_counter ) {
		GSE_ERROR( gse::EC.GAME_ERROR, "Game state is read-only. Try using events?");
	}
}

void Game::SetSlotNum( const size_t slotnum ) {
	m_slot_num = slotnum;
}

void Game::AddFrontendRequest( const FrontendRequest& request ) {
	//MTModule::Log( "Sending frontend request (type=" + std::to_string( request.type ) + ")" ); // spammy
	m_pending_frontend_requests->push_back( request );
}

Game::OwnerTransition::OwnerTransition( std::shared_ptr< WorldAdmission > admission, const StartupStage stage, std::vector< WorldAdmission::Ingress > ingress )
	: gc::Object( nullptr ), admission( std::move( admission ) ), stage( stage ), ingress( std::move( ingress ) ) {}

void Game::OwnerTransition::GetReachableObjects( std::unordered_set< gc::Object* >& reachable_objects ) {
	gc::Object::GetReachableObjects( reachable_objects );
	GC_REACHABLE( admission->state );
	if ( admission->connection ) { GC_REACHABLE( admission->connection ); }
	for ( const auto& entry : ingress ) { if ( entry.owner ) { GC_REACHABLE( entry.owner ); } }
	if ( configure_events ) { GC_REACHABLE( configure_events->roots.get() ); }
}

bool Game::OwnerBlocksNative() const {
	std::lock_guard guard( m_admission_mutex );
	return m_owner_transition && ( m_owner_transition->phase == OwnerTransition::Pending || m_owner_transition->phase == OwnerTransition::Running );
}

bool Game::HasInitializationFailure() const {
	std::lock_guard guard( m_admission_mutex );
	return m_active_admission && m_active_admission->stage == StartupStage::Failure;
}

bool Game::PublishInitializationFailure() {
	std::shared_ptr< WorldAdmission > admission;
	{
		std::lock_guard guard( m_admission_mutex );
		admission = m_active_admission;
		if ( !admission || admission->cancelled || admission->stage != StartupStage::Failure ) { return false; }
	}
	// Claim/cancel is rechecked under the admission mutex by publication and
	// execution. Never hold that mutex while registering work with Space.
	PublishOwnerTransition( admission, StartupStage::Failure );
	return true;
}

void Game::WithWorldIngress( State* state, gc::Object* owner, const State::game_func_t& callback ) {
	const auto session = state->CaptureGameSession();
	{
		std::lock_guard guard( m_admission_mutex );
		if ( m_active_admission && m_active_admission->state == state && m_active_admission->admitted_binding == session &&
			m_active_admission->stage != StartupStage::Complete ) {
			if ( m_active_admission->cancelled ) { return; }
			const bool nested = m_owner_transition && m_owner_transition->admission == m_active_admission &&
				m_owner_transition->phase == OwnerTransition::Running && m_owner_transition->thread == std::this_thread::get_id();
			if ( !nested ) {
				m_active_admission->ingress.push_back( { owner, callback } );
				return;
			}
		}
	}
	// Lobby traffic has no admitted world. Preserve its existing session-bound
	// path; nested work from the claimed owner operation is synchronous here.
	state->WithGameSession( session, owner, callback );
}

void Game::RefreshInitializationContext( const std::shared_ptr< WorldAdmission >& admission ) {
	if ( m_state != admission->state || !admission->admitted_binding || !admission->admitted_binding->IsActive() ||
		m_state->CaptureGameSession() != admission->admitted_binding ) { throw std::runtime_error( "Initialization session was replaced" ); }
	if ( m_state->GetConnection() != admission->connection ) { throw std::runtime_error( "Initialization connection was replaced or lost" ); }
	const auto slot_num = admission->connection ? admission->connection->GetSlotNum() : 0;
	if ( slot_num != admission->slot || slot_num >= m_state->m_slots->GetCount() ) { throw std::runtime_error( "Initialization local slot changed" ); }
	auto& slot = m_state->m_slots->GetSlot( slot_num );
	if ( slot.GetState() != slot::Slot::SS_PLAYER || !slot.GetPlayer() ) { throw std::runtime_error( "Initialization local player is absent" ); }
	m_slot_num = slot_num;
	m_slot = &slot;
	m_player = slot.GetPlayer();
}

bool Game::AdvanceInitialization() {
	std::shared_ptr< WorldAdmission > admission;
	StartupStage stage;
	{
		std::lock_guard guard( m_admission_mutex );
		admission = m_active_admission;
		if ( !admission ) { return false; }
		if ( admission->cancelled ) { return true; }
		if ( admission->stage == StartupStage::Complete ) { return false; }
		stage = admission->stage;
		// Initialize/configure was enqueued logically at admission, before the
		// subsequent network batch. Later ingress retains packet/download order.
		if ( stage != StartupStage::InitializeConfigure && stage != StartupStage::Failure && !admission->ingress.empty() ) { stage = StartupStage::Ingress; }
	}
	try {
		if ( stage != StartupStage::Failure ) {
			RefreshInitializationContext( admission );
			if ( !m_initialization_error.empty() ) { throw std::runtime_error( m_initialization_error ); }
		}
		switch ( stage ) {
			case StartupStage::PrepareBindings:
				PrepareInitializationBindings();
				SetStartupStage( admission, StartupStage::ConfigureEvents );
				break;
			case StartupStage::AcquireMap:
				AcquireInitializationMap();
				SetStartupStage( admission, m_game_state == GS_INITIALIZING ? StartupStage::InitializeMap : StartupStage::AwaitDownload );
				break;
			case StartupStage::InitializeMap:
				InitializeMap();
				SetStartupStage( admission, StartupStage::Start );
				break;
			case StartupStage::AwaitDownload: break;
			default: PublishOwnerTransition( admission, stage ); break;
		}
	}
	catch ( const std::exception& error ) {
		std::lock_guard guard( m_admission_mutex );
		if ( m_active_admission == admission && !admission->cancelled ) {
			admission->error = error.what();
			admission->stage = StartupStage::Failure;
		}
	}
	return true; // never fall through after an inline owner phase
}

void Game::PublishOwnerTransition( const std::shared_ptr< WorldAdmission >& admission, const StartupStage stage ) {
	// Capture one bootstrap batch at the native checkpoint. Events added by
	// this batch or while it is Pending belong to a later checkpoint.
	auto events = stage == StartupStage::ConfigureEvents
		? ExtractEventBatch( admission->state, admission->admitted_binding ) : nullptr;
	std::shared_ptr< OwnerTransition > transition;
	{
		std::lock_guard guard( m_admission_mutex );
		if ( m_active_admission != admission || admission->cancelled ) { return; }
		ASSERT( !m_owner_transition || ( m_owner_transition->phase != OwnerTransition::Pending && m_owner_transition->phase != OwnerTransition::Running ), "owner transition already active" );
		std::vector< WorldAdmission::Ingress > ingress;
		if ( stage == StartupStage::Ingress ) { ingress.swap( admission->ingress ); }
		transition = std::make_shared< OwnerTransition >( admission, stage, std::move( ingress ) );
		transition->configure_events = std::move( events );
		m_owner_transition_handoffs.push_back( transition );
		m_owner_transition = transition; // root handoff before Space registration
	}
	util::FinallyGuard handoff( [ this, transition ]() {
		std::lock_guard guard( m_admission_mutex );
		const auto it = std::find( m_owner_transition_handoffs.begin(), m_owner_transition_handoffs.end(), transition );
		if ( it != m_owner_transition_handoffs.end() ) { m_owner_transition_handoffs.erase( it ); }
	} );
	try {
		admission->state->WithGameSession( admission->admitted_binding, transition.get(),
			[ this, transition ]( Game* const, GSE_CALLABLE ) { RunOwnerTransition( GSE_CALL, transition ); },
			[ this, transition ]() { FinishOwnerTransition( transition ); } );
	}
	catch ( ... ) { FinishOwnerTransition( transition ); throw; }
}

void Game::FinishOwnerTransition( const std::shared_ptr< OwnerTransition >& transition ) {
	std::lock_guard guard( m_admission_mutex );
	if ( transition->phase == OwnerTransition::Finished || transition->phase == OwnerTransition::Cancelled ) {
		if ( m_owner_transition == transition ) { m_owner_transition.reset(); }
		return;
	}
	if ( transition->phase == OwnerTransition::Pending ) { transition->admission->cancelled = true; }
	transition->phase = transition->admission->cancelled ? OwnerTransition::Cancelled : OwnerTransition::Finished;
	if ( transition->admission->cancelled ) {
		transition->admission->state->RetireGameCallbacks( transition->admission->admitted_binding );
	}
	if ( m_owner_transition == transition ) { m_owner_transition.reset(); }
}

void Game::RunOwnerTransition( GSE_CALLABLE, const std::shared_ptr< OwnerTransition >& transition ) {
	const auto& admission = transition->admission;
	{
		std::lock_guard guard( m_admission_mutex );
		if ( m_owner_transition != transition || transition->phase != OwnerTransition::Pending || m_active_admission != admission || admission->cancelled ) { return; }
		transition->phase = OwnerTransition::Running;
		transition->thread = std::this_thread::get_id();
	}
	util::FinallyGuard finish( [ this, transition ]() { FinishOwnerTransition( transition ); } );
	try {
		if ( transition->stage != StartupStage::Failure ) { RefreshInitializationContext( admission ); }
		switch ( transition->stage ) {
			case StartupStage::InitializeConfigure:
				InitializeConfigure( GSE_CALL );
				SetStartupStage( admission, StartupStage::PrepareBindings );
				break;
			case StartupStage::ConfigureEvents:
				DispatchEventBatch( std::move( transition->configure_events ) );
				SetStartupStage( admission, StartupStage::AcquireMap );
				break;
			case StartupStage::Ingress:
				for ( const auto& entry : transition->ingress ) {
					entry.callback( this, GSE_CALL );
					std::lock_guard guard( m_admission_mutex );
					if ( admission->stage == StartupStage::Failure ) { break; }
				}
				break;
			case StartupStage::Start:
				StartInitializedWorld( GSE_CALL );
				SetStartupStage( admission, StartupStage::Complete );
				break;
			case StartupStage::Failure:
				InitFailed( admission->error );
				break;
			default: throw std::logic_error( "Invalid owner initialization stage" );
		}
	}
	catch ( const std::exception& error ) {
		InitFailed( error.what() );
	}
	catch ( ... ) {
		InitFailed( "Unknown native initialization failure" );
	}
}

void Game::SetStartupStage( const std::shared_ptr< WorldAdmission >& admission, const StartupStage stage ) {
	std::lock_guard guard( m_admission_mutex );
	if ( m_active_admission == admission && !admission->cancelled ) { admission->stage = stage; }
}

void Game::InitializeMap() {

	ASSERT( m_state, "state is null" );
	// State::Iterate may have delivered a roster replacement after the
	// download. Reject it before touching cached Slot/Player pointers.
	ValidatePendingSnapshotTurn();

	if ( !m_state->IsMaster() ) {
		ASSERT( m_state->m_connection, "not master but no connection" );
		// notify server of successful download and continue initialization
		m_slot->SetPlayerFlag( slot::PF_MAP_DOWNLOADED );
		m_state->m_connection->UpdateSlot( m_slot_num, m_slot, true );
	}

	auto ec = m_map->Initialize( m_init_cancel );
	if ( !ec && m_init_cancel ) {
		ec = map::Map::EC_ABORTED;
	}

	if ( !ec ) {

#ifdef DEBUG
		const auto* config = g_engine->GetConfig();
		// also handy to have dump of generated map
		if (
			!ec &&
				config->HasDebugFlag( config::Config::DF_MAPDUMP ) &&
				!config->HasDebugFlag( config::Config::DF_QUICKSTART_MAP_DUMP ) // no point saving if we just loaded it
			) {
			MTModule::Log( (std::string)"Saving map dump to " + config->GetDebugPath() + map::s_consts.debug.lastdump_filename );
			SetLoaderText( "Saving dump" );
			util::FS::WriteFile( config->GetDebugPath() + map::s_consts.debug.lastdump_filename, m_map->Serialize().ToString() );
		}
#endif

		ASSERT( m_map, "map not set" );

		NEW( m_response_map_data, response_map_data_t );

		m_response_map_data->map_width = m_map->GetWidth();
		m_response_map_data->map_height = m_map->GetHeight();

		ASSERT( m_map->m_textures.terrain, "map terrain texture not generated" );
		m_response_map_data->terrain_texture = m_map->m_textures.terrain;

		ASSERT( m_map->m_meshes.terrain, "map terrain mesh not generated" );
		m_response_map_data->terrain_mesh = m_map->m_meshes.terrain;

		ASSERT( m_map->m_meshes.terrain_data, "map terrain data mesh not generated" );
		m_response_map_data->terrain_data_mesh = m_map->m_meshes.terrain_data;

		m_response_map_data->sprites.actors = &m_map->m_sprite_actors;
		m_response_map_data->sprites.instances = &m_map->m_sprite_instances;

		m_response_map_data->tiles = m_map->GetTilesPtr()->GetTilesPtr();
		m_response_map_data->tile_states = m_map->GetMapState()->GetTileStatesPtr();

		RetireOldMap();

		// notify server of successful initialization
		m_slot->SetPlayerFlag( slot::PF_GAME_INITIALIZED );
		if ( m_state->m_connection ) {
			m_state->m_connection->UpdateSlot( m_slot_num, m_slot, true );
		}

		{
			const auto factions = m_state->GetFM()->GetAll();
			NEWV( faction_defines, FrontendRequest::faction_defines_t );
			for ( const auto& faction : factions ) {
				faction_defines->push_back( faction );
			}
			auto fr = FrontendRequest( FrontendRequest::FR_FACTION_DEFINE );
			fr.data.faction_define.factiondefs = faction_defines;
			AddFrontendRequest( fr );
		}

		{
			const auto& slots = m_state->m_slots->GetSlots();
			NEWV( slot_defines, FrontendRequest::slot_defines_t );
			for ( const auto& slot : slots ) {
				if ( slot.GetState() == slot::Slot::SS_OPEN || slot.GetState() == slot::Slot::SS_CLOSED ) {
					continue;
				}
				ASSERT( slot.GetState() == slot::Slot::SS_PLAYER, "unknown slot state: " + std::to_string( slot.GetState() ) );
				auto* player = slot.GetPlayer();
				ASSERT( player, "slot player not set" );
				ASSERT( player->GetFaction(), "slot player faction not set" );
				slot_defines->push_back(
					FrontendRequest::slot_define_t{
						slot.GetIndex(),
						player->GetFaction()->m_id
					}
				);
			}
			auto fr = FrontendRequest( FrontendRequest::FR_SLOT_DEFINE );
			fr.data.slot_define.slotdefs = slot_defines;
			AddFrontendRequest( fr );
		}

		HideLoader();

		m_game_state = GS_STARTING;

	}
	else { throw std::runtime_error( map::Map::GetErrorString( ec ) ); }
}

void Game::StartInitializedWorld( GSE_CALLABLE ) {


	if ( m_state->IsMaster() ) {
		try {
			PrepareNewSimulation();
			m_state->TriggerObject(
				this, "create_world", ARGS_F( this ) {
					{
						"game",
						Wrap( GSE_CALL )
					},
				}; }
			);
			InitComplete( GSE_CALL );
		}
		catch ( const gse::Exception& e ) {
			MTModule::Log( (std::string)"Initialization failed: " + e.ToString() );
			InitFailed( e.what() );
			return;
		}
		if ( m_game_state == GS_RUNNING ) {
			FirstTurn( GSE_CALL );
		}
	}
	else {
		InitComplete( GSE_CALL );
	}

	if ( m_game_state == GS_RUNNING ) {
		ProcessEvents();
		m_um->RequireConventionalUnitsReady();
		CheckTurnComplete();
	}
}

void Game::InitGame( const std::shared_ptr< WorldAdmission >& admission, MT_Response& response, MT_CANCELABLE ) {

	if ( m_game_state != GS_NONE ) {
		ResetGame();
	}

	ASSERT( !m_state || !m_state->m_connection || m_game_state == GS_NONE, "multiplayer game already initializing or running" );
	ASSERT( m_game_state == GS_NONE, "game still initializing" );

	// Reset retires the old session before any new initialization work queues.
	if ( !m_state->GetGame() ) { m_state->SetGame( this ); }
	{
		std::lock_guard guard( m_admission_mutex );
		admission->admitted_binding = m_state->CaptureGameSession();
		m_active_admission = admission;
		m_failed_admission.reset();
		m_failed_admission_error.clear();
		if ( admission->cancelled || canceled ) {
			CancelAdmission( admission );
			response.result = R_ABORTED;
			return;
		}
	}
	if ( admission->map_path ) {
		m_state->m_settings.global.map.type = settings::MapSettings::MT_MAPFILE;
		m_state->m_settings.global.map.filename = *admission->map_path;
	}

	ASSERT( m_pending_frontend_requests, "pending events not set" );
	m_pending_frontend_requests->clear();
	m_game_state = GS_PREPARING_MAP;
	m_initialization_error.clear();
	{
		std::lock_guard guard( m_admission_mutex );
		admission->connection = m_state->GetConnection();
		admission->slot = admission->connection ? admission->connection->GetSlotNum() : 0;
	}
	RefreshInitializationContext( admission );
	SetStartupStage( admission, StartupStage::InitializeConfigure );
	response.result = R_SUCCESS; // admitted; owner work starts after MT_SetResponses
}

void Game::InitializeConfigure( GSE_CALLABLE ) {
	m_state->TriggerObject( this, "initialize", ARGS_F( this ) {
		{ "game", Wrap( GSE_CALL ) },
	}; } );
	ASSERT( !m_tm, "tm not null" );
	m_tm = new map::tile::TileManager( this );
	ASSERT( !m_rm, "rm not null" );
	m_rm = new resource::ResourceManager( this );
	ASSERT( !m_um, "um not null" );
	m_um = new unit::UnitManager( this );
	ASSERT( !m_bm, "bm not null" );
	m_bm = new base::BaseManager( this );
	ASSERT( !m_am, "am not null" );
	m_am = new animation::AnimationManager( this );
	m_state->TriggerObject( this, "configure", ARGS_F( this ) {
		{
			"game",
			Wrap( GSE_CALL )
		},
	}; } );
}

void Game::PrepareInitializationBindings() {
	auto* const connection = m_state->m_connection;

	if ( m_state->IsMaster() ) {

		// assign random factions to players
		auto factions = m_state->GetFM()->GetAll();
		std::vector< size_t > m_available_factions = {};
		const auto& slots = m_state->m_slots->GetSlots();
		for ( const auto& slot : slots ) {
			if ( slot.GetState() == slot::Slot::SS_PLAYER ) {
				auto* player = slot.GetPlayer();
				ASSERT( player, "player not set" );
				if ( !player->GetFaction() ) {
					if ( m_available_factions.empty() ) {
						// (re)load factions list
						for ( size_t i = 0 ; i < factions.size() ; i++ ) {
						//for ( const auto& faction : factions ) {
							m_available_factions.push_back( i );
						}
						ASSERT( !m_available_factions.empty(), "no factions found" );
					}
					const auto it = m_available_factions.begin() + m_random->GetUInt( 0, m_available_factions.size() - 1 );
					player->SetFaction( factions.at( *it ) );
					m_available_factions.erase( it );
				}
			}
		}
		if ( connection ) {
			connection->AsServer()->SendPlayersList();
		}

	}

	if ( connection ) {

		m_slot_num = connection->GetSlotNum();

		connection->ResetHandlers();
		connection->IfServer(
			[ this ]( connection::Server* connection ) -> void {

				/*connection->m_on_player_join = [ this, connection ]( const size_t slot_num, slot::Slot* slot, const Player* player ) -> void {
					MTModule::Log( "Player " + player->GetFullName() + " is connecting..." );
					connection->GlobalMessage( "Connection from " + player->GetFullName() + "..." );
					// actual join will happen after he downloads and initializes map
				};*/

				connection->m_on_flags_update = [ this, connection ]( const size_t slot_num, slot::Slot* slot, const slot::player_flag_t old_flags, const slot::player_flag_t new_flags ) -> void {
					if ( !( old_flags & slot::PF_GAME_INITIALIZED ) && ( new_flags & slot::PF_GAME_INITIALIZED ) ) {
						const auto* player = slot->GetPlayer();
						MTModule::Log( player->GetFullName() + " joined the game." );
						connection->GlobalMessage( player->GetFullName() + " joined the game." );
					}
				};

				/*connection->m_on_player_leave = [ this, connection ]( const size_t slot_num, slot::Slot* slot, const Player* player ) -> void {
					MTModule::Log( "Player " + player->GetFullName() + " left the game." );
					connection->GlobalMessage( player->GetFullName() + " left the game." );
					m_tm->ReleaseTileLocks( slot_num );
				};*/

				connection->m_on_download_request = [ this ]() { return SerializeWorldSnapshot(); };

				connection->SetGameState( connection::Connection::GS_INITIALIZING );
			}
		);

		connection->IfClient(
			[ this ]( connection::Client* connection ) -> void {
				connection->m_on_disconnect = [ this ]() -> bool {
					MTModule::Log( "Connection lost" );
					m_state->DetachConnection();
					if ( m_game_state != GS_RUNNING ) {
						m_initialization_error = "Lost connection to server";
					}
					else {
						Quit( "Lost connection to server" );
					}
					return true;
				};
				connection->m_on_error = [ this ]( const std::string& reason ) -> bool {
					m_initialization_error = reason;
					return true;
				};
			}
		);

		connection->m_on_message = [ this ]( const std::string& message ) -> void {
			Message( message );
		};

		connection->m_on_game_event_validate = [ this ]( event::Event* event ) -> void {
			if ( m_state->IsMaster() ) {
				ASSERT( m_game_state == GS_RUNNING, "game is not running but received event" );
			}
			THROW( "TODO: m_on_game_event_validate");
			if ( m_game_state == GS_RUNNING ) {
/*				m_state->WithGSE( this, [ this, event ]( GSE_CALLABLE ) {
					ValidateEvent( GSE_CALL, event );
				} );*/
			}
			else {
				//m_unprocessed_events.push_back( event );
			}

		};

		connection->m_on_game_event_apply = [ this ]( event::Event* event ) -> void {
			if ( m_state->IsMaster() ) {
				ASSERT( m_game_state == GS_RUNNING, "game is not running but received event" );
			}
			THROW( "TODO: m_on_game_event_apply");
			if ( m_game_state == GS_RUNNING ) {
				/*m_state->WithGSE( this, [ this, event ]( GSE_CALLABLE ) {
					ProcessEvent( GSE_CALL, event );
				});*/
			}
		};

		connection->m_on_game_event_rollback = [ this ]( event::Event* event ) -> void {
			if ( m_state->IsMaster() ) {
				ASSERT( m_game_state == GS_RUNNING, "game is not running but received event" );
			}
			THROW( "TODO: m_on_game_event_rollback");
			if ( m_game_state == GS_RUNNING ) {
				/*m_state->WithGSE( this, [ this, event ]( GSE_CALLABLE ) {
					ProcessEvent( GSE_CALL, event );
				});*/
			}
		};

	}
	else {
		m_slot_num = 0;
	}
	m_player = m_state->m_slots->GetSlot( m_slot_num ).GetPlayer();
	m_slot = m_player->GetSlot();

}

void Game::AcquireInitializationMap() {
	auto* const connection = m_state->GetConnection();

	if ( m_state->IsMaster() ) {
		// generate map

		MTModule::Log( "Game seed: " + m_random->GetStateString() );
		m_state->m_settings.global.ValidateWorldSize();

		PrepareInitializationMap();
		// Native generation and its progress/IO calls must not hold collection
		// exclusion. Keep all partially constructed tile storage unpublished.
		map::Map* candidate = nullptr;
		util::FinallyGuard discard( [ & ]() { DiscardInitializationMap( candidate ); } );
		NEW( candidate, map::Map, this );

		const auto* config = g_engine->GetConfig();

#ifdef DEBUG
		// if crash happens - it's handy to have a seed to reproduce it
		util::FS::WriteFile( config->GetDebugPath() + map::s_consts.debug.lastseed_filename, m_random->GetStateString() );
#endif

		map::Map::error_code_t ec = map::Map::EC_UNKNOWN;
		bool generated_map = false;

#ifdef DEBUG
		if ( !connection && config->HasDebugFlag( config::Config::DF_QUICKSTART_MAP_DUMP ) ) {
			const std::string& filename = config->GetQuickstartMapDump();
			ASSERT( util::FS::FileExists( filename ), "map dump file \"" + filename + "\" not found" );
			MTModule::Log( (std::string)"Loading map dump from " + filename );
			SetLoaderText( "Loading dump" );
			candidate->Deserialize( types::Buffer( util::FS::ReadTextFile( filename ) ) );
			ec = map::Map::EC_NONE;
		}
		else
#endif
		{
			if ( !connection && config->HasLaunchFlag( config::Config::LF_QUICKSTART_MAP_FILE ) ) {
				const std::string& filename = config->GetQuickstartMapFile();
				ec = candidate->LoadFromFile( filename );
			}
			else {
				auto& map_settings = m_state->m_settings.global.map;
				if ( map_settings.type == settings::MapSettings::MT_MAPFILE ) {
					ASSERT( !map_settings.filename.empty(), "loading map requested but map file not specified" );
					ec = candidate->LoadFromFile( map_settings.filename );
				}
				else {
					ec = candidate->Generate( &map_settings, m_init_cancel );
					generated_map = ec == map::Map::EC_NONE;
				}
			}

		}

		if ( !ec && m_init_cancel ) {
			ec = map::Map::EC_ABORTED;
		}

		if ( !ec ) {
			PublishInitializationMap( candidate );
			if ( m_active_admission ) { m_active_admission->generated_map = generated_map; }
			m_slot->SetPlayerFlag( slot::PF_MAP_DOWNLOADED ); // map was generated locally
			if ( connection ) {
				connection->UpdateSlot( m_slot_num, m_slot, true );
			}
			m_game_state = GS_INITIALIZING;
		}
		else {
			throw std::runtime_error( map::Map::GetErrorString( ec ) );
		}
	}
	else {
		connection->IfClient(
			[ this ]( connection::Client* connection ) -> void {

				// wait for server to initialize
				SetLoaderText( "Waiting for server" );

				const auto f_download_map = [ this, connection ] {

					SetLoaderText( "Downloading world snapshot" );

					ConfigureWorldDownload( connection );
					connection->RequestDownload();
				};
				if ( connection->GetGameState() == connection::Connection::GS_RUNNING ) {
					// server already initialized
					f_download_map();
				}
				else {
					// wait for server to initialize
					connection->m_on_game_state_change = [ this, f_download_map ]( const connection::Connection::game_state_t state ) -> void {
						switch ( state ) {
							case connection::Connection::GS_INITIALIZING: {
								// waiting
								break;
							}
							case connection::Connection::GS_RUNNING: {
								f_download_map();
								break;
							}
							default: {
								THROW( "unexpected game state: " + std::to_string( state ) );
							}
						}
					};
				}
			}
		);
	}
}

void Game::PrepareInitializationMap() {
	WithNativeRetirement( [ this ]() {
		ASSERT( !m_old_map, "old map not null" );
		// Preserve the existing error path: InitFailed restores this ownership
		// after retiring a failed current map, including a still-private attempt.
		m_old_map = std::exchange( m_map, nullptr );
	} );
}

void Game::PublishInitializationMap( map::Map*& candidate ) {
	WithNativeRetirement( [ & ]() {
		if ( m_init_cancel ) { throw std::runtime_error( map::Map::GetErrorString( map::Map::EC_ABORTED ) ); }
		ASSERT( !m_map && candidate && candidate->m_tiles, "invalid initialization map publication" );
		m_map = std::exchange( candidate, nullptr );
	} );
}

void Game::DiscardInitializationMap( map::Map*& candidate ) {
	// Only unpublished candidates belong here: no script wrapper or renderer
	// borrower can exist. Presentation cleanup may notify the renderer, so it
	// stays outside the native collection guard, including exception unwinding.
	DetachedMapPresentation detached;
	response_map_data_t* no_response = nullptr;
	DetachMapPresentation( candidate, no_response, detached );
	RetireMap( candidate, detached );
}

std::string Game::SerializeWorldSnapshot() const {
	if ( m_um ) { m_um->RequireConventionalUnitsReady(); }
	if ( !m_map ) {
		// map not generated yet
		return "";
	}
	if ( m_map->m_worker_knowledge ) {
		if ( !m_map->m_worker_knowledge_ready ) { THROW( "Worker knowledge restore is incomplete" ); }
		ValidateSmacWorkerKnowledge( *m_map->m_worker_knowledge );
	}
	MTModule::Log( "Preparing snapshot for download" );
	// Capture at the start of the existing synchronous world serialization.
	// Reading/writing this state does not draw from the generator.
	const auto random_state = m_random->GetState();
	const auto turn_state = world_snapshot::CaptureTurnState( m_current_turn.GetId(), m_state->m_slots->GetSlots() );
	if ( m_state->m_settings.global.GetRulesetId() == "smac" ) {
		m_state->m_settings.global.ValidateWorldSize();
		const auto& selection = *m_state->m_settings.global.map.GetSmacWorldSize();
		if ( m_map->GetTilesPtr()->GetWidth() != static_cast< uint32_t >( selection.width )
			|| m_map->GetTilesPtr()->GetHeight() != static_cast< uint32_t >( selection.height ) ) { THROW( "World map dimensions differ from accepted SMAC world size" ); }
	}
	types::Buffer buf;
	world_snapshot::WriteHeader( buf, m_state->m_settings.global );
	world_snapshot::WriteRandomState( buf, random_state );
	world_snapshot::WriteTurnState( buf, turn_state );
	WriteSimulationState( buf );
	map::GlsmacResourceMapPolicy::WriteOptional( buf, m_map->GetResourcePolicy(), m_state->m_settings.global.GetRulesetId() == "smac" );
	world_snapshot::WriteWorkerKnowledge( buf, m_map->m_worker_knowledge, m_state->m_settings.global.GetRulesetId() == "smac" );

	// map
	m_map->SaveToBuffer( buf );

	// resources
	{
		types::Buffer b;
		m_rm->Serialize( b );
		buf.WriteString( b.ToString() );
	}

	// units
	{
		types::Buffer b;
		m_um->Serialize( b );
		buf.WriteString( b.ToString() );
	}

	// bases
	{
		types::Buffer b;
		m_bm->Serialize( b );
		buf.WriteString( b.ToString() );
	}

	// animations
	{
		types::Buffer b;
		m_am->Serialize( b );
		buf.WriteString( b.ToString() );
	}

	return buf.ToString();
}

void Game::ConfigureWorldDownload( connection::Client* connection ) {
	auto* const state = m_state;
	const auto session = state->CaptureGameSession();
	std::shared_ptr< WorldAdmission > admission;
	{
		std::lock_guard guard( m_admission_mutex );
		admission = m_active_admission;
	}
	const auto is_current = [ this, state, session, connection, admission ]() {
		if ( !session || !session->IsActive() ) { return false; }
		std::lock_guard guard( m_admission_mutex );
		return m_active_admission == admission && ( !admission || !admission->cancelled ) &&
			m_state == state && state->GetConnection() == connection;
	};
	m_pending_snapshot_random.reset();
	DiscardSnapshotTurn();
	m_pending_simulation.reset();
	connection->m_on_download_progress = [ this, is_current ]( const float progress ) {
		if ( !is_current() ) { return; }
		SetLoaderText( "Downloading world snapshot:  " + std::to_string( (size_t)std::round( progress * 100 ) ) + "%" );
	};
	connection->m_on_download_complete = [ this, state, connection, admission, is_current ]( const std::string bytes ) {
		if ( !is_current() ) { return; }
		connection->m_on_download_complete = nullptr;
		connection->m_on_download_progress = nullptr;
		WithWorldIngress( state, connection, [ this, connection, admission, is_current, bytes ]( Game* const, GSE_CALLABLE ) {
			if ( !is_current() ) { return; }
			if ( !DecodeInitializationDownload( GSE_CALL, connection, bytes ) ) { return; }
			if ( admission ) { SetStartupStage( admission, StartupStage::InitializeMap ); }
		} );
	};
}

bool Game::DecodeInitializationDownload( GSE_CALLABLE, connection::Client* connection, const std::string& bytes ) {
	try {
		auto buf = types::Buffer( bytes );
		world_snapshot::ValidateHeader( buf, m_state->m_settings.global );
		const auto random_state = world_snapshot::ReadRandomState( buf );
		const auto turn_state = world_snapshot::ReadTurnState( buf, m_state->m_slots->GetSlots() );
		auto simulation = ReadSimulationState( buf, turn_state.id );
		const auto map_policy = map::GlsmacResourceMapPolicy::ReadOptional( buf, m_state->m_settings.global.GetRulesetId() == "smac" );
		auto worker_knowledge = world_snapshot::ReadWorkerKnowledge( buf, m_state->m_settings.global.GetRulesetId() == "smac" );

		// map
		const auto map_payload = buf.ReadString();
		world_snapshot::ValidateMapDimensions( map_payload, m_state->m_settings.global );
		world_snapshot::ValidateWorkerKnowledgeGeometry( worker_knowledge, map_payload );
		NEW( m_map, map::Map, this );
		DeserializeWorldSnapshotBody( GSE_CALL, buf, map_payload, random_state, turn_state, std::move( simulation ), map_policy, std::move( worker_knowledge ) );
		return true;
	}
	catch ( const std::runtime_error& error ) {
		MTModule::Log( std::string( "WARNING: failed to unpack world snapshot: " ) + error.what() );
		connection->Disconnect( "Snapshot format mismatch" );
		std::lock_guard guard( m_admission_mutex );
		if ( m_active_admission && !m_active_admission->cancelled ) {
			m_active_admission->error = error.what();
			m_active_admission->stage = StartupStage::Failure;
		}
		return false;
	}
}

void Game::DeserializeWorldSnapshotBody( GSE_CALLABLE, types::Buffer& body, const std::string& map_payload, const util::random::state_t& random_state, const world_snapshot::TurnState& turn_state, std::optional< BoundSimulation > simulation, std::optional< map::GlsmacResourceMapPolicy > map_policy, std::optional< map::SmacWorkerKnowledge > worker_knowledge ) {
	world_snapshot::ValidateMapDimensions( map_payload, m_state->m_settings.global );
	world_snapshot::ValidateWorkerKnowledgeGeometry( worker_knowledge, map_payload );
	m_pending_snapshot_random.reset();
	DiscardSnapshotTurn();
	m_pending_simulation.reset();
	auto map_buffer = types::Buffer( map_payload );
	const auto ec = m_map->LoadFromBuffer( map_buffer );
	if ( ec != map::Map::EC_NONE ) {
		throw std::runtime_error( "Failed to unpack world snapshot map (code=" + std::to_string( ec ) + ")" );
	}
	m_map->m_resource_policy = map_policy; // decoded before Map allocation; raw loader never infers it
	m_map->m_worker_knowledge = std::move( worker_knowledge );
	m_map->m_worker_knowledge_ready = false; // only bind after both pending entity registries drain
	{
		auto resources = types::Buffer( body.ReadString() );
		m_rm->Deserialize( resources );
	}
	{
		auto units = types::Buffer( body.ReadString() );
		// The downloaded simulation is still private here; UM loads before BM
		// and before pending-state publication. Never consult the old world.
		m_um->Deserialize( GSE_CALL, units, [ this, &simulation ]( const auto& unit, const auto* owner ) {
			if ( !simulation ) { throw std::runtime_error( "Conventional unit snapshot lacks simulation state" ); }
			return ResolveConventionalUnit( unit, owner, *simulation, true );
		}, true );
	}
	{
		auto bases = types::Buffer( body.ReadString() );
		m_bm->Deserialize( GSE_CALL, bases );
	}
	{
		auto animations = types::Buffer( body.ReadString() );
		m_am->Deserialize( animations );
	}
	StageSnapshotTurn( GSE_CALL, turn_state );
	MTModule::Log( "Received turn ID: " + std::to_string( turn_state.id ) );
	// Turn's setter only assigns the ID. Game::AdvanceTurn also runs gameplay
	// hooks and clears completion, so it must never be used for restoration.
	m_current_turn.AdvanceTurn( turn_state.id );
	m_pending_snapshot_random = random_state;
	m_pending_simulation = std::move( simulation );
	m_game_state = GS_INITIALIZING;
}

void Game::StageSnapshotTurn( GSE_CALLABLE, const world_snapshot::TurnState& state ) {
	world_snapshot::ValidateTurnRoster( state, m_state->m_slots->GetSlots() );
	PendingSnapshotTurn pending{ state, {} };
	pending.bindings.reserve( state.players.size() );
	for ( const auto& record : state.players ) {
		auto& slot = m_state->m_slots->GetSlot( record.id );
		// Existing Wrappable links invalidate these tokens on Player destruction,
		// including deletion followed by allocation at the very same address.
		pending.bindings.push_back( { &slot, static_cast< gse::value::Object* >( slot.GetPlayer()->Wrap( GSE_CALL ) ) } );
	}
	// Construct under the caller's accumulation before locking. Root traversal
	// takes Space exclusion then this mutex; reset only drops nonowning tokens.
	std::lock_guard guard( m_snapshot_turn_mutex );
	m_pending_snapshot_turn = std::move( pending );
}

void Game::DiscardSnapshotTurn() {
	std::lock_guard guard( m_snapshot_turn_mutex );
	m_pending_snapshot_turn.reset();
}

void Game::ValidateSnapshotBindings( const PendingSnapshotTurn& pending ) const {
	const auto& slots = m_state->m_slots->GetSlots();
	world_snapshot::ValidateTurnRoster( pending.state, slots );
	if ( pending.bindings.size() != pending.state.players.size() ) { throw std::runtime_error( "Incomplete world snapshot Player bindings" ); }
	for ( size_t i = 0 ; i < pending.bindings.size() ; ++i ) {
		const auto& slot = slots[ pending.state.players[ i ].id ];
		const auto& binding = pending.bindings[ i ];
		const auto* token = binding.player;
		if ( binding.slot != &slot || !token || token->type != gse::VT_OBJECT ||
			token->object_class != Player::WRAP_CLASS || token->wrapobj != slot.GetPlayer() ) {
			throw std::runtime_error( "World snapshot Player identity changed during initialization" );
		}
	}
	// The game and restored owners retain Slot addresses. Do not rebind a new
	// roster to old instances, and never dereference the captured addresses.
	if ( ( m_slot && ( m_slot_num >= slots.size() || m_slot != &slots[ m_slot_num ] ) ) ||
		( m_player && ( m_slot_num >= slots.size() || slots[ m_slot_num ].GetState() != slot::Slot::SS_PLAYER || m_player != slots[ m_slot_num ].GetPlayer() ) ) ) {
		throw std::runtime_error( "World snapshot local Player identity changed during initialization" );
	}
}

void Game::ValidatePendingSnapshotTurn() {
	// Native validation shares exclusion with canonical roster commits and
	// retirement. Failure has no side effects here: the admitted coordinator
	// owns its later Failure checkpoint, including reset/cancellation ordering.
	m_world_space->WithNativeMutation( [ & ]() {
		std::lock_guard guard( m_snapshot_turn_mutex );
		if ( m_pending_snapshot_turn ) { ValidateSnapshotBindings( *m_pending_snapshot_turn ); }
	} );
}

std::optional< size_t > Game::ApplySnapshotTurn() {
	std::optional< size_t > restored;
	m_world_space->WithNativeMutation( [ & ]() {
		std::lock_guard guard( m_snapshot_turn_mutex );
		if ( !m_pending_snapshot_turn ) { return; }
		const auto& pending = *m_pending_snapshot_turn;
		ValidateSnapshotBindings( pending ); // Validate and commit under the same roster exclusion.
		restored = pending.state.id;
		for ( const auto& record : pending.state.players ) {
			auto* player = m_state->m_slots->GetSlot( record.id ).GetPlayer();
			if ( record.completed ) { player->CompleteTurn(); }
			else { player->UncompleteTurn(); }
		}
		m_current_turn.AdvanceTurn( *restored );
		m_pending_snapshot_turn.reset();
	} );
	return restored;
}

void Game::WithNativeRetirement( const std::function< void() >& f ) {
	if ( m_world_space ) { m_world_space->WithNativeMutation( f ); }
	else {
		if ( m_map || m_old_map ) { throw std::runtime_error( "Native world retirement has no owning Space" ); }
		f(); // Before the first State is bound, no script wrappers can exist.
	}
}

void Game::DetachMapPresentation( map::Map* map, response_map_data_t*& response, DetachedMapPresentation& detached ) {
	detached.response = std::exchange( response, nullptr );
	if ( !map ) { return; }
	// The owning response may alias any subset of the Map presentation.
	if ( detached.response ) {
		if ( map->m_textures.terrain == detached.response->terrain_texture ) { map->m_textures.terrain = nullptr; }
		if ( map->m_meshes.terrain == detached.response->terrain_mesh ) { map->m_meshes.terrain = nullptr; }
		if ( map->m_meshes.terrain_data == detached.response->terrain_data_mesh ) { map->m_meshes.terrain_data = nullptr; }
	}
	detached.texture = std::exchange( map->m_textures.terrain, nullptr );
	detached.mesh = std::exchange( map->m_meshes.terrain, nullptr );
	detached.data = std::exchange( map->m_meshes.terrain_data, nullptr );
}

void Game::RetireMap( map::Map*& map, DetachedMapPresentation& detached ) {
	if ( map ) {
		detached.state = std::exchange( map->m_map_state, nullptr );
		auto* retired = std::exchange( map, nullptr );
		DELETE( retired ); // Actual Tile/Map wrapper invalidation stays protected.
	}
}

void Game::RetireOldMap() {
	if ( !m_old_map ) { return; }
	MTModule::Log( "Destroying old map state" );
	DetachedMapPresentation detached;
	WithNativeRetirement( [ & ]() {
		response_map_data_t* no_response = nullptr;
		DetachMapPresentation( m_old_map, no_response, detached );
		RetireMap( m_old_map, detached );
	} );
}

void Game::ClearUntransferredMapData() {
	// Called from the existing owner error checkpoint. Its accumulation owns
	// exclusion; unlike foreign native reset it may already notify the renderer.
	DetachedMapPresentation detached;
	DetachMapPresentation( m_map, m_response_map_data, detached );
}

void Game::ResetGame() {
	{
		std::lock_guard guard( m_admission_mutex );
		ASSERT( !m_owner_transition || m_owner_transition->phase != OwnerTransition::Running || m_owner_transition->thread == std::this_thread::get_id(), "native reset during owner initialization" );
		if ( m_owner_transition && m_owner_transition->phase == OwnerTransition::Pending ) { m_owner_transition->phase = OwnerTransition::Cancelled; }
	}
	if ( m_state ) { m_state->RetireGameCallbacks(); }
	// The coordinator excludes native packet producers and executing owner
	// transitions. Keep State/Connection published throughout this transport wait.
	if ( m_state && m_state->m_connection ) {
		m_state->m_connection->Disconnect();
		m_state->m_connection->DrainEventRequest();
	}
	if ( m_map ) { MTModule::Log( "Resetting map" ); }
	DetachedMapPresentation detached;
	WithNativeRetirement( [ & ]() {
		{
			std::lock_guard guard( m_admission_mutex );
			if ( !m_owner_transition || m_owner_transition->phase != OwnerTransition::Running ) { m_owner_transition.reset(); }
			m_active_admission.reset();
		}

		m_pending_snapshot_random.reset();
		DiscardSnapshotTurn();
		m_pending_simulation.reset(); m_simulation.reset();
		ClearHandlers();

		if ( m_game_state != GS_NONE ) {
			// TODO: do something?
			m_game_state = GS_NONE;
		}
		m_init_cancel = false;
		m_player = nullptr;
		m_slot_num = 0;
		m_slot = nullptr;

		{
			std::lock_guard guard( m_events_waiting_for_responses_mutex );
			m_events_waiting_for_responses.clear();
		}
		{
			std::lock_guard guard( m_pending_event_responses_mutex );
			m_pending_event_responses.clear();
		}
		{
			std::lock_guard guard( m_pending_events_mutex );
			m_pending_events.clear();
		}
		{
			std::lock_guard guard( m_event_handlers_mutex );
			m_event_handlers.clear();
		}
		m_next_event_id = 0;

		m_tm = nullptr;
		m_rm = nullptr;
		m_um = nullptr;
		m_bm = nullptr;
		m_am = nullptr;

		DetachMapPresentation( m_map, m_response_map_data, detached );
		RetireMap( m_map, detached );

		ASSERT( m_pending_frontend_requests, "pending events not set" );
		m_pending_frontend_requests->clear();

		m_current_turn.Reset();
		m_is_turn_complete = false;
		m_turn_status = turn::TS_PLEASE_WAIT;

		if ( m_state ) {
			// ui thread will reset state as needed
			m_state->UnsetGame();
			if ( m_state->m_connection ) {
				m_state->m_connection->ResetHandlers();
			}
		}
	} );
}

bool Game::IsLocalTurnComplete() const {
	for ( const auto& unit : m_um->GetUnits() ) {
		if ( unit.second->m_owner == m_slot && unit.second->HasMovesLeft() ) {
			return false;
		}
	}
	return true;
}

void Game::CheckTurnComplete() {
	if ( m_um->HasPendingConventionalUnits() ) { return; }
	const bool is_turn_complete = IsLocalTurnComplete();
	if ( m_is_turn_complete != is_turn_complete ) {
		m_is_turn_complete = is_turn_complete;
		MTModule::Log( "Sending turn complete status: " + std::to_string( m_is_turn_complete ) );
		SetTurnStatus( m_is_turn_complete
			? turn::TS_TURN_COMPLETE
			: turn::TS_TURN_ACTIVE
		);
	}

}

const bool Game::IsRunning() const {
	return m_game_state == GS_RUNNING;
}

const bool Game::IsMapReady() const {
	return m_game_state > GS_PREPARING_MAP;
}

const std::string Game::GenerateEventId() {
	return std::to_string( m_slot_num ) + "_" + std::to_string( ++m_next_event_id );
}

void Game::SetState( State* const state ) {
	ASSERT( state, "state is null" );
	if ( m_world_space && m_world_space != state->m_gc_space ) {
		throw std::runtime_error( "Game cannot move native wrappers to another Space before Stop" );
	}
	m_world_space = state->m_gc_space;
	if ( state != m_state ) {
		ASSERT( !m_state, "game state already set" );
		m_state = state;
	}
}

}
}
