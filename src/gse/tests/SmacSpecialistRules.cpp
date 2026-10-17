#include "SmacSpecialistRules.h"

#include "fixtures/SmacSpecialistRules.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <utility>
#include <vector>

namespace gse::tests {
namespace {
using namespace game::backend::settings;
using loader::txt::SmacRulesTXTLoader;
namespace fixture = smac_specialist_fixture;
bool Rejects( const std::function< void() >& operation ) { try { operation(); } catch ( const std::exception& ) { return true; } return false; }

types::Buffer ReplaceInteger( const std::string& bytes, const size_t offset, const int64_t value ) {
	types::Buffer next; next.WriteInt( value );
	types::Buffer tail( bytes.substr( offset ) ); tail.ReadInt();
	return types::Buffer( bytes.substr( 0, offset ) + next.ToString() + bytes.substr( offset + tail.lenr ) );
}
}

void AddSmacSpecialistRulesTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC specialist catalog preserves positional metadata ownership and token distinctions", GT() {
		auto source = fixture::ProductionSource() + fixture::SpecialistSource(); auto actual = SmacRulesTXTLoader::ParseSpecialistRules( source );
		source.assign( 1000, 'x' ); const auto expected = fixture::Specialists();
		GT_ASSERT( actual == expected && actual.minimum_base_size == -3, "specialist fields or RULES row 11 changed" );
		const auto references = actual.TechnologyReferences( fixture::Technologies() ); using Kind = SmacTechnologyReference::Kind;
		GT_ASSERT( references[ 0 ][ 0 ].kind == Kind::NONE && references[ 1 ][ 0 ].kind == Kind::DISABLED
			&& references[ 2 ][ 0 ].kind == Kind::UNKNOWN && references[ 3 ][ 0 ].technology_id == 3
			&& references[ 6 ][ 1 ].kind == Kind::DISABLED, "technology reference or sentinel conflated" );
		auto copy = actual; copy.specialists.back().trailing_token = "different"; copy.specialists.front().singular = "renamed";
		GT_ASSERT( actual == expected && !( copy == actual ), "copied catalog aliases source or equality omits metadata" );
		auto moved = std::move( actual ); GT_ASSERT( moved == expected, "moved catalog lost owned fields" );
		GT_OK();
	} );
	task->AddTest( "SMAC specialist text ingestion rejects malformed shapes and every unsupported field", GT() {
		const auto original = fixture::ProductionSource() + fixture::SpecialistSource();
		const auto check = [ & ]( const std::string& old, const std::string& value ) {
			auto source = original; const auto at = source.find( old, old == "-3, ; synthetic annotation" ? 0 : source.find( "#CITIZENS" ) ); if ( at == std::string::npos ) { throw std::runtime_error( "bad replacement fixture" ); }
			source.replace( at, old.size(), value ); return Rejects( [ & ] { SmacRulesTXTLoader::ParseSpecialistRules( source ); } );
		};
		for ( const auto* token : { "Repeated singular", "Repeated plural", "None", "Tech0", "opaque0" } ) {
			for ( const auto& bad : { std::string(), std::string( 257, 'x' ), std::string( "x\0y", 3 ), std::string( "\"quoted\"" ) } ) {
				GT_ASSERT( check( token, bad ), "invalid specialist text admitted" );
			}
		}
		for ( const auto& bad : { "2147483648", "-2147483649", "0x10", "2junk", "+-2", "" } ) {
			GT_ASSERT( check( "-2147483648,0,0", std::string( bad ) + ",0,0" ), "bad economy admitted" );
			GT_ASSERT( check( "-2147483648,0,0", std::string( "-2147483648," ) + bad + ",0" ), "bad psych admitted" );
			GT_ASSERT( check( "-2147483648,0,0", std::string( "-2147483648,0," ) + bad ), "bad labs admitted" );
			GT_ASSERT( check( "-3, ; synthetic annotation", std::string( bad ) + ", ; synthetic annotation" ), "bad minimum size admitted" );
		}
		GT_ASSERT( check( "#CITIZENS", "#OTHER" ) && check( "opaque0", "opaque0,extra" ), "missing section or extra field accepted" );
		GT_ASSERT( check( "Ordinary A,Ordinary plural A,", "Only one" ), "ordinary context not validated" );
		GT_ASSERT( check( "Ordinary C,Ordinary plural C,\n", "" ), "missing ordinary context accepted" );
		GT_ASSERT( Rejects( [ & ] { SmacRulesTXTLoader::ParseSpecialistRules( original + fixture::SpecialistSource() ); } ), "duplicate section accepted" );
		auto crlf = original; for ( size_t i = 0; ( i = crlf.find( '\n', i ) ) != std::string::npos; i += 2 ) { crlf.insert( i, "\r" ); }
		GT_ASSERT( SmacRulesTXTLoader::ParseSpecialistRules( crlf ) == fixture::Specialists(), "CRLF changed specialist metadata" );
		GT_OK();
	} );
	task->AddTest( "SMAC specialist codec bounds every field before narrowing and preserves exact budget", GT() {
		const auto profile = fixture::Specialists(); const auto wire = profile.Serialize().ToString();
		GT_ASSERT( wire == fixture::UncheckedSpecialists( profile ).ToString() && SmacSpecialistRules::Deserialize( types::Buffer( wire ) ) == profile, "independent wire order or roundtrip differs" );
		for ( size_t end = 0; end < wire.size(); ++end ) { GT_ASSERT( Rejects( [ & ] { SmacSpecialistRules::Deserialize( types::Buffer( wire.substr( 0, end ) ) ); } ), "truncated specialist profile accepted" ); }
		types::Buffer fields( wire ); std::vector< size_t > integers;
		integers.push_back( fields.lenr ); fields.ReadInt(); integers.push_back( fields.lenr ); fields.ReadInt(); integers.push_back( fields.lenr ); fields.ReadInt();
		for ( size_t row = 0; row < 7; ++row ) {
			integers.push_back( fields.lenr ); fields.ReadInt(); for ( size_t field = 0; field < 4; ++field ) { fields.ReadString(); }
			for ( size_t field = 0; field < 3; ++field ) { integers.push_back( fields.lenr ); fields.ReadInt(); } fields.ReadString();
		}
		for ( const auto offset : integers ) { for ( const auto value : { int64_t( INT32_MIN ) - 1, int64_t( INT32_MAX ) + 1 } ) {
			GT_ASSERT( Rejects( [ & ] { SmacSpecialistRules::Deserialize( ReplaceInteger( wire, offset, value ) ); } ), "wide specialist integer narrowed" );
		} }
		for ( const auto version : { 0, 2 } ) { GT_ASSERT( Rejects( [ & ] { SmacSpecialistRules::Deserialize( ReplaceInteger( wire, 0, version ) ); } ), "unsupported version accepted" ); }
		for ( const auto count : { -1, 0, 6, 8 } ) { GT_ASSERT( Rejects( [ & ] { SmacSpecialistRules::Deserialize( ReplaceInteger( wire, integers[ 2 ], count ) ); } ), "wrong specialist count accepted" ); }
		GT_ASSERT( Rejects( [ & ] { SmacSpecialistRules::Deserialize( types::Buffer( wire + 'x' ) ); } ), "trailing byte accepted" );
		auto maximum = profile;
		for ( auto& row : maximum.specialists ) { for ( auto* text : { &row.singular, &row.plural, &row.prerequisite, &row.obsolete, &row.trailing_token } ) { *text = std::string( 256, 'x' ); } }
		const auto exact = maximum.Serialize().ToString();
		GT_ASSERT( exact.size() == 9604 && SmacSpecialistRules::MAX_WIRE_BYTES == 9604 && SmacSpecialistRules::Deserialize( types::Buffer( exact ) ) == maximum, "exact maximum profile rejected or bound miscomputed" );
		maximum.specialists.back().trailing_token += 'x';
		GT_ASSERT( Rejects( [ & ] { maximum.Serialize(); } ) && Rejects( [ & ] { SmacSpecialistRules::Deserialize( fixture::UncheckedSpecialists( maximum ) ); } ), "over-budget specialist profile accepted" );
		auto bad = profile; bad.specialists.back().obsolete.assign( 257, 'x' );
		GT_ASSERT( fixture::UncheckedSpecialists( bad ).lenw < SmacSpecialistRules::MAX_WIRE_BYTES && Rejects( [ & ] { SmacSpecialistRules::Deserialize( fixture::UncheckedSpecialists( bad ) ); } ), "individual text budget omitted" );
		for ( size_t i = 0; i < 7; ++i ) { bad = profile; bad.specialists[ i ].source_id = 7; GT_ASSERT( Rejects( [ & ] { SmacSpecialistRules::Deserialize( fixture::UncheckedSpecialists( bad ) ); } ), "unordered specialist identity accepted" ); }
		GT_OK();
	} );
}
}
