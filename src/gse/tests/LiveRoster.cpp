#include "LiveRoster.h"
#include "engine/Engine.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/State.h"
#include "game/backend/connection/Client.h"
#include "game/backend/connection/Server.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/faction/FactionManager.h"
#include "game/backend/slot/Slots.h"
#include "gc/GC.h"
#include "gc/Space.h"
#include "gse/ExecutionPointer.h"
#include "gse/GSE.h"
#include "gse/context/GlobalContext.h"
#include "gse/value/Bool.h"
#include "gse/value/Callable.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include "network/Network.h"
#include "task/gsetests/GSETests.h"
#include "types/Packet.h"
#include "util/FinallyGuard.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>
#include <thread>

namespace gse::tests {
namespace {
using namespace game::backend;
void Check( bool condition, const char* message ) {
	if ( !condition ) {
		throw std::runtime_error( message );
	}
}
class Transport final : public network::Network {
public:
	~Transport() override {
		Stop();
	}
	void Incoming( const network::Event& event ) {
		AddEvent( event );
	}
	network::events_t sent;
	static constexpr size_t FrameLimit = BUFFER_SIZE;

protected:
	network::MT_Response ListenStart() override {
		network::Event event;
		event.type = network::Event::ET_LISTEN;
		event.data.remote_address = "fixture";
		AddEvent( event );
		return Success();
	}
	network::MT_Response ListenStop() override {
		return Success();
	}
	network::MT_Response Connect( const std::string&, MT_CANCELABLE ) override {
		return Success();
	}
	network::MT_Response Disconnect() override {
		return Success();
	}
	network::MT_Response DisconnectClient( network::cid_t ) override {
		return Success();
	}
	void ProcessEvents() override {
		auto events = GetEvents();
		sent.insert( sent.end(), events.begin(), events.end() );
	}
};
class Client final : public connection::Client {
public:
	Client( gc::Space* space, settings::LocalSettings* settings, network::Network* network )
		: connection::Client( space, settings, network ) {}
	void Receive( const types::Packet& packet ) {
		network::Event event;
		event.type = network::Event::ET_PACKET;
		event.data.packet_data = packet.Serialize().ToString();
		ProcessEvent( event );
	}
	size_t* destroyed = nullptr;
	~Client() override {
		if ( destroyed )
			++*destroyed;
	}
	const std::string& ErrorReason() const {
		return m_disconnect_reason;
	}
};
class Server final : public connection::Server {
public:
	Server( gc::Space* space, settings::LocalSettings* settings, network::Network* network )
		: connection::Server( space, settings, network ) {}
	void Receive( const network::Event& event ) {
		ProcessEvent( event );
	}
};
class CountedState final : public State {
public:
	CountedState( gc::Space* space, context::Context* context, size_t& count )
		: State( space, context, nullptr )
		, count( count ) {}
	~CountedState() override {
		++count;
	}

private:
	size_t& count;
};
class Fixture {
public:
	// Transport outlives every collector-owned Connection and queued MT payload.
	Transport transport;
	Game game;
	size_t state_destructions = 0, connection_destructions = 0;
	GSE runtime;
	context::GlobalContext* context = nullptr;
	State* state = nullptr;
	Client* client = nullptr;
	Server* server = nullptr;
	Fixture( bool is_server = false ) {
		runtime.GetGCSpace()->Accumulate( &runtime, [ & ]() {
			context = runtime.CreateGlobalContext();
			state = new CountedState( runtime.GetGCSpace(), context, state_destructions );
			runtime.AddRootObject( state );
			state->m_settings.global.Initialize();
			state->m_settings.global.SelectRuleset( "smax" );
			state->m_settings.global.InitializeHostRules( []( const std::string& ) -> std::string {
				throw std::runtime_error( "SMAX read original assets" );
			} );
			state->m_settings.local.player_name = "Host";
			state->m_settings.local.remote_address = "fixture";
			if ( is_server ) {
				server = new Server( runtime.GetGCSpace(), &state->m_settings.local, &transport );
				state->SetConnection( server );
			}
			else {
				client = new Client( runtime.GetGCSpace(), &state->m_settings.local, &transport );
				client->destroyed = &connection_destructions;
				state->SetConnection( client );
			}
			runtime.AddRootObject( client ? static_cast< gc::Object* >( client ) : static_cast< gc::Object* >( server ) );
		} );
	}
	~Fixture() {
		if ( !state ) {
			return;
		}
		auto* connection = client ? static_cast< connection::Connection* >( client ) : static_cast< connection::Connection* >( server );
		connection->ResetHandlers();
		runtime.GetGCSpace()->Accumulate( &runtime, [ & ]() {
			state->Reset();
		} );
		transport.Iterate();
		connection->IterateAndMaybeDelete( false );
	}
	void With( const std::function< void( GSE_CALLABLE ) >& f ) {
		runtime.GetGCSpace()->Accumulate( &runtime, [ & ]() {
			ExecutionPointer ep;
			f( runtime.GetGCSpace(), context, {}, ep );
		} );
	}
	void BindGame() {
		With( [ & ]( GSE_CALLABLE ) {
			state->SetGame( &game );
			runtime.AddRootObject( &game );
		} );
	}
	void Drain() {
		runtime.GetGCSpace()->ProcessAccumulations();
	}
	roster::RosterRecord Roster() const {
		roster::RosterRecord record;
		record.slots.resize( 7, { roster::SlotState::Open, std::nullopt, 0, {} } );
		record.slots[ 0 ] = { roster::SlotState::Occupied, roster::PlayerRecord{ "Host", roster::Role::Host, "GAIANS", "Citizen" }, 0,
							  "host-gsid" };
		record.slots[ 1 ] = { roster::SlotState::Occupied,
							  roster::PlayerRecord{ "Local", roster::Role::Player, std::nullopt, "Specialist" }, 0,
							  state->m_settings.local.account.GetGSID() };
		record.slots[ 2 ] = { roster::SlotState::Occupied, roster::PlayerRecord{ "Peer", roster::Role::Player, "GAIANS", "Talent" }, 0,
							  "peer-gsid" };
		return record;
	}
	std::string Settings() {
		settings::GlobalSettings global;
		global.Initialize();
		global.SelectRuleset( "smax" );
		global.InitializeHostRules( []( const std::string& ) -> std::string {
			throw std::runtime_error( "SMAX read assets" );
		} );
		faction::FactionManager factions;
		factions.Add( new faction::Faction( "GAIANS", "Canonical Gaian" ) );
		types::Buffer data;
		data.WriteString( global.Serialize().ToString() );
		data.WriteString( factions.Serialize().ToString() );
		return data.ToString();
	}
	void Poll( const network::Event& event, bool foreign = false ) {
		state->Iterate();
		transport.Incoming( event );
		transport.Iterate();
		if ( foreign ) {
			std::exception_ptr error;
			std::thread worker( [ & ]() {
				try {
					state->Iterate();
				}
				catch ( ... ) {
					error = std::current_exception();
				}
			} );
			worker.join();
			if ( error )
				std::rethrow_exception( error );
		}
		else {
			state->Iterate();
		}
	}
	void StartServer() {
		server->Connect();
		transport.Iterate();
		state->Iterate();
		state->Iterate();
		transport.Iterate();
		state->Iterate();
		Check( state->m_slots->GetCount() == 7, "actual listen event did not initialize server roster" );
	}
	void ServerPacket( types::Packet& packet, network::cid_t cid = 17, bool foreign = false ) {
		network::Event event;
		event.type = network::Event::ET_PACKET;
		event.cid = cid;
		event.data.remote_address = "remote";
		event.data.packet_data = packet.Serialize().ToString();
		Poll( event, foreign );
	}
	void Players( const roster::RosterRecord& record, size_t slot = 1 ) {
		types::Packet p( types::Packet::PT_PLAYERS );
		p.data.num = slot;
		p.data.str = roster::EncodeRoster( record, State::ROSTER_LIMITS );
		client->Receive( p );
	}
	void Phase( connection::Connection::game_state_t phase ) {
		types::Packet p( types::Packet::PT_GAME_STATE );
		p.udata.game_state.state = phase;
		client->Receive( p );
	}
	void SettingsPacket() {
		types::Packet p( types::Packet::PT_GLOBAL_SETTINGS );
		p.data.str = Settings();
		client->Receive( p );
	}
	void Init() {
		Players( Roster() );
		Phase( connection::Connection::GS_LOBBY );
		SettingsPacket();
		Check( client->ErrorReason().empty(), "initial packet sequence rejected" );
	}
	void Slot( size_t index, const roster::SlotRecord& slot ) {
		types::Packet p( types::Packet::PT_SLOT_UPDATE );
		p.data.num = index;
		p.data.str = roster::EncodeSlot( slot, State::ROSTER_LIMITS );
		client->Receive( p );
	}
	void Flags( size_t index, size_t flags ) {
		types::Packet p( types::Packet::PT_FLAGS_UPDATE );
		p.udata.flags.slot_num = index;
		p.udata.flags.flags = flags;
		client->Receive( p );
	}
	Player* PlayerAt( size_t index ) {
		return state->m_slots->GetSlot( index ).GetPlayer();
	}
};
}// namespace

void AddLiveRosterTests( task::gsetests::GSETests* task ) {
	const auto add = [ & ]( const std::string& name, const std::function< void() >& test ) {
		task->AddTest(
			"live roster " + name, GT( test ) {
				try {
					test();
				}
				catch ( const std::exception& error ) {
					GT_FAIL( error.what() );
				}
				GT_OK();
			} );
	};
	add( "initial packets wait for canonical settings before publishing", []() {
		Fixture f;
		size_t notifications = 0, lists = 0, settings = 0;
		f.client->m_on_players_list_update = [ & ]( size_t index ) {
			Check( index == 1 && f.PlayerAt( 0 )->GetFaction() == f.state->GetFM()->Get( "GAIANS" ),
				   "native list callback preceded canonical publication" );
			++lists;
		};
		f.client->m_on_global_settings_update = [ & ]() {
			++settings;
		};
		f.With( [ & ]( GSE_CALLABLE ) { f.client->On(GSE_CALL,"player_join",NATIVE_CALL(&f,&notifications) {
				N_EXPECT_ARGS( 1 );
				N_GETVALUE( event, 0, Object );
				N_GETPROP_UNWRAP( player, event, "player", Player );
				Check( f.state->m_slots->GetCount() == 7, "join preceded roster publication" );
				if ( player->GetRecord().faction_id ) {
					Check( player->GetFaction() == f.state->GetFM()->Get( "GAIANS" ), "join used noncanonical Faction" );
				}
				++notifications;
				return VALUE( value::Undefined );
  }));
		} );
		f.Players( f.Roster() );
		Check( f.state->m_slots->GetCount() == 0 && !lists && !notifications, "unconfigured roster escaped staging" );
		f.Phase( connection::Connection::GS_LOBBY );
		Check( f.state->m_slots->GetCount() == 0, "game-state packet published roster" );
		f.SettingsPacket();
		Check( f.client->ErrorReason().empty() && f.client->HasGlobalSettings(), "valid initial profile rejected" );
		Check( lists == 1 && settings == 1 && notifications == 3, "initial publication notification counts differ" );
		Check( f.PlayerAt( 1 ) == f.client->GetPlayer() && f.PlayerAt( 1 )->GetSlot() == &f.state->m_slots->GetSlot( 1 ),
			   "local binding or reverse link missing" );
		Check( !f.PlayerAt( 1 )->GetFaction(), "absent faction silently selected" );
	} );
	add( "continuous updates preserve identity while vacancy ends it", []() {
		Fixture f;
		f.Init();
		auto* original = f.PlayerAt( 2 );
		auto identity = original->GetIdentity();
		auto* slot = &f.state->m_slots->GetSlot( 2 );
		Value* wrapper = nullptr;
		f.With( [ & ]( GSE_CALLABLE ) {
			wrapper = original->Wrap( GSE_CALL );
			ctx->CreateConst( "original", wrapper, si, ep );
			original->CompleteTurn();
		} );
		auto record = f.state->ExportSlot( 2 );
		record.player->name = "Renamed";
		record.player->difficulty_id = "Thinker";
		f.Slot( 2, record );
		f.Flags( 2, slot::PF_READY );
		Check( f.client->ErrorReason().empty() && f.PlayerAt( 2 ) == original && original->GetIdentity() == identity
				   && original->IsTurnCompleted(),
			   "metadata update replaced authoritative identity/state" );
		Check( wrapper->type == VT_OBJECT && original->GetPlayerName() == "Renamed" && &f.state->m_slots->GetSlot( 2 ) == slot,
			   "continuous update invalidated wrapper or Slot" );
		f.Flags( 2, 0 );
		f.Slot( 2, { roster::SlotState::Open, std::nullopt, 0, {} } );
		Check( !identity->active && wrapper->type == VT_UNDEFINED && slot->GetState() == slot::Slot::SS_OPEN,
			   "vacancy retained live identity/wrapper" );
		f.Slot( 2, record );
		Check( f.client->ErrorReason().empty() && f.PlayerAt( 2 )->GetIdentity() != identity && !f.PlayerAt( 2 )->IsTurnCompleted(),
			   "rejoin reused departed Player state" );
	} );
	add( "faction replacement resolves current canonical ownership", []() {
		Fixture f;
		f.Init();
		Value* faction_wrapper = nullptr;
		auto* player = f.PlayerAt( 2 );
		f.With( [ & ]( GSE_CALLABLE ) {
			faction_wrapper = player->GetFaction()->Wrap( GSE_CALL );
			ctx->CreateConst( "old_faction", faction_wrapper, si, ep );
		} );
		f.SettingsPacket();
		Check( faction_wrapper->type == VT_UNDEFINED && player == f.PlayerAt( 2 )
				   && player->GetFaction() == f.state->GetFM()->Get( "GAIANS" ),
			   "settings replacement left embedded or stale Faction" );
		f.With( [ & ]( GSE_CALLABLE ) {
			f.state->GetFM()->Remove( "GAIANS" );
			bool rejected = false;
			try {
				player->GetFaction();
			}
			catch ( const std::runtime_error& ) {
				rejected = true;
			}
			Check( rejected, "missing selected Faction became random" );
		} );
	} );
	add( "invalid slot updates reject before native publication", []() {
		for ( int variant = 0; variant < 9; ++variant ) {
			Fixture f;
			f.Init();
			auto before = f.state->ExportRoster();
			auto* player = f.PlayerAt( 2 );
			auto identity = player->GetIdentity();
			auto record = before.slots[ 2 ];
			if ( variant == 0 )
				record.linked_gsid = "replacement-gsid";
			if ( variant == 1 )
				record.player->role = roster::Role::Host;
			if ( variant == 2 )
				record.player->faction_id = "missing";
			if ( variant == 3 )
				record.player->difficulty_id = "missing";
			if ( variant == 4 )
				record.linked_gsid = before.slots[ 1 ].linked_gsid;
			if ( variant == 5 ) {
				f.Flags( 2, slot::PF_READY );
				before = f.state->ExportRoster();
				record = before.slots[ 2 ];
				record.player->name = "Ready edit";
			}
			if ( variant == 6 ) {
				f.Phase( connection::Connection::GS_RUNNING );
				record.player->name = "Running edit";
			}
			if ( variant == 7 )
				f.Flags( 2, 256 );
			else
				f.Slot( variant == 8 ? 7 : 2, record );
			Check( !f.client->ErrorReason().empty(), "invalid update accepted" );
			Check( f.state->ExportRoster() == before && f.PlayerAt( 2 ) == player && identity->active,
				   "rejected update changed authoritative roster" );
		}
	} );
	add( "authoritative random-faction refresh preserves ready Players", []() {
		Fixture f;
		f.Init();
		auto* local = f.PlayerAt( 1 );
		f.Flags( 0, slot::PF_READY );
		f.Flags( 1, slot::PF_READY );
		auto record = f.state->ExportRoster();
		record.slots[ 1 ].player->faction_id = "GAIANS";
		f.Players( record, 0 );
		Check( f.client->ErrorReason().empty() && f.PlayerAt( 1 ) == local && local->GetFaction() == f.state->GetFM()->Get( "GAIANS" )
				   && f.state->m_slots->GetSlot( 1 ).HasPlayerFlag( slot::PF_READY ),
			   "startup faction assignment rejected or replaced ready Player" );
	} );
	add( "foreign notifications are rooted and reset cancels their epoch", []() {
		for ( bool reset : { false, true } ) {
			Fixture f;
			f.Init();
			size_t joins = 0;
			f.With( [ & ]( GSE_CALLABLE ) { f.client->On(GSE_CALL,"player_join",NATIVE_CALL(&joins) {
					++joins;
					return VALUE( value::Undefined ); }));
			} );
			auto record = f.state->ExportSlot( 2 );
			record.linked_gsid = "new-gsid";
			record.player->name = "New peer";
			f.Slot( 2, { roster::SlotState::Open, std::nullopt, 0, {} } );
			std::exception_ptr error;
			std::thread producer( [ & ]() {
				try {
					f.Slot( 2, record );
				}
				catch ( ... ) {
					error = std::current_exception();
				}
			} );
			producer.join();
			if ( error )
				std::rethrow_exception( error );
			Check( !joins, "foreign native commit ran owner notification" );
			g_engine->GetGC()->Iterate();
			if ( reset )
				f.With( [ & ]( GSE_CALLABLE ) {
					f.state->Reset();
				} );
			f.Drain();
			Check( joins == ( reset ? 0U : 1U ), "queued notification lost roots or crossed roster epoch" );
		}
	} );
	add( "notification values are read-only and expire after callbacks", []() {
		Fixture f;
		f.Init();
		Value* retained = nullptr;
		value::Callable* getter = nullptr;
		size_t joins = 0;
		f.With( [ & ]( GSE_CALLABLE ) { f.client->On(GSE_CALL,"player_join",NATIVE_CALL(&joins,&retained,&getter) {
				N_EXPECT_ARGS( 1 );
				N_GET( event, 0, Object );
				auto* p = static_cast< value::Object* >( event->Get( "player" ) );
				retained = p;
				ctx->CreateConst( "notice", p, si, ep );
				getter = static_cast< value::Callable* >( p->Get( "is_ready" ) );
				ctx->CreateConst( "notice_getter", getter, si, ep );
				bool denied = false;
				try {
					static_cast< value::Callable* >( p->Get( "set_ready" ) )->Run( GSE_CALL, { VALUE( value::Bool, , true ) } );
				}
				catch ( const Exception& ) {
					denied = true;
				}
				Check( denied, "notification mutated current Player" );
				++joins;
				return VALUE( value::Undefined );
  }));
		} );
		auto record = f.state->ExportSlot( 2 );
		f.Slot( 2, { roster::SlotState::Open, std::nullopt, 0, {} } );
		f.Slot( 2, record );
		Check( joins == 1 && retained && retained->type == VT_UNDEFINED, "notification wrapper outlived scope" );
		f.With( [ & ]( GSE_CALLABLE ) {
			bool denied = false;
			try {
				getter->Run( GSE_CALL, {} );
			}
			catch ( const Exception& ) {
				denied = true;
			}
			Check( denied, "retained method dereferenced expired native notification" );
		} );
	} );
	add( "encoded session limits fit real transport packet framing", []() {
		Fixture f;
		auto record = f.Roster();
		size_t text = 0;
		for ( const auto& s : record.slots ) {
			text += s.linked_gsid.size();
			if ( s.player ) {
				text += s.player->name.size() + s.player->difficulty_id.size();
				if ( s.player->faction_id )
					text += s.player->faction_id->size();
			}
		}
		record.slots[ 2 ].player->name.append( State::ROSTER_LIMITS.max_text_bytes - text, 'x' );
		types::Packet p( types::Packet::PT_PLAYERS );
		p.data.num = 1;
		p.data.str = roster::EncodeRoster( record, State::ROSTER_LIMITS );
		auto encoded = p.Serialize().ToString();
		Check( encoded.size() + sizeof( int ) <= Transport::FrameLimit, "live roster envelope exceeds actual SimpleTCP frame" );
		types::Packet decoded( types::Packet::PT_NONE );
		decoded.Deserialize( types::Buffer( encoded ) );
		Check( roster::DecodeRoster( decoded.data.str, State::ROSTER_LIMITS ) == record, "framed maximum roster changed bytes" );
		record.slots[ 2 ].player->name.push_back( 'x' );
		bool rejected = false;
		try {
			roster::EncodeRoster( record, State::ROSTER_LIMITS );
		}
		catch ( const std::invalid_argument& ) {
			rejected = true;
		}
		Check( rejected, "live aggregate text budget ignored" );
	} );
	add( "real server packets adopt update retire and reconnect owned Players", []() {
		Fixture f( true );
		f.StartServer();
		size_t joins = 0, leaves = 0;
		f.With( [ & ]( GSE_CALLABLE ) {f.server->On(GSE_CALL,"player_join",NATIVE_CALL(&joins){
				++joins;
				return VALUE( value::Undefined );}));f.server->On(GSE_CALL,"player_leave",NATIVE_CALL(&leaves){
				++leaves;
				return VALUE( value::Undefined );}));
		} );
		types::Packet auth( types::Packet::PT_AUTH );
		auth.data.vec = { "remote-gsid", "Remote" };
		f.ServerPacket( auth, 17, true );
		Check( !joins && f.state->GetCidSlots().at( 17 ) == 1, "foreign auth did not publish before owner notification" );
		g_engine->GetGC()->Iterate();
		f.Drain();
		Check( joins == 1, "auth join was not rooted/exactly once" );
		auto* player = f.PlayerAt( 1 );
		auto identity = player->GetIdentity();
		Value* wrapper = nullptr;
		f.With( [ & ]( GSE_CALLABLE ) {
			wrapper = player->Wrap( GSE_CALL );
			ctx->CreateConst( "server_player", wrapper, si, ep );
			player->CompleteTurn();
		} );
		f.transport.Iterate();
		std::vector< types::Packet::packet_type_t > order;
		bool got_roster = false;
		for ( const auto& e : f.transport.sent ) {
			if ( e.type != network::Event::ET_PACKET || e.cid != 17 )
				continue;
			types::Packet p( types::Packet::PT_NONE );
			p.Deserialize( types::Buffer( e.data.packet_data ) );
			if ( p.type == types::Packet::PT_PLAYERS || p.type == types::Packet::PT_GAME_STATE
				 || p.type == types::Packet::PT_GLOBAL_SETTINGS )
				order.push_back( p.type );
			if ( p.type == types::Packet::PT_PLAYERS ) {
				auto r = roster::DecodeRoster( p.data.str, State::ROSTER_LIMITS );
				got_roster = p.data.num == 1 && r.slots.size() == 7 && r.slots[ 1 ].linked_gsid == "remote-gsid";
			}
		}
		Check( got_roster
				   && order
					   == std::vector< types::Packet::packet_type_t >{ types::Packet::PT_PLAYERS, types::Packet::PT_GAME_STATE,
																	   types::Packet::PT_GLOBAL_SETTINGS },
			   "server writer changed handshake data/order" );
		auto record = f.state->ExportSlot( 1 );
		record.player->name = "Updated";
		record.player->difficulty_id = "Thinker";
		types::Packet update( types::Packet::PT_UPDATE_SLOT );
		update.data.str = roster::EncodeSlot( record, State::ROSTER_LIMITS );
		f.ServerPacket( update );
		Check( f.PlayerAt( 1 ) == player && player->GetPlayerName() == "Updated" && player->IsTurnCompleted(),
			   "client update replaced server Player identity" );
		types::Packet flags( types::Packet::PT_UPDATE_FLAGS );
		flags.udata.flags.flags = slot::PF_READY;
		f.ServerPacket( flags );
		Check( f.state->m_slots->GetSlot( 1 ).HasPlayerFlag( slot::PF_READY ), "server flags decoder did not commit" );
		network::Event disconnect;
		disconnect.type = network::Event::ET_CLIENT_DISCONNECT;
		disconnect.cid = 17;
		f.Poll( disconnect );
		Check( leaves == 1 && !identity->active && wrapper->type == VT_UNDEFINED
				   && f.state->m_slots->GetSlot( 1 ).GetState() == slot::Slot::SS_OPEN && !f.state->GetCidSlots().count( 17 ),
			   "lobby departure left ownership or notification behind" );
		f.ServerPacket( auth, 23 );
		player = f.PlayerAt( 1 );
		Check( player->GetIdentity() != identity && !player->IsTurnCompleted() && joins == 2, "same-GSID rejoin reused departed identity" );
		identity = player->GetIdentity();
		player->CompleteTurn();
		f.server->SetGameState( connection::Connection::GS_RUNNING );
		disconnect.cid = 23;
		f.Poll( disconnect );
		Check( leaves == 2 && identity->active && f.PlayerAt( 1 ) == player && !player->IsConnected(),
			   "running departure deleted reconnectable Player" );
		f.ServerPacket( auth, 29 );
		Check( joins == 3 && f.PlayerAt( 1 ) == player && player->GetIdentity() == identity && player->IsTurnCompleted()
				   && player->IsConnected() && f.state->GetCidSlots().at( 29 ) == 1,
			   "running reconnect lost native state or ownership links" );
	} );
	add( "initial publication survives throwing and reentrant join callbacks", []() {
		for ( bool reset : { false, true } ) {
			Fixture f;
			size_t calls = 0;
			f.With( [ & ]( GSE_CALLABLE ) {f.client->On(GSE_CALL,"player_join",NATIVE_CALL(&f,&calls,reset){
					++calls;
					if ( reset ) {
						f.state->Reset();
					}
					else {
						throw std::runtime_error( "join callback failure" );
					}
					return VALUE( value::Undefined );}));
			} );
			f.Players( f.Roster() );
			f.Phase( connection::Connection::GS_LOBBY );
			f.SettingsPacket();
			Check( calls == 1, "callback sequence continued after failure/epoch retirement" );
			if ( reset ) {
				Check( f.state->m_slots->GetCount() == 0, "reentrant reset republished its roster" );
			}
			else {
				Check( f.state->m_slots->GetCount() == 7 && f.client->GetPlayer() == f.PlayerAt( 1 ),
					   "throwing callback lost committed ownership" );
			}
			f.Drain();
			g_engine->GetGC()->Iterate();
		}
	} );
	add( "invalid initial identity and late settings keep roster private", []() {
		for ( int variant = 0; variant < 5; ++variant ) {
			Fixture f;
			auto r = f.Roster();
			if ( variant == 0 )
				r.slots[ 1 ].linked_gsid = "foreign";
			if ( variant == 1 )
				r.slots.resize( 6 );
			if ( variant == 2 )
				r.slots[ 0 ].player->role = roster::Role::Player;
			if ( variant == 3 )
				r.slots[ 2 ].player->faction_id = "missing";
			f.Players( r );
			f.Phase( connection::Connection::GS_LOBBY );
			if ( variant == 4 ) {
				types::Packet p( types::Packet::PT_GLOBAL_SETTINGS );
				p.data.str = f.Settings();
				p.data.str.pop_back();
				f.client->Receive( p );
			}
			else
				f.SettingsPacket();
			Check( !f.client->ErrorReason().empty() && f.state->m_slots->GetCount() == 0 && !f.client->HasGlobalSettings(),
				   "invalid initial roster/settings became visible" );
		}
	} );

	add( "slot update callbacks may reset before native packet continuation", []() {
		for ( int mode = 0; mode < 3; ++mode ) {
			Fixture f;
			f.Init();
			f.BindGame();
			size_t callbacks = 0, native_calls = 0;
			f.client->m_on_slot_update = [ & ]( size_t, slot::Slot* ) {
				++native_calls;
			};
			f.client->m_on_flags_update = [ & ]( size_t, slot::Slot*, slot::player_flag_t, slot::player_flag_t ) {
				++native_calls;
			};
			f.client->m_on_players_list_update = [ & ]( size_t ) {
				++native_calls;
			};
			f.With( [ & ]( GSE_CALLABLE ) {f.game.On(GSE_CALL,"player_update",NATIVE_CALL(&f,&callbacks){
					++callbacks;
					f.state->Reset();
					return VALUE( value::Undefined );}));
			} );
			auto record = f.state->ExportSlot( 2 );
			record.player->name = "Reset trigger";
			if ( mode == 0 )
				f.Slot( 2, record );
			else if ( mode == 1 )
				f.Flags( 2, slot::PF_READY );
			else {
				auto roster = f.state->ExportRoster();
				roster.slots[ 2 ] = record;
				f.Players( roster, 0 );
			}
			Check( callbacks == 1 && !native_calls && f.state->m_slots->GetCount() == 0,
				   "packet continued through retired roster after immediate script callback" );
		}
	} );
	add( "actual packet retirement excludes collection and releases before native callbacks", []() {
		Fixture f;
		f.client->Connect();
		f.transport.Iterate();
		f.state->Iterate();
		f.Init();
		struct Gate {
			std::mutex mutex;
			std::condition_variable cv;
			bool entered = false, released = false;
			void Enter() {
				std::unique_lock lock( mutex );
				entered = true;
				cv.notify_all();
				cv.wait( lock, [ & ]() {
					return released;
				} );
			}
			bool Wait( std::chrono::milliseconds timeout ) {
				std::unique_lock lock( mutex );
				return cv.wait_for( lock, timeout, [ & ]() {
					return entered;
				} );
			}
			void Release() {
				std::lock_guard lock( mutex );
				released = true;
				cv.notify_all();
			}
		} destruction, collection;
		struct Observer : Wrappable {
			Value* const Wrap( GSE_CALLABLE ) override {
				return nullptr;
			}
			std::function< void() > notify;
			std::atomic< bool > retired{ false };
			void NotifyDependencyDestruction( const Wrappable* ) override {
				retired = true;
				notify();
			}
		} observer;
		struct Root : gc::Object {
			Root()
				: gc::Object( nullptr ) {}
			std::function< void() > visit;
			void GetReachableObjects( std::unordered_set< gc::Object* >& reachable ) override {
				gc::Object::GetReachableObjects( reachable );
				visit();
			}
		} root;
		auto* player = f.PlayerAt( 2 );
		Value* wrapper = nullptr;
		bool observing = false, rooted = false;
		std::thread producer, collector, callback_collector;
		std::exception_ptr producer_error, collector_error, callback_error;
		std::mutex id_mutex;
		std::thread::id collector_id;
		std::promise< void > started, callback_done;
		auto callback_future = callback_done.get_future();
		bool crossed = false, callback_collected = false;
		util::FinallyGuard cleanup( [ & ]() {
			destruction.Release();
			collection.Release();
			if ( producer.joinable() )
				producer.join();
			if ( collector.joinable() )
				collector.join();
			if ( callback_collector.joinable() )
				callback_collector.join();
			f.Drain();
			f.With( [ & ]( GSE_CALLABLE ) {
				if ( observing && !observer.retired )
					player->Undepend( &observer );
				if ( rooted )
					f.runtime.RemoveRootObject( &root );
			} );
		} );
		observer.notify = [ & ]() {
			destruction.Enter();
		};
		root.visit = [ & ]() {
			std::lock_guard lock( id_mutex );
			if ( std::this_thread::get_id() == collector_id )
				collection.Enter();
		};
		f.With( [ & ]( GSE_CALLABLE ) {
			wrapper = player->Wrap( GSE_CALL );
			ctx->CreateConst( "retiring", wrapper, si, ep );
			player->Depend( &observer );
			observing = true;
			f.runtime.AddRootObject( &root );
			rooted = true;
		} );
		f.client->m_on_slot_update = [ & ]( size_t, slot::Slot* ) {
			callback_collector = std::thread( [ & ]() {
				try {
					g_engine->GetGC()->Iterate();
				}
				catch ( ... ) {
					callback_error = std::current_exception();
				}
				callback_done.set_value();
			} );
			callback_collected = callback_future.wait_for( std::chrono::seconds( 2 ) ) == std::future_status::ready;
		};
		types::Packet packet( types::Packet::PT_SLOT_UPDATE );
		packet.data.num = 2;
		packet.data.str = roster::EncodeSlot( { roster::SlotState::Open, std::nullopt, 0, {} }, State::ROSTER_LIMITS );
		network::Event event;
		event.type = network::Event::ET_PACKET;
		event.data.packet_data = packet.Serialize().ToString();
		f.state->Iterate();
		f.transport.Incoming( event );
		f.transport.Iterate();
		producer = std::thread( [ & ]() {
			try {
				f.state->Iterate();
			}
			catch ( ... ) {
				producer_error = std::current_exception();
			}
		} );
		Check( destruction.Wait( std::chrono::seconds( 2 ) ), "actual MT packet did not reach Player retirement" );
		Check( wrapper->type == VT_UNDEFINED && observer.retired, "retirement observer ran before wrapper invalidation" );
		collector = std::thread( [ & ]() {
			{
				std::lock_guard lock( id_mutex );
				collector_id = std::this_thread::get_id();
			}
			started.set_value();
			try {
				g_engine->GetGC()->Iterate();
			}
			catch ( ... ) {
				collector_error = std::current_exception();
			}
		} );
		Check( started.get_future().wait_for( std::chrono::seconds( 2 ) ) == std::future_status::ready, "collector thread did not start" );
		crossed = collection.Wait( std::chrono::milliseconds( 100 ) );
		collection.Release();
		if ( crossed )
			collector.join();// copied control safely completes while no linked target remains
		destruction.Release();
		producer.join();
		if ( collector.joinable() )
			collector.join();
		if ( callback_collector.joinable() )
			callback_collector.join();
		if ( producer_error )
			std::rethrow_exception( producer_error );
		if ( collector_error )
			std::rethrow_exception( collector_error );
		if ( callback_error )
			std::rethrow_exception( callback_error );
		Check( !crossed, "collection entered during native Player retirement" );
		Check( callback_collected, "native packet callback still held collection exclusion" );
		Check( f.state->m_slots->GetSlot( 2 ).GetState() == slot::Slot::SS_OPEN && !f.client->ErrorReason().size(),
			   "retirement failed to commit its vacant Slot" );
	} );

	add( "queued notification roots survive reset and release after cancellation", []() {
		Fixture f;
		f.Init();
		auto record = f.state->ExportSlot( 2 );
		f.Slot( 2, { roster::SlotState::Open, std::nullopt, 0, {} } );
		std::exception_ptr error;
		std::thread producer( [ & ]() {
			try {
				f.Slot( 2, record );
			}
			catch ( ... ) {
				error = std::current_exception();
			}
		} );
		producer.join();
		if ( error )
			std::rethrow_exception( error );
		auto* state = f.state;
		auto* client = f.client;
		f.With( [ & ]( GSE_CALLABLE ) {
			state->Reset();
			f.runtime.RemoveRootObject( state );
			f.runtime.RemoveRootObject( client );
		} );
		// The pending callback is now the only collector root for both owners.
		g_engine->GetGC()->Iterate();
		Check( !f.state_destructions && !f.connection_destructions, "pending notification lost State or Connection root" );
		f.Drain();
		f.state = nullptr;
		f.client = nullptr;
		g_engine->GetGC()->Iterate();
		Check( f.state_destructions == 1 && f.connection_destructions == 1, "canceled notification retained ownership after queue drain" );
	} );
	add( "queued callback exceptions do not replay consumed notifications", []() {
		for ( bool script_error : { false, true } ) {
			Fixture f;
			f.Init();
			f.BindGame();
			size_t calls = 0;
			Value* snapshot = nullptr;
			f.With( [ & ]( GSE_CALLABLE ) {f.game.On(GSE_CALL,"player_update",NATIVE_CALL(&calls,&snapshot,script_error){
					N_EXPECT_ARGS( 1 );
					N_GET( event, 0, Object );
					snapshot = event->Get( "player" );
					ctx->CreateConst( "throwing_notice" + std::to_string( calls ), snapshot, si, ep );
					++calls;
					if ( calls == 1 ) {
						if ( script_error ) {
							GSE_ERROR( gse::EC.GAME_ERROR, "notification fixture" );
						}
						throw std::runtime_error( "notification fixture" );
					}
					return VALUE( value::Undefined );}));
			} );
			auto roster = f.state->ExportRoster();
			roster.slots[ 0 ].player->name = "First";
			roster.slots[ 2 ].player->name = "Second";
			std::exception_ptr error;
			std::thread producer( [ & ]() {
				try {
					f.Players( roster, 0 );
				}
				catch ( ... ) {
					error = std::current_exception();
				}
			} );
			producer.join();
			if ( error )
				std::rethrow_exception( error );
			bool threw = false;
			try {
				f.Drain();
			}
			catch ( const Exception& ) {
				threw = true;
			}
			catch ( const std::runtime_error& ) {
				threw = true;
			}
			Check( threw && calls == 1 && snapshot->type == VT_UNDEFINED, "throwing notification did not invalidate its scoped value" );
			f.Drain();
			Check( calls == 2 && snapshot->type == VT_UNDEFINED, "retry replayed a consumed notification or retained its value" );
			f.Drain();
			Check( calls == 2, "completed batch dispatched twice" );
		}
	} );

	add( "server admission rejects aggregate overflow before adopting a Player", []() {
		Fixture f( true );
		f.state->m_settings.local.player_name = std::string( 20000, 'h' );
		f.StartServer();
		const auto before = f.state->ExportRoster();
		auto* host = f.PlayerAt( 0 );
		size_t joins = 0;
		f.With( [ & ]( GSE_CALLABLE ) {f.server->On(GSE_CALL,"player_join",NATIVE_CALL(&joins){
				++joins;
				return VALUE( value::Undefined );}));
		} );
		types::Packet auth( types::Packet::PT_AUTH );
		auth.data.vec = { "remote-gsid", std::string( 20000, 'p' ) };
		f.ServerPacket( auth );
		Check( f.state->ExportRoster() == before && f.PlayerAt( 0 ) == host && !f.state->GetCidSlots().count( 17 ) && !joins,
			   "aggregate overflow partially adopted a Player" );
	} );
	add( "stale settings and resized refresh reject without replacing current ownership", []() {
		Fixture f;
		f.Init();
		const auto before = f.state->ExportRoster();
		auto* player = f.PlayerAt( 2 );
		auto resized = before;
		resized.slots.resize( 6 );
		f.Players( resized, 0 );
		Check( !f.client->ErrorReason().empty() && f.state->ExportRoster() == before && f.PlayerAt( 2 ) == player,
			   "resized refresh changed fixed Slot storage" );
		const auto old_epoch = f.state->CaptureRosterEpoch();
		auto settings = f.Settings();
		f.With( [ & ]( GSE_CALLABLE ) {
			f.state->Reset();
			f.state->SetConnection( f.client );
		} );
		auto* retained_faction = f.state->GetFM()->Get( "GAIANS" );
		bool rejected = false;
		try {
			f.state->DeserializeRosterSettings( types::Buffer( settings ), f.client, old_epoch );
		}
		catch ( const std::runtime_error& ) {
			rejected = true;
		}
		Check( rejected && f.state->m_slots->GetCount() == 0 && f.state->GetFM()->Get( "GAIANS" ) == retained_faction,
			   "stale settings crossed a roster epoch" );
	} );
	add( "server listen and ready callbacks may retire their roster before continuation", []() {
		for ( int mode = 0; mode < 4; ++mode ) {
			Fixture f( true );
			size_t callbacks = 0, joins = 0;
			if ( mode == 0 )
				f.server->m_on_listen = [ & ]() {
					++callbacks;
					f.With( [ & ]( GSE_CALLABLE ) {
						f.state->Reset();
					} );
				};
			if ( mode == 1 )
				f.server->m_on_global_settings_update = [ & ]() {
					++callbacks;
					f.With( [ & ]( GSE_CALLABLE ) {
						f.state->Reset();
					} );
				};
			if ( mode == 2 )
				f.With( [ & ]( GSE_CALLABLE ) {f.server->On(GSE_CALL,"player_join",NATIVE_CALL(&f,&callbacks){
						++callbacks;
						f.state->Reset();
						return VALUE( value::Undefined );}));
				} );
			if ( mode < 3 ) {
				network::Event listen;
				listen.type = network::Event::ET_LISTEN;
				listen.data.remote_address = "fixture";
				f.server->Receive( listen );
			}
			else {
				f.StartServer();
				f.BindGame();
				f.With( [ & ]( GSE_CALLABLE ) {
					f.state->m_slots->GetSlot( 0 ).SetPlayerFlag( slot::PF_READY );f.game.On(GSE_CALL,"player_update",NATIVE_CALL(&f,&callbacks){
						++callbacks;
						f.state->Reset();
						return VALUE( value::Undefined );}));f.server->On(GSE_CALL,"player_join",NATIVE_CALL(&joins){
						++joins;
						return VALUE( value::Undefined );}));
				} );
				types::Packet auth( types::Packet::PT_AUTH );
				auth.data.vec = { "new-gsid", "New" };
				f.ServerPacket( auth );
			}
			Check( callbacks == 1 && !joins && f.state->m_slots->GetCount() == 0, "server continued using a synchronously retired roster" );
		}
	} );
	add( "decoded updates retain their exact epoch across same-connection reset and rebind", []() {
		Fixture f;
		f.Init();
		const auto record = f.state->ExportRoster();
		const auto epoch = f.state->CaptureRosterEpoch();
		f.With( [ & ]( GSE_CALLABLE ) {
			f.state->Reset();
			f.state->SetConnection( f.client );
		} );
		Check( !f.client->GetPlayer(), "detached Connection retained a retired local Player binding" );
		const auto replacement = f.state->CaptureRosterEpoch();
		f.state->ApplyInitialRoster( record, f.client, replacement, 1 );
		auto* player = f.PlayerAt( 2 );
		const auto identity = player->GetIdentity();
		auto changed = record;
		changed.slots[ 2 ].player->name = "Stale decoded metadata";
		const std::vector< std::function< void() > > attempts = { [ & ]() {
																	 f.state->ApplySlotUpdate( 2, changed.slots[ 2 ], f.client, epoch );
																 },
																  [ & ]() {
																	  f.state->ApplyFlagsUpdate( 2, slot::PF_READY, f.client, epoch );
																  },
																  [ & ]() {
																	  f.state->ApplyRosterRefresh( changed, f.client, epoch );
																  },
																  [ & ]() {
																	  f.state->ApplyInitialRoster( changed, f.client, epoch, 1 );
																  } };
		for ( const auto& attempt : attempts ) {
			bool rejected = false;
			try {
				attempt();
			}
			catch ( const std::runtime_error& ) {
				rejected = true;
			}
			Check( rejected && f.state->ExportRoster() == record && f.PlayerAt( 2 ) == player && player->GetIdentity() == identity,
				   "decoded old-epoch update mutated replacement roster" );
		}
	} );
	add( "actual open callback waits for initial settings in immediate and deferred modes", []() {
		for ( bool deferred : { false, true } ) {
			Fixture f;
			size_t opened = 0;
			std::vector< std::string > order;
			f.client->m_on_players_list_update = [ & ]( size_t ) {
				order.push_back( "native-list" );
			};
			f.client->m_on_global_settings_update = [ & ]() {
				order.push_back( "native-settings" );
			};
			f.With( [ & ]( GSE_CALLABLE ) {f.client->On(GSE_CALL,"player_join",NATIVE_CALL(&order){
					order.push_back( "join" );
					return VALUE( value::Undefined );}));
				auto* wrapper = static_cast< value::Object* >( f.client->Wrap( GSE_CALL ) );static_cast<value::Callable*>(wrapper->Get("open"))->Run(GSE_CALL,{NATIVE_CALL(&f,&opened,&order){Check(f.client->HasGlobalSettings()&&f.state->m_slots->GetCount()==7&&f.PlayerAt(0)->GetFaction()==f.state->GetFM()->Get("GAIANS"),"open ran before canonical initial publication");++opened;order.push_back("open");return VALUE(value::Undefined);
			} )
		});
	} );
	f.transport.Iterate();
	f.state->Iterate();
	Check( !opened, "transport open announced unconfigured roster" );
	f.Players( f.Roster() );
	f.Phase( connection::Connection::GS_LOBBY );
	Check( !opened && f.state->m_slots->GetCount() == 0, "initial packet escaped settings barrier" );
	if ( deferred ) {
		std::exception_ptr error;
		std::thread producer( [ & ]() {
			try {
				f.SettingsPacket();
			}
			catch ( ... ) {
				error = std::current_exception();
			}
		} );
		producer.join();
		if ( error )
			std::rethrow_exception( error );
		Check( !opened, "foreign settings ran owner callback" );
		f.Drain();
	}
	else
		f.SettingsPacket();
	const std::vector< std::string > expected = deferred
		? std::vector< std::string >{ "native-list", "native-settings", "join", "join", "join", "open" }
		: std::vector< std::string >{ "join", "join", "join", "native-list", "native-settings", "open" };
	Check( opened == 1 && order == expected, "initial publication changed native/queued notification order" );
}
});
}
}// namespace gse::tests
