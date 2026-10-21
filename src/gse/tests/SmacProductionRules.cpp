#include "SmacProductionRules.h"

#include "fixtures/SmacRulesSource.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "loader/txt/TXTLoader.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <limits>
#include <vector>

namespace gse::tests {
namespace {
using namespace game::backend::settings;
namespace fixture = smac_rules_fixture;
using loader::txt::SmacRulesTXTLoader;

bool Rejects( const std::function< void() >& operation ) {
	try { operation(); } catch ( const std::exception& ) { return true; }
	return false;
}

std::string Source( const std::vector< std::string >& rules, const std::vector< std::string >& facilities ) {
	std::string result = "## translator comment\n#RULES\n";
	for ( const auto& row : rules ) { result += " ; full-line comment\n " + row + "\n"; }
	result += "#FACILITIES\n";
	for ( const auto& row : facilities ) { result += row + "\n"; }
	return result;
}

// Independent frame writer allows malformed metadata without bypassing the
// production serializer's validation, and exercises every integer/text field.
types::Buffer Wire( const SmacProductionRules& rules, const size_t integer_index = SIZE_MAX, const int64_t integer = 0,
	const size_t text_index = SIZE_MAX, const std::string& text = "" ) {
	types::Buffer result;
	size_t integers = 0, strings = 0;
	const auto number = [ & ]( int64_t value ) { result.WriteInt( integers++ == integer_index ? integer : value ); };
	const auto string = [ & ]( const std::string& value ) { result.WriteString( strings++ == text_index ? text : value ); };
	number( 1 ); number( rules.mineral_multiplier ); number( rules.retool_loss_percent ); number( rules.retool_strictness ); number( rules.retool_exemption );
	number( rules.facilities.size() );
	for ( const auto& row : rules.facilities ) {
		number( row.source_id ); string( row.name ); number( row.cost ); number( row.maintenance ); string( row.prerequisite );
	}
	return result;
}

}

void AddSmacProductionRulesTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC production parser preserves positional signed metadata without buildability defaults", GT() {
		auto expected = fixture::Production(); expected.prototype_percentages=std::array<int32_t,3>{{0,0,0}};
		auto rules = loader::txt::TXTLoader::ReadUniqueSection( fixture::ProductionSource(), "RULES" );
		auto facilities = loader::txt::TXTLoader::ReadUniqueSection( fixture::ProductionSource(), "FACILITIES" );
		GT_ASSERT( SmacRulesTXTLoader::ParseProductionRules( Source( rules, facilities ) ) == expected, "parsed fields differ from source/positions" );
		// Unused rules and suffixes are not interpreted; comments do not supply IDs.
		rules[ 0 ] = "unused metadata"; rules[ 6 ] = "+000123, ; deliberately misleading retool annotation";
		facilities[ 0 ] = "Repeated display name, -2147483648, -0, None, ignored, text, containing, commas";
		facilities[ 70 ] += " ; arbitrary AI/effect suffix";
		auto source = Source( rules, facilities );
		for ( size_t at = 0 ; ( at = source.find( '\n', at ) ) != std::string::npos ; at += 2 ) { source.insert( at, "\r" ); }
		GT_ASSERT( SmacRulesTXTLoader::ParseProductionRules( source ) == expected, "signs/CRLF/comments changed metadata" );
		std::swap( facilities[ 0 ], facilities[ 70 ] );
		const auto renamed = SmacRulesTXTLoader::ParseProductionRules( Source( rules, facilities ) );
		GT_ASSERT( renamed.facilities[ 0 ].source_id == 1 && renamed.facilities[ 0 ].cost == INT32_MAX
			&& renamed.facilities[ 70 ].source_id == 71 && renamed.facilities[ 70 ].cost == INT32_MIN, "source positions were remapped by name" );
		GT_ASSERT( expected.facilities[ 1 ].prerequisite == "UnknownTech1" && expected.retool_strictness == INT32_MAX
			&& expected.retool_loss_percent < 0, "ingestion silently imposed gameplay domains" );
		GT_OK();
	} );
	task->AddTest( "SMAC production parser rejects incomplete sections prefix and every malformed numeric field", GT() {
		const auto original_rules = loader::txt::TXTLoader::ReadUniqueSection( fixture::ProductionSource(), "RULES" );
		const auto original_facilities = loader::txt::TXTLoader::ReadUniqueSection( fixture::ProductionSource(), "FACILITIES" );
		for ( const auto& source : { std::string(), std::string( "#RULES\n#FACILITIES\n" ), fixture::ProductionSource() + "#RULES\n",
			fixture::ProductionSource() + "#FACILITIES\n", Source( original_rules, {} ), Source( {}, original_facilities ) } ) {
			GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseProductionRules( source ); } ), "invalid requested section accepted" );
		}
		for ( const bool extra : { false, true } ) {
			auto rules = original_rules; auto facilities = original_facilities;
			if ( extra ) { rules.push_back( rules.back() ); facilities.push_back( facilities.back() ); }
			else { rules.pop_back(); facilities.pop_back(); }
			GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseProductionRules( Source( rules, original_facilities ) ); } ), "wrong RULES count accepted" );
			GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseProductionRules( Source( original_rules, facilities ) ); } ), "wrong FACILITIES count accepted" );
		}
		for ( const auto& invalid : { std::string(), std::string( "*" ), std::string( "1x" ), std::string( "1.0" ), std::string( "0x10" ),
			std::string( "+" ), std::string( "+-1" ), std::string( "2147483648" ), std::string( "-2147483649" ), std::string( "1\0x", 3 ) } ) {
			for ( const size_t position : { 6, 14, 15, 16, 42, 43, 44 } ) {
				auto rules = original_rules; rules[ position ] = invalid + ", ; annotation";
				GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseProductionRules( Source( rules, original_facilities ) ); } ), "bad selected RULES integer accepted" );
			}
			for ( const size_t position : { 0, 70 } ) {
				for ( const bool cost : { false, true } ) {
					auto facilities = original_facilities;
					facilities[ position ] = "Name, " + ( cost ? invalid : "1" ) + ", " + ( cost ? "1" : invalid ) + ", None, Disable, effect";
					GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseProductionRules( Source( original_rules, facilities ) ); } ), "bad facility integer accepted" );
				}
			}
		}
		for ( const auto& invalid : { std::string( "Name,1,2,None,Disable" ), std::string( ",1,2,None,Disable,effect" ),
			std::string( "Name,1,2,,Disable,effect" ), std::string( "Name,1,2,None,,effect" ), std::string( "Na\0me", 5 ) + ",1,2,None,Disable,effect" } ) {
			auto facilities = original_facilities; facilities.back() = invalid;
			GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseProductionRules( Source( original_rules, facilities ) ); } ), "bad late fixed prefix accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC production profile codec rejects malformed framing and preserves all metadata", GT() {
		const auto expected = fixture::LegacyProduction();
		GT_ASSERT( expected.Serialize().ToString() == Wire( expected ).ToString(), "production frame schema changed" );
		GT_ASSERT( SmacProductionRules::Deserialize( expected.Serialize() ) == expected, "profile roundtrip lost signed/text fields" );
		for ( const int32_t endpoint : { INT32_MIN, INT32_MAX } ) {
			auto boundary = expected;
			boundary.mineral_multiplier = boundary.retool_loss_percent = boundary.retool_strictness = boundary.retool_exemption = endpoint;
			for ( auto& row : boundary.facilities ) { row.cost = row.maintenance = endpoint; }
			GT_ASSERT( SmacProductionRules::Deserialize( boundary.Serialize() ) == boundary, "signed endpoint narrowed/clamped in a numeric field" );
		}
		std::vector< types::Buffer > invalid;
		types::Buffer wrong_tag; wrong_tag.WriteBool( true ); invalid.push_back( wrong_tag );
		types::Buffer wrong_number; wrong_number.WriteInt( 1 ); wrong_number.WriteString( "123" ); invalid.push_back( wrong_number );
		for ( const auto value : { int64_t( 0 ), int64_t( 2 ), INT64_MAX } ) { invalid.push_back( Wire( expected, 0, value ) ); }
		for ( const auto value : { int64_t( -1 ), int64_t( 0 ), int64_t( 70 ), int64_t( 72 ), INT64_MAX } ) { invalid.push_back( Wire( expected, 5, value ) ); }
		for ( size_t field = 1 ; field < 6 + 71 * 3 ; ++field ) {
			for ( const auto value : { int64_t( INT32_MIN ) - 1, int64_t( INT32_MAX ) + 1, INT64_MIN, INT64_MAX } ) { invalid.push_back( Wire( expected, field, value ) ); }
		}
		for ( size_t row = 0 ; row < 71 ; ++row ) { invalid.push_back( Wire( expected, 6 + row * 3, row ) ); }
		for ( size_t field = 0 ; field < 142 ; ++field ) {
			invalid.push_back( Wire( expected, SIZE_MAX, 0, field, "" ) );
			invalid.push_back( Wire( expected, SIZE_MAX, 0, field, std::string( "x\0y", 3 ) ) );
		}
		auto trailing = expected.Serialize(); trailing.WriteInt( 99 ); invalid.push_back( trailing );
		const auto bytes = expected.Serialize().ToString();
		for ( const size_t length : { size_t( 0 ), size_t( 1 ), size_t( 40 ), bytes.size() / 2, bytes.size() - 1 } ) { invalid.emplace_back( bytes.substr( 0, length ) ); }
		for ( const auto& wire : invalid ) {
			bool rejected = false;
			try { SmacProductionRules::Deserialize( wire ); }
			catch ( const std::runtime_error& ) { rejected = true; } // actual Client protocol catch
			GT_ASSERT( rejected, "malformed profile did not use the protocol rejection path" );
		}
		GT_OK();
	} );
	task->AddTest("SMAC production prototype percentages retain source order and versioned absence",GT(){
        auto source=loader::txt::TXTLoader::ReadUniqueSection(fixture::ProductionSource(),"RULES");
        const auto facilities=loader::txt::TXTLoader::ReadUniqueSection(fixture::ProductionSource(),"FACILITIES");
        source[14]="20";source[15]="70";source[16]="110";
        auto expected=fixture::Production();expected.prototype_percentages=std::array<int32_t,3>{{20,70,110}};
        GT_ASSERT(SmacRulesTXTLoader::ParseProductionRules(Source(source,facilities))==expected,"Prototype domain order changed");
        GT_ASSERT(SmacProductionRules::Deserialize(expected.Serialize())==expected,"V2 lost domain percentages");
        auto legacy=SmacProductionRules::Deserialize(Wire(fixture::LegacyProduction()));
        GT_ASSERT(!legacy.prototype_percentages && legacy.Serialize().ToString()==Wire(fixture::LegacyProduction()).ToString(),"V1 gained invented percentages or changed bytes");
        const auto bytes=expected.Serialize().ToString();const auto legacy_size=legacy.Serialize().ToString().size();
        for(size_t n=legacy_size;n<bytes.size();++n) {GT_ASSERT(Rejects([&]{SmacProductionRules::Deserialize(types::Buffer(bytes.substr(0,n)));}),"Truncated V2 accepted");}
        for(int32_t endpoint:{INT32_MIN,INT32_MAX}) {
            expected.prototype_percentages=std::array<int32_t,3>{{endpoint,endpoint,endpoint}};
            GT_ASSERT(SmacProductionRules::Deserialize(expected.Serialize())==expected,"Signed source percentage narrowed");
        }GT_OK();
    });
	task->AddTest( "SMAC production locked settings compare every modeled field", GT() {
		GlobalSettings settings; settings.Deserialize( fixture::Payload( "smac", true ) );
		const auto before = settings.Serialize().ToString();
		const std::array< int64_t, 9 > base = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 };
		const auto reject = [ & ]( const SmacProductionRules& changed ) {
			return Rejects( [ & ]() { settings.Deserialize( fixture::Payload( "smac", true, base, changed.Serialize().ToString() ) ); } )
				&& settings.Serialize().ToString() == before;
		};
		for ( const auto member : { &SmacProductionRules::mineral_multiplier, &SmacProductionRules::retool_loss_percent,
			&SmacProductionRules::retool_strictness, &SmacProductionRules::retool_exemption } ) {
			auto changed = fixture::Production(); changed.*member = 42;
			GT_ASSERT( reject( changed ), "locked production rule changed" );
		}
		for ( size_t row = 0 ; row < 71 ; ++row ) {
			for ( const auto member : { &SmacFacilityRule::cost, &SmacFacilityRule::maintenance } ) {
				auto changed = fixture::Production(); changed.facilities[ row ].*member = 42;
				GT_ASSERT( reject( changed ), "locked catalog numeric field changed" );
			}
			for ( const auto member : { &SmacFacilityRule::name, &SmacFacilityRule::prerequisite } ) {
				auto changed = fixture::Production(); changed.facilities[ row ].*member = "changed";
				GT_ASSERT( reject( changed ), "locked catalog text field changed" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC production late source and wire failures preserve all live settings", GT() {
		Settings settings; settings.global.SelectRuleset( "smac" );
		settings.global.game_name = "preserve name"; settings.global.map.filename = "preserve/path";
		settings.global.map.size_x = 84; settings.global.map.size_y = 42; settings.global.difficulty_level = -17;
		const auto before_map = settings.global.map.Serialize().ToString();
		auto source = fixture::Source( "alpha.txt" );
		source.replace( source.find( "Facility 71" ), std::string( "Facility 71" ).size(), "" );
		GT_ASSERT( Rejects( [ & ]() { settings.PrepareRuleset( [ & ]( const std::string& ) { return source; } ); } ), "bad last facility initialized a host" );
		GT_ASSERT( !settings.global.IsRulesetLocked() && !settings.global.GetSmacResourceRules() && !settings.global.GetSmacProductionRules(), "late source failure partially published profiles" );
		GT_ASSERT( settings.global.GetRulesetId() == "smac" && settings.global.map.Serialize().ToString() == before_map
			&& settings.global.map.filename == "preserve/path" && settings.global.game_name == "preserve name" && settings.global.difficulty_level == -17,
			"late source failure changed existing settings" );
		settings.PrepareRuleset( fixture::Source );
		const auto before = settings.global.Serialize().ToString();
		const auto profile = fixture::Production().Serialize().ToString();
		const std::array< int64_t, 9 > base = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 };
		std::vector< types::Buffer > invalid = { fixture::Payload( "smac", true, base, "" ),
			fixture::Payload( "smac", true, base, profile.substr( 0, profile.size() - 1 ) ) };
		auto late = settings.global.Serialize().ToString(); late.pop_back(); invalid.emplace_back( late );
		for ( const auto& wire : invalid ) {
			GT_ASSERT( Rejects( [ & ]() { settings.global.Deserialize( wire ); } ), "late malformed profile/settings accepted" );
			GT_ASSERT( settings.global.Serialize().ToString() == before && settings.global.map.filename == "preserve/path"
				&& settings.global.IsRulesetLocked(), "late decode failure mutated existing fields/lock" );
		}
		GT_OK();
	} );
}

}
