#include "SmacBaseResourceContext.h"

#include <stdexcept>

namespace game::backend::base {

void SmacBaseResourceContext::Validate() const {
	if ( owner_simulation_id < 1 || owner_simulation_id > 7 || resource_events > 63 ) {
		throw std::runtime_error( "Invalid SMAC base resource context" );
	}
}

bool SmacBaseResourceContext::operator==( const SmacBaseResourceContext& other ) const {
	return owner_simulation_id == other.owner_simulation_id && golden_age == other.golden_age
		&& resource_events == other.resource_events && resource_event_timer == other.resource_event_timer
		&& governor_options == other.governor_options;
}

types::Buffer SerializeSmacBaseResourceContext( const std::optional< SmacBaseResourceContext >& value ) {
	if ( value ) { value->Validate(); }
	types::Buffer buffer;
	buffer.WriteInt( 1 );
	buffer.WriteBool( value.has_value() );
	if ( value ) {
		buffer.WriteInt( value->owner_simulation_id );
		buffer.WriteBool( value->golden_age );
		buffer.WriteInt( value->resource_events );
		buffer.WriteInt( value->resource_event_timer );
		buffer.WriteInt( value->governor_options );
	}
	return buffer;
}

std::optional< SmacBaseResourceContext > DeserializeSmacBaseResourceContext( types::Buffer buffer ) {
	if ( buffer.lenw > SmacBaseResourceContext::MAX_WIRE_BYTES || buffer.ReadInt() != 1 ) {
		throw std::runtime_error( "Unsupported SMAC base resource context" );
	}
	std::optional< SmacBaseResourceContext > result;
	if ( buffer.ReadBool() ) {
		const auto owner = buffer.ReadInt();
		const auto golden_age = buffer.ReadBool();
		const auto events = buffer.ReadInt();
		const auto timer = buffer.ReadInt();
		const auto governor = buffer.ReadInt();
		if ( owner < 1 || owner > 7 || events < 0 || events > 63 || timer < INT32_MIN || timer > INT32_MAX
			|| governor < 0 || static_cast< uint64_t >( governor ) > UINT32_MAX ) {
			throw std::runtime_error( "Invalid SMAC base resource context field" );
		}
		result = SmacBaseResourceContext{ static_cast< int32_t >( owner ), golden_age, static_cast< uint8_t >( events ),
			static_cast< int32_t >( timer ), static_cast< uint32_t >( governor ) };
	}
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Trailing SMAC base resource context" ); }
	return result;
}

}
