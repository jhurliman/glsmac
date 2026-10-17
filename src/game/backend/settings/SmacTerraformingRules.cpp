#include "SmacTerraformingRules.h"

#include <stdexcept>

namespace game::backend::settings {
namespace {
void Text( const std::string& value, bool optional = false ) {
	if ( ( !optional && value.empty() ) || value.size() > SmacTerraformingRules::MAX_TEXT_BYTES || value.find( '\0' ) != std::string::npos ) {
		throw std::invalid_argument( "SMAC terraforming metadata exceeds admitted text domain" );
	}
}
}

bool SmacTerraformingRule::operator==( const SmacTerraformingRule& other ) const {
	return source_id == other.source_id && land_name == other.land_name && land_prerequisite == other.land_prerequisite
		&& sea_name == other.sea_name && sea_prerequisite == other.sea_prerequisite && work_time == other.work_time
		&& order_text == other.order_text && order_letter == other.order_letter && keystroke == other.keystroke
		&& trailing_annotation == other.trailing_annotation;
}

void SmacTerraformingRules::Validate() const {
	for ( size_t i = 0; i < actions.size(); ++i ) {
		const auto& row = actions[ i ];
		if ( row.source_id != static_cast< int32_t >( i ) ) { throw std::invalid_argument( "SMAC terraforming requires ordered source slots 0..19" ); }
		for ( const auto* text : { &row.land_name, &row.land_prerequisite, &row.sea_name, &row.sea_prerequisite,
			&row.order_text, &row.order_letter, &row.keystroke } ) { Text( *text ); }
		Text( row.trailing_annotation, true );
	}
}

bool SmacTerraformingRules::operator==( const SmacTerraformingRules& other ) const { return actions == other.actions; }

std::array< std::array< SmacTechnologyReference, 2 >, 20 > SmacTerraformingRules::TechnologyReferences( const SmacTechnologyRules& technologies ) const {
	Validate(); technologies.Validate();
	std::array< std::array< SmacTechnologyReference, 2 >, 20 > result;
	for ( size_t i = 0; i < actions.size(); ++i ) {
		result[ i ] = { technologies.ResolveToken( actions[ i ].land_prerequisite ), technologies.ResolveToken( actions[ i ].sea_prerequisite ) };
	}
	return result;
}

types::Buffer SmacTerraformingRules::Serialize() const {
	Validate(); types::Buffer result; result.WriteInt( 1 ); result.WriteInt( actions.size() );
	for ( const auto& row : actions ) {
		result.WriteInt( row.source_id ); result.WriteString( row.land_name ); result.WriteString( row.land_prerequisite );
		result.WriteString( row.sea_name ); result.WriteString( row.sea_prerequisite ); result.WriteInt( row.work_time );
		result.WriteString( row.order_text ); result.WriteString( row.order_letter ); result.WriteString( row.keystroke ); result.WriteString( row.trailing_annotation );
	}
	return result;
}

SmacTerraformingRules SmacTerraformingRules::Deserialize( types::Buffer buffer ) {
	if ( buffer.lenw > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC terraforming profile exceeds wire budget" ); }
	if ( buffer.ReadInt() != 1 ) { throw std::runtime_error( "Unsupported SMAC terraforming profile version" ); }
	SmacTerraformingRules result;
	if ( buffer.ReadInt() != static_cast< int64_t >( result.actions.size() ) ) { throw std::runtime_error( "SMAC terraforming profile requires twenty rows" ); }
	const auto integer = [ & ]() {
		const auto value = buffer.ReadInt();
		if ( value < INT32_MIN || value > INT32_MAX ) { throw std::runtime_error( "SMAC terraforming metadata exceeds signed32" ); }
		return static_cast< int32_t >( value );
	};
	for ( auto& row : result.actions ) {
		row.source_id = integer(); row.land_name = buffer.ReadString( MAX_TEXT_BYTES ); row.land_prerequisite = buffer.ReadString( MAX_TEXT_BYTES );
		row.sea_name = buffer.ReadString( MAX_TEXT_BYTES ); row.sea_prerequisite = buffer.ReadString( MAX_TEXT_BYTES ); row.work_time = integer();
		row.order_text = buffer.ReadString( MAX_TEXT_BYTES ); row.order_letter = buffer.ReadString( MAX_TEXT_BYTES );
		row.keystroke = buffer.ReadString( MAX_TEXT_BYTES ); row.trailing_annotation = buffer.ReadString( MAX_TEXT_BYTES );
	}
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC terraforming profile fields" ); }
	try { result.Validate(); } catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
	return result;
}

}
