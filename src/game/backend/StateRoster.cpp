#include "Game.h"
#include "Player.h"
#include "State.h"
#include "connection/Client.h"
#include "connection/Connection.h"
#include "faction/Faction.h"
#include "faction/FactionManager.h"
#include "gc/Space.h"
#include "gse/ExecutionPointer.h"
#include "slot/Slots.h"
#include "util/FinallyGuard.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend {
namespace {
void Require( bool condition, const char* message ) {
	if ( !condition ) {
		throw std::runtime_error( message );
	}
}
struct DeletePlayer {
	void operator()( Player* player ) const {
		if ( player ) {
			DELETE( player );
		}
	}
};
using OwnedPlayer = std::unique_ptr< Player, DeletePlayer >;
}// namespace

struct State::RosterNotifications final : gc::Object {
	enum Kind { Join, Leave, Update };
	struct Entry {
		Kind kind;
		size_t slot;
		roster::PlayerRecord player;
		bool ready;
		bool connected;
		player_identity_t identity;
	};
	RosterNotifications( State* state, connection::Connection* connection, roster_epoch_t epoch )
		: gc::Object( nullptr )
		, state( state )
		, connection( connection )
		, epoch( std::move( epoch ) ) {}
	void GetReachableObjects( std::unordered_set< gc::Object* >& reachable_objects ) override {
		gc::Object::GetReachableObjects( reachable_objects );
		GC_REACHABLE( state );
		if ( connection ) {
			GC_REACHABLE( connection );
		}
	}
	void Add( Kind kind, size_t slot, Player* player, bool ready ) {
		entries.push_back( { kind, slot, player->GetRecord(), ready, player->IsConnected(), player->GetIdentity() } );
	}
	State* const state;
	connection::Connection* const connection;
	const roster_epoch_t epoch;
	std::vector< Entry > entries;
	size_t next_entry = 0;// consumed before dispatch so a throwing queued callback cannot replay it
};

void State::EnsureRosterEpoch() {
	if ( !m_roster_epoch || !m_roster_epoch->active ) {
		m_roster_epoch = std::make_shared< RosterEpoch >( this );
	}
}
void State::RetireRosterEpoch() {
	if ( m_roster_epoch ) {
		m_roster_epoch->active.store( false );
	}
}
roster_epoch_t State::CaptureRosterEpoch() const {
	roster_epoch_t epoch;
	m_gc_space->WithNativeMutation( [ & ]() {
		epoch = m_roster_epoch;
	} );
	return epoch;
}
faction::FactionManager* State::RequireRoster( const roster_epoch_t& epoch ) const {
	Require( epoch && epoch->active && epoch->state == this && epoch == m_roster_epoch, "Roster identity is no longer active" );
	return m_fm;
}
void State::ValidatePlayerRecord( const roster::PlayerRecord& record ) const {
	roster::EncodePlayer( record, ROSTER_LIMITS );
	Require( m_settings.global.rules.m_difficulty_levels.GetKV().count( record.difficulty_id ), "Unknown Player difficulty ID" );
	if ( record.faction_id ) {
		Require( m_fm->Get( *record.faction_id ), "Selected Player faction is missing from the canonical manager" );
	}
}
roster::RosterRecord State::ExportRosterNative() const {
	roster::RosterRecord result;
	for ( const auto& slot : m_slots->m_slots ) {
		roster::SlotRecord entry{ static_cast< roster::SlotState >( slot.m_slot_state ), std::nullopt, 0, {} };
		if ( slot.m_slot_state == slot::Slot::SS_PLAYER ) {
			Require( slot.m_player_data.player && m_players.count( slot.m_player_data.player ), "Slot Player is not State-owned" );
			Require( slot.m_player_data.player->GetSlot() == &slot, "Player reverse Slot link is invalid" );
			entry.player = slot.m_player_data.player->GetRecord();
			entry.flags = slot.m_player_data.flags;
			entry.linked_gsid = slot.m_linked_gsid;
		}
		result.slots.push_back( std::move( entry ) );
	}
	return result;
}
roster::RosterRecord State::ExportRoster() {
	roster::RosterRecord result;
	m_gc_space->WithNativeMutation( [ & ]() {
		result = ExportRosterNative();
		roster::EncodeRoster( result, ROSTER_LIMITS );
	} );
	return result;
}
roster::SlotRecord State::ExportSlot( size_t index ) {
	auto roster = ExportRoster();
	Require( index < roster.slots.size(), "Slot index out of range" );
	return std::move( roster.slots[ index ] );
}
std::shared_ptr< State::RosterNotifications > State::MakeRosterNotifications() {
	return std::make_shared< RosterNotifications >( this, m_connection, m_roster_epoch );
}
void State::GetRosterRoots( std::unordered_set< gc::Object* >& reachable_objects ) {
	std::lock_guard lock( m_roster_notifications_mutex );
	for ( const auto& roots : m_roster_notifications ) {
		GC_REACHABLE( roots.get() );
	}
}
void State::DispatchRosterNotifications( const std::shared_ptr< RosterNotifications >& batch ) {
	// Native commit published the handoff before leaving collect exclusion.
	// Queue registration must never run while holding the root-list/collect lock.
	util::FinallyGuard handoff( [ & ]() {
		std::lock_guard lock( m_roster_notifications_mutex );
		m_roster_notifications.erase( std::remove( m_roster_notifications.begin(), m_roster_notifications.end(), batch ),
									  m_roster_notifications.end() );
	} );
	m_gc_space->Accumulate( batch.get(), [ batch ]() {
		if ( !batch->epoch || !batch->epoch->active ) {
			return;
		}
		auto* state = batch->state;
		if ( state->CaptureRosterEpoch() != batch->epoch || state->m_connection != batch->connection ) {
			return;
		}
		gse::ExecutionPointer ep;
		while ( batch->next_entry < batch->entries.size() ) {
			const auto& entry = batch->entries[ batch->next_entry++ ];
			if ( !batch->epoch->active || state->m_connection != batch->connection ) {
				return;
			}
			if ( entry.kind == RosterNotifications::Update && !entry.identity->active ) {
				continue;
			}
			// This native value exists only during accumulation. Destruction invalidates
			// all wrappers even on a throwing callback; no f_cleanup assumption is used.
			Player snapshot( entry.player, state, batch->epoch );
			snapshot.m_notification = true;
			snapshot.m_notification_ready = entry.ready;
			snapshot.m_slotnum = entry.slot;
			snapshot.m_is_connected = entry.connected;
			const gse::f_args_t args = [ &snapshot ]( GSE_CALLABLE, gse::value::object_properties_t& args ) {
				args = { { "player", snapshot.Wrap( GSE_CALL ) } };
			};
			if ( entry.kind == RosterNotifications::Update ) {
				if ( state->m_game ) {
					state->m_game->Trigger( state->m_gc_space, state->m_ctx, {}, ep, "player_update", args );
				}
			}
			else if ( batch->connection ) {
				batch->connection->Trigger( state->m_gc_space, state->m_ctx, {}, ep,
											entry.kind == RosterNotifications::Join ? "player_join" : "player_leave", args );
			}
		}
	} );
}

