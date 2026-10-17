#include "SmacSimulationState.h"

#include <set>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace game::backend::faction {
namespace {
constexpr int64_t MAGIC = 0x534d5353; // SMSS, mutable world state, not settings
constexpr int64_t VERSION = 9;

int32_t ReadInt32( types::Buffer& buffer ) {
	const auto value = buffer.ReadInt();
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::runtime_error( "SMAC simulation integer outside signed32" ); }
	return static_cast< int32_t >( value );
}
}

std::array< int32_t, 3 > SmacSatelliteCounts::Intake( int32_t population, bool aerospace_complex, bool owns_space_elevator ) const {
	if ( population < 0 || population > 127 ) { throw std::invalid_argument( "Orbital intake population outside 0..127" ); }
	std::array< int32_t, 3 > result;
	for ( size_t i = 0; i < result.size(); ++i ) {
		auto count = resources[ i ];
		if ( !aerospace_complex && !owns_space_elevator ) {
			if ( count == INT32_MAX ) { throw std::invalid_argument( "Unmeasured orbital count increment overflow" ); }
			count = ( count + 1 ) / 2; // Signed truncation, including diagnostic negatives.
		}
		result[ i ] = std::min( count, population );
	}
	return result;
}

SmacGlobalResourceModifiers SmacGlobalResourceModifiers::NewWorld() noexcept { return { 0, false, false, {} }; }

void SmacGlobalResourceModifiers::Validate() const {
	// Preserve signed countdowns and both flags exactly; aging is a separate
	// turn-boundary operation. A council role is a faction ID, never a slot.
	if ( planetary_governor && ( *planetary_governor < 1 || *planetary_governor > 7 ) ) {
		throw std::runtime_error( "Invalid SMAC Planetary Governor simulation ID" );
	}
}

bool SmacGlobalResourceModifiers::operator==( const SmacGlobalResourceModifiers& other ) const noexcept {
	return atmospheric_dust_countdown == other.atmospheric_dust_countdown && solar_ordinary == other.solar_ordinary
		&& solar_upkeep == other.solar_upkeep && planetary_governor == other.planetary_governor;
}

bool SmacSimulationParticipant::operator==( const SmacSimulationParticipant& other ) const {
	return simulation_id == other.simulation_id && slot == other.slot && gsid == other.gsid && role == other.role && faction_id == other.faction_id;
}

void SmacSimulationState::Validate( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units ) const {
	if ( movement_capability != MovementCapability::Unavailable && movement_capability != MovementCapability::NoProjectsOrdinaryTerrain ) {
		throw std::runtime_error( "Unknown SMAC supported-campaign movement capability" );
	}
	resource_modifiers.Validate();
	technology.Validate( technologies, profiles );
	social.Validate( technologies, profiles );
	units.Validate( technologies ); designs.Validate( units.catalog );
	if ( roster.empty() || roster.size() > 7 || roster.size() != technology.factions.size() || roster.size() != designs.owners.size() || roster.size() != social.factions.size() || roster.size() != satellites.size() || roster.size() != economy.size() ) { throw std::runtime_error( "Incomplete SMAC simulation roster" ); }
	std::array< bool, 8 > ids{};
	std::array< bool, 7 > slots{};
	std::set< std::string > gsids;
	size_t text = 0;
	for ( const auto& participant : roster ) {
		if ( participant.simulation_id < 1 || participant.simulation_id > 7 || ids[ participant.simulation_id ]
			|| participant.slot < 0 || participant.slot > 6 || slots[ participant.slot ] ) { throw std::runtime_error( "Duplicate or invalid SMAC simulation identity" ); }
		ids[ participant.simulation_id ] = true; slots[ participant.slot ] = true;
		if ( !satellites.count( participant.simulation_id ) ) { throw std::runtime_error( "Missing SMAC faction satellite assets" ); }
		const auto finances = economy.find( participant.simulation_id );
		if ( finances == economy.end() ) { throw std::runtime_error( "Missing SMAC faction economy" ); }
		finances->second.Validate();
		if ( ( participant.slot == 0 && participant.role != roster::Role::Single && participant.role != roster::Role::Host )
			|| ( participant.slot != 0 && participant.role != roster::Role::Player ) ) { throw std::runtime_error( "Invalid SMAC simulation controller role" ); }
		for ( const auto* value : { &participant.gsid, &participant.faction_id } ) {
			if ( value->empty() || value->find( '\0' ) != std::string::npos || value->size() > MAX_TEXT_BYTES - text ) {
				throw std::runtime_error( "Invalid or oversized SMAC simulation controller text" );
			}
			text += value->size();
		}
		if ( !gsids.insert( participant.gsid ).second ) { throw std::runtime_error( "Duplicate SMAC simulation controller GSID" ); }
		const auto* state = technology.Find( participant.simulation_id );
		const auto* social_state = social.Find( participant.simulation_id );
		if ( !state || state->admission.faction_id != participant.faction_id || !designs.Find( participant.simulation_id )
			|| !social_state || social_state->admission.faction_id != participant.faction_id ) { throw std::runtime_error( "SMAC simulation roster and technology identities differ" ); }
	}
}

