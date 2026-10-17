#include "SmacResourceRules.h"

#include "fixtures/SmacRulesSource.h"
#include "fixtures/SmacResourceText.h"
#include "fixtures/WorldSnapshotRules.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <vector>

namespace gse::tests {
namespace {
using namespace game::backend::settings;
using loader::txt::SmacRulesTXTLoader;
namespace fixture = smac_rules_fixture;

bool Rejects( const std::function< void() >& operation ) {
	try { operation(); } catch ( const std::exception& ) { return true; }
	return false;
}

std::string Source( const std::string& replacement = {}, size_t position = 0, size_t count = 70 ) {
	std::string result = "#RULES\n";
	for ( size_t row = 1; row <= count; ++row ) {
		result += row == position ? replacement : row == 2 ? "-2147483648" : row == 9 ? "+2147483647"
			: row == 10 ? "-7" : row == 6 ? "10" : row == 13 ? "7" : row == 14 ? "14" : row == 21 ? "37"
			: row == 54 ? "Tech8" : row == 55 ? "tEcH2" : row == 56 ? "None" : row == 58 ? "Disable" : "unused";
		result += ", ; strict RULES annotation\n";
	}
	result += "#RESOURCEINFO\n";
	for ( size_t row = 0; row < 9; ++row ) {
		result += "Repeated label," + std::to_string( row * 3 + 1 ) + "," + std::to_string( row * 3 + 2 ) + "," + std::to_string( row * 3 + 3 ) + ",ignored\n";
	}
	return result + "#END\n";
}

types::Buffer SettingsFrame( const std::string& resources ) {
	auto original = fixture::Payload( "smac", true );
	original.ReadInt(); original.ReadString(); original.ReadBool();
	const auto prefix = original.ToString().substr( 0, original.lenr );
	original.ReadString(); types::Buffer frame; frame.WriteString( resources );
	return types::Buffer( prefix + frame.ToString() + original.ToString().substr( original.lenr ) );
}

// Wire positions are independent of the profile's accessors/equality implementation.
types::Buffer IntegerFrame( const SmacResourceRules& profile, const size_t index, const int64_t value ) {
	auto original = fixture::UncheckedResources( profile );
	for ( size_t i = 0; i < index; ++i ) { original.ReadInt(); }
	const auto prefix = original.ToString().substr( 0, original.lenr );
	original.ReadInt(); types::Buffer integer; integer.WriteInt( value );
	return types::Buffer( prefix + integer.ToString() + original.ToString().substr( original.lenr ) );
}
}

void AddSmacResourceRulesTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC resource profile preserves all positional values and derives independent views", GT() {
		const auto catalog = fixture::Technologies();
		auto text = Source(); const auto profile = SmacRulesTXTLoader::ParseResourceRules( text, catalog );
		text.assign( 100, 'x' );
		for ( size_t row = 0; row < 9; ++row ) { for ( size_t col = 0; col < 3; ++col ) {
			GT_ASSERT( profile.rows[ row ][ col ] == static_cast< int32_t >( row * 3 + col + 1 ), "discarded/reordered RESOURCEINFO triple" );
		} }
		GT_ASSERT( profile.nutrients_per_citizen == INT32_MIN && profile.unroaded_mine_limit == INT32_MAX && profile.mine_nutrient_effect == -7, "RULES numeric positions or signed values changed" );
		GT_ASSERT( profile.nutrient_multiplier == 10 && profile.hab_complex_limit == 7 && profile.hab_dome_limit == 14 && profile.initial_energy == 37, "growth RULES positions changed" );
		const auto n = profile.TileNutrients(); const auto m = profile.TileMinerals(); const auto e = profile.TileEnergy(); const auto center = profile.BaseSquare();
		GT_ASSERT( n.ocean == 1 && n.bonus == 7 && n.forest == 10 && n.improved_land == 16 && n.improved_sea == 19 && n.monolith == 22 && n.borehole == 25 && n.mine_effect == -7, "nutrient view differs" );
		GT_ASSERT( m.ocean == 2 && m.bonus == 8 && m.forest == 11 && m.improved_sea == 20 && m.monolith == 23 && m.borehole == 26 && m.unroaded_mine_limit == INT32_MAX, "mineral view differs" );
		GT_ASSERT( e.ocean == 3 && e.bonus == 9 && e.forest == 12 && e.improved_sea == 21 && e.monolith == 24 && e.borehole == 27, "energy view differs" );
		const auto old = SmacRulesTXTLoader::ParseBaseSquareRules( Source() );
		GT_ASSERT( center.base_square == old.base_square && center.bonus_square == old.bonus_square && center.recycling == old.recycling, "compatibility projection changed" );
		const auto references = profile.TechnologyReferences( catalog );
		using Kind = SmacTechnologyReference::Kind;
		GT_ASSERT( references[ 0 ].technology_id == 8 && references[ 1 ].technology_id == 2 && references[ 2 ].kind == Kind::NONE && references[ 3 ].kind == Kind::DISABLED, "technology identity/sentinels changed" );
		GT_ASSERT( !references[ 2 ].technology_id && !references[ 3 ].technology_id && profile.technology_tokens[ 1 ] == "tEcH2", "token spelling or optional identity changed" );
		auto detached = profile; detached.rows[ 8 ][ 2 ] = -99; detached.technology_tokens[ 3 ] = "Tech0";
		GT_ASSERT( profile.rows[ 8 ][ 2 ] == 27 && profile.technology_tokens[ 3 ] == "Disable", "profile retained aliased values" );
		GT_OK();
	} );
	task->AddTest( "SMAC resource profile matches all nine literal original text rows", GT() {
		const auto catalog = fixture::Technologies();
		const auto rules = fixture::ProductionSource();
		size_t records = 0;
		for ( const auto& original : resource_text_fixture::cases ) {
			const auto profile = SmacRulesTXTLoader::ParseResourceRules( rules + original.text, catalog );
			GT_ASSERT( profile.rows == original.rows, "original RESOURCEINFO rows differ: " + std::string( original.id ) );
			const auto restored = SmacResourceRules::Deserialize( profile.Serialize( catalog ), catalog );
			GT_ASSERT( restored.rows == original.rows && restored == profile, "original rows changed through profile codec" );
			++records;
		}
		GT_ASSERT( records == 37, "literal original text fixture coverage changed" );
		GT_OK();
	} );
	task->AddTest( "SMAC resource RULES ingestion rejects unsupported lexical forms and unresolved tokens", GT() {
		const auto catalog = fixture::Technologies();
		for ( const auto position : { 2u, 6u, 9u, 10u, 13u, 14u, 21u } ) {
			for ( const auto& field : { std::string( "0x10" ), std::string( "2junk" ), std::string( "2147483648" ), std::string( "-2147483649" ), std::string( "1,," ), std::string(), std::string( "2\0x", 3 ) } ) {
				GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseResourceRules( Source( field, position ), catalog ); } ), "RULES silently used RESOURCEINFO prefix/wrap semantics" );
			}
		}
		for ( const auto position : { 54u, 55u, 56u, 58u } ) {
			for ( const auto& field : { std::string(), std::string( "Missing" ), std::string( "Tech0\0x", 7 ), std::string( 257, 'x' ), std::string( "Tech0,extra" ) } ) {
				GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseResourceRules( Source( field, position ), catalog ); } ), "invalid resource technology reference accepted" );
			}
		}
		for ( size_t count : { 0u, 69u, 71u } ) { GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseResourceRules( Source( {}, 0, count ), catalog ); } ), "wrong RULES row count accepted" ); }
		GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseResourceRules( Source() + "#RULES\n3\n", catalog ); } ), "duplicate RULES accepted" );
		auto crlf = Source(); for ( size_t i = 0; ( i = crlf.find( '\n', i ) ) != std::string::npos; i += 2 ) { crlf.insert( i, "\r" ); }
		GT_ASSERT( SmacRulesTXTLoader::ParseResourceRules( crlf, catalog ) == SmacRulesTXTLoader::ParseResourceRules( Source(), catalog ), "CRLF changed resource profile" );
		GT_OK();
	} );
	task->AddTest( "SMAC resource codec validates every field extent and exact text budget", GT() {
		const auto catalog = fixture::Technologies(); const auto profile = SmacRulesTXTLoader::ParseResourceRules( Source(), catalog );
		const auto wire = profile.Serialize( catalog ).ToString();
		GT_ASSERT( wire == fixture::UncheckedResources( profile ).ToString(), "resource wire order differs from independent builder" );
		GT_ASSERT( SmacResourceRules::Deserialize( types::Buffer( wire ), catalog ) == profile, "resource roundtrip changed owned authority" );
		for ( size_t prefix = 0; prefix < wire.size(); ++prefix ) { GT_ASSERT( Rejects( [ & ]() { SmacResourceRules::Deserialize( types::Buffer( wire.substr( 0, prefix ) ), catalog ); } ), "truncated profile accepted" ); }
		for ( size_t index = 1; index <= 34; ++index ) { for ( const auto value : { int64_t{ INT32_MIN } - 1, int64_t{ INT32_MAX } + 1 } ) {
			GT_ASSERT( Rejects( [ & ]() { SmacResourceRules::Deserialize( IntegerFrame( profile, index, value ), catalog ); } ), "out-of-range resource integer narrowed" );
		} }
		for ( const auto version : { 0, 1, 2, 4 } ) { GT_ASSERT( Rejects( [ & ]() { SmacResourceRules::Deserialize( IntegerFrame( profile, 0, version ), catalog ); } ), "old/unknown resource version accepted" ); }
		types::Buffer wrong; wrong.WriteString( "1" );
		GT_ASSERT( Rejects( [ & ]() { SmacResourceRules::Deserialize( wrong, catalog ); } ), "wrong resource field type accepted" );
		GT_ASSERT( Rejects( [ & ]() { SmacResourceRules::Deserialize( types::Buffer( wire + 'x' ), catalog ); } ), "trailing profile byte accepted" );
		auto maximum = profile;
		for ( auto& token : maximum.technology_tokens ) { token = "Tech0" + std::string( 251, ' ' ); }
		const auto exact = maximum.Serialize( catalog ).ToString();
		GT_ASSERT( SmacResourceRules::MAX_WIRE_BYTES == 1538 && exact.size() == 1538, "wire bound does not match exact admitted profile" );
		GT_ASSERT( SmacResourceRules::Deserialize( types::Buffer( exact ), catalog ) == maximum, "exact-cap semantic profile rejected" );
		maximum.technology_tokens[ 3 ] += ' ';
		GT_ASSERT( Rejects( [ & ]() { maximum.Serialize( catalog ); } ), "writer accepted over-budget token" );
		GT_ASSERT( Rejects( [ & ]() { SmacResourceRules::Deserialize( fixture::UncheckedResources( maximum ), catalog ); } ), "reader accepted total budget overflow" );
		maximum = profile; maximum.technology_tokens[ 3 ] = "Tech0" + std::string( 252, ' ' );
		GT_ASSERT( fixture::UncheckedResources( maximum ).lenw < SmacResourceRules::MAX_WIRE_BYTES, "individual token control accidentally exceeds aggregate budget" );
		GT_ASSERT( Rejects( [ & ]() { SmacResourceRules::Deserialize( fixture::UncheckedResources( maximum ), catalog ); } ), "reader omitted individual token bound" );
		GT_OK();
	} );
	task->AddTest( "SMAC resource settings and world compare all fields and restore exact bytes", GT() {
		GlobalSettings host; host.SelectRuleset( "smac" ); host.InitializeHostRules( fixture::Source ); fixture::AcceptWorldSize( host );
		const auto before = host.Serialize().ToString();
		GlobalSettings restored; restored.Deserialize( types::Buffer( before ) );
		GT_ASSERT( restored.Serialize().ToString() == before, "settings restore changed full profile bytes" );
		const auto profile = fixture::Resources();
		for ( size_t index = 0; index < 38; ++index ) {
			auto changed = profile;
			if ( index < 27 ) { auto& value = changed.rows[ index / 3 ][ index % 3 ]; value += value == INT32_MAX ? -1 : 1; }
			else if ( index == 27 ) { ++changed.nutrients_per_citizen; }
			else if ( index == 28 ) { ++changed.unroaded_mine_limit; }
			else if ( index == 29 ) { ++changed.mine_nutrient_effect; }
			else if ( index == 30 ) { ++changed.nutrient_multiplier; }
			else if ( index == 31 ) { ++changed.hab_complex_limit; }
			else if ( index == 32 ) { ++changed.hab_dome_limit; }
			else if ( index == 33 ) { ++changed.initial_energy; }
			else { changed.technology_tokens[ index - 34 ] = "tECH" + std::to_string( index - 34 ); }
			const auto bytes = changed.Serialize( fixture::Technologies() ).ToString();
			GT_ASSERT( !( changed == profile ), "resource equality omitted a field or normalized spelling" );
			GT_ASSERT( Rejects( [ & ]() { host.Deserialize( SettingsFrame( bytes ) ); } ), "locked settings accepted changed resource field" );
			GT_ASSERT( host.Serialize().ToString() == before, "rejected profile mutation changed settings" );
			auto header = world_rules_fixture::ResourceHeader( "smac", bytes );
			GT_ASSERT( Rejects( [ & ]() { game::backend::world_snapshot::ValidateHeader( header, host ); } ), "world accepted different resource field" );
		}
		types::Buffer world; game::backend::world_snapshot::WriteHeader( world, host );
		types::Buffer restored_world; game::backend::world_snapshot::WriteHeader( restored_world, restored );
		GT_ASSERT( world.ToString() == restored_world.ToString(), "restored world header changed profile bytes" );
		GT_OK();
	} );
	task->AddTest( "SMAC resource bounds precede outer copy and late host failures do not publish", GT() {
		GlobalSettings host; host.SelectRuleset( "smac" ); host.InitializeHostRules( fixture::Source ); fixture::AcceptWorldSize( host );
		const auto before = host.Serialize().ToString();
		auto header = world_rules_fixture::Header( "smac", true ); header.ReadInt(); header.ReadInt(); header.ReadString(); header.ReadBool();
		const auto prefix = header.ToString().substr( 0, header.lenr );
		for ( const bool corrupt : { false, true } ) {
			types::Buffer large; large.WriteString( std::string( SmacResourceRules::MAX_WIRE_BYTES + 1, 'x' ) );
			auto bytes = prefix + large.ToString(); if ( corrupt ) { bytes.back() ^= 1; }
			types::Buffer input( bytes ); std::string error;
			try { game::backend::world_snapshot::ValidateHeader( input, host ); } catch ( const std::runtime_error& e ) { error = e.what(); }
			GT_ASSERT( error.find( "buffer read exceeds payload limit" ) != std::string::npos && input.lenr == prefix.size(), "resource frame reached copying/checksum before bound" );
			GT_ASSERT( Rejects( [ & ]() { host.Deserialize( SettingsFrame( std::string( SmacResourceRules::MAX_WIRE_BYTES + 1, 'x' ) ) ); } ), "settings outer resource bound omitted" );
		}
		GT_ASSERT( host.Serialize().ToString() == before, "rejected resource frame changed settings" );
		GlobalSettings pending; pending.SelectRuleset( "smac" ); pending.game_name = "keep";
		GT_ASSERT( Rejects( [ & ]() { pending.InitializeHostRules( []( const std::string& filename ) {
			auto source = fixture::Source( filename ); if ( filename == "alpha.txt" ) { source.replace( source.find( "Tech3, ; synthetic annotation" ), 5, "Missing" ); } return source;
		} ); } ), "unknown late resource token accepted" );
		GT_ASSERT( !pending.IsRulesetLocked() && !pending.GetSmacResourceRules() && !pending.GetSmacTechnologyRules() && pending.game_name == "keep", "failed host parse published partial profiles" );
		pending.InitializeHostRules( fixture::Source ); GT_ASSERT( fixture::Matches( pending ), "failed resource parse prevented valid retry" );
		GT_OK();
	} );
}
}
