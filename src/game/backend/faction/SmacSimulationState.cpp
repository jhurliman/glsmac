#include "SmacSimulationState.h"

#include <set>
#include <stdexcept>
#include <utility>

namespace game::backend::faction {
namespace {
constexpr int64_t MAGIC = 0x534d5353; // SMSS, mutable world state, not settings
constexpr int64_t VERSION = 6;

int32_t ReadInt32( types::Buffer& buffer ) {
	const auto value = buffer.ReadInt();
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::runtime_error( "SMAC simulation integer outside signed32" ); }
	return static_cast< int32_t >( value );
}
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
	if ( roster.empty() || roster.size() > 7 || roster.size() != technology.factions.size() || roster.size() != designs.owners.size() || roster.size() != social.factions.size() ) { throw std::runtime_error( "Incomplete SMAC simulation roster" ); }
	std::array< bool, 8 > ids{};
	std::array< bool, 7 > slots{};
	std::set< std::string > gsids;
	size_t text = 0;
	for ( const auto& participant : roster ) {
		if ( participant.simulation_id < 1 || participant.simulation_id > 7 || ids[ participant.simulation_id ]
			|| participant.slot < 0 || participant.slot > 6 || slots[ participant.slot ] ) { throw std::runtime_error( "Duplicate or invalid SMAC simulation identity" ); }
		ids[ participant.simulation_id ] = true; slots[ participant.slot ] = true;
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

bool SmacSimulationState::operator==( const SmacSimulationState& other ) const { return roster == other.roster && technology == other.technology && designs == other.designs && social == other.social && gameplay_random == other.gameplay_random && movement_capability == other.movement_capability && resource_modifiers == other.resource_modifiers; }

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
		if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC simulation trailing fields" ); }
		result.roster = std::move( roster ); result.technology = std::move( technology ); result.designs = std::move( designs ); result.social = std::move( social );
		result.Validate( technologies, profiles, units );
		return result;
	}
	catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
}

}
