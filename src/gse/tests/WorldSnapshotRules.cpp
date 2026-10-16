#include "WorldSnapshotRules.h"

#include "fixtures/SmacRulesSource.h"
#include "fixtures/WorldSnapshotRules.h"
#include "task/gsetests/GSETests.h"
#include "util/random/Random.h"

namespace gse::tests {
namespace {
bool Rejects( const std::function< void() >& f ) {
	try { f(); }
	catch ( const std::runtime_error& ) { return true; }
	return false;
}
}

void AddWorldSnapshotRulesTests( task::gsetests::GSETests* task ) {
	using game::backend::settings::GlobalSettings;
	using namespace game::backend::world_snapshot;
	task->AddTest( "world RNG codec preserves all four JSF32 words and its zero state", GT() {
		for ( const auto state : { util::random::state_t{ 0, 0, 0, 0 }, util::random::state_t{ 0, UINT32_MAX, 1, 2147483648u }, util::random::state_t{ UINT32_MAX, 0, 2147483648u, 1 } } ) {
			types::Buffer buffer; WriteRandomState( buffer, state );
			const auto size = buffer.lenw; buffer.WriteString( "body" );
			const auto restored = ReadRandomState( buffer );
			GT_ASSERT( restored.a == state.a && restored.b == state.b && restored.c == state.c && restored.d == state.d, "RNG words were narrowed, reordered or reseeded" );
			GT_ASSERT( buffer.lenr == size && buffer.ReadString() == "body", "RNG codec consumed body frames" );
			util::random::Random source( 1 ), destination( 2 ); source.SetState( state ); destination.SetState( restored );
			for ( size_t i = 0 ; i < 16 ; ++i ) {
				GT_ASSERT( source.GetUInt() == destination.GetUInt() && source.GetInt64( -100, 100 ) == destination.GetInt64( -100, 100 ) && source.GetFloat() == destination.GetFloat(), "RNG continuation changed" );
			}
			if ( state.a == 0 && state.b == 0 && state.c == 0 && state.d == 0 ) {
				GT_ASSERT( destination.GetStateString() == "0:0:0:0" && destination.GetUInt() == 0, "existing absorbing zero state was not preserved" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "world RNG codec rejects every malformed word before returning state", GT() {
		GlobalSettings settings; settings.SelectRuleset( "smac" ); settings.InitializeHostRules( smac_rules_fixture::Source );
		for ( const auto& invalid : world_rules_fixture::InvalidRandomHeaders() ) {
			types::Buffer input( invalid.second ); ValidateHeader( input, settings );
			GT_ASSERT( Rejects( [ & ]() { ReadRandomState( input ); } ), "accepted " + invalid.first );
		}
		GT_OK();
	} );
	task->AddTest( "world rules header preserves frozen profiles and leaves body bytes untouched", GT() {
		for ( const auto* id : { "smac", "smax" } ) {
			GlobalSettings settings;
			settings.SelectRuleset( id );
			settings.InitializeHostRules( smac_rules_fixture::Source );
			const auto before = settings.Serialize().ToString();
			types::Buffer snapshot;
			WriteHeader( snapshot, settings );
			const auto header_size = snapshot.lenw;
			const std::string body( "\0arbitrary world body\xff", 22 );
			snapshot.WriteString( body );
			snapshot.WriteInt( 37 );
			ValidateHeader( snapshot, settings );
			GT_ASSERT( snapshot.lenr == header_size, "header consumed world-body bytes" );
			GT_ASSERT( snapshot.ReadString() == body && snapshot.ReadInt() == 37 && snapshot.lenr == snapshot.lenw, "world body changed" );
			GT_ASSERT( settings.Serialize().ToString() == before && settings.IsRulesetLocked(), "header changed the session profile" );
		}
		GT_OK();
	} );
	task->AddTest( "world rules header rejects malformed identity and every changed or out-of-range numeric field", GT() {
		GlobalSettings settings;
		settings.SelectRuleset( "smac" );
		settings.InitializeHostRules( smac_rules_fixture::Source );
		const auto before = settings.Serialize().ToString();
		for ( const auto& invalid : world_rules_fixture::InvalidSmacHeaders() ) {
			types::Buffer snapshot( invalid.second );
			GT_ASSERT( Rejects( [ & ]() { ValidateHeader( snapshot, settings ); } ), "accepted " + invalid.first );
			GT_ASSERT( settings.Serialize().ToString() == before, "rejection mutated settings" );
		}
		settings.ResetRuleset();
		settings.SelectRuleset( "smax" );
		settings.LockRuleset();
		for ( auto snapshot : { world_rules_fixture::Header( "smax", true ), world_rules_fixture::Header() } ) {
			GT_ASSERT( Rejects( [ & ]() { ValidateHeader( snapshot, settings ); } ), "SMAX accepted SMAC numeric data/identity" );
		}
		GT_OK();
	} );
	task->AddTest( "world rules header requires a frozen selected session for reading and writing", GT() {
		for ( const auto* id : { "unselected", "smac", "smax" } ) {
			GlobalSettings settings;
			if ( std::string( id ) != "unselected" ) { settings.SelectRuleset( id ); }
			types::Buffer output;
			GT_ASSERT( Rejects( [ & ]() { WriteHeader( output, settings ); } ), "unfrozen profile wrote a world header" );
			GT_ASSERT( output.lenw == 0, "failed writer partially wrote a header" );
			auto input = world_rules_fixture::Header( id, std::string( id ) == "smac" );
			GT_ASSERT( Rejects( [ & ]() { ValidateHeader( input, settings ); } ), "unfrozen profile accepted a world header" );
			GT_ASSERT( !settings.IsRulesetLocked(), "validator froze the profile" );
		}
		GT_OK();
	} );
}

}
