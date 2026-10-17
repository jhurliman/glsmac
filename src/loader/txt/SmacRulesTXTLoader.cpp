#include "SmacRulesTXTLoader.h"

#include "TXTLoader.h"
#include "util/String.h"

#include <array>
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
