#include "SmacSimulationState.h"

#include "game/backend/faction/SmacSimulationState.h"
#include "fixtures/SmacStartupSource.h"
#include "fixtures/SmacOrbitalIntake.h"
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
	for ( const auto& owner : value.roster ) { value.satellites.emplace( owner.simulation_id, faction::SmacSatelliteCounts::NewWorld() ); }
	for ( const auto& owner : value.roster ) { value.economy.emplace( owner.simulation_id, faction::SmacEconomyState::NewWorld( { 5, 0 }, 100, 0 ) ); }
	value.technology = faction::SmacTechnologyState::NewWorld( { { 7, "GAIANS" }, { 2, "UNIVERSITY" } }, smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() );
	value.designs = unit::SmacStartupDesignState::NewWorld( value.technology, smac_rules_fixture::Technologies(), smac_startup_fixture::Units(), 5 );
	value.social = faction::SmacSocialState::NewWorld( { { 7, "GAIANS" }, { 2, "UNIVERSITY" } }, smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() );
	value.technology.factions[ 0 ].owned.fill( true ); value.technology.factions[ 0 ].labs = INT32_MIN;
	value.technology.factions[ 1 ].owned.fill( false ); value.technology.factions[ 1 ].score = INT32_MAX;
	value.technology.factions[ 1 ].target = 88; value.technology.factions[ 1 ].cached_threshold = -7;
	return value;
}

bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }

std::string Wire( const SmacSimulationState& value, size_t index = SIZE_MAX, int64_t replacement = 0, const std::string* random = nullptr, const std::string* modifiers = nullptr, const std::string* history = nullptr, const std::string* satellites = nullptr, const std::string* lifecycle = nullptr ) {
	types::Buffer buffer; size_t number = 0;
	const auto write = [ & ]( int64_t v ) { buffer.WriteInt( number++ == index ? replacement : v ); };
	write( 0x534d5353 ); write( 11 ); write( value.roster.size() );
	for ( const auto& participant : value.roster ) {
		write( participant.simulation_id ); write( participant.slot ); buffer.WriteString( participant.gsid );
		write( static_cast< int64_t >( participant.role ) ); buffer.WriteString( participant.faction_id );
	}
	buffer.WriteString( value.technology.Serialize( smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() ).ToString() );
	buffer.WriteString( value.designs.Serialize( smac_startup_fixture::Units().catalog ).ToString() );
	buffer.WriteString( value.social.Serialize( smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() ).ToString() );
	buffer.WriteString( random ? *random : value.gameplay_random.Serialize() );
	write( static_cast< int64_t >( value.movement_capability ) );
	if ( modifiers ) { types::Buffer tail; tail.WriteBool( false ); return buffer.ToString() + *modifiers + tail.ToString(); }
	buffer.WriteInt( value.resource_modifiers.atmospheric_dust_countdown );
	buffer.WriteBool( value.resource_modifiers.solar_ordinary ); buffer.WriteBool( value.resource_modifiers.solar_upkeep );
	buffer.WriteBool( value.resource_modifiers.planetary_governor.has_value() );
	if ( value.resource_modifiers.planetary_governor ) { buffer.WriteInt( *value.resource_modifiers.planetary_governor ); }
	buffer.WriteBool( history || value.movement_history.has_value() );
	if ( history || value.movement_history ) { buffer.WriteString( history ? *history : value.movement_history->Serialize() ); }
	if ( satellites ) { return buffer.ToString() + *satellites; }
	buffer.WriteInt( value.satellites.size() );
	for ( const auto& entry : value.satellites ) { buffer.WriteInt( entry.first ); for ( const auto count : entry.second.resources ) { buffer.WriteInt( count ); } }
	buffer.WriteInt( value.economy.size() );
	for ( const auto& entry : value.economy ) { buffer.WriteInt( entry.first ); buffer.WriteString( entry.second.Serialize().ToString() ); }
	if ( lifecycle ) { return buffer.ToString() + *lifecycle; }
	buffer.WriteBool( value.do_or_die ); buffer.WriteInt( value.roster.size() );
	for ( const auto& participant : value.roster ) {
		buffer.WriteInt( participant.simulation_id ); buffer.WriteBool( participant.active ); buffer.WriteBool( participant.restart_prohibited );
	}
	return buffer.ToString();
}
}

