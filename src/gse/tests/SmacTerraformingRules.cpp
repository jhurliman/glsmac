#include "SmacTerraformingRules.h"

#include "fixtures/SmacRulesSource.h"
#include "fixtures/WorldSnapshotRules.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "task/gsetests/GSETests.h"

#include <functional>

namespace gse::tests {
namespace {
using namespace game::backend::settings;
using loader::txt::SmacRulesTXTLoader;
namespace fixture = terraforming_rules_fixture;
bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }
int32_t Identity( const SmacTechnologyReference& ref ) {
	using Kind = SmacTechnologyReference::Kind;
	return ref.kind == Kind::NONE ? -1 : ref.kind == Kind::DISABLED ? -2 : ref.kind == Kind::TECHNOLOGY_ID ? *ref.technology_id : -3;
}
size_t ProfileOffset( types::Buffer& wire, bool world ) {
	wire.ReadInt(); if ( world ) { wire.ReadInt(); } wire.ReadString();
	if ( world ) {
		for ( size_t i = 0; i < 9; ++i ) { if ( wire.ReadBool() ) { wire.ReadString(); if ( i == 3 ) { wire.ReadString(); } } }
	} else {
		if ( wire.ReadBool() ) { for ( size_t i = 0; i < 5; ++i ) { wire.ReadString(); } }
		for ( size_t i = 0; i < 4; ++i ) { if ( wire.ReadBool() ) { wire.ReadString(); } }
	}
	return wire.lenr;
}
types::Buffer ProfileFrame( bool world, const std::string& bytes, bool present = true ) {
	auto wire = world ? world_rules_fixture::Header() : smac_rules_fixture::Payload( "smac", true );
	const auto begin = ProfileOffset( wire, world ); wire.ReadBool(); wire.ReadString();
	types::Buffer tail; tail.WriteBool( present ); if ( present ) { tail.WriteString( bytes ); }
	return types::Buffer( wire.ToString().substr( 0, begin ) + tail.ToString() + wire.ToString().substr( wire.lenr ) );
}
}

void AddSmacTerraformingRulesTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC terraforming catalog preserves twenty captured time and prerequisite projections", GT() {
		auto source = fixture::Source(); const auto parsed = SmacRulesTXTLoader::ParseTerraformingRules( source ); source.assign( 1000, 'x' );
		GT_ASSERT( parsed == fixture::Profile(), "positional source metadata differs" );
		const auto refs = parsed.TechnologyReferences( smac_rules_fixture::Technologies() );
		for ( size_t i = 0; i < 20; ++i ) {
			GT_ASSERT( parsed.actions[ i ].source_id == static_cast< int32_t >( i ) && parsed.actions[ i ].work_time == fixture::times[ i ]
				&& Identity( refs[ i ][ 0 ] ) == fixture::land[ i ] && Identity( refs[ i ][ 1 ] ) == fixture::sea[ i ], "captured active TERRAIN projection differs" );
		}
		auto changed = parsed; changed.actions[ 0 ].land_prerequisite = "Missing"; changed.actions[ 5 ].work_time = INT32_MIN;
		changed.actions[ 19 ].work_time = INT32_MAX;
		GT_ASSERT( changed.TechnologyReferences( smac_rules_fixture::Technologies() )[ 0 ][ 0 ].kind == SmacTechnologyReference::Kind::UNKNOWN
			&& SmacTerraformingRules::Deserialize( changed.Serialize() ) == changed && parsed == fixture::Profile(), "raw bounds, unknown token or detached ownership lost" );
		GT_OK();
	} );
	task->AddTest( "SMAC terraforming parser rejects unsupported text without changing other sections", GT() {
		const auto original = fixture::Source();
		const auto rejects = [ & ]( const std::string& old, const std::string& replacement ) {
			auto source = original; const auto at = source.find( old ); if ( at == std::string::npos ) { throw std::runtime_error( "bad TERRAIN test replacement" ); }
			source.replace( at, old.size(), replacement ); return Rejects( [ & ] { SmacRulesTXTLoader::ParseTerraformingRules( source ); } );
		};
		for ( const auto& bad : { "2147483648", "-2147483649", "4junk", "0x4", "" } ) {
			GT_ASSERT( rejects( ",4,Perform 0", std::string( "," ) + bad + ",Perform 0" ), "unsupported work-time syntax admitted" );
		}
		for ( const auto& bad : { std::string(), std::string( 257, 'x' ), std::string( "x\0y", 3 ), std::string( "\"quoted\"" ) } ) {
			GT_ASSERT( rejects( "Land 0", bad ), "bad fixed metadata admitted" );
		}
		GT_ASSERT( rejects( "#TERRAIN", "#OTHER" ) && rejects( "opaque map annotation", "a,b" )
			&& rejects( "Land 19,Disable,Sea 19,Disable,8,Perform 19,x,Shift+X,opaque map annotation\n", "" ), "section shape admitted" );
		GT_ASSERT( Rejects( [ & ] { SmacRulesTXTLoader::ParseTerraformingRules( original + original ); } ), "duplicate section admitted" );
		const auto full = smac_rules_fixture::Source( "alpha.txt" );
		GT_ASSERT( SmacRulesTXTLoader::ParseMovementRules( full ) == smac_rules_fixture::Movement()
			&& SmacRulesTXTLoader::ParseSpecialistRules( full ) == smac_rules_fixture::Specialists(), "TERRAIN affected unrelated parser" );
		GT_OK();
	} );
	task->AddTest( "SMAC terraforming codec bounds ordered rows and late metadata", GT() {
		const auto profile = fixture::Profile(); const auto bytes = fixture::Wire( profile ).ToString();
		GT_ASSERT( profile.Serialize().ToString() == bytes && SmacTerraformingRules::Deserialize( types::Buffer( bytes ) ) == profile, "independent codec projection differs" );
		for ( const auto length : { size_t( 0 ), size_t( 14 ), size_t( 28 ), bytes.size() - 1 } ) {
			GT_ASSERT( Rejects( [ & ] { SmacTerraformingRules::Deserialize( types::Buffer( bytes.substr( 0, length ) ) ); } ), "truncated profile admitted" );
		}
		auto bad = profile; bad.actions[ 19 ].source_id = 0;
		GT_ASSERT( Rejects( [ & ] { SmacTerraformingRules::Deserialize( fixture::Wire( bad ) ); } ), "late positional mismatch admitted" );
		bad = profile; bad.actions[ 19 ].keystroke.assign( 257, 'x' );
		GT_ASSERT( Rejects( [ & ] { SmacTerraformingRules::Deserialize( fixture::Wire( bad ) ); } ), "late string bound omitted" );
		types::Buffer cursor( bytes ); cursor.ReadInt(); cursor.ReadInt(); cursor.ReadInt();
		for ( size_t i = 0; i < 4; ++i ) { cursor.ReadString(); }
		const auto offset = cursor.lenr; cursor.ReadInt(); types::Buffer wide; wide.WriteInt( int64_t( INT32_MAX ) + 1 );
		GT_ASSERT( Rejects( [ & ] { SmacTerraformingRules::Deserialize( types::Buffer( bytes.substr( 0, offset ) + wide.ToString() + bytes.substr( cursor.lenr ) ) ); } ), "wide work time narrowed" );
		GT_ASSERT( Rejects( [ & ] { SmacTerraformingRules::Deserialize( types::Buffer( bytes + 'x' ) ); } ), "trailing field admitted" );
		auto maximum = profile;
		for ( auto& row : maximum.actions ) { for ( auto* text : { &row.land_name, &row.land_prerequisite, &row.sea_name,
			&row.sea_prerequisite, &row.order_text, &row.order_letter, &row.keystroke, &row.trailing_annotation } ) { text->assign( 256, 'x' ); } }
		GT_ASSERT( maximum.Serialize().lenw == 42508 && SmacTerraformingRules::MAX_WIRE_BYTES == 42508
			&& SmacTerraformingRules::Deserialize( maximum.Serialize() ) == maximum, "exact frame budget differs" );
		GT_OK();
	} );
	task->AddTest( "SMAC terraforming host freeze and world compare every saved field atomically", GT() {
		GlobalSettings host; host.SelectRuleset( "smac" ); size_t alpha_reads = 0;
		host.InitializeHostRules( [ & ]( const std::string& name ) { if ( name == "alpha.txt" ) { ++alpha_reads; } return smac_rules_fixture::Source( name ); } );
		smac_rules_fixture::AcceptWorldSize( host ); const auto before = host.Serialize().ToString();
		GlobalSettings restored; restored.Deserialize( types::Buffer( before ) );
		GT_ASSERT( alpha_reads == 1 && restored.Serialize().ToString() == before && *restored.GetSmacTerraformingRules() == fixture::Profile(), "single-source freeze or restore differs" );
		for ( size_t field = 0; field < 9; ++field ) {
			auto changed = fixture::Profile(); auto& row = changed.actions.back();
			switch ( field ) {
				case 0: row.land_name += " changed"; break; case 1: row.land_prerequisite = "None"; break;
				case 2: row.sea_name += " changed"; break; case 3: row.sea_prerequisite = "None"; break;
				case 4: ++row.work_time; break; case 5: row.order_text += " changed"; break;
				case 6: row.order_letter = "y"; break; case 7: row.keystroke = "Y"; break; case 8: row.trailing_annotation += " changed"; break;
			}
			GT_ASSERT( Rejects( [ & ] { host.Deserialize( ProfileFrame( false, changed.Serialize().ToString() ) ); } ), "locked settings accepted changed TERRAIN field" );
			auto world = ProfileFrame( true, changed.Serialize().ToString() );
			GT_ASSERT( Rejects( [ & ] { game::backend::world_snapshot::ValidateHeader( world, host ); } ) && host.Serialize().ToString() == before, "world accepted mismatch or changed frozen authority" );
		}
		types::Buffer world; game::backend::world_snapshot::WriteHeader( world, host ); const auto body = world.lenw; world.WriteString( "body" );
		game::backend::world_snapshot::ValidateHeader( world, restored );
		GT_ASSERT( world.lenr == body && world.ReadString() == "body", "world profile consumed body" );
		GlobalSettings failed; failed.SelectRuleset( "smac" ); failed.game_name = "keep";
		GT_ASSERT( Rejects( [ & ] { failed.InitializeHostRules( [ & ]( const std::string& name ) { auto source = smac_rules_fixture::Source( name ); if ( name == "alpha.txt" ) { source.replace( source.find( "#TERRAIN" ), 8, "#MISSING" ); } return source; } ); } ), "missing source admitted" );
		GT_ASSERT( !failed.IsRulesetLocked() && !failed.GetSmacTerraformingRules() && !failed.GetSmacResourceRules() && failed.game_name == "keep", "host failure published a partial catalog" );
		GT_OK();
	} );
	task->AddTest( "SMAC terraforming transport rejects outer length before nested decode", GT() {
		GlobalSettings host; host.SelectRuleset( "smac" ); host.InitializeHostRules( smac_rules_fixture::Source ); smac_rules_fixture::AcceptWorldSize( host );
		const auto before = host.Serialize().ToString();
		for ( bool world_path : { false, true } ) {
			auto input = ProfileFrame( world_path, std::string( SmacTerraformingRules::MAX_WIRE_BYTES + 1, 'x' ) );
			auto cursor = input; const auto prefix = ProfileOffset( cursor, world_path ); cursor.ReadBool(); const auto payload = cursor.lenr;
			std::string error;
			try { if ( world_path ) { game::backend::world_snapshot::ValidateHeader( input, host ); } else { host.Deserialize( input ); } }
			catch ( const std::runtime_error& value ) { error = value.what(); }
			GT_ASSERT( ( error.find( "buffer read exceeds payload limit" ) != std::string::npos || ( !world_path && error.find( "network packet budget" ) != std::string::npos ) ) && ( !world_path || input.lenr == payload ) && prefix < payload,
				"oversized frame reached nested decoder or changed cursor" );
			GT_ASSERT( Rejects( [ & ] { auto missing = ProfileFrame( world_path, "", false ); if ( world_path ) { game::backend::world_snapshot::ValidateHeader( missing, host ); } else { host.Deserialize( missing ); } } ), "SMAC missing catalog admitted" );
		}
		GT_ASSERT( host.Serialize().ToString() == before, "transport failure changed host" );
		GlobalSettings smax; smax.SelectRuleset( "smax" ); smax.InitializeHostRules( []( const std::string& ) -> std::string { throw std::runtime_error( "SMAX read alpha" ); } );
		GT_ASSERT( !smax.GetSmacTerraformingRules(), "SMAX inferred TERRAIN profile" );
		GT_OK();
	} );
}
}
