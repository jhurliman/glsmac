#include "SmacMovementRules.h"

#include <algorithm>
#include <stdexcept>

namespace game::backend::settings {

int32_t SmacMovementRules::Quantum() const {
	return std::clamp( m_road_rate, int32_t{ 1 }, int32_t{ 100 } );
}

int32_t SmacMovementRules::OrdinaryGroundAllowance( const GroundChassis chassis, const int32_t parsed_moves ) const {
	if ( chassis != GroundChassis::Infantry && chassis != GroundChassis::Speeder ) {
		throw std::invalid_argument( "SMAC basic movement requires ordinary Infantry or Speeder" );
	}
	const auto moves = static_cast< uint8_t >( parsed_moves );
	return Quantum() * std::clamp( int32_t{ moves }, int32_t{ 1 }, int32_t{ 99 } );
}

types::Buffer SmacMovementRules::Serialize() const {
	types::Buffer buffer;
	buffer.WriteInt( 1 );
	buffer.WriteInt( m_road_rate );
	return buffer;
}

SmacMovementRules SmacMovementRules::Deserialize( types::Buffer buffer ) {
	if ( buffer.ReadInt() != 1 ) { throw std::runtime_error( "Unsupported SMAC movement profile version" ); }
	const auto road_rate = buffer.ReadInt();
	if ( road_rate < INT32_MIN || road_rate > INT32_MAX ) { throw std::runtime_error( "SMAC road rate exceeds signed32" ); }
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC movement profile fields" ); }
	return SmacMovementRules( static_cast< int32_t >( road_rate ) );
}

}