void AddSmacSimulationStateTests( task::gsetests::GSETests* task ) {
	task->AddTest("SMAC simulation retains independent activity restart prohibition and Do or Die",GT(){
		const auto technologies=smac_rules_fixture::Technologies();const auto profiles=smac_startup_fixture::Factions();const auto units=smac_startup_fixture::Units();
		const auto initial=State();
		GT_ASSERT(!initial.do_or_die && initial.roster[0].active && !initial.roster[0].restart_prohibited,"standard new-campaign lifecycle differs");
		for(int scenario=0;scenario<4;++scenario) {
			auto value=initial;value.do_or_die=scenario&1;value.roster[0].active=!(scenario&2);value.roster[1].restart_prohibited=true;
			const auto wire=value.Serialize(technologies,profiles,units).ToString();
			auto restored=SmacSimulationState::Deserialize(wire,technologies,profiles,units);
			GT_ASSERT(restored==value && wire==Wire(value) && restored.Serialize(technologies,profiles,units).ToString()==wire,
				"lifecycle restore inferred activity/restart count, reset technologies or changed identities");
			restored.roster[1].restart_prohibited=false;
			GT_ASSERT(value.roster[1].restart_prohibited && !(restored==value),"lifecycle aliases input or equality omits prohibition");
		}
		for(int scenario=0;scenario<7;++scenario) {
			types::Buffer tail;
			if(scenario==0){tail.WriteInt(0);}else{tail.WriteBool(false);}
			tail.WriteInt(scenario==1 ? 1 : 2);
			tail.WriteInt(scenario==2 ? 2 : 7);
			if(scenario==3){tail.WriteInt(1);}else{tail.WriteBool(true);}
			tail.WriteBool(false);tail.WriteInt(2);tail.WriteBool(false);
			if(scenario==4){tail.WriteInt(0);}else{tail.WriteBool(true);}
			auto raw=tail.ToString();if(scenario==5){raw.pop_back();}if(scenario==6){raw+='x';}
			GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(Wire(initial,SIZE_MAX,0,nullptr,nullptr,nullptr,nullptr,&raw),technologies,profiles,units);}),
				"malformed lifecycle count/identity/bool or framing accepted");
		}
		GT_ASSERT(Rejects([&]{SmacSimulationState::Deserialize(Wire(initial,1,10),technologies,profiles,units);}),"pre-lifecycle simulation accepted");GT_OK();
	});
	task->AddTest( "SMAC simulation economy preserves owner rates and balances on restore", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		auto value = State();
		value.economy.at( 7 ) = { { 1, 7 }, INT32_MIN, INT32_MAX, 99, 0xa5 };
		const auto wire = value.Serialize( technologies, profiles, units ).ToString();
		const auto restored = SmacSimulationState::Deserialize( wire, technologies, profiles, units );
		GT_ASSERT( restored == value && wire == Wire( value ) && restored.Serialize( technologies, profiles, units ).ToString() == wire, "economy restore reset or lost supplied owner state" );
		GT_ASSERT( Rejects( [ & ] { SmacSimulationState::Deserialize( Wire( value, 1, 8 ), technologies, profiles, units ); } ), "old simulation without economy accepted" );
		for ( int scenario = 0; scenario < 3; ++scenario ) {
			auto bad = value; bad.economy.erase( 2 );
			if ( scenario == 1 ) { bad.economy.emplace( 3, value.economy.at( 2 ) ); }
			if ( scenario == 2 ) { bad.economy = value.economy; bad.economy.emplace( 3, value.economy.at( 2 ) ); }
			GT_ASSERT( Rejects( [ & ] { bad.Serialize( technologies, profiles, units ); } ), "missing or foreign financial owner admitted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC orbital intake matches sixteen original independent resource prefixes", GT() {
		for ( const auto& c : fixtures::ORBITAL_INTAKE ) {
			const faction::SmacSatelliteCounts counts{ c.counts };
			GT_ASSERT( counts.Intake( c.population, c.aerospace, c.owned_elevator ) == c.expected, "original orbital prefix differs: " + std::string( c.id ) );
			GT_ASSERT( counts.resources == c.counts && counts.Intake( c.population, c.aerospace, c.owned_elevator ) == c.expected, "per-Base query consumed the faction assets" );
		}
		const faction::SmacSatelliteCounts maximum{ { INT32_MAX, INT32_MIN, 1 } };
		GT_ASSERT( Rejects( [ & ] { maximum.Intake( 4, false, false ); } ), "unmeasured increment overflow accepted" );
		GT_ASSERT( maximum.Intake( 4, true, false ) == ( std::array< int32_t, 3 >{ 4, INT32_MIN, 1 } ), "unused halving overflow incorrectly rejected full output" );
		GT_ASSERT( Rejects( [ & ] { maximum.Intake( -1, true, false ); } ) && Rejects( [ & ] { maximum.Intake( 128, true, false ); } ), "unsupported population accepted" );
		GT_OK();
	} );
	task->AddTest( "SMAC simulation satellite assets restore non-dense owners and exact signed counters", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		auto value = State();
		GT_ASSERT( value.GetSatelliteCounts( 7 ).resources == ( std::array< int32_t, 3 >{ 0, 0, 0 } ), "explicit new-campaign initialization differs" );
		value.satellites.at( 7 ) = { { 3, 7, 19 } }; value.satellites.at( 2 ) = { { INT32_MIN, -1, INT32_MAX } };
		const auto wire = value.Serialize( technologies, profiles, units ).ToString();
		auto restored = SmacSimulationState::Deserialize( wire, technologies, profiles, units );
		GT_ASSERT( restored == value && wire == Wire( value ) && restored.Serialize( technologies, profiles, units ).ToString() == wire, "restore reset/reordered satellite assets or changed framing" );
		auto copy = restored.GetSatelliteCounts( 7 ); copy.resources[ 0 ] = 100;
		restored.satellites.at( 7 ).resources[ 1 ] = 99;
		GT_ASSERT( value.GetSatelliteCounts( 7 ).resources == ( std::array< int32_t, 3 >{ 3, 7, 19 } ) && !( restored == value ), "counter copy aliases owner or equality ignores assets" );
		GT_ASSERT( Rejects( [ & ] { value.GetSatelliteCounts( 1 ); } ), "missing owner silently became zero" );
		GT_OK();
	} );
	task->AddTest( "SMAC simulation satellite suffix rejects missing duplicate and oversized records", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		const auto value = State(); const auto decode = [ & ]( const std::string& wire ) { SmacSimulationState::Deserialize( wire, technologies, profiles, units ); };
		GT_ASSERT( Rejects( [ & ] { decode( Wire( value, 1, 7 ) ); } ), "pre-satellite SimulationV7 accepted" );
		for ( int scenario = 0; scenario < 12; ++scenario ) {
			types::Buffer suffix;
			suffix.WriteInt( scenario == 0 ? -1 : scenario == 1 ? 0 : scenario == 2 ? 8 : scenario == 3 ? 1 : 2 );
			suffix.WriteInt( scenario == 4 ? 0 : scenario == 5 ? 8 : scenario == 6 ? 3 : 2 );
			suffix.WriteInt( scenario == 8 ? int64_t( INT32_MAX ) + 1 : scenario == 9 ? int64_t( INT32_MIN ) - 1 : 3 );
			suffix.WriteInt( 4 ); suffix.WriteInt( 5 ); suffix.WriteInt( scenario == 7 ? 2 : 7 );
			suffix.WriteInt( 6 ); suffix.WriteInt( 7 ); suffix.WriteInt( 8 );
			auto raw = suffix.ToString(); if ( scenario == 10 ) { raw.clear(); } if ( scenario == 11 ) { raw.pop_back(); }
			GT_ASSERT( Rejects( [ & ] { decode( Wire( value, SIZE_MAX, 0, nullptr, nullptr, nullptr, &raw ) ); } ), "invalid satellite suffix accepted: " + std::to_string( scenario ) );
		}
		for ( int scenario = 0; scenario < 3; ++scenario ) {
			auto bad = value; bad.satellites.erase( 2 );
			if ( scenario == 1 ) { bad.satellites.emplace( 3, faction::SmacSatelliteCounts::NewWorld() ); }
			if ( scenario == 2 ) { bad.satellites = value.satellites; bad.satellites.emplace( 3, faction::SmacSatelliteCounts::NewWorld() ); }
			GT_ASSERT( Rejects( [ & ] { bad.Serialize( technologies, profiles, units ); } ) && Rejects( [ & ] { bad.GetSatelliteCounts( 7 ); } ), "incomplete/extra satellite ownership accepted" );
		}
		GT_OK();
	} );
	task->AddTest( "movement history simulation optional maximum bitmap survives exact codec copy", GT() {
		const auto technologies = smac_rules_fixture::Technologies(); const auto profiles = smac_startup_fixture::Factions(); const auto units = smac_startup_fixture::Units();
		auto value = State();
		for ( const bool present : { false, true } ) {
			if ( present ) { value.movement_history = SmacMovementHistory::NewGeneratedWorld( 512, 256 ); value.movement_history->RetainNearest( 9999 ); }
			const auto wire = value.Serialize( technologies, profiles, units ).ToString();
			auto decoded = SmacSimulationState::Deserialize( wire, technologies, profiles, units );
			GT_ASSERT( wire == Wire( value ) && wire.size() <= SmacSimulationState::MAX_WIRE_BYTES && decoded == value, "independent simulation framing or history restore differs" );
			if ( present ) { decoded.movement_history->RetainNearest( {} ); GT_ASSERT( value.movement_history->NearestDistance() == 9999 && !( decoded == value ), "history copy aliases nearest authority" ); }
		}
		GT_ASSERT( Rejects( [ & ] { SmacSimulationState::Deserialize( Wire( value, 1, 6 ), technologies, profiles, units ); } ), "pre-history V6 accepted" );
		std::string large( SmacMovementHistory::MAX_WIRE_BYTES + 1, 'x' ), message;
		try { SmacSimulationState::Deserialize( Wire( value, SIZE_MAX, 0, nullptr, nullptr, &large ), technologies, profiles, units ); }
		catch ( const std::runtime_error& error ) { message = error.what(); }
		GT_ASSERT( message.find( "payload limit" ) != std::string::npos, "history oversized frame reached nested allocation instead of outer admission" );
		GT_OK();
	} );
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
