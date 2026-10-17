#include "SmacFactionStartupRules.h"

#include "fixtures/SmacFactionStartupSource.h"
#include "loader/txt/FactionTXTLoader.h"
#include "task/gsetests/GSETests.h"

#include <functional>

namespace gse::tests {
namespace {
using namespace game::backend::settings;
using Kind = SmacFactionStartupBonus::Kind;
using loader::txt::FactionTXTLoader;
namespace fixture = smac_faction_startup_fixture;

bool Rejects( const std::function< void() >& action ) {
	try { action(); } catch ( const std::exception& ) { return true; }
	return false;
}

SmacFactionStartupRules Profile() { return FactionTXTLoader::ParseStartupRules( fixture::Alpha(), fixture::Source, fixture::Technologies() ); }

SmacFactionStartupRule Rule( const std::string& line ) {
	return { "GAIANS", "gaians.txt", "GAIANS", FactionTXTLoader::ParseStartupBonuses( fixture::Faction( "GAIANS", line ), "GAIANS", fixture::Technologies() ) };
}

types::Buffer Wire( const SmacFactionStartupRules& rules, size_t changed_number = SIZE_MAX, int64_t number_value = 0,
	size_t changed_text = SIZE_MAX, const std::string& text_value = "" ) {
	types::Buffer buffer;
	size_t numbers = 0, texts = 0;
	const auto number = [ & ]( int64_t value ) { buffer.WriteInt( numbers++ == changed_number ? number_value : value ); };
	const auto text = [ & ]( const std::string& value ) { buffer.WriteString( texts++ == changed_text ? text_value : value ); };
	number( 2 ); number( 4 );
	for ( const auto& model : rules.initial_social_models ) { text( model.name ); text( model.prerequisite ); text( model.modifiers ); }
	number( 7 );
	for ( const auto& row : rules.factions ) {
		text( row.port_id ); text( row.filename ); text( row.section ); number( row.bonuses.size() );
		for ( const auto& bonus : row.bonuses ) {
			text( bonus.keyword ); text( bonus.value ); number( static_cast< int32_t >( bonus.kind ) ); number( bonus.parameter ); number( bonus.amount );
		}
	}
	return buffer;
}
}

void AddSmacFactionStartupRulesTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC faction startup model-zero metadata owns positional neutral source rows", GT() {
		const auto catalog = fixture::Technologies();
		auto profile = Profile();
		for ( size_t i = 0; i < 4; ++i ) {
			const auto& model = profile.initial_social_models[ i ];
			GT_ASSERT( model.name == "Fixture model " + std::to_string( i ) && model.prerequisite == "None" && model.modifiers.empty(), "wrong source row or synthesized model metadata" );
		}
		profile.initial_social_models[ 3 ].name = std::string( SmacStartupSocialModel::MAX_NAME_BYTES, 'x' );
		const auto bytes = profile.Serialize( catalog ).ToString();
		const auto restored = SmacFactionStartupRules::Deserialize( types::Buffer( bytes ), catalog );
		GT_ASSERT( restored == profile && Wire( profile ).ToString() == bytes, "model metadata lost in codec" );
		profile.initial_social_models[ 3 ].name = "Changed";
		GT_ASSERT( !( restored == profile ) && restored.Serialize( catalog ).ToString() == bytes, "model metadata equality or owned lifetime lost" );
		GT_OK();
	} );
	task->AddTest( "SMAC faction startup neutral model admission rejects unsupported source and wire before publication", GT() {
		const auto catalog = fixture::Technologies(); const auto original = Profile(); auto retained = original;
		const auto parse_rejects = [ & ]( const std::string& alpha ) {
			size_t reads = 0;
			return Rejects( [ & ]() { retained = FactionTXTLoader::ParseStartupRules( alpha,
				[ & ]( const std::string& filename ) { ++reads; return fixture::Source( filename ); }, catalog ); } ) && reads == 0 && retained == original;
		};
		for ( const auto& replacement : std::vector< std::pair< std::string, std::string > >{
			{ "Politics,Economics", "Economics,Politics" }, { "Unused model,UnusedTech,+UNUSED\n", "" },
			{ "#FACTIONS", "Extra model,None,\n#FACTIONS" }, { "#FACTIONS", "#SOCIO\n#FACTIONS" }
		} ) {
			auto alpha = fixture::Alpha(); alpha.replace( alpha.find( replacement.first ), replacement.first.size(), replacement.second );
			GT_ASSERT( parse_rejects( alpha ), "ambiguous SOCIO layout reached faction source reads" );
		}
		for ( size_t i = 0; i < 4; ++i ) {
			const auto row = "Fixture model " + std::to_string( i ) + ",None,";
			for ( const auto& bad : { std::string( ",None," ), std::string( 257, 'x' ) + ",None,", std::string( "Name,Disable," ),
				std::string( "Name,None,+MORALE" ), std::string( "Name,None" ), std::string( "Name,None,," ) } ) {
				auto alpha = fixture::Alpha(); alpha.replace( alpha.find( row ), row.size(), bad );
				GT_ASSERT( parse_rejects( alpha ), "unsupported model row reached source reads" );
			}
			for ( size_t field = 0; field < 3; ++field ) {
				const auto bad = field == 0 ? std::string( 257, 'x' ) : field == 1 ? "Tech0" : "+MORALE";
				GT_ASSERT( Rejects( [ & ]() { retained = SmacFactionStartupRules::Deserialize( Wire( original, SIZE_MAX, 0, i * 3 + field, bad ), catalog ); } ) && retained == original,
					"unsupported model wire field published" );
			}
			GT_ASSERT( Rejects( [ & ]() { SmacFactionStartupRules::Deserialize( Wire( original, SIZE_MAX, 0, i * 3, std::string( "N\0ame", 5 ) ), catalog ); } ), "model name NUL accepted" );
		}
		for ( const int64_t count : { int64_t( -1 ), int64_t( 0 ), int64_t( 3 ), int64_t( 5 ), INT64_MAX } ) {
			GT_ASSERT( Rejects( [ & ]() { retained = SmacFactionStartupRules::Deserialize( Wire( original, 1, count ), catalog ); } ) && retained == original, "untrusted model count accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC faction startup canonical source mapping and ordered record projection", GT() {
		std::vector< std::string > reads;
		const auto profile = FactionTXTLoader::ParseStartupRules( fixture::Alpha(), [ & ]( const std::string& filename ) {
			reads.push_back( filename ); return fixture::Source( filename );
		}, fixture::Technologies() );
		const std::array< int32_t, 7 > ids = { 6, 43, 2, 1, 5, 4, 0 };
		const std::array< size_t, 7 > counts = { 6, 6, 4, 3, 4, 5, 3 };
		GT_ASSERT( reads.size() == 7, "source reader did not load seven faction files" );
		for ( size_t i = 0 ; i < 7 ; ++i ) {
			const auto& row = profile.factions[ i ]; const auto& binding = SmacFactionStartupRules::Bindings()[ i ];
			GT_ASSERT( reads[ i ] == binding.filename && row.port_id == binding.port_id && row.filename == binding.filename && row.section == binding.section, "source identity inferred from slot/display name" );
			GT_ASSERT( row.NamedTechnologyReferences() == std::vector< int32_t >{ ids[ i ] } && row.Records().size() == counts[ i ], "record list or named reference changed" );
			GT_ASSERT( row.ExtraTechnologyCount() == ( i == 2 ? 1 : 0 ), "numeric TECH became a technology ID" );
			GT_ASSERT( profile.Find( binding.port_id ) == &row, "port faction identity lookup failed" );
		}
		GT_ASSERT( !profile.Find( "UNIV" ) && !profile.Find( "University" ) && !profile.Find( "custom" ), "unsupported port identity guessed" );
		const auto hive = profile.factions[ 1 ].Records();
		GT_ASSERT( hive[ 1 ].kind == Kind::FACILITY && hive[ 1 ].parameter == 4 && hive[ 4 ].kind == Kind::SOCIAL && hive[ 4 ].parameter == 0 && hive[ 4 ].amount == -2
			&& hive[ 5 ].kind == Kind::IMMUNITY && hive[ 5 ].parameter == 1 && hive[ 5 ].amount == 1, "non-TECH records lost typed metadata" );
		GT_OK();
	} );
	task->AddTest( "SMAC faction startup shared eight-record budget preserves duplicates sentinels and numeric assignments", GT() {
		const auto duplicate = Rule( "TECH,Ecology,TECH,Ecology,TECH,None,TECH,Disable,TECH,2,TECH,1" );
		GT_ASSERT( duplicate.NamedTechnologyReferences() == std::vector< int32_t >( { 6, 6, -1, -2 } ) && duplicate.ExtraTechnologyCount() == 1, "duplicate/sentinel order or last numeric assignment changed" );
		GT_ASSERT( Rule( "TECH,-1" ).ExtraTechnologyCount() == -1 && Rule( "TECH,-1" ).Records().empty(), "negative numeric count was dropped or treated as named sentinel" );
		std::string full;
		for ( size_t i = 0 ; i < 8 ; ++i ) { full += "SOCIAL,+SUPPORT,"; }
		const auto limited = Rule( full + "TECH,Ecology,TECH,1" );
		GT_ASSERT( limited.bonuses.size() == 10 && limited.Records().size() == 8 && limited.NamedTechnologyReferences().empty() && limited.ExtraTechnologyCount() == 1, "shared budget or post-cap scalar assignment changed" );
		const auto mixed = Rule( "ENERGY,1,DRONE,2,TALENT,3,POPULATION,4,COMMERCE,5,FREEPROTO,0,FANATIC,0,"
			"FACILITY,4,IMMUNITY,EFFIC,FUNGNUTRIENT,1,VOTES,2,SOCIAL,+SUPPORT,TECH,None,TECH,Disable,TECH,Ecology,TECH,InfNet,TECH,-2" );
		GT_ASSERT( mixed.Records().size() == 8 && mixed.NamedTechnologyReferences() == std::vector< int32_t >( { -1, -2, 6 } ) && mixed.ExtraTechnologyCount() == -2, "scalar metadata consumed a record or record kind escaped budget" );
		GT_OK();
	} );
	task->AddTest( "SMAC faction startup decimal-prefix conversion preserves signed32 boundaries without numeric tech IDs", GT() {
		const auto catalog = fixture::Technologies();
		for ( const auto& test : std::vector< std::pair< std::string, int32_t > >{
			{ "2147483647tail", INT32_MAX }, { "-2147483648tail", INT32_MIN }, { " \t+00012suffix", 12 }, { "-0001", -1 }, { "1.5", 1 }, { "+3x", 3 }
		} ) {
			const auto result = SmacFactionStartupBonus::Parse( "TECH", test.first, catalog );
			GT_ASSERT( result.kind == Kind::EXTRA_TECH && result.parameter == test.second && result.value == test.first, "prefix metadata changed or source text lost" );
		}
		for ( const auto* bad : { "2147483648", "-2147483649", "9999999999999999999999999999999", "0", "-0", "+0", "0x10", "+", "--1", "Unknown", " Ecology", "Ecology\t" } ) {
			GT_ASSERT( Rejects( [ & ]() { SmacFactionStartupBonus::Parse( "TECH", bad, catalog ); } ), "overflow/unknown zero-conversion token accepted as a technology ID" );
		}
		GT_ASSERT( SmacFactionStartupBonus::Parse( "TECH", "eCoLoGy  ", catalog ).parameter == 6, "named lookup lost catalog token semantics" );
		// RE-SMAC-START-TECH-001: named startup references are not filtered by
		// ordinary production availability, disabled definitions or the 0..87 gate.
		auto disabled_catalog = catalog;
		for ( auto& row : disabled_catalog.technologies ) { row.prerequisites = { "Disable", "Disable" }; }
		for ( const auto& row : disabled_catalog.technologies ) {
			const auto bonus = SmacFactionStartupBonus::Parse( "TECH", row.key, disabled_catalog );
			GT_ASSERT( bonus.kind == Kind::NAMED_TECH && bonus.parameter == row.source_id, "named startup reference was filtered by availability or excluded Future Tech" );
		}
		for ( const auto* scalar : { "ENERGY", "DRONE", "TALENT", "POPULATION", "COMMERCE", "FREEPROTO", "FANATIC", "FACILITY", "FUNGNUTRIENT", "FUNGMINERALS", "FUNGENERGY", "VOTES" } ) {
			GT_ASSERT( SmacFactionStartupBonus::Parse( scalar, "-2147483648tail", catalog ).parameter == INT32_MIN, "numeric metadata boundary lost" );
			GT_ASSERT( Rejects( [ & ]() { SmacFactionStartupBonus::Parse( scalar, "2147483648", catalog ); } ), "numeric metadata overflow accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC faction startup file lexical policy preserves empty rows and rejects unsupported inputs", GT() {
		GT_ASSERT( Rule( "" ).bonuses.empty() && Rule( " \t" ).bonuses.empty(), "empty bonus row consumed ideology as bonus data" );
		const auto trimmed = Rule( " \tTECH , Ecology \t, TECH , 1prefix ; comment" );
		GT_ASSERT( trimmed.bonuses[ 0 ].value == "Ecology" && trimmed.bonuses[ 1 ].value == "1prefix" && trimmed.ExtraTechnologyCount() == 1, "declared lexical trimming failed" );
		for ( const auto* keyword : { "MORALE", "PSI", "RESEARCH", "INTEREST", "HURRY", "UNIT", "TECHCOST", "SHARETECH", "TERRAFORM", "ROBUST", "IMPUNITY", "PENALTY", "COMMFREQ", "MINDCONTROL", "UNKNOWN", "tech" } ) {
			GT_ASSERT( Rejects( [ & ]() { Rule( std::string( keyword ) + ",1" ); } ), "unsupported keyword skipped" );
			std::string full; for ( size_t i = 0 ; i < 8 ; ++i ) { full += "SOCIAL,+SUPPORT,"; }
			GT_ASSERT( Rejects( [ & ]() { Rule( full + keyword + ",1" ); } ), "unsupported post-cap keyword silently skipped" );
		}
		for ( const auto* line : { "TECH", "TECH,Ecology,", "TECH,,SOCIAL,+SUPPORT", ",1", "SOCIAL,+UNKNOWN", "IMMUNITY,NoAxis", "SOCIAL,SUP PORT" } ) {
			GT_ASSERT( Rejects( [ & ]() { Rule( line ); } ), "malformed bonus pair accepted" );
		}
		for ( const auto& source : { std::string(), std::string( "#GAIANS\nHeader\n#END\n" ), fixture::Faction( "OTHER", "" ), fixture::Faction( "GAIANS", "" ) + "#GAIANS\n", std::string( "#GAIANS\nHeader\nTECH,Ec" ) + '\0' + "ology\n" } ) {
			GT_ASSERT( Rejects( [ & ]() { FactionTXTLoader::ParseStartupBonuses( source, "GAIANS", fixture::Technologies() ); } ), "missing/duplicate/NUL section accepted" );
		}
		GT_ASSERT( Rule( "SOCIAL,+-SUPPORT,IMMUNITY,--EFFIC" ).bonuses[ 0 ].amount == 1, "zero sign sum default changed" );
		GT_OK();
	} );
	task->AddTest( "SMAC faction startup canonical lookup and roster validation precede file reads", GT() {
		for ( const auto& replacement : std::vector< std::pair< std::string, std::string > >{
			{ "TECH,MORALE", "MORALE,TECH" }, { "ECONOMY,EFFIC", "EFFIC,ECONOMY" }, { "UNIV,UNIV", "CUSTOM,UNIV" },
			{ "PEACE,PEACE", "PEACE,OTHER" }, { "HIVE,HIVE\n", "" }, { "#END\n", "HIVE,HIVE\n#END\n" }, { "#END\n", "#FACTIONS\n" }
		} ) {
			auto alpha = fixture::Alpha(); alpha.replace( alpha.find( replacement.first ), replacement.first.size(), replacement.second );
			size_t reads = 0;
			GT_ASSERT( Rejects( [ & ]() { FactionTXTLoader::ParseStartupRules( alpha, [ & ]( const std::string& filename ) { ++reads; return fixture::Source( filename ); }, fixture::Technologies() ); } ) && reads == 0, "invalid mapping opened a source or was accepted" );
		}
		auto retained = Profile(); const auto before = retained;
		for ( const bool missing : { false, true } ) {
			GT_ASSERT( Rejects( [ & ]() { retained = FactionTXTLoader::ParseStartupRules( fixture::Alpha(), [ & ]( const std::string& filename ) {
				if ( filename == "peace.txt" ) { if ( missing ) { throw std::runtime_error( "missing source" ); } return fixture::Faction( "PEACE", "TECH,Unknown" ); }
				return fixture::Source( filename );
			}, fixture::Technologies() ); } ) && retained == before, "late file failure published partial result" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC faction startup codec owns every raw and derived field including dropped records", GT() {
		const auto catalog = fixture::Technologies();
		auto rules = Profile();
		for ( size_t i = 0 ; i < 9 ; ++i ) { rules.factions[ 0 ].bonuses.push_back( SmacFactionStartupBonus::Parse( "TECH", "None", catalog ) ); }
		const auto bytes = rules.Serialize( catalog ).ToString();
		GT_ASSERT( Wire( rules ).ToString() == bytes && SmacFactionStartupRules::Deserialize( types::Buffer( bytes ), catalog ) == rules, "codec lost metadata/order" );
		for ( size_t row = 0 ; row < rules.factions.size() ; ++row ) {
			for ( size_t i = 0 ; i < rules.factions[ row ].bonuses.size() ; ++i ) {
				auto changed = rules;
				changed.factions[ row ].bonuses[ i ] = SmacFactionStartupBonus::Parse( "TECH", "2suffix", catalog );
				GT_ASSERT( !( changed == rules ) && SmacFactionStartupRules::Deserialize( changed.Serialize( catalog ), catalog ) == changed, "raw/derived differences hidden by projection" );
			}
		}
		auto changed = rules; changed.factions[ 0 ].bonuses[ 0 ].value = "eCoLoGy";
		GT_ASSERT( !( changed == rules ) && changed.factions[ 0 ].NamedTechnologyReferences() == rules.factions[ 0 ].NamedTechnologyReferences(), "raw token case ignored by equality" );
		const auto copied = SmacFactionStartupRules::Deserialize( types::Buffer( bytes ), catalog );
		rules.factions[ 0 ].bonuses.clear();
		GT_ASSERT( copied.Serialize( catalog ).ToString() == bytes, "decoded profile borrowed source storage" );
		for ( auto& row : rules.factions ) { row.bonuses.clear(); }
		GT_ASSERT( SmacFactionStartupRules::Deserialize( rules.Serialize( catalog ), catalog ) == rules, "empty faction bonus lists did not roundtrip" );
		GT_OK();
	} );
	task->AddTest( "SMAC faction startup codec rejects malformed early and late fields before publication", GT() {
		const auto catalog = fixture::Technologies(); const auto original = Profile(); auto target = original;
		const auto rejects = [ & ]( const types::Buffer& wire ) {
			try { target = SmacFactionStartupRules::Deserialize( wire, catalog ); } catch ( const std::runtime_error& ) { return target == original; }
			return false;
		};
		for ( const int64_t value : { int64_t( -1 ), int64_t( 0 ), int64_t( 1 ), int64_t( 3 ), INT64_MAX } ) { GT_ASSERT( rejects( Wire( original, 0, value ) ), "unknown version accepted" ); }
		for ( const int64_t value : { int64_t( -1 ), int64_t( 6 ), int64_t( 8 ), INT64_MAX } ) { GT_ASSERT( rejects( Wire( original, 2, value ) ), "wrong binding count accepted" ); }
		size_t number = 3, text = 12;
		for ( const auto& row : original.factions ) {
			for ( size_t i = 0 ; i < 3 ; ++i ) { GT_ASSERT( rejects( Wire( original, SIZE_MAX, 0, text + i, "Other" ) ), "binding identity modified" ); }
			text += 3;
			for ( const int64_t count : { int64_t( -1 ), INT64_MAX } ) { GT_ASSERT( rejects( Wire( original, number, count ) ), "invalid bonus count accepted" ); }
			++number;
			for ( const auto& bonus : row.bonuses ) {
				for ( const int64_t tag : { int64_t( -1 ), int64_t( 16 ), INT64_MAX } ) { GT_ASSERT( rejects( Wire( original, number, tag ) ), "invalid raw tag narrowed" ); }
				GT_ASSERT( rejects( Wire( original, number, ( static_cast< int32_t >( bonus.kind ) + 1 ) % 16 ) ), "valid tag disagrees with raw keyword/value" );
				for ( size_t field = 1 ; field < 3 ; ++field ) {
					for ( const int64_t value : { int64_t( INT32_MIN ) - 1, int64_t( INT32_MAX ) + 1 } ) { GT_ASSERT( rejects( Wire( original, number + field, value ) ), "integer narrowed" ); }
				}
				GT_ASSERT( rejects( Wire( original, number + 1, int64_t( bonus.parameter ) + 1 ) ), "derived parameter disagrees with source/catalog" );
				GT_ASSERT( rejects( Wire( original, number + 2, int64_t( bonus.amount ) + 1 ) ), "derived amount disagrees with source" );
				GT_ASSERT( rejects( Wire( original, SIZE_MAX, 0, text, "UNKNOWN" ) ) && rejects( Wire( original, SIZE_MAX, 0, text + 1, "" ) ), "bad late raw field accepted" );
				number += 3; text += 2;
			}
		}
		auto trailing = original.Serialize( catalog ); trailing.WriteInt( 0 ); GT_ASSERT( rejects( trailing ), "trailing payload accepted" );
		const auto bytes = original.Serialize( catalog ).ToString();
		for ( const auto length : { size_t( 0 ), size_t( 4 ), bytes.size() / 2, bytes.size() - 1 } ) { GT_ASSERT( rejects( types::Buffer( bytes.substr( 0, length ) ) ), "truncated payload accepted" ); }
		types::Buffer wrong_type; wrong_type.WriteInt( 2 ); wrong_type.WriteString( "7" ); GT_ASSERT( rejects( wrong_type ), "wrong type accepted" );
		auto changed_catalog = catalog; changed_catalog.technologies[ 6 ].key = "Other";
		GT_ASSERT( Rejects( [ & ]() { SmacFactionStartupRules::Deserialize( original.Serialize( catalog ), changed_catalog ); } ), "named references not bound to supplied catalog" );
		GT_OK();
	} );
}
}