std::shared_ptr< State::RosterNotifications > State::ApplyRosterNative( roster::RosterRecord record, connection::Connection* expected,
																		const roster_epoch_t& epoch, bool initial, size_t local_slot,
																		bool flags_only ) {
	Require( expected && m_connection == expected, "Roster update belongs to another connection" );
	RequireRoster( epoch );
	roster::EncodeRoster( record, ROSTER_LIMITS );
	Require( record.slots.size() == ROSTER_LIMITS.max_slots, "Live sessions require seven slots" );
	Require( local_slot < record.slots.size(), "Local slot is out of range" );
	Require( record.slots[ 0 ].player && record.slots[ 0 ].player->role == roster::Role::Host, "Host must occupy slot zero" );
	Require( record.slots[ local_slot ].player.has_value(), "Local Player cannot be removed from its roster" );
	if ( expected->IsClient() ) {
		Require( local_slot != 0 && record.slots[ local_slot ].player->role == roster::Role::Player, "Client slot has an invalid role" );
	}
	if ( initial ) {
		Require( m_players.empty(), "Initial roster already published" );
	}
	else {
		Require( m_slots->GetCount() == record.slots.size(), "Roster size cannot change after publication" );
	}
	for ( size_t i = 0; i < record.slots.size(); ++i ) {
		const auto& value = record.slots[ i ];
		if ( value.player ) {
			ValidatePlayerRecord( *value.player );
			Require( i == 0 || value.player->role == roster::Role::Player, "Non-host slot has invalid role" );
		}
		if ( initial ) {
			continue;
		}
		const auto& current = m_slots->m_slots[ i ];
		const bool occupied = current.m_slot_state == slot::Slot::SS_PLAYER;
		if ( occupied && value.player ) {
			Require( current.m_linked_gsid == value.linked_gsid
						 && current.GetPlayer()->GetRole() == static_cast< Player::role_t >( value.player->role ),
					 "Occupied Slot identity changed without vacancy" );
		}
		if ( expected->m_game_state != connection::Connection::GS_LOBBY ) {
			Require( occupied == value.player.has_value(), "Cannot replace world-bound Players" );
			if ( occupied ) {
				Require( current.GetPlayer()->GetRecord() == *value.player, "Cannot change world-bound Player metadata" );
			}
		}
		else if ( !flags_only ) {
			const bool ready = occupied ? ( current.m_player_data.flags & slot::PF_READY )
										: ( m_slots->m_slots[ 0 ].m_player_data.flags & slot::PF_READY );
			const auto old = ExportRosterNative().slots[ i ];
			if ( !( old == value ) && ready && ( !value.player || ( value.flags & slot::PF_READY ) ) ) {
				throw std::runtime_error( "Slot metadata changed while ready" );
			}
		}
	}
	// All allocations occur before publishing pointers. New Players have no wrappers.
	std::vector< OwnedPlayer > created( record.slots.size() );
	auto owned = m_players;
	auto batch = MakeRosterNotifications();
	std::vector< slot::Slot > initial_slots;
	if ( initial ) {
		initial_slots.reserve( record.slots.size() );
		for ( size_t i = 0; i < record.slots.size(); ++i ) {
			initial_slots.emplace_back( i, this );
		}
	}
	for ( size_t i = 0; i < record.slots.size(); ++i ) {
		auto& value = record.slots[ i ];
		Player* previous =
			!initial && m_slots->m_slots[ i ].m_slot_state == slot::Slot::SS_PLAYER ? m_slots->m_slots[ i ].m_player_data.player : nullptr;
		if ( previous && !value.player ) {
			batch->Add( RosterNotifications::Leave, i, previous, m_slots->m_slots[ i ].m_player_data.flags & slot::PF_READY );
			owned.erase( previous );
		}
		if ( !previous && value.player ) {
			NEWV( player, Player, *value.player, this, m_roster_epoch );
			created[ i ].reset( player );
			owned.insert( player );
			batch->Add( RosterNotifications::Join, i, player, value.flags & slot::PF_READY );
		}
		else if ( previous && value.player
				  && !( previous->GetRecord() == *value.player && m_slots->m_slots[ i ].m_player_data.flags == value.flags ) ) {
			batch->entries.push_back( { RosterNotifications::Update, i, *value.player, bool( value.flags & slot::PF_READY ),
										previous->IsConnected(), previous->GetIdentity() } );
		}
	}
	{
		std::lock_guard lock( m_roster_notifications_mutex );
		m_roster_notifications.push_back( batch );
	}
	if ( initial ) {
		m_slots->m_slots.swap( initial_slots );
	}
	m_players.swap( owned );
	for ( size_t i = 0; i < record.slots.size(); ++i ) {
		auto& slot = m_slots->m_slots[ i ];
		auto& value = record.slots[ i ];
		auto* previous = slot.m_player_data.player;
		if ( value.player ) {
			auto* player = created[ i ] ? created[ i ].release() : previous;
			player->ApplyRecord( *value.player );
			player->SetSlot( &slot );
			slot.m_player_data.player = player;
			slot.m_player_data.flags = value.flags;
		}
		else {
			slot.m_player_data.player = nullptr;
			slot.m_player_data.flags = 0;
			slot.m_player_data.cid = 0;
			slot.m_player_data.remote_address.clear();
			if ( previous ) {
				previous->SetSlot( nullptr );
				DELETE( previous );
			}
		}
		slot.m_slot_state = static_cast< slot::Slot::slot_state_t >( value.state );
		slot.m_linked_gsid.swap( value.linked_gsid );
	}
	expected->m_slot = local_slot;
	expected->m_player = m_slots->m_slots[ local_slot ].m_player_data.player;
	return batch;
}
void State::ApplyInitialRoster( const roster::RosterRecord& record, connection::Connection* expected, const roster_epoch_t& epoch,
								size_t local_slot, const std::function< void() >& on_commit ) {
	std::shared_ptr< RosterNotifications > notifications;
	m_gc_space->WithNativeMutation( [ & ]() {
		notifications = ApplyRosterNative( record, expected, epoch, true, local_slot );
	} );
	util::FinallyGuard abandoned( [ & ]() {
		std::lock_guard lock( m_roster_notifications_mutex );
		m_roster_notifications.erase( std::remove( m_roster_notifications.begin(), m_roster_notifications.end(), notifications ),
									  m_roster_notifications.end() );
	} );
	if ( on_commit ) {
		on_commit();
	}
	DispatchRosterNotifications( notifications );
}
void State::ApplyRosterRefresh( const roster::RosterRecord& record, connection::Connection* expected, const roster_epoch_t& epoch ) {
	std::shared_ptr< RosterNotifications > notifications;
	m_gc_space->WithNativeMutation( [ & ]() {
		Require( expected && expected == m_connection, "Roster update belongs to another connection" );
		notifications = ApplyRosterNative( record, expected, epoch, false, expected->m_slot, true );
	} );
	DispatchRosterNotifications( notifications );
}
void State::ApplySlotUpdate( size_t index, const roster::SlotRecord& record, connection::Connection* expected,
							 const roster_epoch_t& epoch ) {
	std::shared_ptr< RosterNotifications > notifications;
	m_gc_space->WithNativeMutation( [ & ]() {
		Require( expected && expected == m_connection, "Slot update belongs to another connection" );
		auto next = ExportRosterNative();
		Require( index < next.slots.size(), "Slot index out of range" );
		next.slots[ index ] = record;
		notifications = ApplyRosterNative( std::move( next ), expected, epoch, false, expected->m_slot );
	} );
	DispatchRosterNotifications( notifications );
}
void State::ApplyFlagsUpdate( size_t index, size_t flags, connection::Connection* expected, const roster_epoch_t& epoch ) {
	Require( flags <= 7, "Unknown Player flag bits" );
	std::shared_ptr< RosterNotifications > notifications;
	m_gc_space->WithNativeMutation( [ & ]() {
		Require( expected && expected == m_connection, "Flags update belongs to another connection" );
		auto next = ExportRosterNative();
		Require( index < next.slots.size() && next.slots[ index ].player, "Flags require an occupied Slot" );
		next.slots[ index ].flags = static_cast< uint8_t >( flags );
		notifications = ApplyRosterNative( std::move( next ), expected, epoch, false, expected->m_slot, true );
	} );
	DispatchRosterNotifications( notifications );
}

