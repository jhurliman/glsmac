#include "SmacSocialState.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace game::backend::faction {
namespace {
constexpr int64_t MAGIC = 0x534d534f; // SMSO
constexpr int64_t VERSION = 1;
using Bonus = settings::SmacFactionStartupBonus;
using Phase = SmacFactionSocialState::Phase;

const settings::SmacFactionStartupRule& Profile( const std::string& id, const settings::SmacFactionStartupRules& profiles ) {
	const auto* profile = profiles.Find( id );
	if ( !profile ) { throw std::invalid_argument( "SMAC social owner has no frozen source profile" ); }
	return *profile;
}

SmacSocialRatings Intrinsic( const settings::SmacFactionStartupRule& profile ) {
	SmacSocialRatings result{};
	for ( const auto& record : profile.Records() ) {
		if ( record.kind != Bonus::Kind::SOCIAL ) { continue; }
		const int64_t sum = int64_t( result[ record.parameter ] ) + record.amount;
		if ( sum < INT32_MIN || sum > INT32_MAX ) { throw std::invalid_argument( "SMAC intrinsic social sum exceeds signed32" ); }
		result[ record.parameter ] = static_cast< int32_t >( sum );
	}
	return result;
}

SmacSocialRatings Refreshed( SmacSocialRatings result, const settings::SmacFactionStartupRule& profile ) {
	// All supported selected models contribute zero. Apply immunity after the
	// complete intrinsic sum, leaving the separate intrinsic vector unchanged.
	for ( const auto& record : profile.Records() ) {
		if ( record.kind == Bonus::Kind::IMMUNITY ) { result[ record.parameter ] = std::clamp( result[ record.parameter ], 0, 999 ); }
	}
	return result;
}

int32_t ReadInt32( types::Buffer& buffer ) {
	const auto value = buffer.ReadInt();
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::runtime_error( "SMAC social integer outside signed32" ); }
	return static_cast< int32_t >( value );
}

template< size_t N > void WriteArray( types::Buffer& buffer, const std::array< int32_t, N >& values ) {
	for ( const auto value : values ) { buffer.WriteInt( value ); }
}

template< size_t N > void ReadArray( types::Buffer& buffer, std::array< int32_t, N >& values ) {
	for ( auto& value : values ) { value = ReadInt32( buffer ); }
}

void WriteOptional( types::Buffer& buffer, const std::optional< SmacSocialRatings >& values ) {
	buffer.WriteBool( values.has_value() ); if ( values ) { WriteArray( buffer, *values ); }
}

std::optional< SmacSocialRatings > ReadOptional( types::Buffer& buffer ) {
	if ( !buffer.ReadBool() ) { return {}; }
	SmacSocialRatings values{}; ReadArray( buffer, values ); return values;
}
}

bool SmacFactionSocialState::operator==( const SmacFactionSocialState& other ) const {
	return admission == other.admission && selected == other.selected && pending == other.pending && current == other.current
		&& intrinsic == other.intrinsic && retained == other.retained && phase == other.phase;
}

void SmacSocialState::Validate( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) const {
	profiles.Validate( technologies );
	if ( factions.empty() || factions.size() > 7 ) { throw std::invalid_argument( "SMAC social state requires 1..7 admitted owners" ); }
	std::array< bool, 8 > seen{};
	for ( const auto& faction : factions ) {
		const auto id = faction.admission.simulation_id;
		if ( id < 1 || id > 7 || seen[ id ] ) { throw std::invalid_argument( "Invalid or repeated SMAC social simulation identity" ); }
		seen[ id ] = true;
		Profile( faction.admission.faction_id, profiles );
		for ( const auto* values : { &faction.selected, &faction.pending } ) {
			for ( const auto value : *values ) { if ( value != 0 ) { throw std::invalid_argument( "SMAC startup social state requires neutral model-zero selections" ); } }
		}
		if ( faction.phase == Phase::Early ) {
			for ( const auto rating : faction.current ) { if ( rating != 0 ) { throw std::invalid_argument( "SMAC early current social ratings must be zero" ); } }
		}
		else if ( faction.phase != Phase::Refreshed || !faction.intrinsic || !faction.retained ) {
			throw std::invalid_argument( "Incomplete or unknown SMAC social startup phase" );
		}
	}
}

void SmacSocialState::ValidateTurn( size_t turn ) const {
	for ( const auto& faction : factions ) {
		if ( ( faction.phase != Phase::Early && faction.phase != Phase::Refreshed )
			|| ( faction.phase == Phase::Early ) != ( turn == 0 ) ) { throw std::runtime_error( "SMAC social phase does not match world turn" ); }
	}
}

bool SmacSocialState::operator==( const SmacSocialState& other ) const { return factions == other.factions; }

