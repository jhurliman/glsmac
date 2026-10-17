#include "SmacFactionStartupRules.h"

#include <stdexcept>
#include <utility>

namespace game::backend::settings {
namespace {
using Kind = SmacFactionStartupBonus::Kind;

bool Space( char c ) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }

int32_t DecimalPrefix( const std::string& value ) {
	size_t i = 0;
	while ( i < value.size() && Space( value[ i ] ) ) { ++i; }
	const bool negative = i < value.size() && value[ i ] == '-';
	if ( i < value.size() && ( value[ i ] == '+' || value[ i ] == '-' ) ) { ++i; }
	const uint32_t limit = negative ? uint32_t( INT32_MAX ) + 1 : uint32_t( INT32_MAX );
	uint32_t number = 0;
	while ( i < value.size() && value[ i ] >= '0' && value[ i ] <= '9' ) {
		const uint32_t digit = value[ i++ ] - '0';
		if ( number > ( limit - digit ) / 10 ) { throw std::invalid_argument( "SMAC faction decimal prefix outside signed32 range" ); }
		number = number * 10 + digit;
	}
	return static_cast< int32_t >( negative ? -int64_t( number ) : int64_t( number ) );
}

void Text( const std::string& text ) {
	if ( text.empty() || text.find_first_of( std::string( "\0\r\n,;", 5 ) ) != std::string::npos ) {
		throw std::invalid_argument( "SMAC faction bonus requires nonempty single-line fields without NUL, comma or semicolon" );
	}
}

int32_t Integer( types::Buffer& buffer ) {
	const auto value = buffer.ReadInt();
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::runtime_error( "SMAC faction integer outside signed32 range" ); }
	return static_cast< int32_t >( value );
}
}

SmacFactionStartupBonus SmacFactionStartupBonus::Parse( const std::string& keyword, const std::string& value, const SmacTechnologyRules& technologies ) {
	Text( keyword ); Text( value );
	SmacFactionStartupBonus result{ keyword, value, Kind::NAMED_TECH, 0, 0 };
	if ( keyword == "TECH" ) {
		result.parameter = DecimalPrefix( value );
		if ( result.parameter != 0 ) { result.kind = Kind::EXTRA_TECH; }
		else {
			const auto reference = technologies.ResolveToken( value );
			using Reference = SmacTechnologyReference::Kind;
			if ( reference.kind == Reference::UNKNOWN ) { throw std::invalid_argument( "Unknown SMAC named TECH token: " + value ); }
			result.parameter = reference.kind == Reference::NONE ? -1 : reference.kind == Reference::DISABLED ? -2 : *reference.technology_id;
		}
		return result;
	}
	if ( keyword == "SOCIAL" || keyword == "IMMUNITY" ) {
		static const std::array< const char*, 11 > axes = { "ECONOMY", "EFFIC", "SUPPORT", "TALENT", "MORALE", "POLICE", "GROWTH", "PLANET", "PROBE", "INDUSTRY", "RESEARCH" };
		size_t i = 0;
		int64_t amount = 0;
		while ( i < value.size() && ( value[ i ] == '+' || value[ i ] == '-' ) ) {
			amount += value[ i++ ] == '+' ? 1 : -1;
			if ( amount < INT32_MIN || amount > INT32_MAX ) { throw std::invalid_argument( "SMAC SOCIAL amount outside signed32 range" ); }
		}
		const auto axis = value.substr( i );
		for ( size_t j = 0 ; j < axes.size() ; ++j ) {
			if ( axis == axes[ j ] ) {
				result.kind = keyword == "SOCIAL" ? Kind::SOCIAL : Kind::IMMUNITY;
				result.parameter = static_cast< int32_t >( j );
				result.amount = result.kind == Kind::IMMUNITY || amount == 0 ? 1 : static_cast< int32_t >( amount );
				return result;
			}
		}
		throw std::invalid_argument( "Unsupported SMAC social axis: " + value );
	}
	static const std::array< std::pair< const char*, Kind >, 10 > numeric = {{
		{ "FACILITY", Kind::FACILITY }, { "FUNGNUTRIENT", Kind::FUNGNUTRIENT }, { "VOTES", Kind::VOTES },
		{ "ENERGY", Kind::ENERGY }, { "DRONE", Kind::DRONE }, { "TALENT", Kind::TALENT },
		{ "POPULATION", Kind::POPULATION }, { "COMMERCE", Kind::COMMERCE }, { "FREEPROTO", Kind::FREEPROTO }, { "FANATIC", Kind::FANATIC }
	}};
	for ( const auto& entry : numeric ) {
		if ( keyword == entry.first ) { result.kind = entry.second; result.parameter = DecimalPrefix( value ); return result; }
	}
	// Unknown original keywords may consume the same eight slots. Refusing them
	// is preferable to silently granting a later TECH that the original drops.
	throw std::invalid_argument( "Unsupported SMAC faction bonus keyword: " + keyword );
}

