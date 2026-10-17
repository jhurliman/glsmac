#include "SmacTechnologyState.h"

#include <stdexcept>
#include <utility>

namespace game::backend::faction {
namespace {
constexpr int64_t MAGIC = 0x534d5453; // SMTS; separate from definition-profile codecs
constexpr int64_t VERSION = 1;

int32_t ReadInt32( types::Buffer& buffer ) {
	const auto value = buffer.ReadInt();
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::runtime_error( "SMAC technology state integer outside signed32" ); }
	return static_cast< int32_t >( value );
}

std::optional< int32_t > ReadOptional( types::Buffer& buffer ) {
	if ( !buffer.ReadBool() ) { return {}; }
	return ReadInt32( buffer );
}

void WriteOptional( types::Buffer& buffer, const std::optional< int32_t >& value ) {
	buffer.WriteBool( value.has_value() );
	if ( value ) { buffer.WriteInt( *value ); }
}
}

bool SmacSimulationAdmission::operator==( const SmacSimulationAdmission& other ) const {
	return simulation_id == other.simulation_id && faction_id == other.faction_id;
}

bool SmacFactionTechnologyState::operator==( const SmacFactionTechnologyState& other ) const {
	return admission == other.admission && owned == other.owned && score == other.score && future_count == other.future_count
		&& labs == other.labs && target == other.target && cached_threshold == other.cached_threshold;
}

void SmacTechnologyState::Validate( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) const {
	technologies.Validate(); profiles.Validate( technologies );
	if ( factions.empty() || factions.size() > 7 ) { throw std::invalid_argument( "SMAC technology state requires an explicit roster of 1..7 factions" ); }
	std::array< bool, 8 > seen{};
	for ( const auto& faction : factions ) {
		const auto id = faction.admission.simulation_id;
		if ( id < 1 || id > 7 || seen[ id ] ) { throw std::invalid_argument( "SMAC simulation IDs must be distinct within 1..7" ); }
		seen[ id ] = true;
		if ( !profiles.Find( faction.admission.faction_id ) ) { throw std::invalid_argument( "SMAC simulation faction has no frozen source profile" ); }
		if ( faction.target && ( *faction.target < 0 || *faction.target >= 89 ) ) { throw std::invalid_argument( "SMAC research target must be absent or a catalog ID 0..88" ); }
		// -1 is the original invalid-cache sentinel; absence preserves it without
		// conflating it with a valid signed value. Other signed32 values are kept,
		// not judged as a future research-query eligibility policy here.
		if ( faction.cached_threshold == -1 ) { throw std::invalid_argument( "SMAC invalid research cache must be absent" ); }
	}
}

bool SmacTechnologyState::operator==( const SmacTechnologyState& other ) const { return factions == other.factions; }

const SmacFactionTechnologyState* SmacTechnologyState::Find( int32_t simulation_id ) const {
	for ( const auto& faction : factions ) { if ( faction.admission.simulation_id == simulation_id ) { return &faction; } }
	return nullptr;
}

void SmacTechnologyState::ClearOwnershipForNewWorld() noexcept {
	for ( auto& faction : factions ) { faction.owned.fill( false ); }
}

void SmacTechnologyState::InitializeFactionAtTurnZero( int32_t simulation_id, const settings::SmacTechnologyRules& technologies,
	const settings::SmacFactionStartupRules& profiles ) {
	Validate( technologies, profiles );
	for ( auto& faction : factions ) {
		if ( faction.admission.simulation_id != simulation_id ) { continue; }
		// Stage the whole recipient before replacing it; other owners are not
		// cleared, even when two recipients have the same named technology.
		auto initialized = faction;
		initialized.score = 0; initialized.future_count = 0; initialized.labs = 0;
		initialized.target.reset(); initialized.cached_threshold.reset();
		const auto* profile = profiles.Find( initialized.admission.faction_id );
		for ( const auto id : profile->NamedTechnologyReferences() ) {
			if ( id >= 0 ) { initialized.owned[ id ] = true; }
		}
		faction = std::move( initialized );
		return;
	}
	throw std::invalid_argument( "SMAC turn-zero initialization requires an admitted simulation ID" );
}

SmacTechnologyState SmacTechnologyState::NewWorld( const std::vector< SmacSimulationAdmission >& admissions,
	const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) {
	// Check the count before reserving caller-controlled storage.
	if ( admissions.empty() || admissions.size() > 7 ) { throw std::invalid_argument( "SMAC new world requires explicit admission of 1..7 factions" ); }
	SmacTechnologyState result;
	result.factions.reserve( admissions.size() );
	for ( const auto& admission : admissions ) { result.factions.push_back( { admission } ); }
	result.Validate( technologies, profiles );
	result.ClearOwnershipForNewWorld();
	for ( const auto& admission : admissions ) { result.InitializeFactionAtTurnZero( admission.simulation_id, technologies, profiles ); }
	return result;
}

types::Buffer SmacTechnologyState::Serialize( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) const {
	Validate( technologies, profiles );
	types::Buffer buffer;
	buffer.WriteInt( MAGIC ); buffer.WriteInt( VERSION ); buffer.WriteInt( factions.size() );
	for ( const auto& faction : factions ) {
		buffer.WriteInt( faction.admission.simulation_id ); buffer.WriteString( faction.admission.faction_id );
		for ( const auto owned : faction.owned ) { buffer.WriteBool( owned ); }
		buffer.WriteInt( faction.score ); buffer.WriteInt( faction.future_count ); buffer.WriteInt( faction.labs );
		WriteOptional( buffer, faction.target ); WriteOptional( buffer, faction.cached_threshold );
	}
	if ( buffer.lenw > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC technology state exceeds wire budget" ); }
	return buffer;
}

SmacTechnologyState SmacTechnologyState::Deserialize( const std::string& wire, const settings::SmacTechnologyRules& technologies,
	const settings::SmacFactionStartupRules& profiles ) {
	if ( wire.size() > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC technology state exceeds wire budget" ); }
	types::Buffer buffer( wire );
	if ( buffer.ReadInt() != MAGIC || buffer.ReadInt() != VERSION ) { throw std::runtime_error( "Unsupported SMAC technology state format" ); }
	const auto count = buffer.ReadInt();
	if ( count < 1 || count > 7 ) { throw std::runtime_error( "SMAC technology state requires 1..7 factions" ); }
	SmacTechnologyState result;
	result.factions.reserve( static_cast< size_t >( count ) );
	for ( int64_t i = 0 ; i < count ; ++i ) {
		SmacFactionTechnologyState faction;
		faction.admission.simulation_id = ReadInt32( buffer ); faction.admission.faction_id = buffer.ReadString();
		for ( auto& owned : faction.owned ) { owned = buffer.ReadBool(); }
		faction.score = ReadInt32( buffer ); faction.future_count = ReadInt32( buffer ); faction.labs = ReadInt32( buffer );
		faction.target = ReadOptional( buffer ); faction.cached_threshold = ReadOptional( buffer );
		result.factions.push_back( std::move( faction ) );
	}
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC technology state trailing fields" ); }
	result.Validate( technologies, profiles );
	return result;
}

}
