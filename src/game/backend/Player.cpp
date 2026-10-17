#include "Player.h"

#include "game/backend/faction/Faction.h"
#include "game/backend/faction/FactionManager.h"
#include <stdexcept>
#include "game/backend/slot/Slot.h"
#include "game/backend/State.h"
#include "game/backend/slot/Slots.h"

#include "gse/value/String.h"
#include "gse/value/Bool.h"

#include "engine/Engine.h"
#include "Game.h"

namespace game {
namespace backend {

Player::~Player() { m_identity->active.store(false); }
Player::Player(const std::string& name, const role_t role, faction::Faction* faction, const std::string& difficulty)
 : m_name(name), m_role(role), m_faction_id(faction ? std::optional<std::string>(faction->m_id) : std::nullopt), m_difficulty_level(difficulty) {}
Player::Player(roster::PlayerRecord record, State* state, roster_epoch_t epoch)
 : Player(record.name, static_cast<role_t>(record.role), nullptr, record.difficulty_id) {
 m_faction_id = std::move(record.faction_id); BindRoster(state, epoch);
}
void Player::BindRoster(State* state, const roster_epoch_t& epoch) {
 if (!epoch || !epoch->active || epoch->state != state) { throw std::runtime_error("Inactive Player roster binding"); }
 if (m_roster_epoch && m_roster_epoch != epoch) { throw std::runtime_error("Player cannot be rebound to a new roster"); }
 m_roster_state = state; m_roster_epoch = epoch;
}
player_identity_t Player::GetIdentity() const { return m_identity; }
roster::PlayerRecord Player::GetRecord() const { return {m_name, static_cast<roster::Role>(m_role), m_faction_id, m_difficulty_level}; }
void Player::ApplyRecord(roster::PlayerRecord& record) noexcept {
 m_name.swap(record.name); m_difficulty_level.swap(record.difficulty_id); m_faction_id.swap(record.faction_id);
}
void Player::RequireMutable() const { if (m_notification) { throw std::runtime_error("Player notification is read-only"); } }

const std::string& Player::GetPlayerName() const {
	return m_name;
}

const std::string Player::GetFullName() const {
	auto* faction = const_cast<Player*>(this)->GetFaction();
	return GetPlayerName() + (faction ? " (" + faction->m_name + ")" : "");
}

void Player::Connect() {
	ASSERT( !m_is_connected, "player already connected" );
	m_is_connected = true;
}

void Player::Disconnect() {
	ASSERT( m_is_connected, "player not connected" );
	m_is_connected = false;
}

const bool Player::IsConnected() const {
	return m_is_connected;
}

void Player::SetFaction(faction::Faction* faction) {
 RequireMutable();
 if (!faction) { ClearFaction(); return; }
 auto* manager = m_roster_state ? m_roster_state->RequireRoster(m_roster_epoch) : nullptr;
 if (!manager || manager->Get(faction->m_id) != faction) { throw std::runtime_error("Player faction is not canonical in its roster"); }
 m_faction_id = faction->m_id;
}
void Player::ClearFaction() { RequireMutable(); m_faction_id.reset(); }
faction::Faction* Player::GetFaction() {
 if (!m_roster_epoch || !m_roster_epoch->active) { throw std::runtime_error("Player roster is no longer active"); }
 auto* manager = m_roster_state->RequireRoster(m_roster_epoch);
 if (!m_faction_id) { return nullptr; }
 auto* faction = manager->Get(*m_faction_id);
 if (!faction) { throw std::runtime_error("Player faction is missing from its roster: " + *m_faction_id); }
 return faction;
}

void Player::SetDifficultyLevel( const std::string& difficulty_level ) {
	RequireMutable();
	m_difficulty_level = difficulty_level;
}

const std::string& Player::GetDifficultyLevel() const {
	return m_difficulty_level;
}

void Player::SetSlot( slot::Slot* slot ) {
	m_slot = slot;
	if ( m_slot ) {
		m_slotnum = m_slot->GetIndex();
	}
}

slot::Slot* Player::GetSlot() const {
	return m_slot;
}

const Player::role_t Player::GetRole() const {
	return m_role;
}

const bool Player::IsTurnCompleted() const {
	return m_is_turn_completed;
}

void Player::CompleteTurn() {
	m_is_turn_completed = true;
}

void Player::UncompleteTurn() {
	m_is_turn_completed = false;
}

WRAPIMPL_BEGIN( Player )
	auto* const game = g_engine->GetGame();
	const auto identity = GetIdentity();
	const bool notification = m_notification;
	WRAPIMPL_PROPS
			{
				"id",
				VALUE( gse::value::Int, , m_slotnum )
			},
			{
				"type",
				VALUE( gse::value::String, , "human" )
			},
			{
				"name",
				VALUE( gse::value::String, , m_name )
			},
			{
				"is_ready",
				NATIVE_CALL( this, identity ) {
                    if (!identity->active) { GSE_ERROR(gse::EC.GAME_ERROR, "Player no longer exists"); }
					N_EXPECT_ARGS( 0 );
					return VALUE( gse::value::Bool, , m_notification ? m_notification_ready : (m_slot && m_slot->HasPlayerFlag( ::game::backend::slot::PF_READY )) );
				} )
			},
			{
				"set_ready",
				NATIVE_CALL( this, game, identity, notification ) {
                    if (!identity->active || notification) { GSE_ERROR(gse::EC.GAME_ERROR, "Player notification is read-only or expired"); }

					game->CheckRW( GSE_CALL );

					N_EXPECT_ARGS( 1 );
					N_GETVALUE( isready, 0, Bool );

					if ( isready ) {
						m_slot->SetPlayerFlag( ::game::backend::slot::PF_READY );
					}
					else {
						m_slot->UnsetPlayerFlag( ::game::backend::slot::PF_READY );
					}

					return VALUE( gse::value::Undefined );
				} )
			},
			{
				"get_faction",
				NATIVE_CALL( this, identity ) {
                    if (!identity->active) { GSE_ERROR(gse::EC.GAME_ERROR, "Player no longer exists"); }
					N_EXPECT_ARGS( 0 );
					try { auto* faction = GetFaction(); return faction ? faction->Wrap(GSE_CALL) : VALUE(gse::value::Undefined); }
                    catch (const std::runtime_error& error) { GSE_ERROR(gse::EC.GAME_ERROR, error.what()); }
				} )
			},
			{
				"set_faction_by_id",
				NATIVE_CALL( this, game, identity, notification ) {
                    if (!identity->active || notification) { GSE_ERROR(gse::EC.GAME_ERROR, "Player notification is read-only or expired"); }

					game->CheckRW( GSE_CALL );

					N_EXPECT_ARGS( 1 );
					N_GETVALUE( faction_id, 0, String );
					if ( game->IsStarted() ) {
						GSE_ERROR( gse::EC.GAME_ERROR, "Can't change faction after game is started" );
					}
					auto* faction = game->GetFaction( faction_id );
					if ( !faction ) {
						GSE_ERROR( gse::EC.GAME_ERROR, "Faction not found: " + faction_id );
					}
					SetFaction(faction);

					return VALUE( gse::value::Undefined );
				} )
			},
			{
				"unset_faction",
				NATIVE_CALL( this, game, identity, notification ) {
                    if (!identity->active || notification) { GSE_ERROR(gse::EC.GAME_ERROR, "Player notification is read-only or expired"); }

					game->CheckRW( GSE_CALL );

					N_EXPECT_ARGS( 0 );
					ClearFaction();

					return VALUE( gse::value::Undefined );
				} )
			},
			{
				"is_master",
				VALUE( gse::value::Bool, , m_role == PR_SINGLE || m_role == PR_HOST )
			},
		};
WRAPIMPL_END_PTR()

UNWRAPIMPL_PTR( Player )

const types::Buffer Player::Serialize() const {
 return types::Buffer(roster::EncodePlayer(GetRecord(), State::ROSTER_LIMITS));
}
void Player::Deserialize(types::Buffer) {
 throw std::runtime_error("Player metadata must be published through State");
}

WRAPIMPL_SERIALIZE( Player )
 if (obj->m_notification) { throw std::runtime_error("Ephemeral Player notifications cannot be serialized as live references"); }
	buf->WriteInt( obj->m_slotnum );
}

WRAPIMPL_DESERIALIZE( Player )
	const auto slot_num = buf->ReadInt();
	const auto& player = game->GetState()->m_slots->GetSlot( slot_num ).GetPlayer();
	return player->Wrap( GSE_CALL );
}

}
}