const SmacFactionSocialState* SmacSocialState::Find( int32_t simulation_id ) const {
	for ( const auto& faction : factions ) { if ( faction.admission.simulation_id == simulation_id ) { return &faction; } }
	return nullptr;
}

void SmacSocialState::ResetForStartup( SmacFactionSocialState& faction ) noexcept {
	faction.selected.fill( 0 ); faction.pending.fill( 0 ); faction.current.fill( 0 ); faction.phase = Phase::Early;
}

SmacSocialState SmacSocialState::NewWorld( const std::vector< SmacSimulationAdmission >& admissions,
	const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) {
	if ( admissions.empty() || admissions.size() > 7 ) { throw std::invalid_argument( "SMAC new social state requires 1..7 explicit admissions" ); }
	SmacSocialState result;
	for ( const auto& admission : admissions ) { result.factions.push_back( { admission } ); }
	result.Validate( technologies, profiles );
	for ( const auto& faction : result.factions ) {
		// Preflight arithmetic without making the late intrinsic/retained state
		// visible to create_world or initial founding consumers.
		Intrinsic( Profile( faction.admission.faction_id, profiles ) );
	}
	return result;
}

SmacSocialRatings SmacSocialState::BuildIntrinsic( const std::string& faction_id,
	const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) {
	profiles.Validate( technologies ); return Intrinsic( Profile( faction_id, profiles ) );
}

SmacSocialRatings SmacSocialState::RefreshNeutral( const SmacSocialRatings& intrinsic, const std::string& faction_id,
	const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) {
	profiles.Validate( technologies ); return Refreshed( intrinsic, Profile( faction_id, profiles ) );
}

void SmacSocialState::RefreshForFirstCommand( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) {
	Validate( technologies, profiles );
	auto staged = *this;
	for ( auto& faction : staged.factions ) {
		if ( faction.phase != Phase::Early ) { continue; }
		const auto& profile = Profile( faction.admission.faction_id, profiles );
		faction.intrinsic = Intrinsic( profile ); faction.current = Refreshed( *faction.intrinsic, profile );
		faction.retained = faction.current; faction.phase = Phase::Refreshed;
	}
	*this = std::move( staged );
}

types::Buffer SmacSocialState::Serialize( const settings::SmacTechnologyRules& technologies, const settings::SmacFactionStartupRules& profiles ) const {
	Validate( technologies, profiles );
	types::Buffer buffer; buffer.WriteInt( MAGIC ); buffer.WriteInt( VERSION ); buffer.WriteInt( factions.size() );
	for ( const auto& faction : factions ) {
		buffer.WriteInt( faction.admission.simulation_id ); buffer.WriteString( faction.admission.faction_id ); buffer.WriteInt( static_cast< int32_t >( faction.phase ) );
		WriteArray( buffer, faction.selected ); WriteArray( buffer, faction.pending ); WriteArray( buffer, faction.current );
		WriteOptional( buffer, faction.intrinsic ); WriteOptional( buffer, faction.retained );
	}
	if ( buffer.lenw > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC social state exceeds wire budget" ); }
	return buffer;
}

SmacSocialState SmacSocialState::Deserialize( const std::string& wire, const settings::SmacTechnologyRules& technologies,
	const settings::SmacFactionStartupRules& profiles ) {
	if ( wire.size() > MAX_WIRE_BYTES ) { throw std::runtime_error( "SMAC social state exceeds wire budget" ); }
	try {
		types::Buffer buffer( wire );
		if ( buffer.ReadInt() != MAGIC || buffer.ReadInt() != VERSION ) { throw std::runtime_error( "Unsupported SMAC social format" ); }
		const auto count = buffer.ReadInt();
		if ( count < 1 || count > 7 ) { throw std::runtime_error( "SMAC social state requires 1..7 owners" ); }
		SmacSocialState result;
		for ( int64_t i = 0; i < count; ++i ) {
			SmacFactionSocialState faction;
			faction.admission.simulation_id = ReadInt32( buffer ); faction.admission.faction_id = buffer.ReadString( 32 );
			const auto phase = buffer.ReadInt();
			if ( phase < 0 || phase > 1 ) { throw std::runtime_error( "Unknown SMAC social phase" ); }
			faction.phase = static_cast< Phase >( phase );
			ReadArray( buffer, faction.selected ); ReadArray( buffer, faction.pending ); ReadArray( buffer, faction.current );
			faction.intrinsic = ReadOptional( buffer ); faction.retained = ReadOptional( buffer );
			result.factions.push_back( std::move( faction ) );
		}
		if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Unexpected SMAC social trailing fields" ); }
		result.Validate( technologies, profiles ); return result;
	}
	catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
}

}
