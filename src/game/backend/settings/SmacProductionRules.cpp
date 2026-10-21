#include "SmacProductionRules.h"

#include <limits>
#include <stdexcept>

namespace game::backend::settings {
namespace {

void RequiredText( const std::string& value ) {
	if ( value.empty() || value.find( '\0' ) != std::string::npos ) {
		throw std::invalid_argument( "SMAC facility name/prerequisite must be nonempty text without NUL" );
	}
}

int32_t ReadInteger( types::Buffer& buffer ) {
	const auto value = buffer.ReadInt();
	if ( value < INT32_MIN || value > INT32_MAX ) {
		throw std::runtime_error( "SMAC production metadata is outside signed int32 range" );
	}
	return static_cast< int32_t >( value );
}

}

bool SmacFacilityRule::operator==( const SmacFacilityRule& other ) const {
	return source_id == other.source_id && name == other.name && cost == other.cost
		&& maintenance == other.maintenance && prerequisite == other.prerequisite;
}

void SmacProductionRules::Validate() const {
	for ( size_t i = 0 ; i < facilities.size() ; ++i ) {
		const auto& facility = facilities[ i ];
		if ( facility.source_id != static_cast< int32_t >( i + 1 ) ) {
			throw std::invalid_argument( "SMAC facilities require canonical source slots 1..71 in order" );
		}
		RequiredText( facility.name );
		RequiredText( facility.prerequisite );
	}
}

bool SmacProductionRules::operator==( const SmacProductionRules& other ) const {
	return mineral_multiplier == other.mineral_multiplier && retool_loss_percent == other.retool_loss_percent
		&& retool_strictness == other.retool_strictness && retool_exemption == other.retool_exemption
		&& facilities == other.facilities && prototype_percentages == other.prototype_percentages;
}

types::Buffer SmacProductionRules::Serialize() const {
	Validate();
	types::Buffer buffer;
	buffer.WriteInt( prototype_percentages ? 2 : 1 );
	for ( const auto value : { mineral_multiplier, retool_loss_percent, retool_strictness, retool_exemption } ) { buffer.WriteInt( value ); }
	buffer.WriteInt( facilities.size() );
	for ( const auto& facility : facilities ) {
		buffer.WriteInt( facility.source_id ); buffer.WriteString( facility.name );
		buffer.WriteInt( facility.cost ); buffer.WriteInt( facility.maintenance ); buffer.WriteString( facility.prerequisite );
	}
	if(prototype_percentages) {for(const auto value:*prototype_percentages) {buffer.WriteInt(value);}}
	return buffer;
}

SmacProductionRules SmacProductionRules::Deserialize( types::Buffer buffer ) {
	const auto version=buffer.ReadInt();
	if ( version != 1 && version != 2 ) { throw std::runtime_error( "Unsupported SMAC production profile version" ); }
	SmacProductionRules result;
	result.mineral_multiplier = ReadInteger( buffer ); result.retool_loss_percent = ReadInteger( buffer );
	result.retool_strictness = ReadInteger( buffer ); result.retool_exemption = ReadInteger( buffer );
	if ( buffer.ReadInt() != static_cast< int64_t >( result.facilities.size() ) ) {
		throw std::runtime_error( "SMAC production profile requires exactly 71 facilities" );
	}
	for ( auto& facility : result.facilities ) {
		facility.source_id = ReadInteger( buffer ); facility.name = buffer.ReadString();
		facility.cost = ReadInteger( buffer ); facility.maintenance = ReadInteger( buffer ); facility.prerequisite = buffer.ReadString();
	}
	if(version==2) {result.prototype_percentages=std::array<int32_t,3>{{ReadInteger(buffer),ReadInteger(buffer),ReadInteger(buffer)}};}
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC production profile fields" ); }
	try { result.Validate(); }
	catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
	return result;
}

}
