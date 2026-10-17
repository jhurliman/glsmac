#include "RosterRecords.h"

#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace game::backend::roster {
namespace {

constexpr std::string_view player_kind = "GLPR", slot_kind = "GLSL", roster_kind = "GLRS";
constexpr uint32_t version = 1;
constexpr uint8_t flag_mask = 7; // READY, MAP_DOWNLOADED, GAME_INITIALIZED

[[noreturn]] void Invalid( const char* reason ) { throw std::invalid_argument( std::string( "Roster record: " ) + reason ); }

void ValidateLimits( const Limits& limits ) {
	for ( const auto limit : { limits.max_slots, limits.max_text_bytes, limits.max_encoded_bytes } ) {
		if ( !limit || limit > std::numeric_limits< uint32_t >::max() ) { Invalid( "limits must be positive uint32 values" ); }
	}
}

void Add( size_t& total, const size_t amount, const size_t maximum ) {
	if ( total > maximum || amount > maximum - total ) { Invalid( "budget exceeded" ); }
	total += amount;
}

void Text( const std::string_view text, size_t& total, const Limits& limits, const bool empty_allowed = false ) {
	if ( ( text.empty() && !empty_allowed ) || text.find( '\0' ) != std::string_view::npos ) { Invalid( "empty or NUL-containing text" ); }
	Add( total, text.size(), limits.max_text_bytes );
}

void CheckRole( const uint8_t role ) { if ( role < 1 || role > 3 ) { Invalid( "unknown player role" ); } }
void CheckState( const uint8_t state ) { if ( state > 2 ) { Invalid( "unknown slot state" ); } }
void CheckFlags( const uint8_t flags ) { if ( flags & ~flag_mask ) { Invalid( "unknown player flag bits" ); } }

class Cursor {
public:
	explicit Cursor( std::string_view bytes ) : m_bytes( bytes ) {}
	std::string_view Take( const size_t count ) {
		if ( count > m_bytes.size() ) { Invalid( "truncated field" ); }
		const auto result = m_bytes.substr( 0, count ); m_bytes.remove_prefix( count ); return result;
	}
	uint8_t Byte() { return static_cast< uint8_t >( Take( 1 )[ 0 ] ); }
	uint32_t Word() {
		const auto bytes = Take( 4 );
		uint32_t result = 0;
		for ( unsigned i = 0; i < 4; ++i ) { result |= static_cast< uint32_t >( static_cast< uint8_t >( bytes[ i ] ) ) << ( i * 8 ); }
		return result;
	}
	bool Presence() { const auto value = Byte(); if ( value > 1 ) { Invalid( "invalid presence byte" ); } return value != 0; }
	std::string_view Field() { return Take( Word() ); }
	void Header( const std::string_view kind ) { if ( Take( 4 ) != kind || Word() != version ) { Invalid( "unsupported kind/version" ); } }
	void End() const { if ( !m_bytes.empty() ) { Invalid( "trailing bytes" ); } }
	size_t Remaining() const { return m_bytes.size(); }
private:
	std::string_view m_bytes;
};

// One parser serves the allocation-free validation pass and the materializing
// pass. Every optional/output write happens only on the second complete pass.
void ReadPlayer( std::string_view bytes, const Limits& limits, size_t& text_total, PlayerRecord* output ) {
	Cursor cursor( bytes ); cursor.Header( player_kind );
	const auto name = cursor.Field(); Text( name, text_total, limits );
	const auto role = cursor.Byte(); CheckRole( role );
	const bool has_faction = cursor.Presence();
	std::string_view faction;
	if ( has_faction ) { faction = cursor.Field(); Text( faction, text_total, limits ); }
	const auto difficulty = cursor.Field(); Text( difficulty, text_total, limits ); cursor.End();
	if ( output ) { *output = { std::string( name ), static_cast< Role >( role ), has_faction ? std::optional< std::string >( faction ) : std::nullopt, std::string( difficulty ) }; }
}

void ReadSlot( std::string_view bytes, const Limits& limits, size_t& text_total, SlotRecord* output ) {
	Cursor cursor( bytes ); cursor.Header( slot_kind );
	const auto state = cursor.Byte(); CheckState( state );
	const bool present = cursor.Presence();
	if ( present != ( state == static_cast< uint8_t >( SlotState::Occupied ) ) ) { Invalid( "slot/player shape mismatch" ); }
	std::optional< PlayerRecord > player;
	if ( present ) {
		const auto nested = cursor.Field();
		if ( output ) { player.emplace(); }
		ReadPlayer( nested, limits, text_total, output ? &*player : nullptr );
	}
	const auto flags = cursor.Byte(); CheckFlags( flags );
	const auto gsid = cursor.Field(); Text( gsid, text_total, limits, !present );
	if ( !present && ( flags || !gsid.empty() ) ) { Invalid( "vacant slot carries player state" ); }
	cursor.End();
	if ( output ) { *output = { static_cast< SlotState >( state ), std::move( player ), flags, std::string( gsid ) }; }
}

void ReadRoster( std::string_view bytes, const Limits& limits, size_t& text_total, RosterRecord* output ) {
	Cursor cursor( bytes ); cursor.Header( roster_kind );
	const auto count = cursor.Word();
	// Every entry needs at least its four-byte envelope length, independently
	// of the later Slot validation. Never reserve using an unchecked count.
	if ( count > limits.max_slots || count > cursor.Remaining() / 4 ) { Invalid( "slot count exceeds bounds" ); }
	if ( output ) { output->slots.reserve( count ); }
	for ( uint32_t i = 0; i < count; ++i ) {
		const auto nested = cursor.Field();
		if ( output ) {
			SlotRecord slot{}; ReadSlot( nested, limits, text_total, &slot );
			output->slots.push_back( std::move( slot ) );
		}
		else { ReadSlot( nested, limits, text_total, nullptr ); }
	}
	cursor.End();
	if ( output ) {
		std::unordered_set< std::string_view > gsids;
		for ( const auto& slot : output->slots ) {
			if ( slot.player && !gsids.insert( slot.linked_gsid ).second ) { Invalid( "duplicate occupied GSID" ); }
		}
	}
}

void Input( std::string_view bytes, const Limits& limits ) {
	ValidateLimits( limits ); if ( bytes.size() > limits.max_encoded_bytes ) { Invalid( "encoded budget exceeded" ); }
}

template< typename T, typename Reader > T Decode( std::string_view bytes, const Limits& limits, Reader reader ) {
	Input( bytes, limits ); size_t text_total = 0; reader( bytes, limits, text_total, nullptr );
	T result{}; text_total = 0; reader( bytes, limits, text_total, &result ); return result;
}

// Measure all owned inputs before allocating output. All lengths include their
// LE32 prefix; the outer size includes nested records only once.
size_t StringSize( const std::string_view value, size_t& text_total, const Limits& limits, bool empty_allowed = false ) {
	Text( value, text_total, limits, empty_allowed ); size_t result = 4; Add( result, value.size(), limits.max_encoded_bytes ); return result;
}
size_t PlayerSize( const PlayerRecord& value, const Limits& limits, size_t& text_total ) {
	CheckRole( static_cast< uint8_t >( value.role ) ); size_t size = 10;
	Add( size, StringSize( value.name, text_total, limits ), limits.max_encoded_bytes );
	if ( value.faction_id ) { Add( size, StringSize( *value.faction_id, text_total, limits ), limits.max_encoded_bytes ); }
	Add( size, StringSize( value.difficulty_id, text_total, limits ), limits.max_encoded_bytes ); return size;
}
size_t SlotSize( const SlotRecord& value, const Limits& limits, size_t& text_total ) {
	CheckState( static_cast< uint8_t >( value.state ) ); CheckFlags( value.flags );
	if ( value.player.has_value() != ( value.state == SlotState::Occupied ) ) { Invalid( "slot/player shape mismatch" ); }
	if ( !value.player && ( value.flags || !value.linked_gsid.empty() ) ) { Invalid( "vacant slot carries player state" ); }
	size_t size = 11;
	if ( value.player ) { Add( size, 4, limits.max_encoded_bytes ); Add( size, PlayerSize( *value.player, limits, text_total ), limits.max_encoded_bytes ); }
	Add( size, StringSize( value.linked_gsid, text_total, limits, !value.player ), limits.max_encoded_bytes ); return size;
}
size_t RosterSize( const RosterRecord& value, const Limits& limits, size_t& text_total ) {
	if ( value.slots.size() > limits.max_slots ) { Invalid( "slot count exceeds bounds" ); }
	size_t size = 12; Add( size, 0, limits.max_encoded_bytes );
	std::unordered_set< std::string_view > gsids;
	for ( const auto& slot : value.slots ) {
		Add( size, 4, limits.max_encoded_bytes ); Add( size, SlotSize( slot, limits, text_total ), limits.max_encoded_bytes );
		if ( slot.player && !gsids.insert( slot.linked_gsid ).second ) { Invalid( "duplicate occupied GSID" ); }
	}
	return size;
}

void Byte( std::string& out, uint8_t value ) { out.push_back( static_cast< char >( value ) ); }
void Word( std::string& out, size_t value ) { for ( unsigned i = 0; i < 4; ++i ) { Byte( out, static_cast< uint8_t >( value >> ( i * 8 ) ) ); } }
void Header( std::string& out, std::string_view kind ) { out.append( kind ); Word( out, version ); }
void Field( std::string& out, std::string_view value ) { Word( out, value.size() ); out.append( value ); }
void WritePlayer( std::string& out, const PlayerRecord& value ) {
	Header( out, player_kind ); Field( out, value.name ); Byte( out, static_cast< uint8_t >( value.role ) ); Byte( out, value.faction_id.has_value() );
	if ( value.faction_id ) { Field( out, *value.faction_id ); } Field( out, value.difficulty_id );
}
template< typename F > void Nested( std::string& out, F write ) {
	const auto offset = out.size(); Word( out, 0 ); write(); const auto size = out.size() - offset - 4;
	for ( unsigned i = 0; i < 4; ++i ) { out[ offset + i ] = static_cast< char >( static_cast< uint8_t >( size >> ( i * 8 ) ) ); }
}
void WriteSlot( std::string& out, const SlotRecord& value ) {
	Header( out, slot_kind ); Byte( out, static_cast< uint8_t >( value.state ) ); Byte( out, value.player.has_value() );
	if ( value.player ) { Nested( out, [ & ]() { WritePlayer( out, *value.player ); } ); }
	Byte( out, value.flags ); Field( out, value.linked_gsid );
}
void WriteRoster( std::string& out, const RosterRecord& value ) {
	Header( out, roster_kind ); Word( out, value.slots.size() );
	for ( const auto& slot : value.slots ) { Nested( out, [ & ]() { WriteSlot( out, slot ); } ); }
}
template< typename T, typename Measure, typename Writer > std::string Encode( const T& value, const Limits& limits, Measure measure, Writer write ) {
	ValidateLimits( limits ); size_t text_total = 0; const auto size = measure( value, limits, text_total );
	std::string result; result.reserve( size ); write( result, value ); return result;
}

}