bool SmacSimulationState::operator==( const SmacSimulationState& other ) const { return roster == other.roster && technology == other.technology && designs == other.designs && social == other.social && gameplay_random == other.gameplay_random && movement_capability == other.movement_capability && resource_modifiers == other.resource_modifiers && movement_history == other.movement_history && satellites == other.satellites && economy == other.economy; }

SmacSatelliteCounts SmacSimulationState::GetSatelliteCounts( int32_t simulation_id ) const {
	if ( roster.empty() || roster.size() > 7 || satellites.size() != roster.size() ) { throw std::runtime_error( "Incomplete SMAC faction satellite assets" ); }
	for ( const auto& owner : roster ) {
		if ( !satellites.count( owner.simulation_id ) ) { throw std::runtime_error( "Missing SMAC faction satellite assets" ); }
	}
	const auto found = satellites.find( simulation_id );
	if ( simulation_id < 1 || simulation_id > 7 || found == satellites.end() ) { throw std::runtime_error( "SMAC faction satellite assets are unavailable" ); }
	return found->second;
}

const SmacSimulationParticipant* SmacSimulationState::FindSlot( size_t slot ) const {
	for ( const auto& participant : roster ) { if ( static_cast< size_t >( participant.slot ) == slot ) { return &participant; } }
	return nullptr;
}

