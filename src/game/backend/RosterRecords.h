#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace game::backend::roster {

// Inactive value protocol. No native Player/Slot/State or faction ownership.
enum class Role : uint8_t { Single = 1, Host = 2, Player = 3 };
enum class SlotState : uint8_t { Closed = 0, Open = 1, Occupied = 2 };

struct PlayerRecord {
	std::string name;
	Role role;
	std::optional< std::string > faction_id; // absent means unselected/random
	std::string difficulty_id;
	bool operator==( const PlayerRecord& other ) const;
};

struct SlotRecord {
	SlotState state;
	std::optional< PlayerRecord > player;
	uint8_t flags;
	std::string linked_gsid;
	bool operator==( const SlotRecord& other ) const;
};

struct RosterRecord {
	std::vector< SlotRecord > slots; // index is the ordered slot identity
	bool operator==( const RosterRecord& other ) const;
};

// Required caller policy, not game defaults. All must be positive and fit
// uint32. Text is the SUM of every name/ID/GSID in the entire supplied record;
// encoded bytes include nested envelopes and lengths exactly once.
struct Limits {
	size_t max_slots;
	size_t max_text_bytes;
	size_t max_encoded_bytes;
};

// Complete standalone records only: reject unknown versions/kinds, malformed
// shape, unknown enums/flag bits, empty/NUL text, duplicate occupied GSIDs and
// trailing bytes. Vacant slots have no player, flags or linked GSID. Selected
// faction/difficulty IDs remain unresolved exact strings. No Unicode rewriting.
// Decode validates nested extents and aggregate budgets before owned allocation.
// Errors throw std::invalid_argument; input and caller-owned values never change.
std::string EncodePlayer( const PlayerRecord& value, const Limits& limits );
std::string EncodeSlot( const SlotRecord& value, const Limits& limits );
std::string EncodeRoster( const RosterRecord& value, const Limits& limits );
PlayerRecord DecodePlayer( std::string_view bytes, const Limits& limits );
SlotRecord DecodeSlot( std::string_view bytes, const Limits& limits );
RosterRecord DecodeRoster( std::string_view bytes, const Limits& limits );

}
