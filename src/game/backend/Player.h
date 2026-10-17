#pragma once

#include <string>
#include "RosterRecords.h"
#include "RosterIdentity.h"

#include "types/Serializable.h"
#include "gse/Wrappable.h"

namespace game {
namespace backend {

class Game;
class State;

namespace faction {
class Faction;
}

namespace slot {
class Slot;
}

CLASS2( Player, types::Serializable, gse::Wrappable )

	enum role_t {
		PR_NONE,
		PR_SINGLE,
		PR_HOST,
		PR_PLAYER,
	};

	~Player() override;
	Player(
		const std::string& name,
		const role_t role,
		faction::Faction* faction,
		const std::string& difficulty_level
	);
	Player( const Player& ) = delete;
	Player& operator=( const Player& ) = delete;
	player_identity_t GetIdentity() const;
	roster::PlayerRecord GetRecord() const;

	const std::string& GetPlayerName() const;
	const std::string GetFullName() const;

	void Connect();
	void Disconnect();
	const bool IsConnected() const;

	void SetFaction( faction::Faction* faction );
	void ClearFaction();
	faction::Faction* GetFaction();

	void SetDifficultyLevel( const std::string& difficulty_level );
	const std::string& GetDifficultyLevel() const;

	void SetSlot( slot::Slot* slot );
	slot::Slot* GetSlot() const;

	const role_t GetRole() const;

	const bool IsTurnCompleted() const;
	void CompleteTurn();
	void UncompleteTurn();

	WRAPDEFS_PTR( Player );

	const types::Buffer Serialize() const override;
	void Deserialize( types::Buffer buf ) override;

	WRAPDEF_SERIALIZABLE;

private:

	bool m_is_connected = false;

	std::string m_name = "";
	role_t m_role = PR_NONE;

	slot::Slot* m_slot = nullptr;
	size_t m_slotnum = 0;

 friend class State;
 Player(roster::PlayerRecord record, State* state, roster_epoch_t epoch);
 void BindRoster(State* state, const roster_epoch_t& epoch);
 void ApplyRecord(roster::PlayerRecord& record) noexcept;
 void RequireMutable() const;
 State* m_roster_state = nullptr;
 roster_epoch_t m_roster_epoch;
 std::optional<std::string> m_faction_id;
 std::shared_ptr<PlayerIdentity> m_identity = std::make_shared<PlayerIdentity>();
 bool m_notification = false;
 bool m_notification_ready = false;
	std::string m_difficulty_level = "";

	bool m_is_turn_completed = false;
};

}
}
