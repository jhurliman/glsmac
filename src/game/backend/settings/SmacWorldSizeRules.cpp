#include "SmacWorldSizeRules.h"

#include <stdexcept>

namespace game::backend::settings {
namespace {

int32_t ReadInt32( types::Buffer& buffer ) {
	const auto value = buffer.ReadInt();
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::runtime_error( "SMAC world-size integer outside signed32 range" ); }
	return static_cast< int32_t >( value );
}

void End( const types::Buffer& buffer ) {
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC world-size fields" ); }
}

}

bool SmacWorldSizeRule::operator==( const SmacWorldSizeRule& other ) const { return name == other.name && source_x == other.source_x && source_y == other.source_y; }
bool SmacWorldSizeSelection::operator==( const SmacWorldSizeSelection& other ) const { return category == other.category && width == other.width && height == other.height; }
bool SmacWorldSizeRules::operator==( const SmacWorldSizeRules& other ) const { return presets == other.presets; }

void SmacWorldSizeRules::Validate() const {
	for ( const auto& row : presets ) {
		if ( row.name.empty() || row.name.find( '\0' ) != std::string::npos ) { throw std::runtime_error( "SMAC world-size name must be nonempty without NUL" ); }
		// Bounded port ingestion domain. Odd source Y remains represented, but
		// cannot be selected until the backend supports odd-height maps.
		if ( row.source_x < 2 || row.source_x > 256 || row.source_y < 4 || row.source_y > 256 ) { throw std::runtime_error( "SMAC world-size source dimensions outside supported bounds" ); }
	}
}

void SmacWorldSizeSelection::Validate() const {
	if ( ( category < 0 || category > 4 ) && category != 99 ) { throw std::runtime_error( "SMAC world-size category must be preset 0..4 or custom 99" ); }
	if ( width < 4 || width > 512 || height < 4 || height > 256 || ( width & 1 ) || ( height & 1 ) ) { throw std::runtime_error( "GLSMAC requires bounded even world coordinate spans" ); }
	if ( category == 99 && ( width < 32 || height < 16 ) ) { throw std::runtime_error( "SMAC custom coordinate spans must be at least 32 by 16" ); }
}

SmacWorldSizeSelection SmacWorldSizeRules::SelectPreset( const int64_t category ) const {
	Validate();
	if ( category < 0 || category >= static_cast< int64_t >( presets.size() ) ) { throw std::runtime_error( "SMAC world-size preset must be 0..4" ); }
	const auto& row = presets[ static_cast< size_t >( category ) ];
	SmacWorldSizeSelection selected{ static_cast< int32_t >( category ), row.source_x * 2, row.source_y };
	selected.Validate();
	return selected;
}

SmacWorldSizeSelection SmacWorldSizeRules::SelectCustom( const int64_t width, const int64_t height ) {
	if ( width < 32 || width > 512 || height < 16 || height > 256 ) { throw std::runtime_error( "SMAC custom coordinate spans require width 32..512 and height 16..256" ); }
	SmacWorldSizeSelection selected{ 99, static_cast< int32_t >( width ), static_cast< int32_t >( height ) };
	selected.Validate();
	return selected;
}

void SmacWorldSizeRules::ValidateSelection( const SmacWorldSizeSelection& selection ) const {
	Validate(); selection.Validate();
	if ( selection.category != 99 && !( SelectPreset( selection.category ) == selection ) ) { throw std::runtime_error( "SMAC preset dimensions differ from the frozen source row" ); }
}

types::Buffer SmacWorldSizeRules::Serialize() const {
	Validate(); types::Buffer buffer; buffer.WriteInt( 1 ); buffer.WriteInt( presets.size() );
	for ( const auto& row : presets ) { buffer.WriteString( row.name ); buffer.WriteInt( row.source_x ); buffer.WriteInt( row.source_y ); }
	return buffer;
}

SmacWorldSizeRules SmacWorldSizeRules::Deserialize( types::Buffer buffer ) {
	if ( buffer.ReadInt() != 1 || buffer.ReadInt() != 5 ) { throw std::runtime_error( "Unsupported SMAC world-size catalog format" ); }
	SmacWorldSizeRules result;
	for ( auto& row : result.presets ) { row.name = buffer.ReadString(); row.source_x = ReadInt32( buffer ); row.source_y = ReadInt32( buffer ); }
	End( buffer ); result.Validate(); return result;
}

types::Buffer SmacWorldSizeSelection::Serialize() const {
	Validate(); types::Buffer buffer; buffer.WriteInt( 1 ); buffer.WriteInt( category ); buffer.WriteInt( width ); buffer.WriteInt( height ); return buffer;
}

SmacWorldSizeSelection SmacWorldSizeSelection::Deserialize( types::Buffer buffer ) {
	if ( buffer.ReadInt() != 1 ) { throw std::runtime_error( "Unsupported SMAC accepted world-size format" ); }
	SmacWorldSizeSelection result{ ReadInt32( buffer ), ReadInt32( buffer ), ReadInt32( buffer ) };
	End( buffer ); result.Validate(); return result;
}

}
