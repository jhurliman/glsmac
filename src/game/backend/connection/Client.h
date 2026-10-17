#pragma once

#include "Connection.h"
#include "game/backend/RosterRecords.h"
#include "game/backend/RosterIdentity.h"

namespace game {
namespace backend {
namespace connection {

class Client : public Connection {
public:

	Client( gc::Space* const gc_space, settings::LocalSettings* const settings );

	std::function< void( const size_t slot ) > m_on_players_list_update = nullptr;
	std::function< void( const game_state_t game_state ) > m_on_game_state_change = nullptr;
	std::function< void( const float progress ) > m_on_download_progress = nullptr; // progress is from 0.0f to 1.0f
	std::function< void( const std::string serialized_tiles ) > m_on_download_complete = nullptr;

	void UpdateSlot( const size_t slot_num, slot::Slot* slot, const bool only_flags = false ) override;
	void SendMessage( const std::string& message ) override;

	const game_state_t GetGameState() const;
	bool HasGlobalSettings() const;
	void RequestDownload();

	void ResetHandlers() override;

protected:
	Client( gc::Space* const gc_space, settings::LocalSettings* const settings, network::Network* const network );

	void ProcessEvent( const network::Event& event ) override;
	void SendGameEvents( const game_events_t& game_events ) override;

private:
	friend class gse::tests::BackendFixture;
	bool m_are_global_settings_received = false;
 std::optional<roster::RosterRecord> m_initial_roster;
 roster_epoch_t m_initial_roster_epoch;
 size_t m_initial_slot = 0;
 bool m_roster_published = false;
	game_state_t m_announced_game_state = GS_NONE;
	void AnnounceGameState();

	void Error( const std::string& reason );

	struct {
		bool is_downloading = false;
		int total_size = 0;
		int downloaded_size = 0;
		std::string buffer = "";
	} m_download_state = {};
	void DownloadNextChunk();
};

}
}
}
