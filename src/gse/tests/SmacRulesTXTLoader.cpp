#include "SmacRulesTXTLoader.h"

#include "loader/txt/SmacRulesTXTLoader.h"
#include "task/gsetests/GSETests.h"

#include <limits>
#include <stdexcept>
#include <vector>

namespace gse::tests {

using loader::txt::SmacRulesTXTLoader;
using namespace game::backend::resource::smac;

// Synthetic rules, deliberately different from installed defaults. Unused
// rows and the fourth column are outside this parser's numeric contract.
static std::vector< std::string > Rows() {
	return {
		"Ocean Square, *, *, *, unused",
		"Base Square, 17, -3, 0, unused",
		"Bonus Square, 4, 5, 6, unused, an annotation, with commas",
		"Forest Square, *, *, *, unused",
		"Recycling Tanks, 7, 8, 9, unused",
		"Improved Land, *, *, *, unused",
		"Improved Sea, *, *, *, unused",
		"Monolith, *, *, *, unused",
		"Borehole Square, *, *, *, unused"
	};
}

static std::string Source( const std::vector< std::string >& rows, const std::string& newline = "\n" ) {
	std::string source = "#FACILITIES" + newline + "Recycling Tanks, 777, 888, 999" + newline
		+ "##" + newline + "## ignored translator example" + newline + "##" + newline
		+ "  #RESOURCEINFO \t" + newline;
	for ( const auto& row : rows ) {
		source += " ; a comment between records" + newline + "\t" + newline + " " + row + "\t" + newline;
	}
	return source + "#END" + newline + "#FACILITIES" + newline + "Recycling Tanks, 999, 888, 777" + newline;
}

static std::string Rejection( const std::string& source ) {
	try { SmacRulesTXTLoader::ParseBaseSquareRules( source ); }
	catch ( const std::invalid_argument& error ) { return error.what(); }
	return "";
}

void AddSmacRulesTXTLoaderTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC RESOURCEINFO parses only required positional triples", GT() {
		for ( const auto* newline : { "\n", "\r\n" } ) {
			const auto rules = SmacRulesTXTLoader::ParseBaseSquareRules( Source( Rows(), newline ) );
			const RuleTriple base = { 17, -3, 0 }, bonus = { 4, 5, 6 }, recycling = { 7, 8, 9 };
			GT_ASSERT( rules.base_square == base && rules.bonus_square == bonus && rules.recycling == recycling, "rules came from defaults, another section, or the wrong columns" );
			const BaseSquareInputs inputs(
				CenterTerrain( true, ResourceBonus::None, { false, false, false }, false ),
				CenterFacilities( false, true, false ),
				CenterFaction( 0, false, 1, false, false ),
				CenterEvents( { false, false, false }, { false, false, false }, false, false )
			);
			const Yield expected = { 24, 5, 9 };
			GT_ASSERT( CalculateBaseSquareYield( rules, inputs ) == expected, "calculator did not use parsed custom rules" );
		}
		// The subset consumes three mandatory values, not the unused fourth.
		auto rows = Rows();
		rows[ 1 ] = "Base Square, 1, 2, 3";
		std::string source = "#RESOURCEINFO\n";
		for ( const auto& row : rows ) { source += row + "\n"; }
		source.pop_back(); // EOF also ends the section, without a newline or #END.
		const RuleTriple expected = { 1, 2, 3 };
		GT_ASSERT( SmacRulesTXTLoader::ParseBaseSquareRules( source ).base_square == expected, "valid end-of-file section rejected" );
		GT_OK();
	} );
	task->AddTest( "SMAC RESOURCEINFO preserves signed int32 endpoints and explicit signs", GT() {
		auto rows = Rows();
		rows[ 1 ] = "Base Square, -2147483648, +2147483647, -0, 0";
		rows[ 2 ] = "Bonus Square, +00017, -00003, +0, 0";
		rows[ 4 ] = "Recycling Tanks, \t0\t, 2147483647, -2147483648, 0";
		const auto rules = SmacRulesTXTLoader::ParseBaseSquareRules( Source( rows ) );
		const auto minimum = std::numeric_limits< int32_t >::min(), maximum = std::numeric_limits< int32_t >::max();
		const RuleTriple base = { minimum, maximum, 0 }, bonus = { 17, -3, 0 }, recycling = { 0, maximum, minimum };
		GT_ASSERT( rules.base_square == base && rules.bonus_square == bonus && rules.recycling == recycling, "signed rules were clamped, wrapped, or defaulted" );
		GT_OK();
	} );
	task->AddTest( "SMAC RESOURCEINFO rejects missing or duplicated requested sections", GT() {
		for ( const auto& source : {
			std::string(), std::string( "#RESOURCEINFO_OTHER\n" ), std::string( "##RESOURCEINFO\n" ),
			Source( Rows() ) + "#RESOURCEINFO\n", std::string( "#RESOURCEINFO\n#END\n" ) + Source( Rows() )
		} ) {
			GT_ASSERT( !Rejection( source ).empty(), "missing or duplicated RESOURCEINFO was accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC RESOURCEINFO rejects truncated, duplicated, or reordered layout", GT() {
		for ( size_t i = 0 ; i < Rows().size() ; ++i ) {
			auto rows = Rows();
			rows.erase( rows.begin() + i );
			GT_ASSERT( !Rejection( Source( rows ) ).empty(), "missing table row was accepted" );
			rows = Rows();
			rows.insert( rows.begin() + i, rows[ i ] );
			GT_ASSERT( !Rejection( Source( rows ) ).empty(), "additional table row was accepted" );
			rows = Rows();
			rows[ i ] = rows[ ( i + 1 ) % rows.size() ];
			GT_ASSERT( !Rejection( Source( rows ) ).empty(), "duplicated label was accepted" );
			rows = Rows();
			std::swap( rows[ i ], rows[ ( i + 1 ) % rows.size() ] );
			GT_ASSERT( !Rejection( Source( rows ) ).empty(), "reordered rows were silently remapped" );
		}
		auto source = Source( Rows() );
		source.insert( source.find( "Forest Square" ), "#END\n" );
		GT_ASSERT( !Rejection( source ).empty(), "parser read required rows past section end" );
		GT_OK();
	} );
	task->AddTest( "SMAC RESOURCEINFO requires all three numeric columns in every selected row", GT() {
		for ( const auto index : { 1, 2, 4 } ) {
			for ( const auto* suffix : { "", ",", ", 1", ", 1,", ", 1, 2", ", 1, 2," } ) {
				auto rows = Rows();
				rows[ index ] = rows[ index ].substr( 0, rows[ index ].find( ',' ) ) + suffix;
				GT_ASSERT( !Rejection( Source( rows ) ).empty(), "missing N/M/E value was accepted" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC RESOURCEINFO rejects malformed and out-of-range numeric fields", GT() {
		const std::vector< std::string > invalid = {
			"", " ", "*", "1junk", "1.0", "0x10", "1 2", "+", "-", "+-1", "++1", "--1",
			"2147483648", "+2147483648", "-2147483649", "999999999999999999999999999999", std::string( "1\0x", 3 )
		};
		for ( const auto index : { 1, 2, 4 } ) {
			for ( size_t column = 0 ; column < 3 ; ++column ) {
				for ( const auto& field : invalid ) {
					auto rows = Rows();
					const auto label = rows[ index ].substr( 0, rows[ index ].find( ',' ) );
					rows[ index ] = label;
					for ( size_t c = 0 ; c < 3 ; ++c ) { rows[ index ] += ", " + ( c == column ? field : "1" ); }
					rows[ index ] += ", 0";
					const auto error = Rejection( Source( rows ) );
					GT_ASSERT( !error.empty(), "malformed numeric field was accepted: " + field );
					GT_ASSERT( error.find( label ) != std::string::npos && error.find( "column " + std::to_string( column + 1 ) ) != std::string::npos, "numeric error did not identify its row and column" );
				}
			}
		}
		GT_OK();
	} );
}

}
