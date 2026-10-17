#include "RosterRecords.h"

#include "game/backend/RosterRecords.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace game::backend::roster;
const Limits limits{ 16, 1024, 4096 };
const PlayerRecord player{ "A", Role::Host, "G", "D" };
const SlotRecord occupied{ SlotState::Occupied, player, 7, "H" };
const SlotRecord open{ SlotState::Open, std::nullopt, 0, "" };
const SlotRecord closed{ SlotState::Closed, std::nullopt, 0, "" };

bool Rejects( const std::function< void() >& action ) {
	try { action(); } catch ( const std::invalid_argument& ) { return true; }
	return false;
}
std::string Hex( const std::string& source ) {
	std::string result;
	for ( size_t i = 0; i < source.size(); i += 2 ) { result.push_back( static_cast< char >( std::stoul( source.substr( i, 2 ), nullptr, 16 ) ) ); }
	return result;
}
void Word( std::string& bytes, size_t offset, uint32_t value ) {
	for ( unsigned i = 0; i < 4; ++i ) { bytes.at( offset + i ) = static_cast< char >( static_cast< uint8_t >( value >> ( i * 8 ) ) ); }
}

}

void AddRosterRecordTests( task::gsetests::GSETests* task ) {
	task->AddTest( "roster value codecs match independent versioned bytes", GT() {
		const auto expected_player = Hex( "474c5052010000000100000041020101000000470100000044" );
		const auto expected_open = Hex( "474c534c0100000001000000000000" );
		const auto expected_empty = Hex( "474c52530100000000000000" );
		GT_ASSERT( EncodePlayer( player, limits ) == expected_player, "Player wire order, lengths or version changed" );
		GT_ASSERT( DecodePlayer( expected_player, limits ) == player, "independent Player bytes decoded incorrectly" );
		GT_ASSERT( EncodeSlot( open, limits ) == expected_open && DecodeSlot( expected_open, limits ) == open, "vacant Slot bytes changed" );
		GT_ASSERT( EncodeRoster( {}, limits ) == expected_empty && DecodeRoster( expected_empty, limits ).slots.empty(), "empty Roster shape changed" );
		GT_OK();
	} );
	task->AddTest( "roster value codecs own ordered metadata without native defaults", GT() {
		RosterRecord source{ { open, occupied, closed } };
		for ( const auto role : { Role::Single, Role::Host, Role::Player } ) {
			for ( uint8_t flags = 0; flags < 8; ++flags ) {
				source.slots[ 1 ].flags = flags; source.slots[ 1 ].player->role = role;
				source.slots[ 1 ].player->name = "  \xc3\x85 name  ";
				source.slots[ 1 ].player->difficulty_id = "Case-Sensitive Difficulty";
				source.slots[ 1 ].player->faction_id = ( flags & 1 ) ? std::optional< std::string >( "FACTION exact" ) : std::nullopt;
				const auto expected = source; auto bytes = EncodeRoster( source, limits );
				auto decoded = DecodeRoster( bytes, limits ); auto copied = decoded; auto moved = std::move( copied );
				bytes.assign( bytes.size(), 'x' ); decoded.slots[ 1 ].player->name = "changed";
				GT_ASSERT( moved == expected && source == expected, "decoded strings, order or optional ID alias their source" );
				GT_ASSERT( DecodeSlot( EncodeSlot( source.slots[ 1 ], limits ), limits ) == source.slots[ 1 ], "standalone occupied Slot lost metadata" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "roster value codecs reject every truncation and trailing envelope", GT() {
		const auto check = [ & ]( const std::string& bytes, const auto& decode ) {
			for ( size_t size = 0; size < bytes.size(); ++size ) { if ( !Rejects( [ & ]() { decode( bytes.substr( 0, size ), limits ); } ) ) { return false; } }
			return Rejects( [ & ]() { decode( bytes + '\0', limits ); } );
		};
		GT_ASSERT( check( EncodePlayer( player, limits ), DecodePlayer ), "Player truncation/trailing bytes accepted" );
		GT_ASSERT( check( EncodeSlot( occupied, limits ), DecodeSlot ), "Slot truncation/trailing bytes accepted" );
		GT_ASSERT( check( EncodeRoster( { { open, occupied, closed } }, limits ), DecodeRoster ), "nested Roster truncation/trailing bytes accepted" );
		GT_OK();
	} );
	task->AddTest( "roster value codecs reject hostile kinds versions counts and lengths", GT() {
		const auto p = EncodePlayer( player, limits ), s = EncodeSlot( occupied, limits ), r = EncodeRoster( { { open, occupied } }, limits );
		for ( const auto word : { uint32_t( 0 ), uint32_t( 2 ), UINT32_MAX } ) {
			for ( const auto* bytes : { &p, &s, &r } ) {
				auto bad = *bytes; Word( bad, 4, word );
				GT_ASSERT( Rejects( [ & ]() { if ( bytes == &p ) { DecodePlayer( bad, limits ); } else if ( bytes == &s ) { DecodeSlot( bad, limits ); } else { DecodeRoster( bad, limits ); } } ), "unknown record version accepted" );
			}
		}
		GT_ASSERT( Rejects( [ & ]() { DecodePlayer( s, limits ); } ) && Rejects( [ & ]() { DecodeSlot( p, limits ); } ) && Rejects( [ & ]() { DecodeRoster( p, limits ); } ), "wrong record kind accepted" );
		for ( const auto length : { uint32_t( 0 ), uint32_t( 4096 ), UINT32_MAX } ) {
			auto bad = p; Word( bad, 8, length ); GT_ASSERT( Rejects( [ & ]() { DecodePlayer( bad, limits ); } ), "hostile Player text length accepted" );
			bad = s; Word( bad, 10, length ); GT_ASSERT( Rejects( [ & ]() { DecodeSlot( bad, limits ); } ), "hostile nested Player length accepted" );
			bad = r; Word( bad, 12, length ); GT_ASSERT( Rejects( [ & ]() { DecodeRoster( bad, limits ); } ), "hostile nested Slot length accepted" );
		}
		for ( const auto count : { uint32_t( 0 ), uint32_t( 1 ), uint32_t( 3 ), UINT32_MAX } ) {
			auto bad = r; Word( bad, 8, count ); GT_ASSERT( Rejects( [ & ]() { DecodeRoster( bad, limits ); } ), "incorrect/negative-as-unsigned roster count accepted" );
		}
		// A nested record may not hide its own trailing byte inside a valid outer extent.
		auto bad = s; bad.insert( 14 + p.size(), 1, 'x' ); Word( bad, 10, static_cast< uint32_t >( p.size() + 1 ) );
		GT_ASSERT( Rejects( [ & ]() { DecodeSlot( bad, limits ); } ), "nested Player trailing bytes accepted" );
		GT_OK();
	} );
	task->AddTest( "roster value codecs validate raw enums presence and vacant shape", GT() {
		const auto p = EncodePlayer( player, limits ), s = EncodeSlot( occupied, limits );
		for ( const uint8_t value : { 0, 4, 255 } ) {
			auto bad = p; bad[ 13 ] = static_cast< char >( value ); GT_ASSERT( Rejects( [ & ]() { DecodePlayer( bad, limits ); } ), "unknown Player role accepted" );
			auto invalid = player; invalid.role = static_cast< Role >( value ); GT_ASSERT( Rejects( [ & ]() { EncodePlayer( invalid, limits ); } ), "unknown owned role encoded" );
		}
		for ( const uint8_t value : { 2, 255 } ) {
			auto bad = p; bad[ 14 ] = static_cast< char >( value ); GT_ASSERT( Rejects( [ & ]() { DecodePlayer( bad, limits ); } ), "nonboolean faction presence accepted" );
			bad = s; bad[ 9 ] = static_cast< char >( value ); GT_ASSERT( Rejects( [ & ]() { DecodeSlot( bad, limits ); } ), "nonboolean Player presence accepted" );
		}
		for ( const uint8_t value : { 3, 255 } ) {
			auto bad = s; bad[ 8 ] = static_cast< char >( value ); GT_ASSERT( Rejects( [ & ]() { DecodeSlot( bad, limits ); } ), "unknown Slot state accepted" );
		}
		for ( const uint8_t flags : { 8, 128, 255 } ) {
			auto bad = s; bad[ 14 + p.size() ] = static_cast< char >( flags ); GT_ASSERT( Rejects( [ & ]() { DecodeSlot( bad, limits ); } ), "unknown flag bits accepted" );
			auto invalid = occupied; invalid.flags = flags; GT_ASSERT( Rejects( [ & ]() { EncodeSlot( invalid, limits ); } ), "unknown owned flags encoded" );
		}
		for ( const auto invalid : { SlotRecord{ SlotState::Occupied, std::nullopt, 0, "" }, SlotRecord{ SlotState::Open, player, 0, "H" }, SlotRecord{ SlotState::Closed, std::nullopt, 1, "" }, SlotRecord{ SlotState::Open, std::nullopt, 0, "H" } } ) {
			GT_ASSERT( Rejects( [ & ]() { EncodeSlot( invalid, limits ); } ), "invalid owned Slot shape encoded" );
		}
		auto bad = s; bad[ 8 ] = 1; GT_ASSERT( Rejects( [ & ]() { DecodeSlot( bad, limits ); } ), "Player in vacant wire Slot accepted" );
		bad = EncodeSlot( open, limits ); bad[ 8 ] = 2; GT_ASSERT( Rejects( [ & ]() { DecodeSlot( bad, limits ); } ), "occupied wire Slot without Player accepted" );
		GT_OK();
	} );
	task->AddTest( "roster value codecs enforce exact aggregate budgets and limits", GT() {
		const RosterRecord value{ { open, occupied, closed } }; const auto bytes = EncodeRoster( value, limits );
		const Limits exact{ 3, 4, bytes.size() }; // A + G + D + H across the complete roster
		GT_ASSERT( DecodeRoster( bytes, exact ) == value && EncodeRoster( value, exact ) == bytes, "exact budgets rejected" );
		for ( const auto invalid : { Limits{ 2, 4, bytes.size() }, Limits{ 3, 3, bytes.size() }, Limits{ 3, 4, bytes.size() - 1 }, Limits{ 0, 4, bytes.size() }, Limits{ 3, 0, bytes.size() }, Limits{ 3, 4, 0 } } ) {
			GT_ASSERT( Rejects( [ & ]() { DecodeRoster( bytes, invalid ); } ) && Rejects( [ & ]() { EncodeRoster( value, invalid ); } ), "invalid limit or aggregate overflow accepted" );
		}
		if ( std::numeric_limits< size_t >::max() > UINT32_MAX ) {
			const auto large = static_cast< size_t >( UINT32_MAX ) + 1;
			for ( const auto invalid : { Limits{ large, 4, 100 }, Limits{ 3, large, 100 }, Limits{ 3, 4, large } } ) { GT_ASSERT( Rejects( [ & ]() { DecodeRoster( bytes, invalid ); } ), "oversized policy limit accepted" ); }
		}
		auto second = occupied; second.linked_gsid = "I";
		const auto two = EncodeRoster( { { occupied, second } }, limits );
		GT_ASSERT( Rejects( [ & ]() { DecodeRoster( two, { 2, 7, two.size() } ); } ), "nested text budget was reset per Player/Slot" );
		GT_OK();
	} );
	task->AddTest( "roster value codecs reject duplicate identity and invalid text without publication", GT() {
		for ( const auto& text : { std::string(), std::string( "A\0B", 3 ) } ) {
			for ( size_t field = 0; field < 4; ++field ) {
				auto value = occupied;
				if ( field == 0 ) { value.player->name = text; } else if ( field == 1 ) { value.player->faction_id = text; } else if ( field == 2 ) { value.player->difficulty_id = text; } else { value.linked_gsid = text; }
				GT_ASSERT( Rejects( [ & ]() { EncodeSlot( value, limits ); } ), "empty/NUL owned identity accepted" );
			}
		}
		const auto p = EncodePlayer( player, limits );
		for ( const size_t position : { size_t( 12 ), size_t( 19 ), size_t( 24 ) } ) {
			auto bad = p; bad[ position ] = '\0'; GT_ASSERT( Rejects( [ & ]() { DecodePlayer( bad, limits ); } ), "NUL wire text accepted" );
			bad = p; bad.erase( position, 1 ); Word( bad, position - 4, 0 );
			GT_ASSERT( Rejects( [ & ]() { DecodePlayer( bad, limits ); } ), "empty wire text accepted in otherwise complete record" );
		}
		auto empty_gsid = EncodeSlot( occupied, limits ); empty_gsid.pop_back(); Word( empty_gsid, empty_gsid.size() - 4, 0 );
		GT_ASSERT( Rejects( [ & ]() { DecodeSlot( empty_gsid, limits ); } ), "occupied Slot with empty wire GSID accepted" );
		GT_ASSERT( Rejects( [ & ]() { EncodeRoster( { { occupied, occupied } }, limits ); } ), "duplicate owned GSID accepted" );
		auto second = occupied; second.linked_gsid = "I";
		auto bad = EncodeRoster( { { occupied, second } }, limits ); bad.back() = 'H';
		const auto original = bad; RosterRecord published{ { open } }; const auto before = published;
		GT_ASSERT( Rejects( [ & ]() { published = DecodeRoster( bad, limits ); } ), "duplicate wire GSID accepted" );
		GT_ASSERT( bad == original && published == before, "rejected decode changed source or caller destination" );
		GT_OK();
	} );
}
}
