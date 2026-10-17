#include "SmacRulesTXTLoader.h"

#include "loader/txt/SmacRulesTXTLoader.h"
#include "task/gsetests/GSETests.h"
#include "fixtures/SmacResourceText.h"
#include "fixtures/SmacRulesSource.h"

#include <limits>
#include <stdexcept>
#include <vector>

namespace gse::tests {
using loader::txt::SmacRulesTXTLoader;
using namespace game::backend::resource::smac;

static std::vector< std::string > Rows() {
	return { "Ocean,*,*,*", "Base,17,-3,0,unused", "Bonus,4,5,6,unused,annotation,with commas",
		"Forest,*,*,*", "Recycling,7,8,9,unused", "Land,*,*,*", "Sea,*,*,*", "Monolith,*,*,*", "Borehole,*,*,*" };
}

static std::string Source( const std::vector< std::string >& rows, const std::string& newline = "\n" ) {
	// Comments outside RESOURCEINFO are not part of its physical nine-row loop.
	std::string source = "#OTHER" + newline + "; comment" + newline + "## translator" + newline + "  #RESOURCEINFO \t" + newline;
	for ( const auto& row : rows ) { source += row + newline; }
	return source + "#END" + newline;
}

static bool Rejects( const std::string& source ) {
	try { SmacRulesTXTLoader::ParseBaseSquareRules( source ); }
	catch ( const std::invalid_argument& ) { return true; }
	return false;
}

static bool Same( const BaseSquareRules& rules, const std::array< RuleTriple, 3 >& expected ) {
	return rules.base_square == expected[ 0 ] && rules.bonus_square == expected[ 1 ] && rules.recycling == expected[ 2 ];
}

static Yield Calculate( const BaseSquareRules& rules, const size_t query ) {
	return CalculateBaseSquareYield( rules, {
		CenterTerrain( true, static_cast< ResourceBonus >( query / 4 ), { false, false, false }, false ),
		CenterFacilities( false, ( query & 1 ) != 0, ( query & 2 ) != 0 ),
		CenterFaction( 0, false, 2, false, false ),
		CenterEvents( { false, false, false }, { false, false, false }, false, false )
	} );
}

void AddSmacRulesTXTLoaderTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC RESOURCEINFO literal original text produces 592 center yields", GT() {
		size_t queries = 0;
		for ( const auto& row : resource_text_fixture::cases ) {
			GT_ASSERT( !Rejects( row.text ), "measured text rejected: " + std::string( row.id ) );
			const auto rules = SmacRulesTXTLoader::ParseBaseSquareRules( row.text );
			GT_ASSERT( Same( rules, row.rules ), "literal resource rules differ: " + std::string( row.id ) );
			for ( size_t query = 0; query < row.yields.size(); ++query ) {
				GT_ASSERT( Calculate( rules, query ) == row.yields[ query ], "literal center yield differs: " + std::string( row.id ) + "/" + std::to_string( query ) );
				++queries;
			}
		}
		GT_ASSERT( queries == 592, "owned text fixture projection changed" );
		GT_OK();
	} );
	task->AddTest( "SMAC RESOURCEINFO physical blanks comments and extra rows preserve positions", GT() {
		for ( const auto* newline : { "\n", "\r\n" } ) {
			auto rows = Rows(); rows[ 1 ] = " \t"; rows[ 2 ] = "; 999,999,999,999";
			const std::array< RuleTriple, 3 > expected = { RuleTriple{ 0, 0, 0 }, RuleTriple{ 0, 0, 0 }, RuleTriple{ 7, 8, 9 } };
			rows.push_back( "later,99,99,99" );
			GT_ASSERT( Same( SmacRulesTXTLoader::ParseBaseSquareRules( Source( rows, newline ) ), expected ), "physical lines were skipped or extra row was consumed" );
		}
		auto source = Source( Rows() ); source.erase( source.find( "#END" ) ); source.pop_back();
		GT_ASSERT( !Rejects( source ), "complete ninth row without final newline rejected by port policy" );
		auto annotated = Rows(); annotated[ 1 ] = "Owned label,17,-3,0,unused, \"owned, annotation\"";
		GT_ASSERT( !Rejects( Source( annotated ) ), "ignored quoted annotation rejected" );
		GT_ASSERT( SmacRulesTXTLoader::ParseBaseSquareRules( Source( annotated ) ).base_square == SmacRulesTXTLoader::ParseBaseSquareRules( Source( Rows() ) ).base_square, "ignored annotation changed rules" );
		GT_OK();
	} );
	task->AddTest( "SMAC RESOURCEINFO labels signs and row movement remain positional", GT() {
		auto rows = Rows(); rows[ 1 ] = "Renamed,-2147483648,+2147483647,-0";
		rows[ 2 ] = "Renamed,+00017,-00003,+0";
		const std::array< RuleTriple, 3 > endpoints = { RuleTriple{ INT32_MIN, INT32_MAX, 0 }, RuleTriple{ 17, -3, 0 }, RuleTriple{ 7, 8, 9 } };
		GT_ASSERT( Same( SmacRulesTXTLoader::ParseBaseSquareRules( Source( rows ) ), endpoints ), "labels or signs changed numeric fields" );
		std::swap( rows[ 1 ], rows[ 4 ] );
		const std::array< RuleTriple, 3 > moved = { endpoints[ 2 ], endpoints[ 1 ], endpoints[ 0 ] };
		GT_ASSERT( Same( SmacRulesTXTLoader::ParseBaseSquareRules( Source( rows ) ), moved ), "row move was remapped by label" );
		GT_OK();
	} );
	task->AddTest( "SMAC RESOURCEINFO rejects unsupported framing without recovery", GT() {
		for ( const auto& source : { std::string(), std::string( "#RESOURCEINFO_OTHER\n" ), std::string( "##RESOURCEINFO\n" ), Source( Rows() ) + "#RESOURCEINFO\n" } ) {
			GT_ASSERT( Rejects( source ), "missing or duplicate section accepted" );
		}
		for ( size_t count = 0; count < 9; ++count ) {
			auto rows = Rows(); rows.resize( count );
			GT_ASSERT( Rejects( Source( rows ) ), "early section accepted" );
			auto source = Source( rows ); source.erase( source.find( "#END" ) );
			GT_ASSERT( Rejects( source ), "early EOF accepted" );
		}
		for ( size_t index = 0; index < 9; ++index ) {
			for ( const auto& bad : { std::string( "No delimiters" ), std::string( ",1,2,3" ), std::string( "Row,1,2" ),
				std::string( "\"quoted,name\",1,2,3" ), std::string( "Row,1\r2,2,3" ), std::string( "Row,1\0x,2,3", 12 ), std::string( 1025, 'x' ),
				std::string( "Row;comment,1,2,3" ), std::string( "Row,1;comment,2,3" ), std::string( "Row,1,2;comment,3" ), std::string( "Row,1,2,3;comment" ) } ) {
				auto rows = Rows(); rows[ index ] = bad;
				GT_ASSERT( Rejects( Source( rows ) ), "unsupported physical row admitted" );
			}
		}
		auto rows = Rows(); rows[ 1 ] = std::string( 1018, 'L' ) + ",1,2,3";
		GT_ASSERT( !Rejects( Source( rows ) ), "exact port row cap rejected" );
		rows[ 1 ].insert( 0, "L" ); GT_ASSERT( Rejects( Source( rows ) ), "cap+1 row accepted" );
		auto terminal_cr = Source( Rows() ); terminal_cr.erase( terminal_cr.find( "#END" ) ); terminal_cr.back() = '\r';
		GT_ASSERT( Rejects( terminal_cr ), "bare CR at EOF was treated as CRLF" );
		GT_OK();
	} );
	task->AddTest( "SMAC RESOURCEINFO numeric prefix bound rejects beyond measured magnitude", GT() {
		for ( const auto* token : { "4294967297", "-4294967297", "0x100000001", "0b100000000000000000000000000000001", "999999999999999999999999999999" } ) {
			for ( const auto index : { 0, 1, 2, 4, 8 } ) {
				auto rows = Rows(); rows[ index ] = std::string( "Row," ) + token + ",1,2";
				GT_ASSERT( Rejects( Source( rows ) ), "unsupported magnitude accepted" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC RESOURCEINFO host freeze and restored rules use physical text yields", GT() {
		for ( const auto& row : resource_text_fixture::cases ) {
			if ( std::string( row.id ) != "comments_and_blank_lines" && std::string( row.id ) != "token_13" ) { continue; }
			game::backend::settings::Settings host; host.global.SelectRuleset( "smac" );
			host.PrepareRuleset( [ & ]( const std::string& filename ) {
				auto source = smac_rules_fixture::Source( filename );
				if ( filename == "alpha.txt" ) { source.erase( source.find( "#RESOURCEINFO" ) ); source += row.text; }
				return source;
			} );
			game::backend::settings::GlobalSettings restored; restored.Deserialize( host.global.Serialize() );
			GT_ASSERT( host.global.IsRulesetLocked() && restored.IsRulesetLocked() && restored.GetSmacBaseSquareRules(), "host rules were not frozen and restored" );
			const auto& rules = *restored.GetSmacBaseSquareRules();
			GT_ASSERT( Same( rules, row.rules ), "host transport changed resource projection" );
			GT_ASSERT( *host.global.GetSmacMovementRules() == smac_rules_fixture::Movement()
				&& *host.global.GetSmacProductionRules() == smac_rules_fixture::Production()
				&& *host.global.GetSmacTechnologyRules() == smac_rules_fixture::Technologies(), "RESOURCEINFO parsing changed other profiles" );
			for ( size_t query = 0; query < 16; ++query ) { GT_ASSERT( Calculate( rules, query ) == row.yields[ query ], "restored center yield changed" ); }
		}
		GT_OK();
	} );
}
}