bool SmacFactionStartupBonus::ConsumesRecord() const {
	switch ( kind ) {
		case Kind::NAMED_TECH: case Kind::SOCIAL: case Kind::FACILITY: case Kind::IMMUNITY: case Kind::FUNGNUTRIENT: case Kind::VOTES: return true;
		default: return false;
	}
}

bool SmacFactionStartupBonus::operator==( const SmacFactionStartupBonus& other ) const {
	return keyword == other.keyword && value == other.value && kind == other.kind && parameter == other.parameter && amount == other.amount;
}

std::vector< SmacFactionStartupBonus > SmacFactionStartupRule::Records() const {
	std::vector< SmacFactionStartupBonus > result;
	for ( const auto& bonus : bonuses ) { if ( bonus.ConsumesRecord() && result.size() < 8 ) { result.push_back( bonus ); } }
	return result;
}

std::vector< int32_t > SmacFactionStartupRule::NamedTechnologyReferences() const {
	std::vector< int32_t > result;
	for ( const auto& bonus : Records() ) { if ( bonus.kind == Kind::NAMED_TECH ) { result.push_back( bonus.parameter ); } }
	return result;
}

int32_t SmacFactionStartupRule::ExtraTechnologyCount() const {
	int32_t result = 0;
	for ( const auto& bonus : bonuses ) { if ( bonus.kind == Kind::EXTRA_TECH ) { result = bonus.parameter; } }
	return result;
}

bool SmacFactionStartupRule::operator==( const SmacFactionStartupRule& other ) const {
	return port_id == other.port_id && filename == other.filename && section == other.section && bonuses == other.bonuses;
}

const std::array< SmacFactionStartupBinding, 7 >& SmacFactionStartupRules::Bindings() {
	static const std::array< SmacFactionStartupBinding, 7 > bindings = {{
		{ "GAIANS", "gaians.txt", "GAIANS" }, { "HIVE", "hive.txt", "HIVE" }, { "UNIVERSITY", "univ.txt", "UNIV" },
		{ "MORGANITES", "morgan.txt", "MORGAN" }, { "SPARTANS", "spartans.txt", "SPARTANS" },
		{ "BELIEVERS", "believe.txt", "BELIEVE" }, { "PEACEKEEPERS", "peace.txt", "PEACE" }
	}};
	return bindings;
}

void SmacStartupSocialModel::Validate() const {
	Text( name );
	if ( name.size() > MAX_NAME_BYTES || prerequisite != "None" || !modifiers.empty() ) {
		throw std::invalid_argument( "SMAC startup requires bounded neutral model-zero SOCIO rows with prerequisite None" );
	}
}

bool SmacStartupSocialModel::operator==( const SmacStartupSocialModel& other ) const {
	return name == other.name && prerequisite == other.prerequisite && modifiers == other.modifiers;
}