void State::NotifyPlayerUpdate( size_t index ) {
	std::shared_ptr< RosterNotifications > batch;
	m_gc_space->WithNativeMutation( [ & ]() {
		Require( index < m_slots->GetCount() && m_slots->m_slots[ index ].m_slot_state == slot::Slot::SS_PLAYER,
				 "Player update requires an occupied Slot" );
		batch = MakeRosterNotifications();
		const auto& slot = m_slots->m_slots[ index ];
		batch->Add( RosterNotifications::Update, index, slot.m_player_data.player, slot.m_player_data.flags & slot::PF_READY );
		std::lock_guard lock( m_roster_notifications_mutex );
		m_roster_notifications.push_back( batch );
	} );
	DispatchRosterNotifications( batch );
}
void State::NotifyPlayerJoin( size_t index ) {
	std::shared_ptr< RosterNotifications > batch;
	m_gc_space->WithNativeMutation( [ & ]() {
		Require( index < m_slots->GetCount() && m_slots->m_slots[ index ].m_slot_state == slot::Slot::SS_PLAYER,
				 "Join requires an occupied Slot" );
		batch = MakeRosterNotifications();
		const auto& slot = m_slots->m_slots[ index ];
		batch->Add( RosterNotifications::Join, index, slot.m_player_data.player, slot.m_player_data.flags & slot::PF_READY );
		std::lock_guard lock( m_roster_notifications_mutex );
		m_roster_notifications.push_back( batch );
	} );
	DispatchRosterNotifications( batch );
}
Player* State::CreatePlayerSlot( size_t index, roster::PlayerRecord record, std::string gsid, network::cid_t cid, std::string address,
								 bool connected ) {
	Player* result = nullptr;
	m_gc_space->WithNativeMutation( [ & ]() {
		ValidatePlayerRecord( record );
		roster::EncodeSlot( { roster::SlotState::Occupied, record, 0, gsid }, ROSTER_LIMITS );
		Require( index < ROSTER_LIMITS.max_slots, "Slot index out of range" );
		Require( index == 0 ? record.role == ( m_connection ? roster::Role::Host : roster::Role::Single )
							: record.role == roster::Role::Player,
				 "Player role does not match the session Slot" );
		EnsureRosterEpoch();
		std::vector< slot::Slot > slots;
		if ( !m_slots->GetCount() ) {
			slots.reserve( ROSTER_LIMITS.max_slots );
			for ( size_t i = 0; i < ROSTER_LIMITS.max_slots; ++i ) {
				slots.emplace_back( i, this );
			}
		}
		else {
			Require( index < m_slots->GetCount() && m_slots->m_slots[ index ].m_slot_state == slot::Slot::SS_OPEN, "Slot is not open" );
		}
		for ( const auto& slot : m_slots->m_slots ) {
			Require( slot.m_linked_gsid != gsid, "Duplicate occupied GSID" );
		}
		auto proposed = ExportRosterNative();
		if ( proposed.slots.empty() ) {
			proposed.slots.resize( ROSTER_LIMITS.max_slots, { roster::SlotState::Open, std::nullopt, 0, {} } );
		}
		proposed.slots.at( index ) = { roster::SlotState::Occupied, record, 0, gsid };
		roster::EncodeRoster( proposed, ROSTER_LIMITS );// enforce the aggregate before publishing another occupant
		auto owned = m_players;
		auto cids = m_cid_slots;
		Require( !cids.count( cid ), "CID already belongs to a Player" );
		NEWV( player, Player, record, this, m_roster_epoch );
		OwnedPlayer guard( player );
		owned.insert( player );
		cids.emplace( cid, index );
		if ( !slots.empty() ) {
			m_slots->m_slots.swap( slots );
		}
		auto& slot = m_slots->m_slots[ index ];
		slot.m_player_data.remote_address.swap( address );
		slot.m_linked_gsid.swap( gsid );
		slot.m_player_data.player = player;
		slot.m_player_data.cid = cid;
		slot.m_player_data.flags = 0;
		slot.m_slot_state = slot::Slot::SS_PLAYER;
		player->SetSlot( &slot );
		player->m_is_connected = connected;
		m_players.swap( owned );
		m_cid_slots.swap( cids );
		result = guard.release();
		if ( m_connection && record.role == roster::Role::Host ) {
			m_connection->m_player = result;
			m_connection->m_slot = index;
		}
	} );
	return result;
}
void State::ReconnectPlayerSlot( size_t index, network::cid_t cid, std::string address ) {
	m_gc_space->WithNativeMutation( [ & ]() {
		Require( index < m_slots->GetCount() && m_slots->m_slots[ index ].m_slot_state == slot::Slot::SS_PLAYER,
				 "Reconnect requires existing Player" );
		auto& slot = m_slots->m_slots[ index ];
		Require( !slot.m_player_data.player->IsConnected(), "Player is already connected" );
		auto cids = m_cid_slots;
		Require( !cids.count( cid ), "CID already belongs to a Player" );
		cids.emplace( cid, index );
		slot.m_player_data.remote_address.swap( address );
		slot.m_player_data.cid = cid;
		slot.m_player_data.flags = 0;
		slot.m_player_data.player->m_is_connected = true;
		m_cid_slots.swap( cids );
	} );
}
void State::RetirePlayerSlot( size_t index, connection::Connection* expected, bool retain_player,
							  const std::function< void() >& after_retirement ) {
	std::shared_ptr< RosterNotifications > batch;
	m_gc_space->WithNativeMutation( [ & ]() {
		Require( expected && expected == m_connection, "Departure belongs to another connection" );
		Require( index < m_slots->GetCount() && m_slots->m_slots[ index ].m_slot_state == slot::Slot::SS_PLAYER,
				 "Departure requires existing Player" );
		auto& slot = m_slots->m_slots[ index ];
		auto* player = slot.m_player_data.player;
		batch = MakeRosterNotifications();
		batch->Add( RosterNotifications::Leave, index, player, slot.m_player_data.flags & slot::PF_READY );
		batch->entries.back().connected = false;
		{
			std::lock_guard lock( m_roster_notifications_mutex );
			m_roster_notifications.push_back( batch );
		}
		m_cid_slots.erase( slot.m_player_data.cid );
		if ( retain_player ) {
			player->m_is_connected = false;
		}
		else {
			m_players.erase( player );
			player->SetSlot( nullptr );
			slot.m_player_data.player = nullptr;
			slot.m_player_data.flags = 0;
			slot.m_player_data.cid = 0;
			slot.m_player_data.remote_address.clear();
			slot.m_linked_gsid.clear();
			slot.m_slot_state = slot.m_close_after_clear ? slot::Slot::SS_CLOSED : slot::Slot::SS_OPEN;
			slot.m_close_after_clear = false;
			DELETE( player );
		}
	} );
	// Transport and callback work after commit must not hold collection exclusion.
	util::FinallyGuard abandoned( [ & ]() {
		std::lock_guard lock( m_roster_notifications_mutex );
		m_roster_notifications.erase( std::remove( m_roster_notifications.begin(), m_roster_notifications.end(), batch ),
									  m_roster_notifications.end() );
	} );
	after_retirement();
	DispatchRosterNotifications( batch );
}

}// namespace game::backend
