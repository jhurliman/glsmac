#include "SmacSimulationState.h"

#include "game/backend/faction/SmacSimulationState.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace game::backend;
using faction::SmacSimulationState;

SmacSimulationState State() {
	SmacSimulationState value;
	value.roster = { { 7, 0, "controller-one", roster::Role::Host, "GAIANS" }, { 2, 6, "controller-two", roster::Role::Player, "UNIVERSITY" } };
	value.technology = faction::SmacTechnologyState::NewWorld( { { 7, "GAIANS" }, { 2, "UNIVERSITY" } }, smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() );
	value.designs = unit::SmacStartupDesignState::NewWorld( value.technology, smac_rules_fixture::Technologies(), smac_startup_fixture::Units(), 5 );
	value.technology.factions[ 0 ].owned.fill( true ); value.technology.factions[ 0 ].labs = INT32_MIN;
	value.technology.factions[ 1 ].owned.fill( false ); value.technology.factions[ 1 ].score = INT32_MAX;
	value.technology.factions[ 1 ].target = 88; value.technology.factions[ 1 ].cached_threshold = -7;
	return value;
}

bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }

std::string Wire( const SmacSimulationState& value, size_t index = SIZE_MAX, int64_t replacement = 0 ) {
	types::Buffer buffer; size_t number = 0;
	const auto write = [ & ]( int64_t v ) { buffer.WriteInt( number++ == index ? replacement : v ); };
	write( 0x534d5353 ); write( 2 ); write( value.roster.size() );
	for ( const auto& participant : value.roster ) {
		write( participant.simulation_id ); write( participant.slot ); buffer.WriteString( participant.gsid );
		write( static_cast< int64_t >( participant.role ) ); buffer.WriteString( participant.faction_id );
	}
	buffer.WriteString( value.technology.Serialize( smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() ).ToString() );
	buffer.WriteString( value.designs.Serialize( smac_startup_fixture::Units().catalog ).ToString() );
	return buffer.ToString();
}
}

void AddSmacSimulationStateTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC simulation codec owns explicit non-dense identities and exact technology continuation", GT() {
		const auto expected = State();
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions();
		auto decoded = SmacSimulationState::Deserialize( expected.Serialize( technologies, profiles, smac_startup_fixture::Units() ).ToString(), technologies, profiles, smac_startup_fixture::Units() );
		GT_ASSERT( decoded == expected && decoded.FindSlot( 6 )->simulation_id == 2 && !decoded.FindSlot( 1 ), "restoration reallocated simulation IDs or replayed named grants" );
		auto copied = decoded; decoded.roster.clear(); decoded.technology.factions.clear();
		GT_ASSERT( copied == expected && SmacSimulationState::Deserialize( Wire( copied ), technologies, profiles, smac_startup_fixture::Units() ) == expected, "runtime state borrowed input or copy storage" );
		GT_OK();
	} );
	task->AddTest( "SMAC simulation codec rejects malformed identity and late research records", GT() {
		const auto expected = State();
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions();
		const auto decode = [ & ]( const std::string& bytes ) { SmacSimulationState::Deserialize( bytes, technologies, profiles, smac_startup_fixture::Units() ); };
		for ( const auto index : { 0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u } ) {
			for ( const int64_t value : { INT64_MIN, int64_t( -1 ), INT64_MAX, int64_t( 256 ) } ) { GT_ASSERT( Rejects( [ & ]() { decode( Wire( expected, index, value ) ); } ), "invalid identity integer accepted" ); }
		}
		const auto wire = Wire( expected );
		for ( const auto size : { size_t( 0 ), size_t( 1 ), wire.size() - 1 } ) { GT_ASSERT( Rejects( [ & ]() { decode( wire.substr( 0, size ) ); } ), "truncated state accepted" ); }
		GT_ASSERT( Rejects( [ & ]() { decode( wire + "x" ); } ) && Rejects( [ & ]() { decode( std::string( SmacSimulationState::MAX_WIRE_BYTES + 1, 'x' ) ); } ), "trailing or oversized runtime state accepted" );
		for ( size_t scenario = 0; scenario < 9; ++scenario ) {
			auto bad = expected;
			if ( scenario == 0 ) { bad.roster[ 1 ].simulation_id = 7; }
			if ( scenario == 1 ) { bad.roster[ 1 ].slot = 0; }
			if ( scenario == 2 ) { bad.roster[ 1 ].gsid = bad.roster[ 0 ].gsid; }
			if ( scenario == 3 ) { bad.roster[ 1 ].gsid = std::string( "a\0b", 3 ); }
			if ( scenario == 4 ) { bad.roster[ 1 ].faction_id = "GAIANS"; }
			if ( scenario == 5 ) { bad.roster.pop_back(); }
			if ( scenario == 6 ) { bad.technology.factions[ 1 ].target = 89; }
			if ( scenario == 7 ) { bad.technology.factions[ 1 ].cached_threshold = -1; }
			if ( scenario == 8 ) { bad.roster[ 1 ].gsid.assign( SmacSimulationState::MAX_TEXT_BYTES, 'a' ); }
			GT_ASSERT( Rejects( [ & ]() { bad.Serialize( technologies, profiles, smac_startup_fixture::Units() ); } ), "invalid complete runtime state serialized" );
			if ( scenario < 6 || scenario == 8 ) { GT_ASSERT( Rejects( [ & ]() { decode( Wire( bad ) ); } ), "invalid complete runtime state decoded" ); }
		}
		GT_OK();
	} );
	task->AddTest( "bounded Buffer string read rejects complete over-budget frames before cursor mutation", GT() {
		constexpr uint32_t cap = SmacSimulationState::MAX_WIRE_BYTES;
		types::Buffer exact; exact.WriteString( std::string( cap, 'x' ) ); exact.WriteInt( 91 );
		GT_ASSERT( exact.ReadString( cap ).size() == cap && exact.ReadInt() == 91, "exact-cap frame or following field changed" );
		types::Buffer oversized; oversized.WriteString( std::string( cap + 1, 'x' ) );
		const auto* cursor = oversized.dr;
		std::string error;
		try { oversized.ReadString( cap ); } catch ( const std::runtime_error& e ) { error = e.what(); }
		GT_ASSERT( error.find( "payload limit" ) != std::string::npos && oversized.lenr == 0 && oversized.dr == cursor, "complete cap+1 frame was accepted or advanced cursor" );
		GT_ASSERT( oversized.ReadString().size() == cap + 1, "default string reader changed its admitted size" );
		types::Buffer wrong_type; wrong_type.WriteInt( cap + 1 ); error.clear();
		try { wrong_type.ReadString( cap ); } catch ( const std::runtime_error& e ) { error = e.what(); }
		GT_ASSERT( error.find( "unexpected type" ) != std::string::npos && wrong_type.lenr == 0, "type error was confused with length admission" );
		types::Buffer truncated( exact.ToString().substr( 0, 8 ) ); error.clear();
		try { truncated.ReadString( cap ); } catch ( const std::runtime_error& e ) { error = e.what(); }
		GT_ASSERT( error.find( "prematurely" ) != std::string::npos && truncated.lenr == 0, "truncated frame bypassed extent validation" );
		GT_OK();
	} );
}

}
