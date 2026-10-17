#include "WorldSnapshotRules.h"

#include "fixtures/SmacRulesSource.h"
#include "fixtures/WorldSnapshotRules.h"
#include "task/gsetests/GSETests.h"
#include "util/random/Random.h"

#include <algorithm>

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
	task->AddTest( "world rules header bounds predefined frame before copying and preserves its cursor", GT() {
		using Profile = game::backend::settings::SmacPredefinedUnitRules;
		GlobalSettings settings; settings.SelectRuleset( "smac" ); settings.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( settings );
		const auto before = settings.Serialize().ToString();
		types::Buffer header; WriteHeader( header, settings );
		// Keep all preceding production fields valid and locate the predefined
		// payload itself, independently of any later header/body extensions.
		header.ReadInt(); header.ReadInt(); header.ReadString();
		GT_ASSERT( header.ReadBool(), "missing numeric profile in test header" );
		for ( size_t i = 0; i < 9; ++i ) { header.ReadInt(); }
		for ( size_t i = 0; i < 2; ++i ) { GT_ASSERT( header.ReadBool(), "missing profile in test header" ); header.ReadString(); }
		GT_ASSERT( header.ReadBool(), "missing size profile in test header" ); header.ReadString(); header.ReadString();
		GT_ASSERT( header.ReadBool(), "missing faction profile in test header" ); header.ReadString();
		GT_ASSERT( header.ReadBool(), "missing predefined profile in test header" );
		const auto prefix = header.ToString().substr( 0, header.lenr );
		for ( const auto length : { Profile::MAX_WIRE_BYTES + 1, Profile::MAX_WIRE_BYTES * 4 } ) {
			const std::string payload( length, 'x' );
			for ( const bool corrupt_checksum : { false, true } ) {
				types::Buffer frame; frame.WriteString( payload );
				auto bytes = prefix + frame.ToString();
				if ( corrupt_checksum ) { bytes.back() ^= 1; }
				types::Buffer input( bytes ); input.WriteString( "unread world body" );
				std::string error;
				try { ValidateHeader( input, settings ); }
				catch ( const std::runtime_error& caught ) { error = caught.what(); }
				GT_ASSERT( error.find( "buffer read exceeds payload limit" ) != std::string::npos, "oversized profile reached codec/checksum processing instead of bounded frame admission" );
				GT_ASSERT( input.lenr == prefix.size(), "rejected profile was copied/consumed before its wire limit" );
				GT_ASSERT( settings.Serialize().ToString() == before, "frame rejection changed frozen settings" );
				if ( !corrupt_checksum ) {
					// The rejected frame is complete, with a valid checksum; the
					// separate unbounded test read proves it was not truncation.
					GT_ASSERT( input.ReadString() == payload && input.ReadString() == "unread world body", "failed frame read changed bytes or body cursor" );
				}
			}
		}
		GT_OK();
	} );
	task->AddTest( "world predefined profile exact byte cap remains a valid codec domain", GT() {
		using Profile = game::backend::settings::SmacPredefinedUnitRules;
		const auto technologies = smac_rules_fixture::Technologies();
		auto profile = smac_rules_fixture::PredefinedUnits();
		const auto initial_size = profile.Serialize( technologies ).lenw;
		GT_ASSERT( initial_size < Profile::MAX_WIRE_BYTES, "fixture no longer has room below the cap" );
		size_t remaining = Profile::MAX_WIRE_BYTES - initial_size;
		const auto grow = [ & ]( std::string& text, const size_t limit ) {
			const auto count = std::min( remaining, limit - text.size() ); text.append( count, 'x' ); remaining -= count;
		};
		// Display text is owned metadata and does not change resolved IDs or
		// derived costs. Respect each existing codec text limit while padding.
		for ( auto& ability : profile.catalog.abilities.abilities ) { grow( ability.description, 512 ); }
		for ( auto& weapon : profile.catalog.weapons.weapons ) { grow( weapon.name, 128 ); }
		for ( auto& unit : profile.catalog.source.units ) { grow( unit.name, 128 ); }
		GT_ASSERT( remaining == 0, "fixture cannot reach the exact profile byte cap" );
		const auto bytes = profile.Serialize( technologies ).ToString();
		GT_ASSERT( bytes.size() == Profile::MAX_WIRE_BYTES && Profile::Deserialize( bytes, technologies ) == profile, "exact-cap valid profile failed semantic codec validation" );
		types::Buffer frame; frame.WriteString( bytes ); const auto end = frame.lenw; frame.WriteString( "body" );
		GT_ASSERT( frame.ReadString( Profile::MAX_WIRE_BYTES ) == bytes && frame.lenr == end && frame.ReadString() == "body", "exact-cap bounded frame read consumed the body or rejected equality" );
		GT_OK();
	} );
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
		GlobalSettings settings; settings.SelectRuleset( "smac" ); settings.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( settings );
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
			settings.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( settings );
			const auto before = settings.Serialize().ToString();
			types::Buffer snapshot;
			WriteHeader( snapshot, settings );
			auto wire = snapshot;
			GT_ASSERT( wire.ReadInt() == rules_marker && wire.ReadInt() == 8 && wire.ReadString() == id, "writer omitted the V8 identity" );
			const bool smac = std::string( id ) == "smac";
			GT_ASSERT( wire.ReadBool() == smac, "numeric presence changed" );
			if ( smac ) { for ( size_t field = 0; field < 9; ++field ) { wire.ReadInt(); } }
			GT_ASSERT( wire.ReadBool() == smac, "production presence changed" );
			if ( smac ) {
				GT_ASSERT( wire.ReadString() == settings.GetSmacProductionRules()->Serialize().ToString(), "header changed the production codec payload" );
			}
			GT_ASSERT( wire.ReadBool() == smac, "technology presence changed" );
			if ( smac ) {
				GT_ASSERT( wire.ReadString() == settings.GetSmacTechnologyRules()->Serialize().ToString(), "header changed the technology codec payload" );
			}
			GT_ASSERT( wire.ReadBool() == smac, "world-size presence changed" );
			if ( smac ) {
				GT_ASSERT( wire.ReadString() == settings.GetSmacWorldSizeRules()->Serialize().ToString(), "world-size profile changed" );
				GT_ASSERT( wire.ReadString() == settings.map.GetSmacWorldSize()->Serialize().ToString(), "accepted selection changed" );
			}
			GT_ASSERT( wire.ReadBool() == smac, "faction startup presence changed" );
			if ( smac ) { GT_ASSERT( wire.ReadString() == settings.GetSmacFactionStartupRules()->Serialize( *settings.GetSmacTechnologyRules() ).ToString(), "faction startup metadata changed" ); }
			GT_ASSERT( wire.ReadBool() == smac, "predefined unit presence changed" );
			if ( smac ) { GT_ASSERT( wire.ReadString() == settings.GetSmacPredefinedUnitRules()->Serialize( *settings.GetSmacTechnologyRules() ).ToString(), "predefined source/derived metadata changed" ); }
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
		settings.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( settings );
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
		smac_rules_fixture::AcceptWorldSize( source );
		types::Buffer snapshot; WriteHeader( snapshot, source );
		const auto reject = [ & ]( const SmacProductionRules& changed ) {
			GlobalSettings destination;
			destination.Deserialize( smac_rules_fixture::Payload( "smac", true,
				{ 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, changed.Serialize().ToString() ) );
			smac_rules_fixture::AcceptWorldSize( destination );
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
	task->AddTest( "world rules header binds every frozen technology field without changing settings", GT() {
		using game::backend::settings::SmacTechnologyRules;
		using game::backend::settings::SmacTechnologyRule;
		GlobalSettings source; source.SelectRuleset( "smac" ); source.InitializeHostRules( smac_rules_fixture::Source );
		smac_rules_fixture::AcceptWorldSize( source );
		types::Buffer snapshot; WriteHeader( snapshot, source );
		const auto reject = [ & ]( const SmacTechnologyRules& changed ) {
			GlobalSettings destination;
			destination.Deserialize( smac_rules_fixture::Payload( "smac", true,
				{ 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, {}, changed.Serialize().ToString() ) );
			smac_rules_fixture::AcceptWorldSize( destination );
			const auto before = destination.Serialize().ToString(); auto input = snapshot;
			return Rejects( [ & ]() { ValidateHeader( input, destination ); } ) && destination.Serialize().ToString() == before;
		};
		for ( size_t row = 0; row < 89; ++row ) {
			for ( const auto member : { &SmacTechnologyRule::name, &SmacTechnologyRule::key } ) {
				auto changed = smac_rules_fixture::Technologies(); changed.technologies[ row ].*member = "Changed";
				GT_ASSERT( reject( changed ), "world accepted different technology identity text" );
			}
			for ( size_t field = 0; field < 4; ++field ) {
				auto changed = smac_rules_fixture::Technologies(); auto& priority = changed.technologies[ row ].priorities[ field ]; priority = priority == 42 ? 43 : 42;
				GT_ASSERT( reject( changed ), "world accepted different technology priority" );
			}
			for ( size_t field = 0; field < 2; ++field ) {
				auto changed = smac_rules_fixture::Technologies(); changed.technologies[ row ].prerequisites[ field ] = "Disable";
				GT_ASSERT( reject( changed ), "world accepted different technology prerequisite" );
			}
			auto changed = smac_rules_fixture::Technologies(); changed.technologies[ row ].flags ^= 1;
			GT_ASSERT( reject( changed ), "world accepted different technology flags" );
		}
		GT_OK();
	} );
	task->AddTest( "world rules header binds every ordered faction bonus and scalar without changing settings", GT() {
		using game::backend::settings::SmacFactionStartupRules;
		using game::backend::settings::SmacFactionStartupBonus;
		GlobalSettings source; source.SelectRuleset( "smac" ); source.InitializeHostRules( smac_rules_fixture::Source );
		smac_rules_fixture::AcceptWorldSize( source );
		types::Buffer snapshot; WriteHeader( snapshot, source );
		const auto technologies = smac_rules_fixture::Technologies();
		const auto reject = [ & ]( const SmacFactionStartupRules& changed ) {
			GlobalSettings destination;
			destination.Deserialize( smac_rules_fixture::Payload( "smac", true,
				{ 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, {}, {}, {}, changed.Serialize( technologies ).ToString() ) );
			smac_rules_fixture::AcceptWorldSize( destination );
			const auto before = destination.Serialize().ToString(); auto input = snapshot;
			return Rejects( [ & ]() { ValidateHeader( input, destination ); } ) && destination.Serialize().ToString() == before;
		};
		const std::array< const char*, 12 > values = { "Tech3", "tech2 ", "Disable", "None", "--SUPPORT", "17", "POLICE", "2", "3", "+1suffix", "7", "23" };
		for ( size_t faction = 0; faction < 7; ++faction ) {
			for ( size_t index = 0; index < values.size(); ++index ) {
				auto changed = smac_rules_fixture::Factions(); auto& bonus = changed.factions[ faction ].bonuses[ index ];
				bonus = SmacFactionStartupBonus::Parse( bonus.keyword, values[ index ], technologies );
				GT_ASSERT( reject( changed ), "world accepted a changed raw/derived faction bonus" );
			}
			auto changed = smac_rules_fixture::Factions(); std::swap( changed.factions[ faction ].bonuses[ 0 ], changed.factions[ faction ].bonuses[ 2 ] );
			GT_ASSERT( reject( changed ), "world ignored faction record order" );
			changed = smac_rules_fixture::Factions(); changed.factions[ faction ].bonuses.pop_back();
			GT_ASSERT( reject( changed ), "world accepted omitted numeric metadata" );
			changed = smac_rules_fixture::Factions(); changed.factions[ faction ].bonuses.back() = SmacFactionStartupBonus::Parse( "DRONE", "16", technologies );
			GT_ASSERT( reject( changed ), "world accepted changed scalar keyword/tag" );
		}
		GT_OK();
	} );
	task->AddTest( "world rules header binds complete predefined sources and derived records before body", GT() {
		for ( const auto* id : { "smac", "smax" } ) {
			GlobalSettings settings; settings.SelectRuleset( id ); settings.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( settings );
			const auto before = settings.Serialize().ToString();
			for ( const auto& invalid : world_rules_fixture::InvalidPredefinedHeaders( id ) ) {
				types::Buffer input( invalid.second ); const auto end = input.lenw; input.WriteString( "unread world body" );
				GT_ASSERT( Rejects( [ & ]() { ValidateHeader( input, settings ); } ), "accepted " + invalid.first );
				GT_ASSERT( settings.Serialize().ToString() == before, "unit rejection changed frozen settings" );
				// Missing/mistyped fields can consume a replacement body frame while
				// rejecting its type. Complete framed mismatches stop at the prefix.
				if ( invalid.first.find( "different valid" ) == 0 ) { GT_ASSERT( input.lenr == end, "profile mismatch consumed world body" ); }
			}
		}
		GT_OK();
	} );
	task->AddTest( "world rules header rejects missing mistyped and late malformed faction metadata", GT() {
		for ( const auto* id : { "smac", "smax" } ) {
			GlobalSettings settings; settings.SelectRuleset( id ); settings.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( settings );
			const auto before = settings.Serialize().ToString();
			for ( const auto& invalid : world_rules_fixture::InvalidFactionHeaders( id ) ) {
				types::Buffer input( invalid.second ); input.WriteString( "unread world body" );
				GT_ASSERT( Rejects( [ & ]() { ValidateHeader( input, settings ); } ), "accepted " + invalid.first );
				GT_ASSERT( settings.Serialize().ToString() == before, "faction rejection changed frozen settings" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "world rules header rejects malformed technology catalogs without changing frozen settings", GT() {
		for ( const auto* id : { "smac", "smax" } ) {
			GlobalSettings settings; settings.SelectRuleset( id ); settings.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( settings );
			const auto before = settings.Serialize().ToString();
			for ( const auto& invalid : world_rules_fixture::InvalidTechnologyHeaders( id ) ) {
				types::Buffer input( invalid.second ); input.WriteString( "unread world body" );
				GT_ASSERT( Rejects( [ & ]() { ValidateHeader( input, settings ); } ), "accepted " + invalid.first );
				GT_ASSERT( settings.Serialize().ToString() == before, "technology rejection changed settings" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "world rules header rejects malformed production profiles without changing frozen settings", GT() {
		for ( const auto* id : { "smac", "smax" } ) {
			GlobalSettings settings; settings.SelectRuleset( id ); settings.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( settings );
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
