#include "State.h"

#include "game/backend/faction/FactionManager.h"
#include "Game.h"
#include "game/backend/connection/Connection.h"
#include "Bindings.h"
#include "game/backend/slot/Slots.h"
#include "Player.h"
#include "GLSMAC.h"
#include "gc/Space.h"
#include "network/Network.h"
#include "types/Packet.h"

namespace game {
namespace backend {
namespace {

void ValidateSettingsEnvelope( const types::Buffer& state ) {
	types::Packet packet( types::Packet::PT_GLOBAL_SETTINGS );
	packet.data.str = state.ToString();
	if ( packet.Serialize().lenw > network::Network::MAX_PACKET_BYTES ) {
		throw std::runtime_error( "Global settings and faction metadata exceed the network packet budget" );
	}
}

// This native owner is retained by the queued callable, not registered with
// the collector. The pending queue traverses it while holding its own lock.
class GameCallbackRoots final : public gc::Object {
public:
	GameCallbackRoots( State* state, gc::Object* owner ) : gc::Object( nullptr ), m_state( state ), m_owner( owner ) {}
	void GetReachableObjects( std::unordered_set< gc::Object* >& reachable_objects ) override {
		gc::Object::GetReachableObjects( reachable_objects );
		GC_REACHABLE( m_state );
		if ( m_owner ) { GC_REACHABLE( m_owner ); }
	}
private:
	State* const m_state;
	gc::Object* const m_owner;
};

}

State::State( gc::Space* const gc_space, gse::context::Context* const ctx, GLSMAC* glsmac )
	: gse::GCWrappable( gc_space )
	, m_gc_space( gc_space )
	, m_ctx( ctx )
	, m_glsmac( glsmac )
	, m_slots( new slot::Slots( this ) ) {
	NEW( m_fm, faction::FactionManager );
 EnsureRosterEpoch();
}

State::~State() {
	Reset();
	if ( m_bindings ) {
		delete m_bindings;
	}
	if ( m_connection ) {
		m_connection->SetState( nullptr );
	}
	delete m_slots;
	DELETE( m_fm );
}

void State::SetGame( Game* game ) {
	ASSERT( !m_game, "game already set" );
	const auto session = std::shared_ptr< GameSession >( new GameSession( this, game ) );
	{
		std::lock_guard guard( m_game_session_mutex );
		m_game = game;
		m_game_session = session;
	}
	m_on_gse_error = [ this ]( const gse::Exception& e ) -> void {
		m_game->OnGSEError( e );
	};
}

void State::RetireGameCallbacks() {
	std::lock_guard guard( m_game_session_mutex );
	if ( m_game_session ) { m_game_session->active.store( false ); }
}

void State::RetireGameCallbacks( const game_session_t& expected_session ) {
	std::lock_guard guard( m_game_session_mutex );
	if ( m_game_session == expected_session && m_game_session ) { m_game_session->active.store( false ); }
}

void State::UnsetGame() {
	RetireGameCallbacks();
	m_game = nullptr;
	m_on_gse_error = nullptr;
}

Game* State::GetGame() const {
	return m_game;
}

void State::WithGSE( gc::Object* const owner, const gse_func_t& f, const f_cleanup_t& f_cleanup, const bool need_now ) {
	m_gc_space->Accumulate(
		owner,
		[ this, f ]() {
			gse::ExecutionPointer ep;
			f( m_gc_space, m_ctx, {}, ep );
		},
		f_cleanup,
		need_now
	);
}

void State::WithGame( gc::Object* const owner, const game_func_t& f, const f_cleanup_t& cleanup, const bool need_now ) {
	WithGameSession( CaptureGameSession(), owner, f, cleanup, need_now );
}

State::game_session_t State::CaptureGameSession() {
	std::lock_guard guard( m_game_session_mutex );
	return m_game_session;
}

bool State::IsGameSessionActive( const game_session_t& session ) const {
	return session && session->state == this && session->IsActive();
}

void State::WithGameSession( const game_session_t& session, gc::Object* const owner, const game_func_t& f, const f_cleanup_t& cleanup, const bool need_now ) {
	const auto roots = std::make_shared< GameCallbackRoots >( this, owner );
	m_gc_space->Accumulate( roots.get(), [ this, session, roots, f ]() {
		// Check before any context, captured world object, or Game access. A new
		// binding always gets another identity, even for the same State/Game.
		if ( !IsGameSessionActive( session ) ) { return; }
		gse::ExecutionPointer ep;
		f( session->game, m_gc_space, m_ctx, {}, ep );
	}, cleanup, need_now );
}

void State::Iterate() {
	//std::lock_guard guard( m_connection_mutex );
	if ( m_connection ) {
		if ( m_connection->IterateAndMaybeDelete() ) {
			// Polling and callbacks above can queue work; only the native
			// terminal ownership cleanup may share collection exclusion.
			m_gc_space->WithNativeMutation( [ & ]() {
				if ( m_connection ) {
					m_connection->SetState( nullptr );
					m_connection = nullptr;
				}
				RetireRosterEpoch();
				for ( auto& player : m_players ) {
					DELETE( player );
				}
				m_players.clear();
				m_slots->Clear();
				m_cid_slots.clear();
			} );
		}
	}
}

bool State::IsMaster() {
	//std::lock_guard guard( m_connection_mutex );
	return !m_connection || m_connection->IsServer();
}

bool State::IsSlave() {
	//std::lock_guard guard( m_connection_mutex );
	return m_connection && m_connection->IsClient();
}

void State::AddPlayer( Player* player ) {
 EnsureRosterEpoch(); player->BindRoster(this, m_roster_epoch);
	Log( "adding player: " + player->GetPlayerName() );
	ASSERT( m_players.find( player ) == m_players.end(), "duplicate player add" );
	m_players.insert( player );
}

void State::RemovePlayer( Player* player ) {
	Log( "removing player: " + player->GetPlayerName() );
#if defined( DEBUG ) || defined( FASTDEBUG )
	ASSERT( m_players.find( player ) != m_players.end(), "player not found" );
	for ( auto& it : m_cid_slots ) {
		ASSERT( m_slots->GetSlot( it.second ).GetPlayer() != player, "player still in cid slots" );
	}
#endif
	m_players.erase( player );
}

void State::AddCIDSlot( const network::cid_t cid, const size_t slot ) {
	Log( "adding CID " + std::to_string( cid ) + " for slot " + std::to_string( slot ) );
#if defined( DEBUG ) || defined( FASTDEBUG )
	ASSERT( m_cid_slots.find( cid ) == m_cid_slots.end(), "duplicate cid add" );
	for ( auto& it : m_cid_slots ) {
		ASSERT( it.second != slot, "slot " + std::to_string( slot ) + " already in cids" );
	}
#endif
	m_cid_slots[ cid ] = slot;
}

const bool State::HasCIDSlot( const network::cid_t cid ) {
	return m_cid_slots.find( cid ) != m_cid_slots.end();
}

void State::RemoveCIDSlot( const network::cid_t cid ) {
	Log( "removing CID " + std::to_string( cid ) );
	ASSERT( m_cid_slots.find( cid ) != m_cid_slots.end(), "cid not found" );
	m_cid_slots.erase( cid );
}

const std::unordered_map< network::cid_t, size_t >& State::GetCidSlots() const {
	return m_cid_slots;
}

void State::SetConnection( connection::Connection* connection ) {
	//std::lock_guard guard( m_connection_mutex );
	ASSERT( !m_connection, "state connection already set" );
	ASSERT( connection, "connection is null" );
	m_gc_space->WithNativeMutation([&]() { EnsureRosterEpoch(); connection->SetState(this); m_connection=connection; });
}

connection::Connection* State::GetConnection() {
	//std::lock_guard guard( m_connection_mutex );
	return m_connection;
}

void State::Reset() {
	RetireGameCallbacks();
 RetireRosterEpoch();
	{
		//std::lock_guard guard( m_connection_mutex );
		if ( m_connection ) {
			if ( m_connection->IsConnected() ) {
				auto* connection = m_connection;
				m_connection->m_on_disconnect = [ connection ]() -> bool {
					// TODO: do this synchronously?
					return true;
				};
				m_connection->Disconnect();
			}
			m_connection->SetState( nullptr );
			m_connection = nullptr;
		}
	}
	for ( auto& player : m_players ) {
		DELETE( player );
	}
	m_players.clear();
	m_slots->Clear();
	m_cid_slots.clear();
	m_game = nullptr;
	m_on_gse_error = nullptr;
}

void State::DetachConnection() {
	if ( m_connection ) {
		m_connection->SetState( nullptr );
		m_connection = nullptr;
	}
}

faction::FactionManager* State::GetFM() const {
	return m_fm;
}

gse::Value* const State::TriggerObject( gse::GCWrappable* object, const std::string& event, const gse::f_args_t& f_args ) {
	ASSERT( m_glsmac, "glsmac not set" );
	CHECKACCUM( m_gc_space );
	return m_glsmac->TriggerObject( object, event, f_args );
}

WRAPIMPL_BEGIN( State )
	WRAPIMPL_PROPS
		WRAPIMPL_TRIGGERS
		{
			"fm",
			m_fm->Wrap( GSE_CALL )
		},
	};
WRAPIMPL_END_PTR()

UNWRAPIMPL_PTR( State )

const types::Buffer State::Serialize() const {
	types::Buffer buf;

	buf.WriteString( m_settings.global.Serialize().ToString() );
	buf.WriteString( m_fm->Serialize().ToString() );
	ValidateSettingsEnvelope( buf );

	return buf;
}

void State::Deserialize( types::Buffer buf ) {
	ValidateSettingsEnvelope( buf );
	// Native packet dispatch can replace linked Factions while GC is active.
	// Keep queueing, notifications and transport in the caller, outside this
	// synchronous native-only mutation scope.
	m_gc_space->WithNativeMutation( [ & ]() {
		m_settings.global.Deserialize( buf.ReadString() );
		m_fm->Deserialize( buf.ReadString() );
	} );
}

void State::DeserializeRosterSettings(types::Buffer buf, connection::Connection* expected, const roster_epoch_t& epoch) {
 m_gc_space->WithNativeMutation([&]() {
  if (!expected || m_connection != expected) { throw std::runtime_error("Settings belong to another connection"); }
  RequireRoster(epoch);
  m_settings.global.Deserialize(buf.ReadString());
  m_fm->Deserialize(buf.ReadString());
 });
}

void State::GetReachableObjects( std::unordered_set< Object* >& reachable_objects ) {
	gse::GCWrappable::GetReachableObjects( reachable_objects );
 GetRosterRoots(reachable_objects);

	GC_DEBUG_BEGIN( "State" );

	if ( m_game ) {
		GC_DEBUG_BEGIN( "game" );
		GC_REACHABLE( m_game );
		GC_DEBUG_END();
	}

	if ( m_connection ) {
		GC_DEBUG_BEGIN( "connection" );
		GC_REACHABLE( m_connection );
		GC_DEBUG_END();
	}

	GC_DEBUG_END();
}

}
}
