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
			auto wire = snapshot;
			GT_ASSERT( wire.ReadInt() == rules_marker && wire.ReadInt() == 4 && wire.ReadString() == id, "writer omitted the V4 identity" );
			const bool smac = std::string( id ) == "smac";
			GT_ASSERT( wire.ReadBool() == smac, "numeric presence changed" );
			if ( smac ) { for ( size_t field = 0; field < 9; ++field ) { wire.ReadInt(); } }
			GT_ASSERT( wire.ReadBool() == smac, "production presence changed" );
			if ( smac ) {
				GT_ASSERT( wire.ReadString() == settings.GetSmacProductionRules()->Serialize().ToString(), "header changed the production codec payload" );
			}
			GT_ASSERT( wire.lenr == wire.lenw, "unexpected fields in written header" );
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
	task->AddTest( "world rules header binds every frozen production field including the last catalog row", GT() {
		using game::backend::settings::SmacProductionRules;
		using game::backend::settings::SmacFacilityRule;
		GlobalSettings source; source.SelectRuleset( "smac" ); source.InitializeHostRules( smac_rules_fixture::Source );
		types::Buffer snapshot; WriteHeader( snapshot, source );
		const auto reject = [ & ]( const SmacProductionRules& changed ) {
			GlobalSettings destination;
			destination.Deserialize( smac_rules_fixture::Payload( "smac", true,
				{ 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, changed.Serialize().ToString() ) );
			const auto before = destination.Serialize().ToString();
			auto input = snapshot;
			return Rejects( [ & ]() { ValidateHeader( input, destination ); } ) && destination.Serialize().ToString() == before;
		};
		for ( const auto member : { &SmacProductionRules::mineral_multiplier, &SmacProductionRules::retool_loss_percent,
			&SmacProductionRules::retool_strictness, &SmacProductionRules::retool_exemption } ) {
			auto changed = smac_rules_fixture::Production(); changed.*member = 42;
			GT_ASSERT( reject( changed ), "world accepted a different frozen production rule" );
		}
		for ( size_t row = 0; row < 71; ++row ) {
			for ( const auto member : { &SmacFacilityRule::cost, &SmacFacilityRule::maintenance } ) {
				auto changed = smac_rules_fixture::Production(); changed.facilities[ row ].*member = 42;
				GT_ASSERT( reject( changed ), "world accepted a different frozen catalog number" );
			}
			for ( const auto member : { &SmacFacilityRule::name, &SmacFacilityRule::prerequisite } ) {
				auto changed = smac_rules_fixture::Production(); changed.facilities[ row ].*member = "changed";
				GT_ASSERT( reject( changed ), "world accepted a different frozen catalog text" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "world rules header rejects malformed production profiles without changing frozen settings", GT() {
		for ( const auto* id : { "smac", "smax" } ) {
			GlobalSettings settings; settings.SelectRuleset( id ); settings.InitializeHostRules( smac_rules_fixture::Source );
			const auto before = settings.Serialize().ToString();
			for ( const auto& invalid : world_rules_fixture::InvalidProductionHeaders( id ) ) {
				types::Buffer input( invalid.second );
				input.WriteString( "unread world body" );
				GT_ASSERT( Rejects( [ & ]() { ValidateHeader( input, settings ); } ), "accepted " + invalid.first );
				GT_ASSERT( settings.Serialize().ToString() == before, "production rejection changed settings" );
			}
		}
		GT_OK();
	} );
}

}