void SmacFactionStartupRules::Validate( const SmacTechnologyRules& technologies ) const {
	technologies.Validate();
	for ( const auto& model : initial_social_models ) { model.Validate(); }
	for ( size_t i = 0 ; i < factions.size() ; ++i ) {
		const auto& row = factions[ i ]; const auto& binding = Bindings()[ i ];
		if ( row.port_id != binding.port_id || row.filename != binding.filename || row.section != binding.section ) {
			throw std::invalid_argument( "SMAC faction startup profile requires the canonical seven source bindings in order" );
		}
		for ( const auto& bonus : row.bonuses ) {
			if ( !( bonus == SmacFactionStartupBonus::Parse( bonus.keyword, bonus.value, technologies ) ) ) {
				throw std::invalid_argument( "SMAC faction derived bonus metadata does not match its source fields/catalog" );
			}
		}
	}
}

bool SmacFactionStartupRules::operator==( const SmacFactionStartupRules& other ) const { return initial_social_models == other.initial_social_models && factions == other.factions; }

const SmacFactionStartupRule* SmacFactionStartupRules::Find( const std::string& port_id ) const {
	for ( const auto& row : factions ) { if ( row.port_id == port_id ) { return &row; } }
	return nullptr;
}

types::Buffer SmacFactionStartupRules::Serialize( const SmacTechnologyRules& technologies ) const {
	Validate( technologies ); types::Buffer buffer;
	buffer.WriteInt( 2 ); buffer.WriteInt( initial_social_models.size() );
	for ( const auto& model : initial_social_models ) {
		buffer.WriteString( model.name ); buffer.WriteString( model.prerequisite ); buffer.WriteString( model.modifiers );
	}
	buffer.WriteInt( factions.size() );
	for ( const auto& row : factions ) {
		buffer.WriteString( row.port_id ); buffer.WriteString( row.filename ); buffer.WriteString( row.section );
		buffer.WriteInt( row.bonuses.size() );
		for ( const auto& bonus : row.bonuses ) {
			buffer.WriteString( bonus.keyword ); buffer.WriteString( bonus.value ); buffer.WriteInt( static_cast< int32_t >( bonus.kind ) );
			buffer.WriteInt( bonus.parameter ); buffer.WriteInt( bonus.amount );
		}
	}
	return buffer;
}

SmacFactionStartupRules SmacFactionStartupRules::Deserialize( types::Buffer buffer, const SmacTechnologyRules& technologies ) {
	if ( buffer.ReadInt() != 2 ) { throw std::runtime_error( "Unsupported SMAC faction startup profile version" ); }
	SmacFactionStartupRules result;
	if ( buffer.ReadInt() != static_cast< int64_t >( result.initial_social_models.size() ) ) { throw std::runtime_error( "SMAC startup requires four model-zero rows" ); }
	for ( auto& model : result.initial_social_models ) {
		model.name = buffer.ReadString( SmacStartupSocialModel::MAX_NAME_BYTES );
		model.prerequisite = buffer.ReadString( 4 ); model.modifiers = buffer.ReadString( 0 );
	}
	if ( buffer.ReadInt() != static_cast< int64_t >( result.factions.size() ) ) { throw std::runtime_error( "SMAC faction startup profile requires seven bindings" ); }
	for ( auto& row : result.factions ) {
		row.port_id = buffer.ReadString(); row.filename = buffer.ReadString(); row.section = buffer.ReadString();
		const auto count = buffer.ReadInt();
		// At least five type bytes per record, even before any lengths/payloads.
		// This conservative check never allocates from an untrusted count.
		if ( count < 0 || count > ( buffer.lenw - buffer.lenr ) / 5 ) { throw std::runtime_error( "Invalid SMAC faction bonus count" ); }
		for ( int64_t i = 0 ; i < count ; ++i ) {
			SmacFactionStartupBonus bonus;
			bonus.keyword = buffer.ReadString(); bonus.value = buffer.ReadString();
			const auto kind = buffer.ReadInt();
			if ( kind < 0 || kind > static_cast< int32_t >( Kind::FANATIC ) ) { throw std::runtime_error( "Invalid SMAC faction bonus tag" ); }
			bonus.kind = static_cast< Kind >( kind ); bonus.parameter = Integer( buffer ); bonus.amount = Integer( buffer );
			row.bonuses.push_back( std::move( bonus ) );
		}
	}
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC faction startup fields" ); }
	try { result.Validate( technologies ); }
	catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
	return result;
}

}
