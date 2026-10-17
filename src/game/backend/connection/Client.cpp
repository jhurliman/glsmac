#include "Client.h"

#include <algorithm>

#include "game/backend/State.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/event/Event.h"
#include "game/backend/Player.h"
#include "game/backend/Game.h"
#include "engine/Engine.h"
#include "types/Packet.h"
#include "network/Network.h"
#include "gse/value/Array.h"

namespace game {
namespace backend {
namespace connection {

Client::Client( gc::Space* const gc_space, settings::LocalSettings* const settings )
	: Client( gc_space, settings, g_engine->GetNetwork() ) {}

Client::Client( gc::Space* const gc_space, settings::LocalSettings* const settings, network::Network* const network )
	: Connection( gc_space, network::CM_CLIENT, settings, network ) {}

void Client::ProcessEvent( const network::Event& event ) {
	Connection::ProcessEvent( event );

	ASSERT( !event.cid, "client connection received event with non-zero cid" );

	switch ( event.type ) {
		case network::Event::ET_PACKET: {
			try {
				if ( !event.data.packet_data.empty() ) {
					types::Packet packet( types::Packet::PT_NONE );
					packet.Deserialize( types::Buffer( event.data.packet_data ) );
					switch ( packet.type ) {
						case types::Packet::PT_REQUEST_AUTH: {
							Log( "Authenticating" );
							types::Packet p( types::Packet::PT_AUTH );
							p.data.vec = {
								m_settings->account.GetGSID(),
								m_settings->player_name,
							};
							m_network->MT_SendPacket( &p );
							break;
						}
                        case types::Packet::PT_PLAYERS: {
                            auto* const state=m_state; const auto epoch=state->CaptureRosterEpoch();
                            auto record = roster::DecodeRoster(packet.data.str, State::ROSTER_LIMITS);
                            if (packet.data.num) {
                                if (m_roster_published || m_initial_roster) { throw std::runtime_error("Duplicate initial roster"); }
                                if (packet.data.num >= record.slots.size() || packet.data.num >= State::ROSTER_LIMITS.max_slots) { throw std::runtime_error("Invalid initial client slot"); }
                                if (!record.slots[packet.data.num].player || record.slots[packet.data.num].linked_gsid != m_settings->account.GetGSID()) { throw std::runtime_error("Assigned client Slot has another GSID"); }
                                m_initial_roster = std::move(record); m_initial_slot = packet.data.num;
                                m_initial_roster_epoch = epoch;
                            } else {
                                if (!m_roster_published) { throw std::runtime_error("Roster refresh before initial publication"); }
                                state->ApplyRosterRefresh(record, this, epoch);
                                if (!epoch || !epoch->active || m_state!=state || state->GetConnection()!=this) { break; }
                                const auto on_players=m_on_players_list_update;
                                if (on_players) { on_players(m_slot); }
                            }
                            break;
                        }
						case types::Packet::PT_GLOBAL_SETTINGS: {
							Log( "Got global settings update from server" );
                            auto* const roster_state=m_state;
                            const auto roster_epoch=m_state->CaptureRosterEpoch();
                            const auto current=[&]() { return roster_epoch && roster_epoch->active && m_state==roster_state && roster_state->GetConnection()==this; };
							if (!m_roster_published && (!m_initial_roster || m_initial_roster->slots.empty()
                                || !m_initial_roster->slots[0].player || m_initial_roster_epoch != roster_epoch)) { throw std::runtime_error("Settings arrived without a current initial roster"); }
                            const bool host_ready = m_roster_published ? m_state->m_slots->GetSlot(0).HasPlayerFlag(slot::PF_READY) : (m_initial_roster->slots[0].flags & slot::PF_READY) != 0;
							bool ok = true;
							switch ( m_game_state ) {
								case Connection::GS_LOBBY: {
									if ( host_ready ) {
										Error( "host updated settings while ready" );
										ok = false;
									}
									break;
								}
								case Connection::GS_INITIALIZING:
								case Connection::GS_RUNNING: {
									if ( m_are_global_settings_received ) {
										Error( "duplicate global settings update" );
										ok = false;
									}
									break;
								}
								default: {
									Error( "unexpected global settings update for this state" );
									ok = false;
								}
							}
							if ( !ok ) {
								break; // something went wrong
							}
							//m_state->m_settings.global.Deserialize( types::Buffer( packet.data.str ) );
							m_state->DeserializeRosterSettings(types::Buffer(packet.data.str), this, m_roster_published ? roster_epoch : m_initial_roster_epoch);
                            if (!m_roster_published) {
                                m_state->ApplyInitialRoster(*m_initial_roster, this, m_initial_roster_epoch, m_initial_slot, [this]() {
                                    m_roster_published = true; m_initial_roster.reset(); m_initial_roster_epoch.reset();
                                    if (m_state->GetGame()) { m_state->GetGame()->SetSlotNum(m_slot); }
                                });
                                if (!current()) { break; }
                                const auto on_players=m_on_players_list_update;
                                if (on_players) { on_players(m_slot); }
                            }
                            if (!current()) { break; }
                            m_are_global_settings_received = true;
                            const auto on_settings=m_on_global_settings_update;
                            if (on_settings) { on_settings(); }
                            if (!current()) { break; }
                            OnOpen();
                            if (current()) { AnnounceGameState(); }
							break;
						}
                        case types::Packet::PT_SLOT_UPDATE:
                        case types::Packet::PT_FLAGS_UPDATE: {
                            if (!m_roster_published) { throw std::runtime_error("Slot update before roster publication"); }
                            auto* const state=m_state; const auto epoch=state->CaptureRosterEpoch();
                            const auto current=[&]() {return epoch && epoch->active && m_state==state && state->GetConnection()==this;};
                            const bool flags_only = packet.type == types::Packet::PT_FLAGS_UPDATE;
                            const auto index = flags_only ? packet.udata.flags.slot_num : packet.data.num;
                            const auto old = m_state->ExportSlot(index);
                            if (flags_only) { m_state->ApplyFlagsUpdate(index, packet.udata.flags.flags, this, epoch); }
                            else { m_state->ApplySlotUpdate(index, roster::DecodeSlot(packet.data.str, State::ROSTER_LIMITS), this, epoch); }
                            if (!current()) { break; }
                            auto& slot = state->m_slots->GetSlot(index);
                            const auto on_slot=m_on_slot_update;
                            if (!flags_only && on_slot) { on_slot(index, &slot); }
                            if (!current()) { break; }
                            const auto on_flags=m_on_flags_update;
                            if (on_flags) { on_flags(index, &slot, old.flags, slot.GetState()==slot::Slot::SS_PLAYER ? slot.GetPlayerFlags() : 0); }
                            break;
                        }
						case types::Packet::PT_KICK: {
							Disconnect( packet.data.str );
							break;
						}
						case types::Packet::PT_MESSAGE: {
							Log( "Got chat message: " + packet.data.str );
							if ( m_on_message ) {
								m_on_message( packet.data.str );
							}
							break;
						}
						case types::Packet::PT_GAME_STATE: {
							Log( "Got game state: " + std::to_string( packet.udata.game_state.state ) );
							if ( packet.udata.game_state.state != m_game_state ) {
								m_game_state = (game_state_t)packet.udata.game_state.state;
								AnnounceGameState();
							}
							break;
						}
						case types::Packet::PT_DOWNLOAD_RESPONSE: {
							Log( "Got download response" );
							if ( !m_download_state.is_downloading ) {
								Error( "download response received while not downloading" );
							}
							if ( packet.data.num == 0 ) {
								Log( "No download response received from server" );
								Disconnect( "No download response received from server" );
							}
							else {
								m_download_state.total_size = packet.data.num;
								Log( "Allocating download buffer (" + std::to_string( m_download_state.total_size ) + " bytes)" );
								m_download_state.buffer.reserve( m_download_state.total_size );
								DownloadNextChunk();
							}
							break;
						}
						case types::Packet::PT_DOWNLOAD_NEXT_CHUNK_RESPONSE: {
							Log( "Downloaded next chunk ( offset=" + std::to_string( packet.udata.download.offset ) + " size=" + std::to_string( packet.udata.download.size ) + " )" );
							const size_t end = packet.udata.download.offset + packet.udata.download.size;
							if ( !m_download_state.is_downloading ) {
								Error( "chunk received while not downloading" );
							}
							else if ( end > m_download_state.total_size ) {
								Error( "chunk overflow ( " + std::to_string( packet.udata.download.offset ) + " + " + std::to_string( packet.udata.download.size ) + " >= " + std::to_string( m_download_state.total_size ) + " )" );
							}
							else if ( packet.udata.download.offset != m_download_state.downloaded_size ) {
								Error( "inconsistent chunk offset ( " + std::to_string( packet.udata.download.offset ) + " != " + std::to_string( m_download_state.downloaded_size ) + " )" );
							}
							else if (
								packet.udata.download.size != DOWNLOAD_CHUNK_SIZE &&
									end != m_download_state.total_size // last chunk can be smaller
								) {
								Error( "inconsistent map chunk size ( " );
							}
							else {
								ASSERT( packet.data.str.size() == packet.udata.download.size, "download buffer size mismatch" );
								m_download_state.buffer.append( packet.data.str );
								if ( end < m_download_state.total_size ) {
									if ( m_on_download_progress ) {
										m_on_download_progress( (float)m_download_state.downloaded_size / m_download_state.total_size );
									}
									m_download_state.downloaded_size = end;
									DownloadNextChunk();
								}
								else {
									Log( "Download completed successfully" );
									// The completion callback may clear its own installed handler.
									const auto on_complete = m_on_download_complete;
									if ( on_complete ) {
										on_complete( m_download_state.buffer );
									}
									m_download_state.buffer.clear();
									m_download_state.is_downloading = false;
								}
							}
							break;
						}
						case types::Packet::PT_GAME_EVENT: {
							//Log( "Got game event packet" );
							if ( !m_state->GetGame() ) { break; }
							m_state->GetGame()->WithWorldIngress( m_state,
								this,
								[ this, state = m_state, packet ]( Game* const game, GSE_CALLABLE ) {
									if ( state->GetConnection() != this ) { return; }
									auto buf = types::Buffer( packet.data.str );
									game->AddEvent( event::Event::Deserialize( game, event::Event::ES_SERVER, GSE_CALL, buf.ReadString() ) );
								}
							);
							break;
						}
						case types::Packet::PT_GAME_EVENT_RESPONSE: {
							//Log( "Got game event response packet" );
							if ( !m_state->GetGame() ) { break; }
							m_state->GetGame()->WithWorldIngress( m_state,
								this,
								[ this, state = m_state, packet ]( Game* const game, GSE_CALLABLE ) {
									if ( state->GetConnection() != this ) { return; }
									gse::Value* resolved = nullptr;
									if ( !packet.data.str2.empty() ) {
										auto buf = types::Buffer( packet.data.str2 );
										resolved = gse::Value::Deserialize( GSE_CALL, &buf, game );
									}
									game->AddEventResponse( packet.data.str, packet.data.boolean, resolved );
								}
							);
							break;
						}
						default: {
							Log( "WARNING: invalid packet type from server: " + std::to_string( packet.type ) );
						}
					}
				}
			}
			catch ( const std::invalid_argument& err ) {
                Error( err.what() );
            }
            catch ( std::runtime_error& err ) {
				Error( err.what() );
			}
			break;
		}
		case network::Event::ET_ERROR: {
			Error( event.data.packet_data );
			break;
		}
		case network::Event::ET_DISCONNECT: {
			Disconnect( "Connection to server lost." );
			break;
		}
		default: {
			Log( "WARNING: invalid event type from server: " + std::to_string( event.type ) );
		}
	}
}

void Client::SendGameEvents( const game_events_t& game_events ) {
	Log( "Sending " + std::to_string( game_events.size() ) + " game events" );
	for ( const auto& event : game_events ) {
		types::Buffer buf;
		buf.WriteString( event.serialized_data );
		types::Packet p( types::Packet::PT_GAME_EVENT );
		p.data.str = buf.ToString();
		m_network->MT_SendPacket( &p );
	}
}

void Client::UpdateSlot( const size_t slot_num, slot::Slot* slot, const bool only_flags ) {
	if ( only_flags ) {
		Log( "Sending flags update" );
		types::Packet p( types::Packet::PT_UPDATE_FLAGS );
		// not sending slot num because server knows it anyway
		p.udata.flags.flags = slot->GetPlayerFlags();
		m_network->MT_SendPacket( &p );
	}
	else {
		Log( "Sending slot update" );
		types::Packet p( types::Packet::PT_UPDATE_SLOT );
		// not sending slot num because server knows it anyway
		p.data.str = roster::EncodeSlot(m_state->ExportSlot(slot_num), State::ROSTER_LIMITS);
		m_network->MT_SendPacket( &p );
	}
}

void Client::SendMessage( const std::string& message ) {
	Log( "Sending chat message: " + message );
	types::Packet p( types::Packet::PT_MESSAGE );
	p.data.str = message;
	m_network->MT_SendPacket( &p );
}

const Connection::game_state_t Client::GetGameState() const {
	return m_game_state;
}

bool Client::HasGlobalSettings() const {
	return m_are_global_settings_received;
}

void Client::AnnounceGameState() {
	// The server sends game state before settings, including when rejoining a
	// running match. Never start profile-dependent scripts using local defaults.
	if ( !m_are_global_settings_received || m_announced_game_state == m_game_state ) { return; }
	m_announced_game_state = m_game_state;
	auto* const state=m_state;
    const auto epoch=state->CaptureRosterEpoch();
    const auto on_change=m_on_game_state_change;
    if (on_change) { on_change(m_game_state); }
    if (!epoch || !epoch->active || m_state!=state || state->GetConnection()!=this) { return; }
	WTrigger( "game_state", ARGS_F( this ) {
		{ "state", VALUE( gse::value::String,, GetGameStateStr( m_game_state ) ) }
	}; } );
}

void Client::RequestDownload() {
	Log( "Requesting download from server" );
	ASSERT( !m_download_state.is_downloading, "download already started" );
	ASSERT( m_on_download_complete, "download requested but m_on_download_complete is not set" );
	m_download_state.is_downloading = true;
	types::Packet p( types::Packet::PT_DOWNLOAD_REQUEST );
	m_network->MT_SendPacket( &p );
}

void Client::ResetHandlers() {
	Connection::ResetHandlers();
	m_on_players_list_update = nullptr;
	m_on_game_state_change = nullptr;
	m_on_download_progress = nullptr;
	m_on_download_complete = nullptr;
}

void Client::Error( const std::string& reason ) {
	Log( "Network protocol error: " + reason );
	Disconnect( "Network protocol error" );
}

void Client::DownloadNextChunk() {
	ASSERT( m_download_state.is_downloading, "download not initialized" );
	ASSERT( m_download_state.buffer.size() == m_download_state.downloaded_size, "download buffer size mismatch" );
	ASSERT( m_download_state.downloaded_size < m_download_state.total_size, "download already finished" );
	const size_t size = std::min( DOWNLOAD_CHUNK_SIZE, m_download_state.total_size - m_download_state.downloaded_size );
	Log( "Requesting next chunk ( offset=" + std::to_string( m_download_state.downloaded_size ) + " size=" + std::to_string( size ) + " )" );
	types::Packet p( types::Packet::PT_DOWNLOAD_NEXT_CHUNK_REQUEST );
	p.udata.download.offset = m_download_state.downloaded_size;
	p.udata.download.size = size;
	m_network->MT_SendPacket( &p );
}

}
}
}