bool PlayerRecord::operator==( const PlayerRecord& other ) const { return name == other.name && role == other.role && faction_id == other.faction_id && difficulty_id == other.difficulty_id; }
bool SlotRecord::operator==( const SlotRecord& other ) const { return state == other.state && player == other.player && flags == other.flags && linked_gsid == other.linked_gsid; }
bool RosterRecord::operator==( const RosterRecord& other ) const { return slots == other.slots; }
std::string EncodePlayer( const PlayerRecord& value, const Limits& limits ) { return Encode( value, limits, PlayerSize, WritePlayer ); }
std::string EncodeSlot( const SlotRecord& value, const Limits& limits ) { return Encode( value, limits, SlotSize, WriteSlot ); }
std::string EncodeRoster( const RosterRecord& value, const Limits& limits ) { return Encode( value, limits, RosterSize, WriteRoster ); }
PlayerRecord DecodePlayer( std::string_view bytes, const Limits& limits ) { return Decode< PlayerRecord >( bytes, limits, ReadPlayer ); }
SlotRecord DecodeSlot( std::string_view bytes, const Limits& limits ) { return Decode< SlotRecord >( bytes, limits, ReadSlot ); }
RosterRecord DecodeRoster( std::string_view bytes, const Limits& limits ) { return Decode< RosterRecord >( bytes, limits, ReadRoster ); }

}
