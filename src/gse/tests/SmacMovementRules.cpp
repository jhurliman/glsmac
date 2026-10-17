#include "SmacMovementRules.h"

#include "loader/txt/SmacRulesTXTLoader.h"
#include "loader/txt/UnitRulesTXTLoader.h"
#include "task/gsetests/GSETests.h"

#include <array>
#include <functional>
#include <stdexcept>

namespace gse::tests {
namespace {
using game::backend::settings::SmacMovementRules;
using loader::txt::SmacRulesTXTLoader;
using Chassis = SmacMovementRules::GroundChassis;

std::string Source( const std::string& first, size_t count = 70 ) {
	std::string source = "## translation note\n#RULES\n";
	if ( count ) { source += first + "\n"; }
	for ( size_t i = 1; i < count; ++i ) { source += "unused metadata\n"; }
	return source + "#END\n";
}

bool Rejects( const std::function< void() >& operation ) {
	try { operation(); } catch ( const std::exception& ) { return true; }
	return false;
}

types::Buffer Wire( int64_t version, int64_t raw ) {
	types::Buffer buffer; buffer.WriteInt( version ); buffer.WriteInt( raw ); return buffer;
}
}

void AddSmacMovementRulesTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC movement rules preserve raw rate and derive one clamped quantum", GT() {
		const std::array< std::array< int32_t, 2 >, 11 > cases{{
			{ INT32_MIN, 1 }, { -1, 1 }, { 0, 1 }, { 1, 1 }, { 2, 2 }, { 3, 3 },
			{ 99, 99 }, { 100, 100 }, { 101, 100 }, { 256, 100 }, { INT32_MAX, 100 }
		}};
		for ( const auto& row : cases ) {
			const auto profile = SmacRulesTXTLoader::ParseMovementRules( Source( std::to_string( row[ 0 ] ) + ", ; misleading label" ) );
			GT_ASSERT( profile.RawRoadRate() == row[ 0 ] && profile.Quantum() == row[ 1 ], "raw/derived road rate differs" );
			const auto restored = SmacMovementRules::Deserialize( profile.Serialize() );
			GT_ASSERT( restored == profile && restored.Quantum() == row[ 1 ], "movement profile roundtrip changes authority" );
		}
		GT_ASSERT( !( SmacMovementRules( -1 ) == SmacMovementRules( 0 ) ), "equality lost distinct raw values with equal Q" );
		GT_ASSERT( !( SmacMovementRules( 100 ) == SmacMovementRules( 101 ) ), "upper clamped raw metadata collapsed" );
		auto source = Source( "+0003, ; arbitrary annotation" );
		for ( size_t at = 0; ( at = source.find( '\n', at ) ) != std::string::npos; at += 2 ) { source.insert( at, "\r" ); }
		GT_ASSERT( SmacRulesTXTLoader::ParseMovementRules( source ).RawRoadRate() == 3, "CRLF/sign/comment ingestion differs" );
		GT_OK();
	} );
	task->AddTest( "SMAC movement parser rejects malformed positional input without defaults", GT() {
		for ( const auto& source : { std::string(), Source( "3", 0 ), Source( "3", 69 ), Source( "3", 71 ), Source( "3" ) + "#RULES\n3\n" } ) {
			GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseMovementRules( source ); } ), "invalid RULES frame accepted" );
		}
		for ( const auto& value : { std::string(), std::string( "1x" ), std::string( "1.0" ), std::string( "0x3" ), std::string( "+-1" ),
			std::string( "+" ), std::string( "2147483648" ), std::string( "-2147483649" ), std::string( "3,," ), std::string( "3\0x", 3 ) } ) {
			GT_ASSERT( Rejects( [ & ]() { SmacRulesTXTLoader::ParseMovementRules( Source( value ) ); } ), "invalid first integer accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC movement profile codec rejects wrong types bounds versions and trailing data", GT() {
		const auto bytes = SmacMovementRules( -123 ).Serialize().ToString();
		GT_ASSERT( SmacMovementRules::MAX_WIRE_BYTES == 28 && bytes.size() == SmacMovementRules::MAX_WIRE_BYTES, "movement frame budget differs from its two integer frames" );
		GT_ASSERT( bytes == Wire( 1, -123 ).ToString(), "movement wire schema changed" );
		for ( size_t length = 0; length < bytes.size(); ++length ) {
			GT_ASSERT( Rejects( [ & ]() { SmacMovementRules::Deserialize( types::Buffer( bytes.substr( 0, length ) ) ); } ), "truncated movement profile accepted" );
		}
		for ( int64_t version : { int64_t{ 0 }, int64_t{ 2 }, INT64_MIN, INT64_MAX } ) {
			GT_ASSERT( Rejects( [ & ]() { SmacMovementRules::Deserialize( Wire( version, 3 ) ); } ), "unknown movement version accepted" );
		}
		for ( int64_t raw : { int64_t{ INT32_MIN } - 1, int64_t{ INT32_MAX } + 1, INT64_MIN, INT64_MAX } ) {
			GT_ASSERT( Rejects( [ & ]() { SmacMovementRules::Deserialize( Wire( 1, raw ) ); } ), "movement raw value narrowed" );
		}
		types::Buffer wrong_type; wrong_type.WriteInt( 1 ); wrong_type.WriteString( "3" );
		GT_ASSERT( Rejects( [ & ]() { SmacMovementRules::Deserialize( wrong_type ); } ), "wrong movement type accepted" );
		GT_ASSERT( Rejects( [ & ]() { SmacMovementRules::Deserialize( types::Buffer( bytes + 'x' ) ); } ), "trailing movement byte accepted" );
		GT_OK();
	} );
	task->AddTest( "SMAC movement basic allowance consumes parsed Moves byte independently of Cost", GT() {
		// Independent literal expected results at Q3; inputs include modulo and clamp boundaries.
		const std::array< std::array< int32_t, 2 >, 12 > cases{{
			{ INT32_MIN, 3 }, { -1, 297 }, { 0, 3 }, { 1, 3 }, { 2, 6 }, { 3, 9 },
			{ 99, 297 }, { 100, 297 }, { 255, 297 }, { 256, 3 }, { 257, 3 }, { INT32_MAX, 297 }
		}};
		const auto rules = SmacRulesTXTLoader::ParseMovementRules( Source( "3" ) );
		for ( const auto chassis : { Chassis::Infantry, Chassis::Speeder } ) {
			for ( const auto& row : cases ) {
				const auto source = "#CHASSIS\nSynthetic,M1,Plural,M2,Synthetic,F1,Plural,F2," + std::to_string( row[ 0 ] )
					+ ",0,0,0,0,17,None,Large,N1,LargePlural,N2\n#END\n";
				const auto table = loader::txt::UnitRulesTXTLoader::ParseChassis( source, "smac" );
				const auto& component = table.chassis.at( 0 );
				GT_ASSERT( component.speed == row[ 0 ] && component.cost == 17, "Moves/Cost metadata source changed" );
				GT_ASSERT( rules.OrdinaryGroundAllowance( chassis, component.speed ) == row[ 1 ], "Moves byte store/clamp/order differs" );
			}
		}
		GT_ASSERT( SmacMovementRules( INT32_MAX ).OrdinaryGroundAllowance( Chassis::Speeder, -1 ) == 9900, "maximum basic result differs" );
		GT_ASSERT( Rejects( [ & ]() { rules.OrdinaryGroundAllowance( static_cast< Chassis >( 2 ), 1 ); } ), "unsupported chassis admitted" );
		GT_OK();
	} );
}
}
