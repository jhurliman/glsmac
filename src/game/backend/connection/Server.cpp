#include "Server.h"

#include "engine/Engine.h"
#include "types/Packet.h"
#include "network/Network.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/Player.h"
#include "game/backend/faction/FactionManager.h"
#include "game/backend/event/Event.h"
#include "gc/Space.h"

namespace game {
namespace backend {
namespace connection {

Server::Server( gc::Space* const gc_space, settings::LocalSettings* const settings )
	: Server( gc_space, settings, g_engine->GetNetwork() ) {}

Server::Server( gc::Space* const gc_space, settings::LocalSettings* const settings, network::Network* const network )
	: Connection( gc_space, network::CM_SERVER, settings, network ) {}

void Server::ProcessEvent( const network::Event& event ) {
	Connection::ProcessEvent( event );

	ASSERT( event.cid || event.type == network::Event::ET_LISTEN || event.type == network::Event::ET_DISCONNECT, "server connection received event without cid" );

	switch ( event.type ) {
		case network::Event::ET_LISTEN: {
			m_game_state = GS_LOBBY; // tmp: start with lobby for now
			ASSERT( !m_player, "player already set" );
			Log( "Listening" );
			m_state->m_settings.global.Initialize();
            const auto& rules = m_state->m_settings.global.rules;
            m_player = m_state->CreatePlayerSlot(0, {m_state->m_settings.local.player_name, roster::Role::Host, std::nullopt, rules.GetDefaultDifficultyLevel()},
                m_state->m_settings.local.account.GetGSID(), 0, event.data.remote_address, false);
            m_slot = 0;
            auto* const state = m_state;
            const auto epoch = state->CaptureRosterEpoch();
            const auto current = [&]() { return epoch && epoch->active && m_state == state && state->GetConnection() == this; };
            const auto on_listen = m_on_listen;
            if (on_listen) { on_listen(); }
            if (!current()) { break; }
            const auto on_settings = m_on_global_settings_update;
            if (on_settings) { on_settings(); }
            if (!current()) { break; }
            state->NotifyPlayerJoin(m_slot);
            if (!current()) { break; }
            OnOpen();

			break;
		}
		case network::Event::ET_CLIENT_CONNECT: {
			Log( "Client " + std::to_string( event.cid ) + " connected" );
			ASSERT( m_state->GetCidSlots().find( event.cid ) == m_state->GetCidSlots().end(), "player cid already in slots" );

			const auto& banned = m_settings->banned_addresses;
			if ( banned.find( event.data.remote_address ) != banned.end() ) {
				Kick( event.cid, "You are banned" );
				break;
			}

			{
				types::Packet packet( types::Packet::PT_REQUEST_AUTH ); // ask to authenticate
				m_network->MT_SendPacket( &packet, event.cid );
			}
			break;
		}
        case network::Event::ET_CLIENT_DISCONNECT: {
            const auto it = m_state->GetCidSlots().find(event.cid);
            if (it != m_state->GetCidSlots().end()) {
                const auto index = it->second;
                m_state->RetirePlayerSlot(index, this, m_game_state != GS_LOBBY, [&]() {
                    const auto& slot = m_state->m_slots->GetSlot(index);
                    SendSlotUpdate(index, &slot, event.cid);
                    m_download_data.erase(event.cid);
                    if (m_game_state == GS_LOBBY) { ClearReadyFlags(); }
                });
            }
            break;
        }
		case network::Event::ET_PACKET: {
			try {
				types::Packet packet( types::Packet::PT_NONE );
				packet.Deserialize( types::Buffer( event.data.packet_data ) );
				switch ( packet.type ) {
					case types::Packet::PT_AUTH: {
						if (packet.data.vec.size() != 2) { throw std::runtime_error("Invalid authentication field count"); }
						const std::string& gsid = packet.data.vec[ 0 ];
						const std::string& player_name = packet.data.vec[ 1 ];
						if ( gsid.empty() || player_name.empty() ) {
							Log( "Authentication from cid " + std::to_string( event.cid ) + " failed, disconnecting" );
							m_network->MT_DisconnectClient( event.cid );
							break;
						}

						if ( m_state->GetCidSlots().find( event.cid ) != m_state->GetCidSlots().end() ) {
							Log( "Duplicate authentication from cid " + std::to_string( event.cid ) + ", disconnecting" );
							m_network->MT_DisconnectClient( event.cid );
							break;
						}

						Log( "Got authentication from " + gsid + " (cid " + std::to_string( event.cid ) + ") as '" + player_name + "'" );
						ASSERT( m_game_state != GS_NONE, "player connected but game state not set" );

						// TODO: implementing 'load game' from lobby will need extra gsid logic

						size_t slot_num = 0;

						// check if player is allowed to join
						bool is_player_of_game = false;
						bool is_already_in_game = false;
						for ( auto& slot : m_state->m_slots->GetSlots() ) {
							if ( slot.GetLinkedGSID() == gsid ) {
								is_player_of_game = true;
								ASSERT( slot.GetState() == slot::Slot::SS_PLAYER, "slot state is not player" );
								const auto* player = slot.GetPlayer();
								ASSERT( player, "slot player not set" );
								if ( player->IsConnected() ) {
									is_already_in_game = true;
								}
								else {
									slot_num = slot.GetIndex();
								}
								break;
							}
						}
						if ( m_game_state != GS_LOBBY && !is_player_of_game ) {
							Log( gsid + "is not part of the game, disconnecting" );
							Kick( event.cid, "You are not player of this game" );
							break;
						}
						if ( is_already_in_game ) {
							Log( "Duplicate connection from " + gsid + ", disconnecting" );
							Kick( event.cid, "You are already connected" );
							break;
						}

						if ( m_game_state == GS_LOBBY ) {
							// on lobby stage everyone can connect and occupy first free slot
							for ( auto& slot : m_state->m_slots->GetSlots() ) {
								if ( slot.GetState() == slot::Slot::SS_OPEN ) {
									break;
								}
								slot_num++;
							}
							if ( slot_num >= m_state->m_slots->GetCount() ) { // no available slots left
								Log( "No free slots for player " + gsid + " (" + player_name + "), rejecting" );
								Kick( event.cid, "Server is full!" );
								break;
							}
						}

                        auto* const state = m_state;
                        const auto epoch = state->CaptureRosterEpoch();
                        const auto current = [&]() { return epoch && epoch->active && m_state == state && state->GetConnection() == this; };
                        const auto& rules = state->m_settings.global.rules;
                        if (m_game_state == GS_LOBBY) {
                            m_state->CreatePlayerSlot(slot_num, {player_name, roster::Role::Player, std::nullopt, rules.GetDefaultDifficultyLevel()}, gsid, event.cid, event.data.remote_address, true);
                            ClearReadyFlags();
                            if (!current()) { break; }
                        } else {
                            if (!slot_num) { throw std::runtime_error("A remote connection cannot assume the host Slot"); }
                            m_state->ReconnectPlayerSlot(slot_num, event.cid, event.data.remote_address);
                        }
                        auto& slot = m_state->m_slots->GetSlot(slot_num);

						SendPlayersList( event.cid, slot_num );
						SendGameState( event.cid );
						SendGlobalSettings( event.cid );

						SendSlotUpdate( slot_num, &slot, event.cid ); // notify others

                        m_state->NotifyPlayerJoin(slot_num);

						break;
					}
                    case types::Packet::PT_UPDATE_SLOT:
                    case types::Packet::PT_UPDATE_FLAGS: {
                        auto* const state=m_state; const auto epoch=state->CaptureRosterEpoch();
                        const auto current=[&]() {return epoch && epoch->active && m_state==state && state->GetConnection()==this;};
                        const auto it = m_state->GetCidSlots().find(event.cid);
                        if (it == m_state->GetCidSlots().end()) { throw std::runtime_error("Unknown Player CID"); }
                        const auto index = it->second;
                        const auto old = m_state->ExportSlot(index);
                        if (!old.player) { throw std::runtime_error("Update requires occupied Slot"); }
                        const bool flags_only = packet.type == types::Packet::PT_UPDATE_FLAGS;
                        if (flags_only) { m_state->ApplyFlagsUpdate(index, packet.udata.flags.flags, this, epoch); }
                        else {
                            const auto next = roster::DecodeSlot(packet.data.str, State::ROSTER_LIMITS);
                            if (!next.player || next.linked_gsid != old.linked_gsid || next.player->role != old.player->role) { throw std::runtime_error("Client cannot replace its occupant identity"); }
                            m_state->ApplySlotUpdate(index, next, this, epoch);
                        }
                        if (!current()) { break; }
                        auto& slot = state->m_slots->GetSlot(index);
                        if (flags_only) { SendFlagsUpdate(index, &slot, event.cid); }
                        else { SendSlotUpdate(index, &slot, event.cid); const auto on_slot=m_on_slot_update; if (on_slot) { on_slot(index, &slot); } }
                        if (!current()) { break; }
                        const auto on_flags=m_on_flags_update;
                        if (on_flags) { on_flags(index, &slot, old.flags, slot.GetPlayerFlags()); }
                        break;
                    }
					case types::Packet::PT_MESSAGE: {
						Log( "Got chat message from " + std::to_string( event.cid ) + ": " + packet.data.str );
						const auto& slots = m_state->GetCidSlots();
						const auto& it = slots.find( event.cid );
						if ( it == slots.end() ) {
							Error( event.cid, "slot index mismatch" );
							break;
						}
						GlobalMessage( FormatChatMessage( m_state->m_slots->GetSlot( it->second ).GetPlayer(), packet.data.str ) );
						break;
					}
					case types::Packet::PT_DOWNLOAD_REQUEST: {
						Log( "Got download request from " + std::to_string( event.cid ) );
						types::Packet p( types::Packet::PT_DOWNLOAD_RESPONSE );
						if ( m_on_download_request ) {
							m_download_data[ event.cid ] = download_data_t{ // override previous request
								0,
								m_on_download_request()
							};
							p.data.num = m_download_data.at( event.cid ).serialized_snapshot.size();
						}
						else {
							// no handler set - no data to return
							Log( "WARNING: download requested but no download handler was set, sending empty header" );
							p.data.num = 0;
						}
						m_network->MT_SendPacket( &p, event.cid );
						break;
					}
					case types::Packet::PT_DOWNLOAD_NEXT_CHUNK_REQUEST: {
						Log( "Got next chunk request from " + std::to_string( event.cid ) + " ( offset=" + std::to_string( packet.udata.download.offset ) + " size=" + std::to_string( packet.udata.download.size ) + " )" );
						const auto& it = m_download_data.find( event.cid );
						const size_t end = packet.udata.download.offset + packet.udata.download.size;
						if ( it == m_download_data.end() ) {
							Error( event.cid, "download not initialized" );
						}
						else if ( end > it->second.serialized_snapshot.size() ) {
							Error( event.cid, "download offset overflow ( " + std::to_string( packet.udata.download.offset ) + " + " + std::to_string( packet.udata.download.size ) + " >= " + std::to_string( it->second.serialized_snapshot.size() ) + " )" );
						}
						else if ( packet.udata.download.offset != it->second.next_expected_offset ) {
							Error( event.cid, "inconsistent download offset ( " + std::to_string( packet.udata.download.offset ) + " != " + std::to_string( it->second.next_expected_offset ) + " )" );
						}
						else if (
							packet.udata.download.size != DOWNLOAD_CHUNK_SIZE &&
								end != it->second.serialized_snapshot.size() // last chunk can be smaller
							) {
							Error( event.cid, "inconsistent download size ( " );
						}
						else {
							types::Packet p( types::Packet::PT_DOWNLOAD_NEXT_CHUNK_RESPONSE );
							p.udata.download.offset = packet.udata.download.offset;
							p.udata.download.size = packet.udata.download.size;
							p.data.str = it->second.serialized_snapshot.substr( packet.udata.download.offset, packet.udata.download.size );
							m_network->MT_SendPacket( &p, event.cid );
							if ( end < it->second.serialized_snapshot.size() ) {
								it->second.next_expected_offset = end;
							}
							else {
								// snapshot was sent fully, can free memory now
								Log( "Snapshot was sent successfully to " + std::to_string( event.cid ) + ", cleaning up" );
								m_download_data.erase( it );
							}
						}
						break;
					}
					case types::Packet::PT_GAME_EVENT: {
						//Log( "Got game event packet" );
						if ( !m_state->GetGame() ) { break; }
						m_state->GetGame()->WithWorldIngress( m_state,
							this,
							[ this, state = m_state, packet, event ]( Game* const game, GSE_CALLABLE ) {
								if ( state->GetConnection() != this ) { return; }
								auto buf = types::Buffer( packet.data.str );
								auto* const ev = event::Event::Deserialize( game, event::Event::ES_CLIENT, GSE_CALL, buf.ReadString() );
								const auto caller_slot = ev->GetCaller();
								if ( caller_slot >= state->m_slots->GetCount() ) {
									Error( event.cid, "event caller slot overflow" );
									return;
								}
								const auto& slot = state->m_slots->GetSlot( caller_slot );
								if ( slot.GetState() != slot::Slot::SS_PLAYER || slot.GetCid() != event.cid ) {
									Error( event.cid, "event caller slot mismatch" );
									return;
								}
								game->AddEvent( ev );
							}
						);
						break;
					}
					default: {
						Log( "WARNING: invalid packet type from client " + std::to_string( event.cid ) + " : " + std::to_string( packet.type ) );
					}
				}
			}
			catch ( const std::invalid_argument& err ) {
                Error( event.cid, err.what() );
            }
            catch ( std::runtime_error& err ) {
				Error( event.cid, err.what() );
			}
			break;
		}
		case network::Event::ET_ERROR: {
			Error( event.cid, event.data.packet_data );
			break;
		}
		default: {
			Log( "WARNING: invalid event type from client " + std::to_string( event.cid ) + " : " + std::to_string( event.type ) );
		}
	}
}

static const std::unordered_set< std::string > s_clear_ready_on_events = {
	"game_settings",
	"select_faction",
};

void Server::SendGameEvents( const game_events_t& game_events ) {
	//Log( "Sending " + std::to_string( game_events.size() ) + " game events" );
	bool need_ready_clear = false;
	Broadcast(
		[ this, &game_events, &need_ready_clear ]( const network::cid_t cid ) -> void {
			for ( const auto& event : game_events ) {
				if ( m_game_state == GS_LOBBY && s_clear_ready_on_events.find( event.name ) != s_clear_ready_on_events.end() ) {
					need_ready_clear = true;
				}
				const auto& sender_slot = m_state->m_slots->GetSlot( event.caller );
				const auto& target_slot = m_state->m_slots->GetSlot( m_state->GetCidSlots().at( cid ) );
				if ( sender_slot.GetCid() != cid && (
					m_game_state == GS_LOBBY ||
						target_slot.HasPlayerFlag(
							slot::PF_GAME_INITIALIZED
						)
				) ) {
					types::Buffer buf;
					buf.WriteString( event.serialized_data );
					types::Packet p( types::Packet::PT_GAME_EVENT );
					p.data.str = buf.ToString();
					m_network->MT_SendPacket( &p, cid );
				}
			}
		}
	);
	if ( need_ready_clear ) {
		ClearReadyFlags();
	}
}

void Server::Broadcast( std::function< void( const network::cid_t cid ) > callback ) {
	for ( const auto& slot : m_state->m_slots->GetSlots() ) {
		if ( slot.GetState() == slot::Slot::SS_PLAYER && slot.GetPlayer()->IsConnected() ) {
			const auto cid = slot.GetCid();
			if ( cid != 0 ) { // don't send to self
				callback( cid );
			}
		}
	}
}

void Server::KickFromSlot( const size_t slot_num, const std::string& reason ) {
	ASSERT( slot_num < m_state->m_slots->GetSlots().size(), "slot index overflow" );
	auto& slot = m_state->m_slots->GetSlot( slot_num );
	ASSERT( slot.GetState() == slot::Slot::SS_PLAYER, "kick on non-player slot" );
	KickFromSlot( slot, reason );
}

void Server::BanFromSlot( const size_t slot_num, const std::string& reason ) {
	ASSERT( slot_num < m_state->m_slots->GetSlots().size(), "slot index overflow" );
	auto& slot = m_state->m_slots->GetSlot( slot_num );
	ASSERT( slot.GetState() == slot::Slot::SS_PLAYER, "ban on non-player slot" );
	Log( "Banning address: " + slot.GetRemoteAddress() );
	m_settings->banned_addresses.insert( slot.GetRemoteAddress() );
	KickFromSlot( slot, reason );
}

void Server::SetGameState( const game_state_t game_state ) {
	m_game_state = game_state;
	Broadcast(
		[ this ]( const network::cid_t cid ) -> void {
			SendGameState( cid );
		}
	);
}

void Server::SendPlayersList() {
	Broadcast(
		[ this ]( const network::cid_t cid ) -> void {
			SendPlayersList( cid );
		}
	);
}

void Server::SendGameEventResponse( const size_t slot_num, const std::string& event_id, const bool result, const gse::Value* const resolved ) {
	const auto cid = m_state->m_slots->GetSlot( slot_num ).GetCid();
	types::Packet p( types::Packet::PT_GAME_EVENT_RESPONSE );
	p.data.str = event_id;
	p.data.boolean = result;
	if ( resolved ) {
		types::Buffer buf;
		resolved->Serialize( &buf, resolved );
		p.data.str2 = buf.ToString();
	}
	else {
		p.data.str2 = "";
	}
	m_network->MT_SendPacket( &p, cid );
}

void Server::UpdateSlot( const size_t slot_num, slot::Slot* slot, const bool only_flags ) {
    auto* const state = m_state;
    const auto epoch = state->CaptureRosterEpoch();
    const auto current = [&]() { return epoch && epoch->active && m_state == state && state->GetConnection() == this; };
    if (!only_flags) {
        const auto on_slot = m_on_slot_update;
        if (on_slot) { on_slot(slot_num, slot); }
        if (!current()) { return; }
        SendSlotUpdate(slot_num, slot);
    }
    const auto on_flags = m_on_flags_update;
    if (on_flags) { on_flags(slot_num, slot, slot->GetPlayerFlags(), slot->GetPlayerFlags()); }
    if (!current()) { return; }
    if (only_flags) { SendFlagsUpdate(slot_num, slot); }
}

void Server::SendMessage( const std::string& message ) {
	GlobalMessage( FormatChatMessage( GetPlayer(), message ) );
}

void Server::ResetHandlers() {
	Connection::ResetHandlers();
	m_on_listen = nullptr;
	m_on_download_request = nullptr;
}

void Server::UpdateGameSettings() {
	Broadcast(
		[ this ]( const network::cid_t cid ) -> void {
			SendGlobalSettings( cid );
		}
	);
}

void Server::GlobalMessage( const std::string& message ) {
	if ( m_on_message ) {
		m_on_message( message );
	}
	Broadcast(
		[ this, message ]( const network::cid_t cid ) -> void {
			types::Packet p( types::Packet::PT_MESSAGE );
			p.data.str = message;
			m_network->MT_SendPacket( &p, cid );
		}
	);
}

void Server::Kick( const network::cid_t cid, const std::string& reason = "" ) {
	Log(
		"Kicking " + std::to_string( cid ) + ( !reason.empty()
			? " (reason: " + reason + ")"
			: ""
		)
	);
	IgnoreCID( cid );
	types::Packet p( types::Packet::PT_KICK );
	p.data.str = reason;
	m_network->MT_SendPacket( &p, cid );
	m_network->MT_DisconnectClient( cid );
}

void Server::KickFromSlot( slot::Slot& slot, const std::string& reason ) {
	slot.SetCloseAfterClear();
	Kick( slot.GetCid(), reason );
}

void Server::Error( const network::cid_t cid, const std::string& reason ) {
	Log( "Network protocol error (cid: " + std::to_string( cid ) + "): " + reason );
	Kick( cid, "Network protocol error" );
}

void Server::SendGlobalSettings( network::cid_t cid ) {
	Log( "Sending global settings to " + std::to_string( cid ) );
	types::Packet p( types::Packet::PT_GLOBAL_SETTINGS );
	p.data.str = m_state->Serialize().ToString();
	m_network->MT_SendPacket( &p, cid );
}

void Server::SendGameState( const network::cid_t cid ) {
	Log( "Sending game state change (" + std::to_string( m_game_state ) + ") to " + std::to_string( cid ) );
	types::Packet p( types::Packet::PT_GAME_STATE );
	p.udata.game_state.state = m_game_state;
	m_network->MT_SendPacket( &p, cid );
}

void Server::SendPlayersList( const network::cid_t cid, const size_t slot_num ) {
	Log( "Sending players list to " + std::to_string( cid ) );
	types::Packet p( types::Packet::PT_PLAYERS );
	p.data.num = slot_num;
	p.data.str = roster::EncodeRoster(m_state->ExportRoster(), State::ROSTER_LIMITS);
	m_network->MT_SendPacket( &p, cid );
}

void Server::SendSlotUpdate( const size_t slot_num, const slot::Slot* slot, network::cid_t skip_cid ) {
	Broadcast(
		[ this, slot_num, slot, skip_cid ]( const network::cid_t cid ) -> void {
			if ( cid != skip_cid ) {
				Log( "Sending slot update to " + std::to_string( cid ) );
				types::Packet p( types::Packet::PT_SLOT_UPDATE );
				p.data.num = slot_num;
				p.data.str = roster::EncodeSlot(m_state->ExportSlot(slot_num), State::ROSTER_LIMITS);
				m_network->MT_SendPacket( &p, cid );
			}
		}
	);
}

void Server::SendFlagsUpdate( const size_t slot_num, const slot::Slot* slot, network::cid_t skip_cid ) {
	Broadcast(
		[ this, slot_num, slot, skip_cid ]( const network::cid_t cid ) -> void {
			if ( cid != skip_cid ) {
				Log( "Sending flags update to " + std::to_string( cid ) );
				types::Packet p( types::Packet::PT_FLAGS_UPDATE );
				p.udata.flags.slot_num = slot_num;
				p.udata.flags.flags = slot->GetPlayerFlags();
				m_network->MT_SendPacket( &p, cid );
			}
		}
	);
}

const std::string Server::FormatChatMessage( const Player* player, const std::string& message ) const {
	return "<" + player->GetPlayerName() + "> " + message;
}

void Server::ClearReadyFlags() {
    ASSERT(m_game_state, "unexpected game state");
    auto* const state = m_state;
    const auto epoch = state->CaptureRosterEpoch();
    const auto current = [&]() { return epoch && epoch->active && m_state == state && state->GetConnection() == this; };
    // A notification or native callback may retire this roster synchronously.
    // Never retain a Slot/vector reference across that boundary without checking.
    for (size_t num = 0; current() && num < state->m_slots->GetCount(); ++num) {
        auto& slot = state->m_slots->GetSlot(num);
        if (slot.GetState() == slot::Slot::SS_PLAYER && slot.HasPlayerFlag(slot::PF_READY)) {
            Log("Clearing 'ready' flag of " + slot.GetPlayer()->GetPlayerName());
            const auto old_flags = slot.GetPlayerFlags();
            slot.UnsetPlayerFlag(slot::PF_READY);
            if (!current()) { return; }
            UpdateSlot(num, &slot, true);
            if (!current()) { return; }
            const auto on_flags = m_on_flags_update;
            if (on_flags) { on_flags(num, &slot, old_flags, slot.GetPlayerFlags()); }
        }
    }
}

}
}
}
