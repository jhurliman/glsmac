#include "SmacBaseResourceContext.h"

#include <stdexcept>

namespace game::backend::base {

uint32_t SmacBaseResourceContext::NewGameGovernorOptions( int32_t difficulty ) {
	if ( difficulty < 0 || difficulty > 5 ) { throw std::runtime_error("Unknown startup governor difficulty"); }
	return 0x00dfbe41u | ( difficulty == 0 ? GOVERNOR_ACTIVE | STARTUP_AUTOMATION : 0u );
}

SmacWorkerPriorityContext SmacBaseResourceContext::WorkerPriorities( const std::array<int32_t,5>& faction ) const {
	return { true, GovernorActive(), bool(governor_options & 0x01000000u), bool(governor_options & 0x02000000u),
		bool(governor_options & 0x04000000u), bool(governor_options & 0x08000000u),
		{ faction[0], faction[1], faction[2], faction[3], faction[4] } };
}

void SmacBaseResourceContext::Validate() const {
	if ( owner_simulation_id < 1 || owner_simulation_id > 7 || resource_events > 63 ) {
		throw std::runtime_error( "Invalid SMAC base resource context" );
	}
	if(conquest && (conquest->former_owner<1 || conquest->former_owner>7)) {throw std::runtime_error("Invalid SMAC conquered Base former owner");}
}

bool SmacBaseResourceContext::operator==( const SmacBaseResourceContext& other ) const {
	return owner_simulation_id == other.owner_simulation_id && golden_age == other.golden_age
		&& resource_events == other.resource_events && resource_event_timer == other.resource_event_timer
		&& governor_options == other.governor_options && drone_riots == other.drone_riots && conquest==other.conquest;
}

types::Buffer SerializeSmacBaseResourceContext( const std::optional< SmacBaseResourceContext >& value ) {
	if ( value ) { value->Validate(); }
	types::Buffer buffer;
	buffer.WriteInt( 3 );
	buffer.WriteBool( value.has_value() );
	if ( value ) {
		buffer.WriteInt( value->owner_simulation_id );
		buffer.WriteBool( value->golden_age );
		buffer.WriteInt( value->resource_events );
		buffer.WriteInt( value->resource_event_timer );
		buffer.WriteInt( value->governor_options );
		buffer.WriteBool( value->drone_riots );
		buffer.WriteBool(value->conquest.has_value());
		if(value->conquest) {buffer.WriteInt(value->conquest->former_owner);buffer.WriteInt(value->conquest->assimilation);}
	}
	return buffer;
}

std::optional< SmacBaseResourceContext > DeserializeSmacBaseResourceContext( types::Buffer buffer ) {
	if ( buffer.lenw > SmacBaseResourceContext::MAX_WIRE_BYTES ) {
		throw std::runtime_error( "Unsupported SMAC base resource context" );
	}
	const auto version=buffer.ReadInt();if(version!=2 && version!=3) {throw std::runtime_error("Unsupported SMAC base resource context");}
	std::optional< SmacBaseResourceContext > result;
	if ( buffer.ReadBool() ) {
		const auto owner = buffer.ReadInt();
		const auto golden_age = buffer.ReadBool();
		const auto events = buffer.ReadInt();
		const auto timer = buffer.ReadInt();
		const auto governor = buffer.ReadInt();
		const auto riots = buffer.ReadBool();
		if ( owner < 1 || owner > 7 || events < 0 || events > 63 || timer < INT32_MIN || timer > INT32_MAX
			|| governor < 0 || static_cast< uint64_t >( governor ) > UINT32_MAX ) {
			throw std::runtime_error( "Invalid SMAC base resource context field" );
		}
		result = SmacBaseResourceContext{ static_cast< int32_t >( owner ), golden_age, static_cast< uint8_t >( events ),
			static_cast< int32_t >( timer ), static_cast< uint32_t >( governor ), riots };
		if(version==3 && buffer.ReadBool()) {
			const auto former=buffer.ReadInt(),assimilation=buffer.ReadInt();
			if(former<1 || former>7 || assimilation<0 || assimilation>255) {throw std::runtime_error("Invalid SMAC conquered Base history");}
			result->conquest=SmacConquestState{static_cast<int32_t>(former),static_cast<uint8_t>(assimilation)};
		}
	}
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Trailing SMAC base resource context" ); }
	return result;
}

}
