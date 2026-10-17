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
				GT_ASSERT( reads == ( smac ? 8 : 0 ), "rules file read for the wrong profile/role" );
				GT_ASSERT( smac ? fixture::Matches( settings.global ) : !settings.global.GetSmacResourceRules(), "host numeric rules do not match explicit profile" );
				const auto before = settings.global.Serialize().ToString();
				settings.PrepareRuleset( []( const std::string& ) -> std::string { throw std::runtime_error( "locked session reread file" ); } );
				GT_ASSERT( settings.global.Serialize().ToString() == before, "repeat preparation changed frozen rules" );
				GT_ASSERT( Rejects( [ & ]() { settings.global.SelectRuleset( id ); } ), "host selection remained mutable" );
				settings.local.network_role = LocalSettings::NR_CLIENT;
				settings.PrepareRuleset( source );
				GT_ASSERT( reads == ( smac ? 8 : 0 ), "joining loaded local rules" );
				GT_ASSERT( settings.global.GetRulesetId() == "unselected" && !settings.global.GetSmacResourceRules() && !settings.global.GetSmacProductionRules() && !settings.global.GetSmacTechnologyRules() && !settings.global.GetSmacMovementRules(), "joining retained the previous host's rules" );
				settings.global.SelectRuleset( "smax" );
				GT_ASSERT( !settings.global.GetSmacResourceRules() && !settings.global.GetSmacProductionRules() && !settings.global.GetSmacTechnologyRules() && !settings.global.GetSmacMovementRules(), "new selection inherited SMAC numeric rules" );
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
			GT_ASSERT( settings.global.GetRulesetId() == "smac" && !settings.global.GetSmacResourceRules(), "failed source published numeric rules" );
			GT_ASSERT( settings.global.game_name == "preserved" && settings.global.map.size_x == 88, "failed source changed unrelated settings" );
			settings.global.SelectRuleset( "smac" ); // Failure did not freeze selection.
		}
		settings.PrepareRuleset( fixture::Source );
		GT_ASSERT( fixture::Matches( settings.global ), "retry did not load supplied values" );
		settings.global.ResetRuleset();
		GT_ASSERT( !settings.global.GetSmacResourceRules() && !settings.global.GetSmacProductionRules() && !settings.global.GetSmacTechnologyRules() && !settings.global.GetSmacMovementRules(), "reset retained numeric rules" );
		GT_OK();
	} );
	task->AddTest( "SMAC V11 settings preserve signed rules and reject changes to every locked numeric field", GT() {
		GlobalSettings settings;
		settings.Deserialize( fixture::Payload( "smac", true ) );
		GT_ASSERT( fixture::Matches( settings ), "V11 lost signed values or endpoints" );
		const auto before = settings.Serialize().ToString();
		GlobalSettings restored;
		restored.Deserialize( types::Buffer( before ) );
		GT_ASSERT( restored.Serialize().ToString() == before, "V11 roundtrip changed settings bytes" );
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
	task->AddTest( "SMAC V11 rejects absent, unexpected, out-of-range and incomplete rules before mutation", GT() {
		std::vector< types::Buffer > invalid = { fixture::Payload( "smac", false ), fixture::Payload( "smax", true ) };
		for ( const auto version : { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12 } ) { types::Buffer b; b.WriteInt( version ); invalid.push_back( b ); }
		for ( size_t i = 0 ; i < 9 ; ++i ) {
			for ( const int64_t value : { int64_t( INT32_MIN ) - 1, int64_t( INT32_MAX ) + 1, INT64_MIN, INT64_MAX } ) {
				std::array< int64_t, 9 > values = { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 };
				values[ i ] = value;
				invalid.push_back( fixture::Payload( "smac", true, values ) );
			}
			types::Buffer short_rules;
			short_rules.WriteInt( 11 ); short_rules.WriteString( "smac" ); short_rules.WriteBool( true );
			types::Buffer prefix; prefix.WriteInt( 1 );
			for ( size_t field = 0 ; field < i ; ++field ) { prefix.WriteInt( 1 ); }
			short_rules.WriteString( prefix.ToString() );
			invalid.push_back( short_rules );
		}
		const auto valid = fixture::Payload( "smac", true );
		for ( const bool bad_map : { false, true } ) {
			auto reader = valid;
			types::Buffer rebuilt;
			rebuilt.WriteInt( reader.ReadInt() ); rebuilt.WriteString( reader.ReadString() ); rebuilt.WriteBool( reader.ReadBool() );
			rebuilt.WriteString( reader.ReadString() ); // complete resource profile
			rebuilt.WriteString( reader.ReadString() );
			rebuilt.WriteString( reader.ReadString() );
			rebuilt.WriteString( reader.ReadString() );
			rebuilt.WriteString( reader.ReadString() ); // faction startup profile
			rebuilt.WriteBool( reader.ReadBool() );
			rebuilt.WriteString( reader.ReadString() ); // predefined unit profile
			rebuilt.WriteBool( reader.ReadBool() ); rebuilt.WriteString( reader.ReadString() ); // movement profile
			rebuilt.WriteBool( reader.ReadBool() ); rebuilt.WriteString( reader.ReadString() ); // specialist profile
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
			GT_ASSERT( Rejects( [ & ]() { settings.Deserialize( payload ); } ), "invalid V11 settings accepted" );
			GT_ASSERT( settings.Serialize().ToString() == before, "invalid payload partially mutated settings" );
			settings.Deserialize( valid ); // Rejection left the destination unlocked/retryable.
			GT_ASSERT( fixture::Matches( settings ), "valid retry after rejected payload failed" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC faction host preparation stages all eight sources before publishing any profile", GT() {
		Settings settings; settings.global.SelectRuleset( "smac" );
		settings.global.game_name = "kept name"; settings.global.difficulty_level = -19;
		settings.global.map.filename = "kept/full/path"; settings.global.map.size_x = 90;
		const auto map = settings.global.map.Serialize().ToString();
		for ( const bool missing : { false, true } ) {
			std::vector< std::string > reads;
			GT_ASSERT( Rejects( [ & ]() { settings.PrepareRuleset( [ & ]( const std::string& filename ) {
				reads.push_back( filename );
				if ( filename == "peace.txt" ) {
					if ( missing ) { throw std::runtime_error( "seventh faction is missing" ); }
					return std::string( "#PEACE\nSynthetic header\nUNSUPPORTED,1\n#END\n" );
				}
				return fixture::Source( filename );
			} ); } ), "late faction source failure was accepted" );
			GT_ASSERT( reads == std::vector< std::string >( { "alpha.txt", "gaians.txt", "hive.txt", "univ.txt", "morgan.txt", "spartans.txt", "believe.txt", "peace.txt" } ), "host did not read exactly the selected canonical sources" );
			GT_ASSERT( !settings.global.IsRulesetLocked() && !settings.global.GetSmacResourceRules()
				&& !settings.global.GetSmacProductionRules() && !settings.global.GetSmacTechnologyRules() && !settings.global.GetSmacMovementRules()
				&& !settings.global.GetSmacWorldSizeRules() && !settings.global.GetSmacFactionStartupRules(), "failed late source published a profile" );
			GT_ASSERT( settings.global.GetRulesetId() == "smac" && settings.global.game_name == "kept name" && settings.global.difficulty_level == -19
				&& settings.global.map.filename == "kept/full/path" && settings.global.map.Serialize().ToString() == map, "failed preparation changed existing settings" );
		}
		settings.PrepareRuleset( fixture::Source );
		GT_ASSERT( fixture::Matches( settings.global ), "retry did not freeze all supplied profiles" );
		const auto& profile = *settings.global.GetSmacFactionStartupRules();
		GT_ASSERT( profile.Find( "GAIANS" )->NamedTechnologyReferences() == std::vector< int32_t >( { 2, 2, -1, -2 } )
			&& profile.Find( "GAIANS" )->ExtraTechnologyCount() == -3 && profile.Find( "GAIANS" )->Records().size() == 8,
			"transport preparation conflated named grants, numeric count or shared record budget" );
		settings.global.ResetRuleset();
		GT_ASSERT( !settings.global.GetSmacFactionStartupRules(), "reset retained old faction metadata" );
		settings.global.SelectRuleset( "smax" ); settings.PrepareRuleset( []( const std::string& ) -> std::string { throw std::runtime_error( "SMAX read a SMAC faction" ); } );
		GT_ASSERT( !settings.global.GetSmacFactionStartupRules(), "SMAX acquired inferred faction metadata" );
		GT_OK();
	} );
	task->AddTest( "SMAC frozen faction equality includes ordered raw records discarded bonuses and numeric metadata", GT() {
		GlobalSettings settings; settings.Deserialize( fixture::Payload( "smac", true ) );
		const auto before = settings.Serialize().ToString();
		const auto technologies = fixture::Technologies();
		const auto reject = [ & ]( const SmacFactionStartupRules& changed ) {
			return Rejects( [ & ]() { settings.Deserialize( fixture::Payload( "smac", true,
				{ 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, {}, {}, {}, changed.Serialize( technologies ).ToString() ) ); } )
				&& settings.Serialize().ToString() == before;
		};
		for ( size_t i = 0; i < 4; ++i ) {
			auto changed = fixture::Factions(); changed.initial_social_models[ i ].name += " changed";
			GT_ASSERT( reject( changed ), "locked model-zero source name changed" );
		}
		const std::array< const char*, 12 > changed_values = { "Tech3", "tech2 ", "Disable", "None", "--SUPPORT", "17", "POLICE", "2", "3", "+1suffix", "7", "23" };
		for ( size_t faction = 0; faction < 7; ++faction ) {
			for ( size_t index = 0; index < changed_values.size(); ++index ) {
				auto changed = fixture::Factions(); auto& bonus = changed.factions[ faction ].bonuses[ index ];
				bonus = SmacFactionStartupBonus::Parse( bonus.keyword, changed_values[ index ], technologies );
				GT_ASSERT( reject( changed ), "frozen raw/derived faction field changed" );
			}
			auto changed = fixture::Factions();
			std::swap( changed.factions[ faction ].bonuses[ 0 ], changed.factions[ faction ].bonuses[ 2 ] );
			GT_ASSERT( reject( changed ), "ordered named records were treated as a set" );
			changed = fixture::Factions(); changed.factions[ faction ].bonuses.pop_back();
			GT_ASSERT( reject( changed ), "omitted scalar metadata accepted" );
			changed = fixture::Factions(); changed.factions[ faction ].bonuses.back() = SmacFactionStartupBonus::Parse( "DRONE", "16", technologies );
			GT_ASSERT( reject( changed ), "changed scalar keyword/tag accepted" );
		}
		GlobalSettings copy; copy.Deserialize( types::Buffer( before ) );
		GT_ASSERT( copy.Serialize().ToString() == before, "configured faction metadata did not roundtrip exactly" );
		copy.game_name = "ordinary lobby update"; settings.Deserialize( copy.Serialize() );
		GT_ASSERT( settings.game_name == copy.game_name && *settings.GetSmacFactionStartupRules() == fixture::Factions(), "unchanged faction profile blocked ordinary lobby state" );
		GT_OK();
	} );
	task->AddTest( "SMAC late malformed faction payload preserves every existing setting and lifecycle lock", GT() {
		const auto profile = fixture::Factions(); const auto bytes = profile.Serialize( fixture::Technologies() ).ToString();
		std::vector< std::string > invalid = { "", bytes.substr( 0, bytes.size() - 1 ) };
		auto trailing = profile.Serialize( fixture::Technologies() ); trailing.WriteInt( 12 ); invalid.push_back( trailing.ToString() );
		for ( const int field : { 0, 1, 2, 3, 4, 5 } ) {
			auto changed = profile; auto& faction = changed.factions.back(); auto& bonus = faction.bonuses.back();
			if ( field == 0 ) { faction.port_id = "CUSTOM"; }
			if ( field == 1 ) { faction.filename = "custom.txt"; }
			if ( field == 2 ) { faction.section = "CUSTOM"; }
			if ( field == 3 ) { bonus.kind = SmacFactionStartupBonus::Kind::FANATIC; }
			if ( field == 4 ) { ++bonus.parameter; }
			if ( field == 5 ) { ++bonus.amount; }
			invalid.push_back( fixture::UncheckedFactions( changed ).ToString() );
		}
		for ( const bool locked : { false, true } ) {
			GlobalSettings settings;
			if ( locked ) { settings.Deserialize( fixture::Payload( "smac", true ) ); fixture::AcceptWorldSize( settings ); settings.LockWorldSize(); }
			else { settings.SelectRuleset( "smax" ); }
			settings.game_name = "keep"; settings.difficulty_level = -23; settings.map.filename = "keep/full/path";
			const auto before = settings.Serialize().ToString();
			for ( const auto& bad : invalid ) {
				GT_ASSERT( Rejects( [ & ]() { settings.Deserialize( fixture::Payload( "smac", true,
					{ 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, {}, {}, {}, bad ) ); } ), "malformed late faction record accepted" );
				GT_ASSERT( settings.Serialize().ToString() == before && settings.map.filename == "keep/full/path" && settings.IsRulesetLocked() == locked,
					"late faction failure changed settings or lock" );
			}
			if ( locked ) { GT_ASSERT( Rejects( [ & ]() { settings.SelectSmacWorldSize( 1 ); } ), "failed decode lost world-size lifecycle lock" ); }
		}
		GT_OK();
	} );
	task->AddTest( "SMAC movement host freezes the raw rule once and stages malformed sources", GT() {
		for ( const int32_t raw : { INT32_MIN, -1, 0, 1, 3, 100, 101, INT32_MAX } ) {
			Settings settings; settings.global.SelectRuleset( "smac" );
			size_t reads = 0;
			settings.PrepareRuleset( [ & ]( const std::string& filename ) {
				++reads; auto source = fixture::Source( filename );
				if ( filename == "alpha.txt" ) {
					const auto begin = source.find( "#RULES\n" ) + 7; const auto end = source.find( ',', begin );
					source.replace( begin, end - begin, std::to_string( raw ) );
				}
				return source;
			} );
			GT_ASSERT( reads == 8 && settings.global.GetSmacMovementRules() && settings.global.GetSmacMovementRules()->RawRoadRate() == raw,
				"host did not freeze the supplied raw movement rule" );
			GlobalSettings copy; copy.Deserialize( settings.global.Serialize() );
			GT_ASSERT( copy.GetSmacMovementRules() == settings.global.GetSmacMovementRules(), "movement transport lost raw value" );
			settings.PrepareRuleset( []( const std::string& ) -> std::string { throw std::runtime_error( "cached host reread source" ); } );
		}
		Settings settings; settings.global.SelectRuleset( "smac" ); settings.global.game_name = "keep";
		settings.global.map.filename = "keep/path"; settings.global.difficulty_level = -23;
		const auto map = settings.global.map.Serialize().ToString();
		for ( const auto* bad : { "2147483648", "3suffix", "" } ) {
			GT_ASSERT( Rejects( [ & ]() { settings.PrepareRuleset( [ & ]( const std::string& filename ) {
				auto source = fixture::Source( filename );
				if ( filename == "alpha.txt" ) { const auto begin = source.find( "#RULES\n" ) + 7; source.replace( begin, source.find( ',', begin ) - begin, bad ); }
				return source;
			} ); } ), "malformed movement source accepted" );
			GT_ASSERT( !settings.global.IsRulesetLocked() && !settings.global.GetSmacMovementRules() && !settings.global.GetSmacPredefinedUnitRules()
				&& !settings.global.GetSmacResourceRules() && settings.global.game_name == "keep" && settings.global.difficulty_level == -23
				&& settings.global.map.filename == "keep/path" && settings.global.map.Serialize().ToString() == map, "failed movement parse published partial settings" );
		}
		settings.PrepareRuleset( fixture::Source ); GT_ASSERT( fixture::Matches( settings.global ), "movement retry failed" ); GT_OK();
	} );
	task->AddTest( "SMAC movement settings reject oversized complete frames before nested decode", GT() {
		const auto payload = []( const std::string& bytes ) {
			return fixture::Payload( "smac", true, { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, {}, {}, {}, {}, {}, bytes );
		};
		const auto valid = fixture::Movement().Serialize().ToString();
		GT_ASSERT( valid.size() == SmacMovementRules::MAX_WIRE_BYTES, "valid movement frame is not at the cap" );
		for ( const bool locked : { false, true } ) {
			GlobalSettings settings; settings.SelectRuleset( "smax" );
			if ( locked ) { settings.Deserialize( payload( valid ) ); }
			const auto before = settings.Serialize().ToString();
			for ( const size_t size : { size_t( SmacMovementRules::MAX_WIRE_BYTES ) + 1, size_t( 4096 ) } ) {
				auto oversized = valid; oversized.append( size - valid.size(), 'x' );
				std::string error;
				try { settings.Deserialize( payload( oversized ) ); } catch ( const std::runtime_error& e ) { error = e.what(); }
				GT_ASSERT( error.find( "buffer read exceeds payload limit" ) != std::string::npos, "oversized movement reached nested settings decode" );
				GT_ASSERT( settings.Serialize().ToString() == before, "oversized movement partially published settings" );
			}
			settings.Deserialize( payload( valid ) );
			GT_ASSERT( settings.GetSmacMovementRules() == fixture::Movement(), "exact-cap movement did not publish" );
		}
		// Exact-cap malformed contents reach the nested validator, distinguishing
		// payload admission from version/checksum rejection after allocation.
		for ( const bool checksum : { false, true } ) {
			auto bytes = valid;
			if ( checksum ) { bytes.back() ^= 1; }
			else { types::Buffer version; version.WriteInt( 2 ); version.WriteInt( -1 ); bytes = version.ToString(); }
			std::string error; GlobalSettings settings;
			try { settings.Deserialize( payload( bytes ) ); } catch ( const std::runtime_error& e ) { error = e.what(); }
			GT_ASSERT( error.find( checksum ? "checksum mismatch" : "Unsupported SMAC movement profile version" ) != std::string::npos,
				"exact-cap malformed contents failed at the wrong boundary" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC movement settings reject same-Q raw changes and malformed late frames atomically", GT() {
		const auto payload = []( const std::string& bytes, const bool present = true, const std::string& id = "smac" ) {
			return fixture::Payload( id, id == "smac", { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, {}, {}, {}, {}, {}, bytes, present );
		};
		for ( const auto pair : { std::pair< int32_t, int32_t >{ -1, 0 }, { 100, 101 } } ) {
			GlobalSettings settings; settings.Deserialize( payload( SmacMovementRules( pair.first ).Serialize().ToString() ) );
			const auto before = settings.Serialize().ToString();
			GT_ASSERT( SmacMovementRules( pair.first ).Quantum() == SmacMovementRules( pair.second ).Quantum(), "test does not isolate raw equality" );
			GT_ASSERT( Rejects( [ & ]() { settings.Deserialize( payload( SmacMovementRules( pair.second ).Serialize().ToString() ) ); } )
				&& settings.Serialize().ToString() == before, "same-Q raw change bypassed frozen equality" );
		}
		const auto valid = fixture::Movement().Serialize().ToString();
		std::vector< types::Buffer > invalid = { payload( "", false ), payload( valid, true, "smax" ), payload( "" ), payload( valid.substr( 0, valid.size() - 1 ) ) };
		for ( const int64_t raw : { int64_t( INT32_MIN ) - 1, int64_t( INT32_MAX ) + 1 } ) { types::Buffer bad; bad.WriteInt( 1 ); bad.WriteInt( raw ); invalid.push_back( payload( bad.ToString() ) ); }
		types::Buffer wrong; wrong.WriteInt( 1 ); wrong.WriteString( "3" ); invalid.push_back( payload( wrong.ToString() ) );
		types::Buffer version; version.WriteInt( 2 ); version.WriteInt( 3 ); invalid.push_back( payload( version.ToString() ) );
		auto extra = fixture::Movement().Serialize(); extra.WriteInt( 3 ); invalid.push_back( payload( extra.ToString() ) );
		const auto complete = payload( valid ).ToString(); invalid.emplace_back( complete.substr( 0, complete.size() - 1 ) );
		for ( const bool locked : { false, true } ) {
			GlobalSettings settings;
			if ( locked ) { settings.Deserialize( payload( valid ) ); fixture::AcceptWorldSize( settings ); settings.LockWorldSize(); }
			else { settings.SelectRuleset( "smax" ); }
			settings.game_name = "keep"; settings.difficulty_level = -23; settings.map.filename = "keep/full/path";
			const auto before = settings.Serialize().ToString();
			for ( const auto& bad : invalid ) {
				GT_ASSERT( Rejects( [ & ]() { settings.Deserialize( bad ); } ), "invalid movement settings accepted" );
				GT_ASSERT( settings.Serialize().ToString() == before && settings.map.filename == "keep/full/path" && settings.IsRulesetLocked() == locked,
					"movement or late-tail failure partially changed settings" );
			}
			if ( locked ) { GT_ASSERT( Rejects( [ & ]() { settings.SelectSmacWorldSize( 1 ); } ), "movement failure lost world-size lock" ); }
		}
		GT_OK();
	} );

}

}
