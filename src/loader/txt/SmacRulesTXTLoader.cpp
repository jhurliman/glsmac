#include "SmacRulesTXTLoader.h"

#include "TXTLoader.h"
#include "util/String.h"

#include <array>
#include <algorithm>
#include <charconv>
#include <stdexcept>
#include <string_view>

namespace loader::txt {

using game::backend::resource::smac::BaseSquareRules;
using game::backend::resource::smac::RuleTriple;

static int32_t ParseField( const std::string& field, const std::string& row, const size_t column, const std::string& section = "RESOURCEINFO" ) {
	const auto* begin = field.data();
	const auto* end = begin + field.size();
	// from_chars accepts minus but not plus. Permit one explicit plus before
	// digits, without accepting malformed combinations such as +-1 or ++1.
	if ( begin != end && *begin == '+' ) {
		++begin;
		if ( begin == end || *begin < '0' || *begin > '9' ) {
			throw std::invalid_argument( "#" + section + " " + row + ": invalid signed int32 column " + std::to_string( column ) );
		}
	}
	int32_t value = 0;
	const auto parsed = std::from_chars( begin, end, value, 10 );
	if ( parsed.ec != std::errc() || parsed.ptr != end ) {
		throw std::invalid_argument( "#" + section + " " + row + ": invalid signed int32 column " + std::to_string( column ) );
	}
	return value;
}

namespace {

std::string_view ResourceTrim( std::string_view text ) {
	const auto first = text.find_first_not_of( " \t" );
	if ( first == std::string_view::npos ) { return {}; }
	return text.substr( first, text.find_last_not_of( " \t" ) - first + 1 );
}

int32_t ResourceNumber( std::string_view text ) {
	text = ResourceTrim( text );
	unsigned radix = 10;
	bool negative = false;
	if ( text.size() >= 2 && text[ 0 ] == '0' ) {
		switch ( text[ 1 ] ) {
			case 'x': case 'X': radix = 16; text.remove_prefix( 2 ); break;
			case 'b': case 'B': radix = 2; text.remove_prefix( 2 ); break;
			case 'd': case 'D': text.remove_prefix( 2 ); break;
			default: break;
		}
	}
	if ( radix == 10 && !text.empty() && ( text.front() == '-' || text.front() == '+' ) ) {
		negative = text.front() == '-'; text.remove_prefix( 1 );
	}
	// Explicit port admission bound, not a claim about original long-token
	// recovery. 2^32 is the largest magnitude in BASE-TEXT-001's fixture.
	constexpr uint64_t limit = uint64_t{ 1 } << 32;
	uint64_t magnitude = 0;
	for ( const char ch : text ) {
		const unsigned digit = ch >= '0' && ch <= '9' ? ch - '0'
			: ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : 16;
		if ( digit >= radix ) { break; }
		if ( magnitude > ( limit - digit ) / radix ) { throw std::invalid_argument( "#RESOURCEINFO numeric prefix exceeds admitted magnitude" ); }
		magnitude = magnitude * radix + digit;
	}
	uint32_t bits = static_cast< uint32_t >( magnitude );
	if ( negative ) { bits = uint32_t{ 0 } - bits; }
	return static_cast< int32_t >( bits <= INT32_MAX ? int64_t( bits ) : int64_t( bits ) - ( int64_t{ 1 } << 32 ) );
}

std::array< std::string_view, 9 > ResourceRows( const std::string& source ) {
	std::array< std::string_view, 9 > rows{};
	bool found = false;
	size_t count = 0, offset = 0;
	while ( offset < source.size() ) {
		const auto end = source.find( '\n', offset );
		auto line = std::string_view( source ).substr( offset, ( end == std::string::npos ? source.size() : end ) - offset );
		offset = end == std::string::npos ? source.size() : end + 1;
		if ( end != std::string::npos && !line.empty() && line.back() == '\r' ) { line.remove_suffix( 1 ); }
		const auto trimmed = ResourceTrim( line );
		if ( trimmed == "#RESOURCEINFO" ) {
			if ( found ) { throw std::invalid_argument( "Duplicate section #RESOURCEINFO" ); }
			found = true; continue;
		}
		if ( found && count < rows.size() ) {
			if ( line.size() > 1024 || line.find_first_of( std::string_view( "\0\r", 2 ) ) != std::string_view::npos
				|| ( !trimmed.empty() && trimmed.front() == '#' ) ) {
				throw std::invalid_argument( "#RESOURCEINFO unsupported physical row or early section boundary" );
			}
			rows[ count++ ] = line;
		}
	}
	if ( !found || count != rows.size() ) { throw std::invalid_argument( "#RESOURCEINFO requires nine physical rows" ); }
	return rows;
}

}

BaseSquareRules SmacRulesTXTLoader::ParseBaseSquareRules( const std::string& source ) {
	const auto rows = ResourceRows( source );
	std::array< RuleTriple, 9 > values{};
	for ( size_t i = 0; i < rows.size(); ++i ) {
		auto row = ResourceTrim( rows[ i ] );
		// Physical blank/comment rows consume their position and store zeros.
		if ( row.empty() || row.front() == ';' ) { continue; }
		// The installed file has quotes in its ignored annotation after the
		// fourth value. Quotes in the fixed prefix remain unsupported CSV.
		size_t annotation = 0;
		for ( size_t comma = 0; comma < 5 && annotation != std::string_view::npos; ++comma ) {
			annotation = row.find( ',', annotation );
			if ( annotation != std::string_view::npos ) { ++annotation; }
		}
		if ( row.substr( 0, annotation ).find( '"' ) != std::string_view::npos ) {
			throw std::invalid_argument( "#RESOURCEINFO quoted fixed fields are unsupported" );
		}
		const auto comment = row.find( ';' );
		if ( comment != std::string_view::npos && std::count( row.begin(), row.begin() + comment, ',' ) < 4 ) {
			throw std::invalid_argument( "#RESOURCEINFO inline comment before fourth field is unsupported" );
		}
		const auto name_end = row.find( ',' );
		if ( name_end == std::string_view::npos || ResourceTrim( row.substr( 0, name_end ) ).empty() ) {
			throw std::invalid_argument( "#RESOURCEINFO nonempty row requires a label and N/M/E fields" );
		}
		row.remove_prefix( name_end + 1 );
		for ( size_t column = 0; column < 3; ++column ) {
			const auto end = row.find( ',' );
			if ( end == std::string_view::npos && column != 2 ) { throw std::invalid_argument( "#RESOURCEINFO missing N/M/E field delimiters" ); }
			values[ i ][ column ] = ResourceNumber( row.substr( 0, end ) );
			if ( end != std::string_view::npos ) { row.remove_prefix( end + 1 ); }
		}
	}
	// Positions select identity. Fourth values and annotations remain outside
	// this center-yield subset; rows beyond the nine consumed lines are ignored.
	return BaseSquareRules( values[ 1 ], values[ 2 ], values[ 4 ] );
}

game::backend::settings::SmacTechnologyRules SmacRulesTXTLoader::ParseTechnologyRules( const std::string& source ) {
	const auto rows = TXTLoader::ReadUniqueSection( source, "TECHNOLOGY" );
	game::backend::settings::SmacTechnologyRules result;
	if ( rows.size() != result.technologies.size() ) { throw std::invalid_argument( "#TECHNOLOGY requires 89 SMAC rows" ); }
	for ( size_t i = 0 ; i < rows.size() ; ++i ) {
		const auto line = rows[ i ].substr( 0, rows[ i ].find( ';' ) );
		const auto fields = util::String::Split( line, ',' );
		if ( fields.size() != 9 || std::count( line.begin(), line.end(), ',' ) != 8 ) { throw std::invalid_argument( "#TECHNOLOGY requires nine columns" ); }
		auto& row = result.technologies[ i ];
		row.source_id = static_cast< int32_t >( i ); row.name = fields[ 0 ];
		if ( fields[ 1 ].find( '\0' ) != std::string::npos ) { throw std::invalid_argument( "#TECHNOLOGY key contains NUL" ); }
		row.key = fields[ 1 ].substr( 0, 7 );
		for ( size_t field = 0 ; field < 4 ; ++field ) { row.priorities[ field ] = ParseField( fields[ field + 2 ], "row " + std::to_string( i ), field + 3, "TECHNOLOGY" ); }
		row.prerequisites = { fields[ 6 ], fields[ 7 ] };
		const auto& flags = fields[ 8 ];
		if ( flags.size() != 9 || flags.find_first_not_of( "01" ) != std::string::npos ) { throw std::invalid_argument( "#TECHNOLOGY flags require nine binary digits" ); }
		row.flags = 0;
		for ( const char bit : flags ) { row.flags = static_cast< uint16_t >( row.flags * 2 + ( bit - '0' ) ); }
	}
	result.Validate();
	return result;
}

game::backend::settings::SmacWorldSizeRules SmacRulesTXTLoader::ParseWorldSizeRules( const std::string& source ) {
	const auto rows = TXTLoader::ReadUniqueSection( source, "WORLDSIZE" );
	if ( rows.size() != 6 || ParseField( util::String::TrimCopy( rows.front().substr( 0, rows.front().find( ';' ) ) ), "count", 1, "WORLDSIZE" ) != 5 ) {
		throw std::invalid_argument( "#WORLDSIZE requires a count of five and five SMAC rows" );
	}
	game::backend::settings::SmacWorldSizeRules result;
	for ( size_t i = 0; i < result.presets.size(); ++i ) {
		const auto line = rows[ i + 1 ].substr( 0, rows[ i + 1 ].find( ';' ) );
		const auto fields = util::String::Split( line, ',' );
		if ( fields.size() != 3 || std::count( line.begin(), line.end(), ',' ) != 2 ) { throw std::invalid_argument( "#WORLDSIZE requires exactly three columns" ); }
		result.presets[ i ] = { fields[ 0 ], ParseField( fields[ 1 ], "row " + std::to_string( i ), 2, "WORLDSIZE" ), ParseField( fields[ 2 ], "row " + std::to_string( i ), 3, "WORLDSIZE" ) };
	}
	try { result.Validate(); } catch ( const std::runtime_error& error ) { throw std::invalid_argument( error.what() ); }
	return result;
}

game::backend::settings::SmacMovementRules SmacRulesTXTLoader::ParseMovementRules( const std::string& source ) {
	const auto rules = TXTLoader::ReadUniqueSection( source, "RULES" );
	if ( rules.size() != 70 ) { throw std::invalid_argument( "SMAC movement requires 70 positional RULES rows" ); }
	auto field = util::String::TrimCopy( rules.front().substr( 0, rules.front().find( ';' ) ) );
	if ( !field.empty() && field.back() == ',' ) { field.pop_back(); field = util::String::TrimCopy( field ); }
	return game::backend::settings::SmacMovementRules( ParseField( field, "row 1", 1, "RULES" ) );
}

game::backend::settings::SmacProductionRules SmacRulesTXTLoader::ParseProductionRules( const std::string& source ) {
	const auto rules = TXTLoader::ReadUniqueSection( source, "RULES" );
	const auto facilities = TXTLoader::ReadUniqueSection( source, "FACILITIES" );
	if ( rules.size() != 70 || facilities.size() != 71 ) {
		throw std::invalid_argument( "SMAC production requires 70 RULES and 71 FACILITIES rows" );
	}
	const auto rule = [ & ]( size_t position ) {
		auto field = util::String::TrimCopy( rules[ position - 1 ].substr( 0, rules[ position - 1 ].find( ';' ) ) );
		if ( !field.empty() && field.back() == ',' ) { field.pop_back(); field = util::String::TrimCopy( field ); }
		return ParseField( field, "row " + std::to_string( position ), 1, "RULES" );
	};
	game::backend::settings::SmacProductionRules result{ rule( 7 ), rule( 43 ), rule( 44 ), rule( 45 ), {} };
	for ( size_t i = 0 ; i < facilities.size() ; ++i ) {
		// The effect can contain commas; projects append additional AI metadata.
		// Consume only the fixed prefix, never infer identity from a display name.
		const auto& line = facilities[ i ];
		std::array< std::string, 5 > fields;
		size_t begin = 0;
		for ( auto& field : fields ) {
			const auto end = line.find( ',', begin );
			if ( end == std::string::npos ) { throw std::invalid_argument( "#FACILITIES row " + std::to_string( i + 1 ) + ": missing fixed prefix" ); }
			field = util::String::TrimCopy( line.substr( begin, end - begin ) ); begin = end + 1;
			if ( field.empty() || field.find( '\0' ) != std::string::npos ) {
				throw std::invalid_argument( "#FACILITIES row " + std::to_string( i + 1 ) + ": empty or invalid prefix" );
			}
		}
		const auto row = "row " + std::to_string( i + 1 );
		result.facilities[ i ] = { static_cast< int32_t >( i + 1 ), fields[ 0 ],
			ParseField( fields[ 1 ], row, 2, "FACILITIES" ), ParseField( fields[ 2 ], row, 3, "FACILITIES" ), fields[ 3 ] };
	}
	result.Validate();
	return result;
}

}
