#pragma once

#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>

#include "gse/GCWrappable.h"

#include "network/Types.h"
#include "RosterRecords.h"
#include "RosterIdentity.h"

#include "gse/Exception.h"
#include "game/backend/settings/Settings.h"

class GLSMAC;
namespace gse::tests { class BackendFixture; }

namespace game {
namespace backend {

namespace faction {
class FactionManager;
}

class Bindings;

class Game;

class Player;

namespace connection {
class Connection;
}
namespace slot {
class Slots;
}

CLASS( State, gse::GCWrappable )

	State( gc::Space* const gc_space, gse::context::Context* const ctx, GLSMAC* glsmac );
	virtual ~State();

	void SetGame( Game* game );
	void UnsetGame();
	void RetireGameCallbacks();
	Game* GetGame() const;

	typedef std::function< void( GSE_CALLABLE ) > gse_func_t;
	typedef std::function< void( Game* const game, GSE_CALLABLE ) > game_func_t;
	class GameSession {
	public:
		bool IsActive() const { return active.load(); }
	private:
		friend class State;
		GameSession( State* state, Game* game ) : state( state ), game( game ) {}
		State* const state;
		Game* const game;
		std::atomic< bool > active{ true };
	};
	using game_session_t = std::shared_ptr< const GameSession >;
	game_session_t CaptureGameSession();
	bool IsGameSessionActive( const game_session_t& session ) const;
	void RetireGameCallbacks( const game_session_t& expected_session );

	// Cancels work that has not started when its session is retired. This does
	// not exclude concurrent teardown from an already executing callback.
	void WithGame( gc::Object* owner, const game_func_t& f, const f_cleanup_t& cleanup = nullptr, bool need_now = false );
	// Producers that stage work before queue registration must keep the identity
	// captured before extraction/handler installation; never resample it later.
	void WithGameSession( const game_session_t& session, gc::Object* owner, const game_func_t& f, const f_cleanup_t& cleanup = nullptr, bool need_now = false );

	void WithGSE( gc::Object* const owner, const gse_func_t& f, const f_cleanup_t& f_cleanup = nullptr, const bool need_now = false );

	settings::Settings m_settings = {};
	slot::Slots* m_slots;

	Bindings* m_bindings = nullptr;

	std::function< void( const gse::Exception& ) > m_on_gse_error = nullptr;

	void Iterate();

	bool IsMaster();
	bool IsSlave();

 // Supported live protocol policy, separate from the generic value codec.
 static constexpr roster::Limits ROSTER_LIMITS{7, 32 * 1024, 48 * 1024};
 roster_epoch_t CaptureRosterEpoch() const;
 faction::FactionManager* RequireRoster(const roster_epoch_t& epoch) const;
 roster::RosterRecord ExportRoster();
 roster::SlotRecord ExportSlot(size_t index);
 void ApplyInitialRoster(const roster::RosterRecord& record, connection::Connection* expected, const roster_epoch_t& epoch, size_t local_slot, const std::function<void()>& on_commit = nullptr);
 void ApplyRosterRefresh(const roster::RosterRecord& record, connection::Connection* expected, const roster_epoch_t& epoch);
 void ApplySlotUpdate(size_t index, const roster::SlotRecord& record, connection::Connection* expected, const roster_epoch_t& epoch);
 void ApplyFlagsUpdate(size_t index, size_t flags, connection::Connection* expected, const roster_epoch_t& epoch);
 Player* CreatePlayerSlot(size_t index, roster::PlayerRecord record, std::string gsid, network::cid_t cid, std::string address, bool connected);
 void RetirePlayerSlot(size_t index, connection::Connection* expected, bool retain_player, const std::function<void()>& after_retirement);
 void ReconnectPlayerSlot(size_t index, network::cid_t cid, std::string address);
 void NotifyPlayerUpdate(size_t index);
 void NotifyPlayerJoin(size_t index);
 void AddPlayer( Player* player );
	void RemovePlayer( Player* player );

	// used only on host
	void AddCIDSlot( const network::cid_t cid, const size_t slot );
	const bool HasCIDSlot( const network::cid_t cid );
	void RemoveCIDSlot( const network::cid_t cid );

	const std::unordered_map< network::cid_t, size_t >& GetCidSlots() const;

	void SetConnection( connection::Connection* connection );
	connection::Connection* GetConnection();
	void Reset();
	void DetachConnection();

	faction::FactionManager* GetFM() const;

	gse::Value* const TriggerObject( gse::GCWrappable* object, const std::string& event, const gse::f_args_t& f_args );

	WRAPDEFS_PTR( State )

	const types::Buffer Serialize() const;
	void Deserialize( types::Buffer buf );
 void DeserializeRosterSettings(types::Buffer buf, connection::Connection* expected, const roster_epoch_t& epoch);

	gc::Space* const m_gc_space = nullptr;
	gse::context::Context* const m_ctx = nullptr;

	void GetReachableObjects( std::unordered_set< Object* >& reachable_objects ) override;

	connection::Connection* m_connection = nullptr;

private:
	friend class gse::tests::BackendFixture;

 struct RosterNotifications;
 std::shared_ptr<RosterEpoch> m_roster_epoch;
 std::mutex m_roster_notifications_mutex;
 std::vector<std::shared_ptr<RosterNotifications>> m_roster_notifications;
 void EnsureRosterEpoch();
 void RetireRosterEpoch();
 roster::RosterRecord ExportRosterNative() const;
 std::shared_ptr<RosterNotifications> ApplyRosterNative(roster::RosterRecord record, connection::Connection* expected, const roster_epoch_t& epoch, bool initial, size_t local_slot, bool flags_only = false);
 void DispatchRosterNotifications(const std::shared_ptr<RosterNotifications>& batch);
 std::shared_ptr<RosterNotifications> MakeRosterNotifications();
 void ValidatePlayerRecord(const roster::PlayerRecord& record) const;
 void GetRosterRoots(std::unordered_set<gc::Object*>& reachable);
 GLSMAC* m_glsmac = nullptr;

	faction::FactionManager* m_fm = nullptr;
	Game* m_game = nullptr;
	std::mutex m_game_session_mutex;
	std::shared_ptr< GameSession > m_game_session;

	std::unordered_set< Player* > m_players = {}; // persistent
	std::unordered_map< network::cid_t, size_t > m_cid_slots = {}; // volatile ( { cid, slot_num } )

};

}
}
