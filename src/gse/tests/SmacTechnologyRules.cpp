#include "SmacTechnologyRules.h"

#include "fixtures/SmacRulesSource.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "loader/txt/TXTLoader.h"
#include "task/gsetests/GSETests.h"
#include "util/String.h"

#include <functional>
#include <algorithm>
#include <vector>

namespace gse::tests {
namespace {
using namespace game::backend::settings;
using Status = SmacProductionPrerequisite::Status;
namespace fixture = smac_rules_fixture;
using loader::txt::SmacRulesTXTLoader;

bool Rejects( const std::function< void() >& operation ) {
	try { operation(); } catch ( const std::exception& ) { return true; }
	return false;
}

std::string Source( const std::vector< std::string >& rows ) {
	std::string result = "## ignored comment\r\n#TECHNOLOGY\r\n";
	for ( const auto& row : rows ) { result += " ; comment\r\n " + row + " ; annotation\r\n"; }
	return result + "#END\r\n";
}

types::Buffer Wire( const SmacTechnologyRules& catalog, size_t number_index = SIZE_MAX, int64_t value = 0,
	size_t text_index = SIZE_MAX, const std::string& text = "" ) {
	types::Buffer result;
	size_t numbers = 0, texts = 0;
	const auto number = [ & ]( int64_t original ) { result.WriteInt( numbers++ == number_index ? value : original ); };
	const auto string = [ & ]( const std::string& original ) { result.WriteString( texts++ == text_index ? text : original ); };
	number( 1 ); number( 89 );
	for ( const auto& row : catalog.technologies ) {
		number( row.source_id ); string( row.name ); string( row.key );
		for ( const auto priority : row.priorities ) { number( priority ); }
		for ( const auto& prerequisite : row.prerequisites ) { string( prerequisite ); }
		number( row.flags );
	}
	return result;
}

}

void AddSmacTechnologyRulesTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC technology parser retains all positional metadata and exact reference keys", GT() {
		auto rows = loader::txt::TXTLoader::ReadUniqueSection( fixture::TechnologySource(), "TECHNOLOGY" );
		auto expected = fixture::Technologies();
		GT_ASSERT( SmacRulesTXTLoader::ParseTechnologyRules( Source( rows ) ) == expected, "signed priorities, flags or positional identities lost" );
		rows[ 0 ] = "Changed display name, E=mc2, +0001, -0, -2147483648, 2147483647, Unknown, Disable, 111111111";
		expected.technologies[ 0 ] = { 0, "Changed display name", "E=mc2", { 1, 0, INT32_MIN, INT32_MAX }, { "Unknown", "Disable" }, 511 };
		GT_ASSERT( SmacRulesTXTLoader::ParseTechnologyRules( Source( rows ) ) == expected, "text or signed metadata normalized into guessed semantics" );
		std::swap( rows[ 0 ], rows[ 88 ] );
		const auto reordered = SmacRulesTXTLoader::ParseTechnologyRules( Source( rows ) );
		GT_ASSERT( reordered.technologies[ 0 ].key == "Tech88" && reordered.technologies[ 0 ].source_id == 0
			&& reordered.technologies[ 88 ].key == "E=mc2" && reordered.technologies[ 88 ].source_id == 88, "source identity remapped by key/name" );
		GT_OK();
	} );
	task->AddTest( "SMAC technology parser rejects malformed rows numbers flags and text", GT() {
		const auto original = loader::txt::TXTLoader::ReadUniqueSection( fixture::TechnologySource(), "TECHNOLOGY" );
		for ( const auto& source : { std::string(), Source( {} ), Source( original ) + "#TECHNOLOGY\n" } ) {
			GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseTechnologyRules( source ); } ), "bad technology section accepted" );
		}
		for ( const bool extra : { false, true } ) {
			auto rows = original; if ( extra ) { rows.push_back( rows.back() ); } else { rows.pop_back(); }
			GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseTechnologyRules( Source( rows ) ); } ), "wrong technology count accepted" );
		}
		for ( size_t field = 0 ; field < 9 ; ++field ) {
			std::vector< std::string > bad = { "", std::string( "x\0y", 3 ) };
			if ( field >= 2 && field <= 5 ) { bad.insert( bad.end(), { "*", "1x", "1.0", "0x10", "+", "+-1", "2147483648", "-2147483649" } ); }
			if ( field == 8 ) { bad.insert( bad.end(), { "1", "0000000000", "000000002", "-00000001" } ); }
			for ( const auto& value : bad ) {
				auto rows = original; auto fields = util::String::Split( rows.back(), ',' ); fields[ field ] = value;
				rows.back() = util::String::Join( fields, ',' );
				GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseTechnologyRules( Source( rows ) ); } ), "malformed late field accepted" );
			}
		}
		for ( const auto& suffix : { ",", ",extra" } ) {
			auto rows = original; rows.back() += suffix;
			GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseTechnologyRules( Source( rows ) ); } ), "extra column accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC technology codec preserves every field and rejects malformed framed metadata", GT() {
		const auto expected = fixture::Technologies();
		GT_ASSERT( expected.Serialize().ToString() == Wire( expected ).ToString(), "technology schema changed" );
		GT_ASSERT( SmacTechnologyRules::Deserialize( expected.Serialize() ) == expected, "technology roundtrip lost fields" );
		const auto rejects = []( const types::Buffer& wire ) {
			try { SmacTechnologyRules::Deserialize( wire ); } catch ( const std::runtime_error& ) { return true; }
			return false;
		};
		for ( const int64_t version : { 0, 2 } ) { GT_ASSERT( rejects( Wire( expected, 0, version ) ), "unknown catalog version accepted" ); }
		for ( const int64_t count : { int64_t( -1 ), int64_t( 88 ), int64_t( 90 ), INT64_MAX } ) { GT_ASSERT( rejects( Wire( expected, 1, count ) ), "wrong catalog count accepted" ); }
		for ( size_t row = 0 ; row < 89 ; ++row ) {
			GT_ASSERT( rejects( Wire( expected, 2 + row * 6, row + 1 ) ), "out-of-order source ID accepted" );
			for ( size_t field = 1 ; field < 5 ; ++field ) {
				for ( const auto value : { int64_t( INT32_MIN ) - 1, int64_t( INT32_MAX ) + 1 } ) {
					GT_ASSERT( rejects( Wire( expected, 2 + row * 6 + field, value ) ), "out-of-range priority narrowed" );
				}
			}
			for ( const auto value : { int64_t( -1 ), int64_t( 512 ), INT64_MAX } ) {
				GT_ASSERT( rejects( Wire( expected, 2 + row * 6 + 5, value ) ), "invalid flags narrowed" );
			}
			for ( size_t field = 0 ; field < 4 ; ++field ) {
				GT_ASSERT( rejects( Wire( expected, SIZE_MAX, 0, row * 4 + field, "" ) ), "empty text accepted" );
				GT_ASSERT( rejects( Wire( expected, SIZE_MAX, 0, row * 4 + field, std::string( "a\0b", 3 ) ) ), "embedded NUL accepted" );
			}
		}
		GT_ASSERT( rejects( Wire( expected, SIZE_MAX, 0, 88 * 4 + 1, "EightKey" ) ), "overlength stored key accepted" );
		types::Buffer wrong_type; wrong_type.WriteInt( 1 ); wrong_type.WriteString( "89" );
		GT_ASSERT( rejects( wrong_type ), "wrong field type accepted" );
		auto trailing = expected.Serialize(); trailing.WriteInt( 0 );
		GT_ASSERT( rejects( trailing ), "trailing technology bytes accepted" );
		const auto bytes = expected.Serialize().ToString();
		for ( const size_t length : { size_t( 0 ), size_t( 4 ), bytes.size() / 2, bytes.size() - 1 } ) {
			GT_ASSERT( rejects( types::Buffer( bytes.substr( 0, length ) ) ), "truncated catalog accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC technology token lookup preserves seven byte keys ordered collisions and exact whitespace", GT() {
		using Kind = SmacTechnologyReference::Kind;
		auto rows = loader::txt::TXTLoader::ReadUniqueSection( fixture::TechnologySource(), "TECHNOLOGY" );
		rows[ 0 ] = "First display, SevenKeySuffix, 0,0,0,0, Tech88,None,000000000";
		rows[ 1 ] = "Second display, sevenkeCollision, 0,0,0,0, None,None,000000000";
		const auto catalog = SmacRulesTXTLoader::ParseTechnologyRules( Source( rows ) );
		GT_ASSERT( catalog.technologies[ 0 ].key == "SevenKe" && catalog.technologies[ 1 ].key == "sevenke", "loader did not retain first seven source bytes" );
		GT_ASSERT( SmacTechnologyRules::Deserialize( catalog.Serialize() ) == catalog, "colliding ordered keys did not survive codec" );
		for ( const auto* token : { "SevenKe", "sEvEnKe   " } ) {
			const auto ref = catalog.ResolveToken( token );
			GT_ASSERT( ref.kind == Kind::TECHNOLOGY_ID && ref.technology_id == 0, "lookup did not use first case-insensitive positional match" );
		}
		GT_ASSERT( catalog.ResolveToken( "nOnE   " ).kind == Kind::NONE && catalog.ResolveToken( "dIsAbLe  " ).kind == Kind::DISABLED, "case/trailing-space sentinel lookup failed" );
		GT_ASSERT( catalog.ResolveToken( "tEcH88" ).technology_id == 88, "last catalog key did not resolve" );
		for ( const auto* token : { "", "   ", "0", "88", "First display", "SevenKeySuffix", " SevenKe", "SevenKe\t", "None\t", " None" } ) {
			const auto ref = catalog.ResolveToken( token );
			GT_ASSERT( ref.kind == Kind::UNKNOWN && !ref.technology_id, "unknown token silently normalized to a reference" );
		}
		GT_ASSERT( catalog.ResolveProductionPrerequisite( "SevenKe" ).status == Status::TECHNOLOGY_ID, "forward catalog reference did not resolve" );
		GT_OK();
	} );
	task->AddTest( "SMAC production prerequisite query distinguishes definition identity without ownership defaults", GT() {
		auto catalog = fixture::Technologies();
		const auto check = [ & ]( const std::string& key, Status status, std::optional< int32_t > id ) {
			const auto result = catalog.ResolveProductionPrerequisite( key ); return result.status == status && result.technology_id == id;
		};
		GT_ASSERT( check( "None", Status::NO_REQUIREMENT, {} ) && check( "Disable", Status::DISABLED, {} ), "sentinel meaning changed" );
		GT_ASSERT( check( "misspelled", Status::UNAVAILABLE, {} ) && check( "Missing", Status::UNAVAILABLE, {} ), "unknown/case-mismatched key accepted" );
		GT_ASSERT( check( "Tech0", Status::TECHNOLOGY_ID, 0 ) && check( "Tech87", Status::TECHNOLOGY_ID, 87 )
			&& check( "Tech88", Status::UNAVAILABLE, 88 ), "production reference range changed" );
		for ( const auto& first : { "None", "Disable", "Tech88", "Missing" } ) {
			for ( const auto& second : { "None", "Disable", "Tech0", "Missing" } ) {
				catalog.technologies[ 1 ].prerequisites = { first, second };
				const bool unknown = std::string( first ) == "Missing" || std::string( second ) == "Missing";
				const bool disabled = std::string( first ) == "Disable" || ( std::string( first ) != "None" && std::string( second ) == "Disable" );
				const auto status = unknown ? Status::UNAVAILABLE : disabled ? Status::DISABLED : Status::TECHNOLOGY_ID;
				GT_ASSERT( check( "Tech1", status, 1 ), "evidenced definition exception or unknown policy changed" );
			}
		}
		catalog.technologies[ 1 ].prerequisites = { "None", "Disable" };
		GlobalSettings settings;
		settings.Deserialize( fixture::Payload( "smac", true, { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, {}, catalog.Serialize().ToString() ) );
		GT_ASSERT( settings.GetSmacFacilityPrerequisite( 1 ).status == Status::NO_REQUIREMENT
			&& settings.GetSmacFacilityPrerequisite( 38 ).status == Status::NO_REQUIREMENT
			&& settings.GetSmacFacilityPrerequisite( 71 ).status == Status::DISABLED, "real frozen facility query lost prerequisite identity" );
		const auto known = settings.GetSmacFacilityPrerequisite( 3 );
		GT_ASSERT( known.status == Status::TECHNOLOGY_ID && known.technology_id == 2, "facility prerequisite did not resolve against host catalog" );
		for ( const int32_t invalid : { INT32_MIN, 0, 72, INT32_MAX } ) { GT_ASSERT( settings.GetSmacFacilityPrerequisite( invalid ).status == Status::UNAVAILABLE, "unknown facility slot accepted" ); }
		settings.ResetRuleset(); settings.SelectRuleset( "smax" );
		GT_ASSERT( settings.GetSmacFacilityPrerequisite( 1 ).status == Status::UNAVAILABLE, "absent SMAC catalog supplied a default" );
		GT_OK();
	} );
	task->AddTest( "SMAC locked technology catalog compares all fields and preserves settings on rejection", GT() {
		GlobalSettings settings; settings.Deserialize( fixture::Payload( "smac", true ) );
		const auto before = settings.Serialize().ToString();
		const auto reject = [ & ]( const SmacTechnologyRules& changed ) {
			return Rejects( [ & ]() { settings.Deserialize( fixture::Payload( "smac", true, { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, {}, changed.Serialize().ToString() ) ); } )
				&& settings.Serialize().ToString() == before;
		};
		for ( size_t i = 0 ; i < 89 ; ++i ) {
			for ( size_t field = 0 ; field < 4 ; ++field ) { auto changed = fixture::Technologies(); auto& value = changed.technologies[ i ].priorities[ field ]; value = value == 42 ? 43 : 42; GT_ASSERT( reject( changed ), "locked priority changed" ); }
			for ( const auto member : { &SmacTechnologyRule::name, &SmacTechnologyRule::key } ) { auto changed = fixture::Technologies(); changed.technologies[ i ].*member = "Changed"; GT_ASSERT( reject( changed ), "locked identity text changed" ); }
			for ( size_t field = 0 ; field < 2 ; ++field ) { auto changed = fixture::Technologies(); changed.technologies[ i ].prerequisites[ field ] = "Disable"; GT_ASSERT( reject( changed ), "locked prerequisite changed" ); }
			auto changed = fixture::Technologies(); changed.technologies[ i ].flags ^= 1; GT_ASSERT( reject( changed ), "locked flags changed" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC technology late parse and decode failures leave every prior setting intact", GT() {
		Settings settings; settings.global.SelectRuleset( "smac" );
		settings.global.game_name = "preserved"; settings.global.map.filename = "preserved path"; settings.global.map.size_x = 92; settings.global.difficulty_level = -17;
		const auto map_before = settings.global.map.Serialize().ToString();
		auto source = fixture::Source( "alpha.txt" );
		source.replace( source.find( "Tech88" ), std::string( "Tech88" ).size(), "" );
		GT_ASSERT( Rejects( [ & ]() { settings.PrepareRuleset( [ & ]( const std::string& ) { return source; } ); } ), "late malformed technology accepted" );
		GT_ASSERT( !settings.global.IsRulesetLocked() && !settings.global.GetSmacResourceRules() && !settings.global.GetSmacProductionRules()
			&& !settings.global.GetSmacTechnologyRules(), "late technology failure published earlier profiles" );
		GT_ASSERT( settings.global.GetRulesetId() == "smac" && settings.global.game_name == "preserved" && settings.global.map.filename == "preserved path"
			&& settings.global.difficulty_level == -17 && settings.global.map.Serialize().ToString() == map_before, "failed read changed unrelated settings" );
		settings.PrepareRuleset( fixture::Source );
		const auto before = settings.global.Serialize().ToString();
		const auto catalog = fixture::Technologies().Serialize().ToString();
		for ( const auto& bytes : { std::string(), catalog.substr( 0, catalog.size() - 1 ) } ) {
			GT_ASSERT( Rejects( [ & ]() { settings.global.Deserialize( fixture::Payload( "smac", true, { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, {}, bytes ) ); } ), "invalid late catalog accepted" );
			GT_ASSERT( settings.global.Serialize().ToString() == before && settings.global.map.filename == "preserved path", "late catalog decode partially published settings" );
		}
		GT_OK();
	} );
}

}
