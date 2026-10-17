#include "SmacTechnologyRules.h"

#include <stdexcept>

namespace game::backend::settings {
namespace {

void RequiredText( const std::string& value ) {
	if ( value.empty() || value.find( '\0' ) != std::string::npos ) {
		throw std::invalid_argument( "SMAC technology text must be nonempty without NUL" );
	}
}

bool SameAsciiKey( const std::string& a, const std::string& b ) {
	if ( a.size() != b.size() ) { return false; }
	const auto lower = []( unsigned char c ) { return c >= 'A' && c <= 'Z' ? c + ( 'a' - 'A' ) : c; };
	for ( size_t i = 0 ; i < a.size() ; ++i ) { if ( lower( a[ i ] ) != lower( b[ i ] ) ) { return false; } }
	return true;
}

int32_t ReadInteger( types::Buffer& buffer ) {
	const auto value = buffer.ReadInt();
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::runtime_error( "SMAC technology integer outside signed32 range" ); }
	return static_cast< int32_t >( value );
}

}

bool SmacTechnologyRule::operator==( const SmacTechnologyRule& other ) const {
	return source_id == other.source_id && name == other.name && key == other.key && priorities == other.priorities
		&& prerequisites == other.prerequisites && flags == other.flags;
}

void SmacTechnologyRules::Validate() const {
	for ( size_t i = 0 ; i < technologies.size() ; ++i ) {
		const auto& row = technologies[ i ];
		if ( row.source_id != static_cast< int32_t >( i ) ) { throw std::invalid_argument( "SMAC technology slots must be 0..88 in order" ); }
		RequiredText( row.name ); RequiredText( row.key );
		for ( const auto& prerequisite : row.prerequisites ) { RequiredText( prerequisite ); }
		if ( row.key.size() > 7 ) { throw std::invalid_argument( "SMAC stored technology keys require at most seven bytes" ); }
		if ( row.flags > 0x1ff ) { throw std::invalid_argument( "SMAC technology flags require nine bits" ); }
	}
}

bool SmacTechnologyRules::operator==( const SmacTechnologyRules& other ) const { return technologies == other.technologies; }

types::Buffer SmacTechnologyRules::Serialize() const {
	Validate();
	types::Buffer buffer;
	buffer.WriteInt( 1 ); buffer.WriteInt( technologies.size() );
	for ( const auto& row : technologies ) {
		buffer.WriteInt( row.source_id ); buffer.WriteString( row.name ); buffer.WriteString( row.key );
		for ( const auto priority : row.priorities ) { buffer.WriteInt( priority ); }
		for ( const auto& prerequisite : row.prerequisites ) { buffer.WriteString( prerequisite ); }
		buffer.WriteInt( row.flags );
	}
	return buffer;
}

SmacTechnologyRules SmacTechnologyRules::Deserialize( types::Buffer buffer ) {
	if ( buffer.ReadInt() != 1 ) { throw std::runtime_error( "Unsupported SMAC technology catalog version" ); }
	SmacTechnologyRules result;
	if ( buffer.ReadInt() != static_cast< int64_t >( result.technologies.size() ) ) { throw std::runtime_error( "SMAC technology catalog requires 89 rows" ); }
	for ( auto& row : result.technologies ) {
		row.source_id = ReadInteger( buffer ); row.name = buffer.ReadString(); row.key = buffer.ReadString();
		for ( auto& priority : row.priorities ) { priority = ReadInteger( buffer ); }
		for ( auto& prerequisite : row.prerequisites ) { prerequisite = buffer.ReadString(); }
		const auto flags = buffer.ReadInt();
		if ( flags < 0 || flags > 0x1ff ) { throw std::runtime_error( "SMAC technology flags require nine bits" ); }
		row.flags = static_cast< uint16_t >( flags );
	}
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC technology catalog fields" ); }
	try { result.Validate(); }
	catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
	return result;
}

SmacTechnologyReference SmacTechnologyRules::ResolveToken( const std::string& token ) const {
	Validate();
	using Kind = SmacTechnologyReference::Kind;
	auto key = token;
	while ( !key.empty() && key.back() == ' ' ) { key.pop_back(); }
	if ( SameAsciiKey( key, "None" ) ) { return { Kind::NONE, {} }; }
	if ( SameAsciiKey( key, "Disable" ) ) { return { Kind::DISABLED, {} }; }
	// RE-SMAC-TECH-PRE-001: first positional match, including collisions after
	// seven-byte key storage. A lookup token itself is never truncated.
	for ( const auto& row : technologies ) { if ( SameAsciiKey( key, row.key ) ) { return { Kind::TECHNOLOGY_ID, row.source_id }; } }
	// Original diagnostic presentation is not invoked by this query. UNKNOWN
	// remains distinct from explicit Disable even if that dialog would return -2.
	return { Kind::UNKNOWN, {} };
}

SmacProductionPrerequisite SmacTechnologyRules::ResolveProductionPrerequisite( const std::string& key ) const {
	using Status = SmacProductionPrerequisite::Status;
	using Kind = SmacTechnologyReference::Kind;
	const auto reference = ResolveToken( key );
	if ( reference.kind == Kind::NONE ) { return { Status::NO_REQUIREMENT, {} }; }
	if ( reference.kind == Kind::DISABLED ) { return { Status::DISABLED, {} }; }
	if ( reference.kind == Kind::UNKNOWN ) { return { Status::UNAVAILABLE, {} }; }
	const auto id = *reference.technology_id;
	// RE-SMAC-FACILITY-ELIGIBLE-001 / RE-SMAC-QUEUE-001: production
	// references admit 0..87, even though technology acquisition includes 88.
	if ( id >= 88 ) { return { Status::UNAVAILABLE, id }; }
	const auto first = ResolveToken( technologies[ id ].prerequisites[ 0 ] ).kind;
	const auto second = ResolveToken( technologies[ id ].prerequisites[ 1 ] ).kind;
	// Refuse to silently continue an original rules-error diagnostic. In
	// particular, None/unknown does not become the explicit None/Disable case.
	if ( first == Kind::UNKNOWN || second == Kind::UNKNOWN ) { return { Status::UNAVAILABLE, id }; }
	if ( first == Kind::DISABLED || ( first != Kind::NONE && second == Kind::DISABLED ) ) { return { Status::DISABLED, id }; }
	// This is a definition check, not a recursive ancestor-ownership check.
	return { Status::TECHNOLOGY_ID, id };
}

}