types::Buffer SmacSimulationState::Serialize( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units ) const {
	Validate( technologies, profiles, units );
	const auto state = technology.Serialize( technologies, profiles );
	const auto design_state = designs.Serialize( units.catalog );
	const auto social_state = social.Serialize( technologies, profiles );
	types::Buffer buffer;
	buffer.WriteInt( MAGIC ); buffer.WriteInt( VERSION ); buffer.WriteInt( roster.size() );
	for ( const auto& participant : roster ) {
		buffer.WriteInt( participant.simulation_id ); buffer.WriteInt( participant.slot ); buffer.WriteString( participant.gsid );
		buffer.WriteInt( static_cast< int64_t >( participant.role ) ); buffer.WriteString( participant.faction_id );
	}
	buffer.WriteString( state.ToString() );
	buffer.WriteString( design_state.ToString() );
	buffer.WriteString( social_state.ToString() );
	buffer.WriteString( gameplay_random.Serialize() );
	buffer.WriteInt( static_cast< int64_t >( movement_capability ) );
	buffer.WriteInt( resource_modifiers.atmospheric_dust_countdown );
	buffer.WriteBool( resource_modifiers.solar_ordinary ); buffer.WriteBool( resource_modifiers.solar_upkeep );
	buffer.WriteBool( resource_modifiers.planetary_governor.has_value() );
	if ( resource_modifiers.planetary_governor ) { buffer.WriteInt( *resource_modifiers.planetary_governor ); }
	buffer.WriteBool( movement_history.has_value() );
	if ( movement_history ) { buffer.WriteString( movement_history->Serialize() ); }
	buffer.WriteInt( satellites.size() );
	for ( const auto& entry : satellites ) {
		buffer.WriteInt( entry.first );
		for ( const auto count : entry.second.resources ) { buffer.WriteInt( count ); }
	}
	buffer.WriteInt( economy.size() );
	for ( const auto& entry : economy ) { buffer.WriteInt( entry.first ); buffer.WriteString( entry.second.Serialize().ToString() ); }
	if ( buffer.lenw > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC simulation state exceeds wire budget" ); }
	return buffer;
}

SmacSimulationState SmacSimulationState::Deserialize( const std::string& wire, const settings::SmacTechnologyRules& technologies,
	const settings::SmacFactionStartupRules& profiles, const settings::SmacPredefinedUnitRules& units ) {
	if ( wire.size() > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC simulation state exceeds wire budget" ); }
	try {
		types::Buffer buffer( wire );
		if ( buffer.ReadInt() != MAGIC || buffer.ReadInt() != VERSION ) { throw std::runtime_error( "Unsupported SMAC simulation state format" ); }
		const auto count = buffer.ReadInt();
		if ( count < 1 || count > 7 ) { throw std::runtime_error( "SMAC simulation roster requires 1..7 participants" ); }
		std::vector< SmacSimulationParticipant > roster;
		roster.reserve( static_cast< size_t >( count ) );
		for ( int64_t i = 0; i < count; ++i ) {
			SmacSimulationParticipant participant;
			participant.simulation_id = ReadInt32( buffer ); participant.slot = ReadInt32( buffer ); participant.gsid = buffer.ReadString();
			const auto role = buffer.ReadInt();
			if ( role < 1 || role > 3 ) { throw std::runtime_error( "Invalid SMAC simulation role" ); }
			participant.role = static_cast< roster::Role >( role ); participant.faction_id = buffer.ReadString();
			roster.push_back( std::move( participant ) );
		}
		auto technology = SmacTechnologyState::Deserialize( buffer.ReadString( SmacTechnologyState::MAX_WIRE_BYTES ), technologies, profiles );
		auto designs = unit::SmacStartupDesignState::Deserialize( buffer.ReadString( unit::SmacStartupDesignState::MAX_WIRE_BYTES ), units.catalog );
		auto social = SmacSocialState::Deserialize( buffer.ReadString( SmacSocialState::MAX_WIRE_BYTES ), technologies, profiles );
		const auto random = SmacGameplayRandomState::Deserialize( buffer.ReadString( SmacGameplayRandomState::WIRE_SIZE ) );
		const auto capability = buffer.ReadInt();
		if ( capability != 0 && capability != 1 ) { throw std::runtime_error( "Unknown SMAC supported-campaign movement capability" ); }
		const auto dust = ReadInt32( buffer );
		const auto ordinary = buffer.ReadBool(); const auto upkeep = buffer.ReadBool();
		std::optional< int32_t > governor;
		if ( buffer.ReadBool() ) { governor = ReadInt32( buffer ); }
		SmacSimulationState result( random, static_cast< MovementCapability >( capability ), { dust, ordinary, upkeep, governor } );
		if ( buffer.ReadBool() ) { result.movement_history = SmacMovementHistory::Deserialize( buffer.ReadString( SmacMovementHistory::MAX_WIRE_BYTES ) ); }
		const auto satellite_owners = buffer.ReadInt();
		if ( satellite_owners < 1 || satellite_owners > 7 || satellite_owners != count ) { throw std::runtime_error( "Invalid SMAC satellite owner count" ); }
		for ( int64_t i = 0; i < satellite_owners; ++i ) {
			const auto id = ReadInt32( buffer );
			SmacSatelliteCounts assets;
			for ( auto& resource : assets.resources ) { resource = ReadInt32( buffer ); }
			if ( id < 1 || id > 7 || !result.satellites.emplace( id, assets ).second ) { throw std::runtime_error( "Invalid or duplicate SMAC satellite owner" ); }
		}
		const auto economy_owners = buffer.ReadInt();
		if ( economy_owners < 1 || economy_owners > 7 || economy_owners != count ) { throw std::runtime_error( "Invalid SMAC economy owner count" ); }
		for ( int64_t i = 0; i < economy_owners; ++i ) {
			const auto id = ReadInt32( buffer );
			auto finances = SmacEconomyState::Deserialize( buffer.ReadString( SmacEconomyState::MAX_WIRE_BYTES ) );
			if ( id < 1 || id > 7 || !result.economy.emplace( id, std::move( finances ) ).second ) { throw std::runtime_error( "Invalid or duplicate SMAC economy owner" ); }
		}
		if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC simulation trailing fields" ); }
		result.roster = std::move( roster ); result.technology = std::move( technology ); result.designs = std::move( designs ); result.social = std::move( social );
		result.Validate( technologies, profiles, units );
		return result;
	}
	catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
}

}
