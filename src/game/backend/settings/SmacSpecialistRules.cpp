#include "SmacSpecialistRules.h"

#include <stdexcept>

namespace game::backend::settings {
namespace {
void Text( const std::string& value ) {
	if ( value.empty() || value.size() > SmacSpecialistRules::MAX_TEXT_BYTES || value.find( '\0' ) != std::string::npos ) {
		throw std::invalid_argument( "SMAC specialist metadata exceeds admitted text domain" );
	}
}
}

bool SmacSpecialistRule::operator==( const SmacSpecialistRule& other ) const {
	return source_id == other.source_id && singular == other.singular && plural == other.plural
		&& prerequisite == other.prerequisite && obsolete == other.obsolete
		&& economy == other.economy && psych == other.psych && labs == other.labs && trailing_token == other.trailing_token;
}

void SmacSpecialistRules::Validate() const {
	for ( size_t i = 0; i < specialists.size(); ++i ) {
		const auto& row = specialists[ i ];
		if ( row.source_id != static_cast< int32_t >( i ) ) { throw std::invalid_argument( "SMAC specialists require ordered source slots 0..6" ); }
		for ( const auto* text : { &row.singular, &row.plural, &row.prerequisite, &row.obsolete, &row.trailing_token } ) { Text( *text ); }
	}
}

bool SmacSpecialistRules::operator==( const SmacSpecialistRules& other ) const {
	return minimum_base_size == other.minimum_base_size && specialists == other.specialists;
}

std::array< std::array< SmacTechnologyReference, 2 >, 7 > SmacSpecialistRules::TechnologyReferences( const SmacTechnologyRules& technologies ) const {
	Validate(); technologies.Validate();
	std::array< std::array< SmacTechnologyReference, 2 >, 7 > result;
	for ( size_t i = 0; i < specialists.size(); ++i ) {
		result[ i ] = { technologies.ResolveToken( specialists[ i ].prerequisite ), technologies.ResolveToken( specialists[ i ].obsolete ) };
	}
	return result;
}

types::Buffer SmacSpecialistRules::Serialize() const {
	Validate();
	types::Buffer result; result.WriteInt( 1 ); result.WriteInt( minimum_base_size ); result.WriteInt( specialists.size() );
	for ( const auto& row : specialists ) {
		result.WriteInt( row.source_id ); result.WriteString( row.singular ); result.WriteString( row.plural );
		result.WriteString( row.prerequisite ); result.WriteString( row.obsolete );
		result.WriteInt( row.economy ); result.WriteInt( row.psych ); result.WriteInt( row.labs ); result.WriteString( row.trailing_token );
	}
	return result;
}

SmacSpecialistRules SmacSpecialistRules::Deserialize( types::Buffer buffer ) {
	if ( buffer.lenw > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC specialist profile exceeds wire budget" ); }
	if ( buffer.ReadInt() != 1 ) { throw std::runtime_error( "Unsupported SMAC specialist profile version" ); }
	const auto integer = [ & ]() {
		const auto value = buffer.ReadInt();
		if ( value < INT32_MIN || value > INT32_MAX ) { throw std::runtime_error( "SMAC specialist metadata exceeds signed32" ); }
		return static_cast< int32_t >( value );
	};
	SmacSpecialistRules result; result.minimum_base_size = integer();
	if ( buffer.ReadInt() != static_cast< int64_t >( result.specialists.size() ) ) { throw std::runtime_error( "SMAC specialist profile requires seven rows" ); }
	for ( auto& row : result.specialists ) {
		row.source_id = integer(); row.singular = buffer.ReadString( MAX_TEXT_BYTES ); row.plural = buffer.ReadString( MAX_TEXT_BYTES );
		row.prerequisite = buffer.ReadString( MAX_TEXT_BYTES ); row.obsolete = buffer.ReadString( MAX_TEXT_BYTES );
		row.economy = integer(); row.psych = integer(); row.labs = integer(); row.trailing_token = buffer.ReadString( MAX_TEXT_BYTES );
	}
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC specialist profile fields" ); }
	try { result.Validate(); } catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
	return result;
}

}
