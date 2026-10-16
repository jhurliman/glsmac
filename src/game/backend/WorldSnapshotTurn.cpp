#include "WorldSnapshotTurn.h"

#include "Player.h"
#include "slot/Slot.h"
#include "types/Buffer.h"

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace game::backend::world_snapshot {
namespace {

constexpr auto format = "turn-state-v1";

size_t TurnId( const int64_t id ) {
	if ( id < 0 || id > std::numeric_limits< int64_t >::max() - 2100 ||
		static_cast< uint64_t >( id ) > std::numeric_limits< size_t >::max() - 2100 ) {
		throw std::runtime_error( "Invalid world snapshot turn ID" );
	}
	return static_cast< size_t >( id );
}

std::vector< size_t > OccupiedSlots( const std::vector< slot::Slot >& slots ) {
	std::vector< size_t > result;
	for ( size_t i = 0 ; i < slots.size() ; ++i ) {
		const auto& slot = slots[ i ];
		if ( slot.GetIndex() != i ) { throw std::runtime_error( "Noncanonical world snapshot slot index" ); }
		switch ( slot.GetState() ) {
			case slot::Slot::SS_OPEN: case slot::Slot::SS_CLOSED: break;
			case slot::Slot::SS_PLAYER: {
				const auto* player = slot.GetPlayer();
				if ( !player || player->GetSlot() != &slot ) { throw std::runtime_error( "Noncanonical world snapshot Player link" ); }
				result.push_back( i );
				break;
			}
			default: throw std::runtime_error( "Unknown world snapshot slot state" );
		}
	}
	return result;
}

}

TurnState CaptureTurnState( const size_t id, const std::vector< slot::Slot >& slots ) {
	if ( id > static_cast< uint64_t >( std::numeric_limits< int64_t >::max() ) ) { throw std::runtime_error( "World snapshot turn ID exceeds wire range" ); }
	TurnState result{ TurnId( static_cast< int64_t >( id ) ), {} };
	for ( const auto index : OccupiedSlots( slots ) ) {
		result.players.push_back( { index, slots[ index ].GetPlayer()->IsTurnCompleted() } );
	}
	return result;
}

void WriteTurnState( types::Buffer& buffer, const TurnState& state ) {
	if ( state.id > static_cast< uint64_t >( std::numeric_limits< int64_t >::max() ) ||
		state.players.size() > static_cast< uint64_t >( std::numeric_limits< int64_t >::max() ) ) {
		throw std::runtime_error( "World snapshot turn state exceeds wire range" );
	}
	TurnId( static_cast< int64_t >( state.id ) );
	for ( size_t i = 0 ; i < state.players.size() ; ++i ) {
		const auto id = state.players[ i ].id;
		if ( id > static_cast< uint64_t >( std::numeric_limits< int64_t >::max() ) || ( i && id <= state.players[ i - 1 ].id ) ) {
			throw std::runtime_error( "Noncanonical world snapshot completion records" );
		}
	}
	buffer.WriteString( format ); buffer.WriteInt( state.id ); buffer.WriteInt( state.players.size() );
	for ( const auto& player : state.players ) { buffer.WriteInt( player.id ); buffer.WriteBool( player.completed ); }
}

void ValidateTurnRoster( const TurnState& state, const std::vector< slot::Slot >& slots ) {
	const auto occupied = OccupiedSlots( slots );
	if ( occupied.size() != state.players.size() ) { throw std::runtime_error( "World snapshot occupied roster differs from the session" ); }
	for ( size_t i = 0 ; i < occupied.size() ; ++i ) {
		if ( state.players[ i ].id != occupied[ i ] ) { throw std::runtime_error( "World snapshot completion slot differs from the session" ); }
	}
}

TurnState ReadTurnState( types::Buffer& buffer, const std::vector< slot::Slot >& slots ) {
	if ( buffer.ReadString() != format ) { throw std::runtime_error( "Unsupported world snapshot turn-state format" ); }
	TurnState result{ TurnId( buffer.ReadInt() ), {} };
	const auto occupied = OccupiedSlots( slots );
	const auto count = buffer.ReadInt();
	if ( count < 0 || static_cast< uint64_t >( count ) != occupied.size() ) {
		throw std::runtime_error( "World snapshot occupied roster count differs from the session" );
	}
	// Allocation is bounded by the already-established local roster, never by
	// an unchecked wire count. Exact ascending IDs reject omissions/duplicates.
	result.players.reserve( occupied.size() );
	for ( const auto expected : occupied ) {
		const auto id = buffer.ReadInt();
		if ( id < 0 || static_cast< uint64_t >( id ) != expected ) { throw std::runtime_error( "World snapshot completion slot differs from the session" ); }
		result.players.push_back( { expected, buffer.ReadBool() } );
	}
	return result;
}

}
