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
	SmacSimulationState value( SmacGameplayRandomState( 0, UINT32_MAX ), SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain, faction::SmacGlobalResourceModifiers::NewWorld() );
	value.roster = { { 7, 0, "controller-one", roster::Role::Host, "GAIANS" }, { 2, 6, "controller-two", roster::Role::Player, "UNIVERSITY" } };
	value.technology = faction::SmacTechnologyState::NewWorld( { { 7, "GAIANS" }, { 2, "UNIVERSITY" } }, smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() );
	value.designs = unit::SmacStartupDesignState::NewWorld( value.technology, smac_rules_fixture::Technologies(), smac_startup_fixture::Units(), 5 );
	value.social = faction::SmacSocialState::NewWorld( { { 7, "GAIANS" }, { 2, "UNIVERSITY" } }, smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() );
	value.technology.factions[ 0 ].owned.fill( true ); value.technology.factions[ 0 ].labs = INT32_MIN;
	value.technology.factions[ 1 ].owned.fill( false ); value.technology.factions[ 1 ].score = INT32_MAX;
	value.technology.factions[ 1 ].target = 88; value.technology.factions[ 1 ].cached_threshold = -7;
	return value;
}

bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }

std::string Wire( const SmacSimulationState& value, size_t index = SIZE_MAX, int64_t replacement = 0, const std::string* random = nullptr, const std::string* modifiers = nullptr ) {
	types::Buffer buffer; size_t number = 0;
	const auto write = [ & ]( int64_t v ) { buffer.WriteInt( number++ == index ? replacement : v ); };
	write( 0x534d5353 ); write( 6 ); write( value.roster.size() );
	for ( const auto& participant : value.roster ) {
		write( participant.simulation_id ); write( participant.slot ); buffer.WriteString( participant.gsid );
		write( static_cast< int64_t >( participant.role ) ); buffer.WriteString( participant.faction_id );
	}
	buffer.WriteString( value.technology.Serialize( smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() ).ToString() );
	buffer.WriteString( value.designs.Serialize( smac_startup_fixture::Units().catalog ).ToString() );
	buffer.WriteString( value.social.Serialize( smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() ).ToString() );
	buffer.WriteString( random ? *random : value.gameplay_random.Serialize() );
	write( static_cast< int64_t >( value.movement_capability ) );
	if ( modifiers ) { return buffer.ToString() + *modifiers; }
	buffer.WriteInt( value.resource_modifiers.atmospheric_dust_countdown );
	buffer.WriteBool( value.resource_modifiers.solar_ordinary ); buffer.WriteBool( value.resource_modifiers.solar_upkeep );
	buffer.WriteBool( value.resource_modifiers.planetary_governor.has_value() );
	if ( value.resource_modifiers.planetary_governor ) { buffer.WriteInt( *value.resource_modifiers.planetary_governor ); }
	return buffer.ToString();
}
}

void AddSmacSimulationStateTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC simulation resource modifiers retain independent phases and council identity", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		const faction::SmacGlobalResourceModifiers reset{ 0, false, false, {} };
		GT_ASSERT( faction::SmacGlobalResourceModifiers::NewWorld() == reset, "new-world resource modifiers differ from original reset" );
		for ( const auto dust : { INT32_MIN, -1, 0, 1, INT32_MAX } ) {
			for ( const bool ordinary : { false, true } ) { for ( const bool upkeep : { false, true } ) {
				for ( const std::optional< int32_t > governor : { std::optional< int32_t >{}, { 1 }, { 2 }, { 7 } } ) {
					auto value = State(); value.resource_modifiers = { dust, ordinary, upkeep, governor };
					const auto wire = value.Serialize( technologies, profiles, units ).ToString();
					auto decoded = SmacSimulationState::Deserialize( wire, technologies, profiles, units );
					GT_ASSERT( decoded == value && decoded.Serialize( technologies, profiles, units ).ToString() == wire, "restore aged events collapsed phases or rebound council ID" );
					decoded.resource_modifiers = reset;
					GT_ASSERT( value.resource_modifiers == ( faction::SmacGlobalResourceModifiers{ dust, ordinary, upkeep, governor } ), "copied modifier state aliases original" );
				}
			} }
		}
		GT_OK();
	} );
	task->AddTest( "SMAC simulation resource modifiers reject missing malformed and oversized late fields", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		const auto decode = [ & ]( const std::string& wire ) { SmacSimulationState::Deserialize( wire, technologies, profiles, units ); };
		for ( int scenario = 0; scenario < 13; ++scenario ) {
			types::Buffer suffix;
			suffix.WriteInt( scenario == 1 ? int64_t( INT32_MAX ) + 1 : scenario == 2 ? int64_t( INT32_MIN ) - 1 : 10 );
			if ( scenario == 3 ) { suffix.WriteInt( 1 ); } else { suffix.WriteBool( true ); }
			if ( scenario == 4 ) { suffix.WriteInt( 0 ); } else { suffix.WriteBool( false ); }
			if ( scenario == 5 ) { suffix.WriteInt( 1 ); } else { suffix.WriteBool( true ); }
			suffix.WriteInt( scenario == 6 ? 0 : scenario == 7 ? 8 : scenario == 8 ? -1 : scenario == 9 ? INT64_MAX : 7 );
			auto raw = suffix.ToString();
			if ( scenario == 0 ) { raw.clear(); }
			if ( scenario == 10 ) { raw.pop_back(); }
			if ( scenario == 11 ) { raw += "x"; }
			if ( scenario == 12 ) { raw[ 19 ] = 2; raw[ 20 ] = 2; } // invalid first Bool payload with matching checksum
			GT_ASSERT( Rejects( [ & ] { decode( Wire( State(), SIZE_MAX, 0, nullptr, &raw ) ); } ), "invalid resource suffix accepted" );
		}
		for ( const auto id : { INT32_MIN, -1, 0, 8, INT32_MAX } ) {
			auto bad = State(); bad.resource_modifiers.planetary_governor = id;
			GT_ASSERT( Rejects( [ & ] { bad.Serialize( technologies, profiles, units ); } ), "invalid council identity serialized" );
		}
		GT_ASSERT( Rejects( [ & ] { decode( Wire( State(), 1, 5 ) ); } ), "pre-modifier V5 accepted" );
		GT_OK();
	} );
	task->AddTest( "SMAC simulation movement capability is explicit bounded and preserved on restore", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		for ( const auto capability : { SmacSimulationState::MovementCapability::Unavailable, SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain } ) {
			auto value = State(); value.movement_capability = capability;
			const auto decoded = SmacSimulationState::Deserialize( value.Serialize( technologies, profiles, units ).ToString(), technologies, profiles, units );
			GT_ASSERT( decoded == value && decoded.movement_capability == capability, "restoration inferred supported campaign from absent project data" );
		}
		for ( const int64_t invalid : { int64_t( -1 ), int64_t( 2 ), INT64_MIN, INT64_MAX } ) {
			auto value = State(); value.movement_capability = static_cast< SmacSimulationState::MovementCapability >( invalid );
			GT_ASSERT( Rejects( [ & ] { value.Serialize( technologies, profiles, units ); } )
				&& Rejects( [ & ] { SmacSimulationState::Deserialize( Wire( value ), technologies, profiles, units ); } ), "unknown capability accepted" );
		}
		GT_ASSERT( Rejects( [ & ] { SmacSimulationState::Deserialize( Wire( State(), 1, 4 ), technologies, profiles, units ); } ), "pre-capability V4 accepted" );
		GT_OK();
	} );
	task->AddTest( "SMAC simulation gameplay RNG pair is required bounded and exactly owned", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		auto value = State();
		for ( const uint32_t shared : { uint32_t( 0 ), UINT32_MAX } ) {
			for ( const uint32_t crt : { uint32_t( 0 ), UINT32_MAX } ) {
				value.gameplay_random = SmacGameplayRandomState( shared, crt );
				const auto wire = value.Serialize( technologies, profiles, units ).ToString();
				auto decoded = SmacSimulationState::Deserialize( wire, technologies, profiles, units );
				GT_ASSERT( decoded == value && value.gameplay_random.Shared() == shared && value.gameplay_random.Crt() == crt, "writer/reader changed zero or endpoint words" );
				const auto saved = value.gameplay_random; auto copy = decoded;
				for ( size_t i = 0; i < 32; ++i ) {
					GT_ASSERT( copy.gameplay_random.NextSharedLow16() == decoded.gameplay_random.NextSharedLow16() && copy.gameplay_random.NextCrtRaw15() == decoded.gameplay_random.NextCrtRaw15(), "owned pair continuation changed after copy" );
				}
				GT_ASSERT( value.gameplay_random == saved && copy == decoded, "copy borrowed original pair state" );
			}
		}
		for ( size_t mode = 0; mode < 6; ++mode ) {
			auto random = value.gameplay_random.Serialize();
			if ( mode == 0 ) { random[ 0 ] = 'X'; }
			if ( mode == 1 ) { random[ 4 ] = 2; }
			if ( mode == 2 ) { random.pop_back(); }
			if ( mode == 3 ) { random.push_back( 'x' ); }
			if ( mode == 4 ) { random.assign( 4096, 'x' ); }
			if ( mode == 5 ) { random.clear(); }
			std::string message;
			try { SmacSimulationState::Deserialize( Wire( value, SIZE_MAX, 0, &random ), technologies, profiles, units ); } catch ( const std::runtime_error& error ) { message = error.what(); }
			GT_ASSERT( !message.empty() && ( ( message.find( "payload limit" ) != std::string::npos ) == ( mode == 3 || mode == 4 ) ), "pair rejection confused outer limit with nested codec validation" );
		}
		GT_ASSERT( Rejects( [ & ]() { SmacSimulationState::Deserialize( Wire( value, 1, 3 ), technologies, profiles, units ); } ), "old simulation V3 accepted without a gameplay continuation contract" );
		types::Buffer large; large.WriteString( std::string( SmacGameplayRandomState::WIRE_SIZE + 1, 'x' ) ); const auto* cursor = large.dr;
		GT_ASSERT( Rejects( [ & ]() { large.ReadString( SmacGameplayRandomState::WIRE_SIZE ); } ) && large.dr == cursor && large.lenr == 0, "over-budget pair was copied/consumed before admission" );
		GT_OK();
	} );
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
		for ( size_t scenario = 0; scenario < 13; ++scenario ) {
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
			if ( scenario == 9 ) { bad.social.factions.pop_back(); }
			if ( scenario == 10 ) { bad.social.factions[ 1 ].admission.simulation_id = 5; }
			if ( scenario == 11 ) { bad.social.factions[ 1 ].admission.faction_id = "HIVE"; }
			if ( scenario == 12 ) { bad.social.factions.push_back( { { 4, "HIVE" } } ); }
			GT_ASSERT( Rejects( [ & ]() { bad.Serialize( technologies, profiles, smac_startup_fixture::Units() ); } ), "invalid complete runtime state serialized" );
			if ( scenario < 6 || scenario >= 8 ) { GT_ASSERT( Rejects( [ & ]() { decode( Wire( bad ) ); } ), "invalid complete runtime state decoded" ); }
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
