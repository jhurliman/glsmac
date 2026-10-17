#include "SmacHostRules.h"

#include "fixtures/SmacRulesSource.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <limits>
#include <vector>

namespace gse::tests {

using namespace game::backend::settings;
namespace fixture = smac_rules_fixture;

static bool Rejects( const std::function< void() >& operation ) {
	try { operation(); }
	catch ( const std::exception& ) { return true; }
	return false;
}

void AddSmacHostRulesTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC host preparation loads once while SMAX and joining never read rules files", GT() {
		for ( const auto role : { LocalSettings::NR_NONE, LocalSettings::NR_SERVER } ) {
			for ( const auto* id : { "smac", "smax" } ) {
				Settings settings;
				settings.local.network_role = role;
				settings.global.SelectRuleset( id );
				size_t reads = 0;
				const auto source = [ & ]( const std::string& filename ) { ++reads; return fixture::Source( filename ); };
				settings.PrepareRuleset( source );
				const bool smac = settings.global.GetRulesetId() == "smac";
				GT_ASSERT( reads == ( smac ? 1 : 0 ), "rules file read for the wrong profile/role" );
				GT_ASSERT( smac ? fixture::Matches( settings.global ) : !settings.global.GetSmacBaseSquareRules(), "host numeric rules do not match explicit profile" );
				const auto before = settings.global.Serialize().ToString();
				settings.PrepareRuleset( []( const std::string& ) -> std::string { throw std::runtime_error( "locked session reread file" ); } );
				GT_ASSERT( settings.global.Serialize().ToString() == before, "repeat preparation changed frozen rules" );
				GT_ASSERT( Rejects( [ & ]() { settings.global.SelectRuleset( id ); } ), "host selection remained mutable" );
				settings.local.network_role = LocalSettings::NR_CLIENT;
				settings.PrepareRuleset( source );
				GT_ASSERT( reads == ( smac ? 1 : 0 ), "joining loaded local rules" );
				GT_ASSERT( settings.global.GetRulesetId() == "unselected" && !settings.global.GetSmacBaseSquareRules() && !settings.global.GetSmacProductionRules(), "joining retained the previous host's rules" );
				settings.global.SelectRuleset( "smax" );
				GT_ASSERT( !settings.global.GetSmacBaseSquareRules() && !settings.global.GetSmacProductionRules(), "new selection inherited SMAC numeric rules" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC missing or malformed source leaves initialization retryable without defaults", GT() {
		Settings settings;
		size_t reads = 0;
		GT_ASSERT( Rejects( [ & ]() { settings.PrepareRuleset( [ & ]( const std::string& name ) { ++reads; return fixture::Source( name ); } ); } ), "unselected session initialized" );
		GT_ASSERT( reads == 0, "unselected session read a rules file" );
		settings.global.SelectRuleset( "smac" );
		settings.global.game_name = "preserved";
		settings.global.map.size_x = 88;
		GT_ASSERT( Rejects( [ & ]() { settings.global.LockRuleset(); } ), "SMAC locked without numeric rules" );
		GT_ASSERT( Rejects( [ & ]() { settings.global.Serialize(); } ), "SMAC serialized missing numeric rules" );
		for ( const bool missing : { false, true } ) {
			GT_ASSERT( Rejects( [ & ]() { settings.PrepareRuleset( [ & ]( const std::string& ) -> std::string {
				if ( missing ) { throw std::runtime_error( "synthetic missing file" ); }
				return "#RESOURCEINFO\nBase Square,broken\n";
			} ); } ), "bad source initialized a host" );
			GT_ASSERT( settings.global.GetRulesetId() == "smac" && !settings.global.GetSmacBaseSquareRules(), "failed source published numeric rules" );
			GT_ASSERT( settings.global.game_name == "preserved" && settings.global.map.size_x == 88, "failed source changed unrelated settings" );
			settings.global.SelectRuleset( "smac" ); // Failure did not freeze selection.
		}
		settings.PrepareRuleset( fixture::Source );
		GT_ASSERT( fixture::Matches( settings.global ), "retry did not load supplied values" );
		settings.global.ResetRuleset();
		GT_ASSERT( !settings.global.GetSmacBaseSquareRules() && !settings.global.GetSmacProductionRules(), "reset retained numeric rules" );
		GT_OK();
	} );
	task->AddTest( "SMAC V3 settings preserve signed rules and reject changes to every locked numeric field", GT() {
		GlobalSettings settings;
		settings.Deserialize( fixture::Payload( "smac", true ) );
		GT_ASSERT( fixture::Matches( settings ), "V3 lost signed values or endpoints" );
		const auto before = settings.Serialize().ToString();
		GlobalSettings restored;
		restored.Deserialize( types::Buffer( before ) );
		GT_ASSERT( restored.Serialize().ToString() == before, "V3 roundtrip changed settings bytes" );
		for ( size_t i = 0 ; i < 9 ; ++i ) {
			std::array< int64_t, 9 > values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 };
			values[ i ] += values[ i ] == INT32_MAX ? -1 : 1;
			GT_ASSERT( Rejects( [ & ]() { settings.Deserialize( fixture::Payload( "smac", true, values ) ); } ), "locked numeric field changed" );
			GT_ASSERT( settings.Serialize().ToString() == before, "rejected numeric change mutated settings" );
		}
		GT_ASSERT( Rejects( [ & ]() { settings.Deserialize( fixture::Payload( "smax", false ) ); } ), "locked profile changed" );
		// Same identity and rules still permit ordinary lobby setting updates.
		restored.game_name = "Updated lobby name";
		restored.map.size_x = 92;
		settings.Deserialize( restored.Serialize() );
		GT_ASSERT( settings.game_name == restored.game_name && settings.map.size_x == 92 && fixture::Matches( settings ), "identical rules blocked a lobby update" );
		GT_OK();
	} );
	task->AddTest( "SMAC V3 rejects absent, unexpected, out-of-range and incomplete rules before mutation", GT() {
		std::vector< types::Buffer > invalid = { fixture::Payload( "smac", false ), fixture::Payload( "smax", true ) };
		for ( const auto version : { 1, 2, 4 } ) { types::Buffer b; b.WriteInt( version ); invalid.push_back( b ); }
		for ( size_t i = 0 ; i < 9 ; ++i ) {
			for ( const int64_t value : { int64_t( INT32_MIN ) - 1, int64_t( INT32_MAX ) + 1, INT64_MIN, INT64_MAX } ) {
				std::array< int64_t, 9 > values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 };
				values[ i ] = value;
				invalid.push_back( fixture::Payload( "smac", true, values ) );
			}
			types::Buffer short_rules;
			short_rules.WriteInt( 3 ); short_rules.WriteString( "smac" ); short_rules.WriteBool( true );
			for ( size_t field = 0 ; field < i ; ++field ) { short_rules.WriteInt( 1 ); }
			invalid.push_back( short_rules );
		}
		const auto valid = fixture::Payload( "smac", true );
		for ( const bool bad_map : { false, true } ) {
			auto reader = valid;
			types::Buffer rebuilt;
			rebuilt.WriteInt( reader.ReadInt() ); rebuilt.WriteString( reader.ReadString() ); rebuilt.WriteBool( reader.ReadBool() );
			for ( size_t field = 0 ; field < 9 ; ++field ) { rebuilt.WriteInt( reader.ReadInt() ); }
			rebuilt.WriteString( reader.ReadString() );
			types::Buffer map( reader.ReadString() );
			if ( bad_map ) { map.WriteInt( 123 ); }
			rebuilt.WriteString( map.ToString() );
			reader.ReadString();
			rebuilt.WriteString( bad_map ? "" : "unknown default rules" );
			rebuilt.WriteInt( reader.ReadInt() ); rebuilt.WriteString( reader.ReadString() );
			invalid.push_back( rebuilt );
		}
		auto trailing = valid; trailing.WriteInt( 77 ); invalid.push_back( trailing );
		invalid.emplace_back( valid.ToString().substr( 0, valid.lenw - 1 ) ); // Late failure after a complete map.
		for ( const auto& payload : invalid ) {
			GlobalSettings settings;
			settings.SelectRuleset( "smax" ); // Unlocked: reject invalid SMAC data, not an identity change.
			settings.game_name = "unchanged"; settings.map.size_x = 88;
			const auto before = settings.Serialize().ToString();
			GT_ASSERT( Rejects( [ & ]() { settings.Deserialize( payload ); } ), "invalid V3 settings accepted" );
			GT_ASSERT( settings.Serialize().ToString() == before, "invalid payload partially mutated settings" );
			settings.Deserialize( valid ); // Rejection left the destination unlocked/retryable.
			GT_ASSERT( fixture::Matches( settings ), "valid retry after rejected payload failed" );
		}
		GT_OK();
	} );
}

}
