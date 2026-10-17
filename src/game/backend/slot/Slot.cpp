#include "Slot.h"

#include "engine/Engine.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "gc/Space.h"
#include "game/backend/Player.h"
#include "game/backend/faction/Faction.h"

#include "gse/value/String.h"
#include "gse/value/Int.h"

namespace game {
namespace backend {
namespace slot {

Slot::Slot( const size_t index, State* state )
	: m_index( index )
	, m_state( state ) {}

const Slot::slot_state_t Slot::GetState() const {
	return m_slot_state;
}

void Slot::Open() {
	ASSERT( m_slot_state == SS_CLOSED, "attempted to open non-closed slot" );
	m_slot_state = SS_OPEN;
}

void Slot::Close() {
	ASSERT( m_slot_state == SS_OPEN, "attempted to close non-open slot" );
	m_slot_state = SS_CLOSED;
}

void Slot::SetCloseAfterClear() {
	ASSERT( m_slot_state == SS_PLAYER, "SetCloseAfterClear on non-player slot" );
	m_close_after_clear = true;
}

Player* Slot::GetPlayer() const {
	ASSERT( m_slot_state == SS_PLAYER, "attempted to get player from non-player slot" );
	return m_player_data.player;
}

const size_t Slot::GetIndex() const {
	return m_index;
}

const network::cid_t Slot::GetCid() const {
	ASSERT( m_slot_state == SS_PLAYER, "attempted to get cid from non-player slot" );
	return m_player_data.cid;
}

const std::string& Slot::GetRemoteAddress() const {
	ASSERT( m_slot_state == SS_PLAYER, "attempted to get remote address from non-player slot" );
	return m_player_data.remote_address;
}

void Slot::SetPlayerFlag( const player_flag_t flag ) {
 bool changed=false;
 m_state->m_gc_space->WithNativeMutation([&]() {
  if (m_slot_state != SS_PLAYER) { throw std::runtime_error("Flags require an occupied Slot"); }
  if (!(m_player_data.flags & flag)) { m_player_data.flags |= flag; changed=true; }
 });
 if (changed) { Update(); }
}

const bool Slot::HasPlayerFlag( const player_flag_t flag ) const {
	ASSERT( m_slot_state == SS_PLAYER, "attempted to get player flag on non-player slot" );
	return m_player_data.flags & flag;
}

void Slot::UnsetPlayerFlag( const player_flag_t flag ) {
 bool changed=false;
 m_state->m_gc_space->WithNativeMutation([&]() {
  if (m_slot_state != SS_PLAYER) { throw std::runtime_error("Flags require an occupied Slot"); }
  if (m_player_data.flags & flag) { m_player_data.flags &= ~flag; changed=true; }
 });
 if (changed) { Update(); }
}

Player* Slot::GetPlayerAndClose() {
	ASSERT( m_slot_state != SS_CLOSED, "attempted to close already closed slot" );
	Player* result = m_player_data.player;
	ASSERT( m_player_data.player->GetSlot() == this, "player-slot connection broken" );
	m_player_data.player->SetSlot( nullptr );
	m_player_data.player = nullptr;
	m_player_data.cid = 0;
	m_player_data.remote_address = "";
	m_player_data.flags = PF_NONE;
	if ( m_close_after_clear ) {
		m_slot_state = SS_CLOSED;
		m_close_after_clear = false;
	}
	else {
		m_slot_state = SS_OPEN;
	}
	return result;
}

void Slot::SetPlayer( Player* player, const network::cid_t cid, const std::string& remote_address ) {
	if ( m_slot_state == SS_PLAYER ) {
		ASSERT( m_player_data.player == player, "attempted to overwrite player with another player" );
	}
	else {
		ASSERT( m_slot_state == SS_OPEN, "attempted to set player to non-open slot" );
		ASSERT( !player->GetSlot(), "attempted to set slot to player with non-empty slot" );
		m_player_data.player = player;
		m_slot_state = SS_PLAYER;
		player->SetSlot( this );
	}
	m_player_data.cid = cid;
	m_player_data.remote_address = remote_address;
	m_player_data.flags = PF_NONE;
}

const std::string& Slot::GetLinkedGSID() const {
	return m_linked_gsid;
}

void Slot::SetLinkedGSID( const std::string& gsid ) {
	ASSERT( m_linked_gsid.empty(), "linked gsid already set" );
	ASSERT( !gsid.empty(), "linked gsid is empty" );
	Log( "Linking GSID: " + gsid );
	m_linked_gsid = gsid;
}

void Slot::UnsetLinkedGSID() {
	ASSERT( !m_linked_gsid.empty(), "linked gsid not set" );
	Log( "Unlinking GSID: " + m_linked_gsid );
	m_linked_gsid.clear();
}

const player_flag_t Slot::GetPlayerFlags() const {
	ASSERT( m_slot_state == SS_PLAYER, "attempted to get player flags from non-player slot" );
	return m_player_data.flags;
}

void Slot::SetPlayerFlags( const player_flag_t flags ) {
 bool changed=false;
 m_state->m_gc_space->WithNativeMutation([&]() {
  if (m_slot_state != SS_PLAYER) { throw std::runtime_error("Flags require an occupied Slot"); }
  if (m_player_data.flags != flags) { m_player_data.flags = flags; changed=true; }
 });
 if (changed) { Update(); }
}

const types::Buffer Slot::Serialize() const { return types::Buffer(roster::EncodeSlot(m_state->ExportSlot(m_index),State::ROSTER_LIMITS)); }
void Slot::Deserialize(types::Buffer) { throw std::runtime_error("Slot metadata must be published through State"); }

WRAPIMPL_BEGIN( Slot )
	ASSERT( m_slot_state == SS_PLAYER, "only player slots can be wrapped for now" );
	auto* player = m_player_data.player;
	ASSERT( player, "can't wrap slot without player" );
	return player->Wrap( GSE_CALL );
}

UNWRAPIMPL_PTR( Slot )

void Slot::Update() { m_state->NotifyPlayerUpdate(m_index); }

}
}
}
