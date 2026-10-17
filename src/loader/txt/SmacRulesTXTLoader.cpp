#include "SmacRulesTXTLoader.h"

#include "TXTLoader.h"
#include "util/String.h"

#include <array>
#include <algorithm>
#include <charconv>
#include <stdexcept>

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

game::backend::settings::SmacSpecialistRules SmacRulesTXTLoader::ParseSpecialistRules( const std::string& source ) {
	using game::backend::settings::SmacSpecialistRules;
	const auto rules = TXTLoader::ReadUniqueSection( source, "RULES" );
	const auto rows = TXTLoader::ReadUniqueSection( source, "CITIZENS" );
	if ( rules.size() != 70 || rows.size() != 10 ) { throw std::invalid_argument( "SMAC specialists require 70 RULES and ten CITIZENS rows" ); }
	const auto scalar_line = util::String::TrimCopy( rules[ 10 ].substr( 0, rules[ 10 ].find( ';' ) ) );
	const auto scalar = util::String::Split( scalar_line, ',' );
	if ( scalar.size() != 1 || std::count( scalar_line.begin(), scalar_line.end(), ',' ) > 1 ) { throw std::invalid_argument( "RULES row 11 requires one specialist size field" ); }
	SmacSpecialistRules result;
	result.minimum_base_size = ParseField( scalar[ 0 ], "row 11", 1, "RULES" );
	for ( size_t i = 0; i < rows.size(); ++i ) {
		const auto line = rows[ i ].substr( 0, rows[ i ].find( ';' ) );
		if ( line.find('"') != std::string::npos ) { throw std::invalid_argument( "Quoted CITIZENS fields are unsupported" ); }
		const auto fields = util::String::Split( line, ',' );
		const auto commas = std::count( line.begin(), line.end(), ',' );
		if ( i >= result.specialists.size() ) {
			// Ordinary citizen names establish the supported section shape only.
			if ( fields.size() != 2 || commas < 1 || commas > 2 ) { throw std::invalid_argument( "CITIZENS ordinary rows require two names" ); }
			for ( const auto& field : fields ) {
				if ( field.empty() || field.size() > SmacSpecialistRules::MAX_TEXT_BYTES || field.find( '\0' ) != std::string::npos ) { throw std::invalid_argument( "Invalid ordinary citizen name" ); }
			}
			continue;
		}
		if ( fields.size() != 8 || commas != 7 ) { throw std::invalid_argument( "CITIZENS specialist rows require eight columns" ); }
		auto& row = result.specialists[ i ];
		row.source_id = static_cast< int32_t >( i ); row.singular = fields[ 0 ]; row.plural = fields[ 1 ];
		row.prerequisite = fields[ 2 ]; row.obsolete = fields[ 3 ];
		row.economy = ParseField( fields[ 4 ], "specialist", 5, "CITIZENS" );
		row.psych = ParseField( fields[ 5 ], "specialist", 6, "CITIZENS" );
		row.labs = ParseField( fields[ 6 ], "specialist", 7, "CITIZENS" ); row.trailing_token = fields[ 7 ];
	}
	result.Validate(); return result;
}

BaseSquareRules SmacRulesTXTLoader::ParseBaseSquareRules( const std::string& source ) {
	const auto rows = TXTLoader::ReadUniqueSection( source, "RESOURCEINFO" );
	// RE-SMAC-BASE-ROWS-001: positions select the stored rule rows. Labels
	// can be renamed or repeated; moving a complete row changes its meaning.
	if ( rows.size() != 9 ) {
		throw std::invalid_argument( "#RESOURCEINFO requires nine positional SMAC rows" );
	}
	std::array< RuleTriple, 3 > triples;
	size_t target = 0;
	for ( size_t i = 0 ; i < rows.size() ; ++i ) {
		const auto fields = util::String::Split( rows[ i ], ',' );
		if ( fields.empty() || fields.front().empty() ) {
			throw std::invalid_argument( "#RESOURCEINFO row " + std::to_string( i + 1 ) + ": missing label" );
		}
		if ( i == 1 || i == 2 || i == 4 ) {
			if ( fields.size() < 4 ) {
				throw std::invalid_argument( "#RESOURCEINFO " + fields.front() + ": missing N/M/E columns" );
			}
			for ( size_t column = 0 ; column < 3 ; ++column ) {
				triples[ target ][ column ] = ParseField( fields[ column + 1 ], fields.front(), column + 1 );
			}
			++target;
		}
	}
	return BaseSquareRules( triples[ 0 ], triples[ 1 ], triples[ 2 ] );
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
