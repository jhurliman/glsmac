#include "SmacBaseAllocationState.h"

#include <limits>
#include <stdexcept>

namespace game::backend::base {

void SmacBaseAllocationState::Validate() const {
	if ( specialists.total_count < 0 || specialists.total_count > 127 ) {
		throw std::invalid_argument( "SMAC saved specialist count exceeds 0..127" );
	}
	for ( const auto type : specialists.types ) {
		if ( type >= 7 ) { throw std::invalid_argument( "SMAC saved specialist slot is outside catalog 0..6" ); }
	}
	// Upper selected bits are diagnostic, not extra radius positions. They
	// still affect the original admission popcount and must survive storage.
}

bool SmacBaseAllocationState::operator==( const SmacBaseAllocationState& other ) const {
	return selected == other.selected && specialists.total_count == other.specialists.total_count
		&& specialists.types == other.specialists.types;
}

types::Buffer SmacBaseAllocationState::Serialize() const {
	Validate();
	types::Buffer buffer;
	buffer.WriteInt( 1 ); buffer.WriteInt( selected ); buffer.WriteInt( specialists.total_count );
	std::string slots;
	slots.reserve( specialists.types.size() );
	for ( const auto type : specialists.types ) { slots.push_back( static_cast< char >( type ) ); }
	buffer.WriteString( slots );
	return buffer;
}

SmacBaseAllocationState SmacBaseAllocationState::Deserialize( const types::Buffer& source ) {
	if ( source.lenr != 0 || source.lenw != WIRE_BYTES ) {
		throw std::invalid_argument( "SMAC base allocation requires one complete fixed-size frame" );
	}
	types::Buffer buffer( source );
	if ( buffer.ReadInt() != 1 ) { throw std::invalid_argument( "Unsupported SMAC base allocation version" ); }
	const auto selected = buffer.ReadInt(), count = buffer.ReadInt();
	if ( selected < 0 || selected > std::numeric_limits< uint32_t >::max() || count < 0 || count > 127 ) {
		throw std::invalid_argument( "SMAC base allocation field exceeds its stored domain" );
	}
	const auto slots = buffer.ReadString( 16 );
	if ( slots.size() != 16 || buffer.lenr != buffer.lenw ) {
		throw std::invalid_argument( "SMAC base allocation requires all sixteen stored slots" );
	}
	SmacBaseAllocationState result{ static_cast< uint32_t >( selected ), { {}, static_cast< int32_t >( count ) } };
	for ( size_t i = 0; i < result.specialists.types.size(); ++i ) {
		result.specialists.types[ i ] = static_cast< uint8_t >( slots[ i ] );
	}
	result.Validate();
	return result;
}

}
