#include "Game.h"

#include <stdexcept>
#include <utility>

#include "Player.h"
#include "slot/Slot.h"
#include "slot/Slots.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"

namespace game::backend {
namespace {
int32_t OriginalGlobalDifficulty( const settings::GlobalSettings& settings ) {
	const auto& names = settings.rules.m_difficulty_levels.GetVK();
	if ( settings.difficulty_level < INT32_MIN || settings.difficulty_level > INT32_MAX ) { throw std::runtime_error( "Unknown SMAC global difficulty encoding" ); }
	const auto found = names.find( static_cast< int >( settings.difficulty_level ) );
	if ( found == names.end() ) { throw std::runtime_error( "Unknown SMAC global difficulty encoding" ); }
	return unit::SmacStartupDesignState::OriginalDifficulty( found->second );
}
}


Game::BoundSimulation Game::BindSimulation( faction::SmacSimulationState value ) const {
	const auto& settings = m_state->m_settings.global;
	if ( settings.GetRulesetId() != "smac" || !settings.IsRulesetLocked()
		|| !settings.GetSmacTechnologyRules() || !settings.GetSmacFactionStartupRules() || !settings.GetSmacPredefinedUnitRules() ) {
		throw std::runtime_error( "SMAC simulation requires frozen source profiles" );
	}
	try { value.Validate( *settings.GetSmacTechnologyRules(), *settings.GetSmacFactionStartupRules(), *settings.GetSmacPredefinedUnitRules() ); }
	catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
	BoundSimulation result{ std::move( value ), m_state->CaptureRosterEpoch(), m_state->CaptureGameSession(), {} };
	m_state->RequireRoster( result.epoch );
	const auto& slots = m_state->m_slots->GetSlots();
	if ( slots.size() != 7 ) { throw std::runtime_error( "SMAC simulation requires the accepted seven-slot controller roster" ); }
	result.players.reserve( result.value.roster.size() );
	for ( const auto& participant : result.value.roster ) {
		const auto& slot = slots[ participant.slot ];
		if ( slot.GetState() != slot::Slot::SS_PLAYER ) { throw std::runtime_error( "SMAC simulation controller is not occupied" ); }
		result.players.push_back( slot.GetPlayer()->GetIdentity() );
	}
	ValidateSimulation( result );
	return result;
}

void Game::ValidateSimulation( const BoundSimulation& simulation ) const {
	if ( !m_state || !simulation.epoch || !simulation.epoch->active || m_state->CaptureRosterEpoch() != simulation.epoch
		|| !m_state->IsGameSessionActive( simulation.session ) ) {
		throw std::runtime_error( "SMAC simulation controller roster was retired" );
	}
	m_state->RequireRoster( simulation.epoch );
	if ( simulation.value.designs.global_difficulty != OriginalGlobalDifficulty( m_state->m_settings.global ) ) {
		throw std::runtime_error( "SMAC saved global difficulty differs from accepted settings" );
	}
	const auto& slots = m_state->m_slots->GetSlots();
	if ( slots.size() != 7 || simulation.players.size() != simulation.value.roster.size() ) { throw std::runtime_error( "SMAC simulation controller roster changed" ); }
	size_t occupied = 0;
	for ( const auto& slot : slots ) { if ( slot.GetState() == slot::Slot::SS_PLAYER ) { ++occupied; } }
	if ( occupied != simulation.value.roster.size() ) { throw std::runtime_error( "SMAC simulation participant count changed" ); }
	for ( size_t i = 0; i < simulation.value.roster.size(); ++i ) {
		const auto& saved = simulation.value.roster[ i ];
		const auto& identity = simulation.players[ i ];
		const auto& slot = slots[ saved.slot ];
		if ( !identity || !identity->active || slot.GetState() != slot::Slot::SS_PLAYER || slot.GetPlayer()->GetIdentity() != identity ) {
			throw std::runtime_error( "SMAC simulation controller identity changed" );
		}
		const auto record = slot.GetPlayer()->GetRecord();
		if ( slot.GetLinkedGSID() != saved.gsid || record.role != saved.role || record.faction_id != saved.faction_id ) {
			throw std::runtime_error( "SMAC simulation controller metadata changed" );
		}
	}
}

void Game::PrepareNewSimulation() {
	if ( m_state->m_settings.global.GetRulesetId() != "smac" ) {
		if ( m_simulation || m_pending_simulation ) { throw std::runtime_error( "Unexpected simulation state for non-SMAC world" ); }
		return;
	}
	if ( m_simulation || m_pending_simulation ) { throw std::runtime_error( "New-world simulation was already admitted" ); }
	const auto& settings = m_state->m_settings.global;
	if ( !settings.GetSmacTechnologyRules() || !settings.GetSmacFactionStartupRules() || !settings.GetSmacPredefinedUnitRules() ) { throw std::runtime_error( "Missing SMAC simulation profiles" ); }
	std::vector< faction::SmacSimulationParticipant > roster;
	faction::SmacTechnologyState technology;
	faction::SmacSocialState social;
	unit::SmacStartupDesignState designs;
	std::vector< faction::SmacSimulationAdmission > admissions;
	for ( const auto& slot : m_state->m_slots->GetSlots() ) {
		if ( slot.GetState() != slot::Slot::SS_PLAYER ) { continue; }
		const auto record = slot.GetPlayer()->GetRecord();
		if ( !record.faction_id ) { throw std::runtime_error( "SMAC simulation participant has no selected source faction" ); }
		// GLSMAC new-world policy only: dense IDs in ascending occupied-slot
		// order. Restores use the explicit saved IDs and never call this path.
		const auto id = static_cast< int32_t >( admissions.size() + 1 );
		if ( slot.GetIndex() > 6 ) { throw std::runtime_error( "SMAC simulation slot exceeds supported roster" ); }
		roster.push_back( { id, static_cast< int32_t >( slot.GetIndex() ), slot.GetLinkedGSID(), record.role, *record.faction_id } );
		admissions.push_back( { id, *record.faction_id } );
	}
	try {
		technology = faction::SmacTechnologyState::NewWorld( admissions, *settings.GetSmacTechnologyRules(), *settings.GetSmacFactionStartupRules() );
		social = faction::SmacSocialState::NewWorld( admissions, *settings.GetSmacTechnologyRules(), *settings.GetSmacFactionStartupRules() );
		designs = unit::SmacStartupDesignState::NewWorld( technology, *settings.GetSmacTechnologyRules(), *settings.GetSmacPredefinedUnitRules(), OriginalGlobalDifficulty( settings ) );
	}
	catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
	// GLSMAC policy, not an original startup seed reconstruction. Each word
	// is sampled exactly once, in this order, without constructing/seeding JSF.
	// A second-sample failure publishes neither word nor the staged simulation.
	const auto shared = m_gameplay_entropy_word_source();
	const auto crt = m_gameplay_entropy_word_source();
	faction::SmacSimulationState value( SmacGameplayRandomState( shared, crt ) );
	value.roster = std::move( roster ); value.technology = std::move( technology ); value.social = std::move( social ); value.designs = std::move( designs );
	m_simulation = BindSimulation( std::move( value ) );
}

void Game::PublishPendingSimulation() {
	if ( m_pending_simulation ) {
		if ( m_simulation ) { throw std::runtime_error( "Cannot replace an admitted SMAC simulation" ); }
		ValidateSimulation( *m_pending_simulation );
		m_simulation = std::move( m_pending_simulation );
		m_pending_simulation.reset();
	}
	ValidateCurrentSimulation();
}

void Game::ValidateCurrentSimulation() const {
	if ( m_state->m_settings.global.GetRulesetId() == "smac" ) {
		if ( !m_simulation ) { throw std::runtime_error( "SMAC simulation has not been admitted" ); }
		ValidateSimulation( *m_simulation );
		m_simulation->value.social.ValidateTurn( m_current_turn.GetId() );
	}
	else if ( m_simulation || m_pending_simulation ) { throw std::runtime_error( "Unexpected simulation state for non-SMAC world" ); }
}

void Game::WriteSimulationState( types::Buffer& buffer ) const {
	ValidateCurrentSimulation();
	std::optional< std::string > wire;
	if ( m_simulation ) {
		const auto& settings = m_state->m_settings.global;
		wire = m_simulation->value.Serialize( *settings.GetSmacTechnologyRules(), *settings.GetSmacFactionStartupRules(), *settings.GetSmacPredefinedUnitRules() ).ToString();
	}
	buffer.WriteBool( wire.has_value() );
	if ( wire ) { buffer.WriteString( *wire ); }
}

std::optional< Game::BoundSimulation > Game::ReadSimulationState( types::Buffer& buffer, const size_t turn ) const {
	const auto& settings = m_state->m_settings.global;
	const bool expected = settings.GetRulesetId() == "smac";
	if ( buffer.ReadBool() != expected ) { throw std::runtime_error( "World simulation presence differs from selected ruleset" ); }
	if ( !expected ) { return {}; }
	if ( !settings.IsRulesetLocked() || !settings.GetSmacTechnologyRules() || !settings.GetSmacFactionStartupRules() || !settings.GetSmacPredefinedUnitRules() ) {
		throw std::runtime_error( "SMAC simulation requires frozen source profiles" );
	}
	// The nested reader bounds the framed payload before allocating records.
	auto value = faction::SmacSimulationState::Deserialize( buffer.ReadString( faction::SmacSimulationState::MAX_WIRE_BYTES ), *settings.GetSmacTechnologyRules(), *settings.GetSmacFactionStartupRules(), *settings.GetSmacPredefinedUnitRules() );
	value.social.ValidateTurn( turn );
	return BindSimulation( std::move( value ) );
}

void Game::RefreshSimulationForTurn( const size_t turn ) {
	if ( m_state->m_settings.global.GetRulesetId() != "smac" ) { return; }
	ValidateCurrentSimulation();
	const auto& settings = m_state->m_settings.global;
	try {
		m_simulation->value.Validate( *settings.GetSmacTechnologyRules(), *settings.GetSmacFactionStartupRules(), *settings.GetSmacPredefinedUnitRules() );
		auto staged = m_simulation->value.social;
		if ( m_current_turn.GetId() == 0 ) {
			if ( turn != 1 ) { throw std::runtime_error( "SMAC early social state requires the first turn transition" ); }
			staged.RefreshForFirstCommand( *settings.GetSmacTechnologyRules(), *settings.GetSmacFactionStartupRules() );
		}
		staged.ValidateTurn( turn );
		m_simulation->value.social = std::move( staged );
	}
	catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
}

const faction::SmacSimulationParticipant& Game::RequireSimulationParticipant( const Player* requested ) const {
	ValidateCurrentSimulation();
	if ( !m_simulation ) { throw std::runtime_error( "SMAC simulation state is unavailable for this ruleset" ); }
	const auto& slots = m_state->m_slots->GetSlots();
	for ( const auto& participant : m_simulation->value.roster ) {
		if ( slots[ participant.slot ].GetPlayer() == requested ) { return participant; }
	}
	throw std::runtime_error( "Player is not an admitted simulation participant" );
}

gse::Value* Game::CopyTechnologyState( GSE_CALLABLE, const Player* player ) const {
	const auto& participant = RequireSimulationParticipant( player );
	const auto& state = *m_simulation->value.technology.Find( participant.simulation_id );
	gse::value::array_elements_t owned;
	owned.reserve( state.owned.size() );
	for ( const auto flag : state.owned ) { owned.push_back( VALUE( gse::value::Bool,, flag ) ); }
	const auto optional = [ & ]( const std::optional< int32_t >& value ) -> gse::Value* {
		if ( value ) { return VALUE( gse::value::Int,, *value ); }
		return VALUE( gse::value::Undefined );
	};
	// Every field is a fresh scalar/array value. No Ptr/native mutator escapes.
	return VALUEEXT( gse::value::Object, GSE_CALL, ( gse::value::object_properties_t{
		{ "simulation_id", VALUE( gse::value::Int,, participant.simulation_id ) },
		{ "faction_id", VALUE( gse::value::String,, participant.faction_id ) },
		{ "owned_technologies", VALUE( gse::value::Array,, owned ) },
		{ "score", VALUE( gse::value::Int,, state.score ) },
		{ "future_count", VALUE( gse::value::Int,, state.future_count ) },
		{ "labs", VALUE( gse::value::Int,, state.labs ) },
		{ "target", optional( state.target ) },
		{ "cached_threshold", optional( state.cached_threshold ) },
	} ) );
}

gse::Value* Game::CopySocialState( GSE_CALLABLE, const Player* player ) const {
	const auto& participant = RequireSimulationParticipant( player );
	const auto& state = *m_simulation->value.social.Find( participant.simulation_id );
	const auto array = [ & ]( const auto& values ) -> gse::Value* {
		gse::value::array_elements_t result;
		for ( const auto value : values ) { result.push_back( VALUE( gse::value::Int,, value ) ); }
		return VALUE( gse::value::Array,, result );
	};
	const auto optional = [ & ]( const std::optional< faction::SmacSocialRatings >& values ) -> gse::Value* {
		if ( values ) { return array( *values ); }
		return VALUE( gse::value::Undefined );
	};
	return VALUEEXT( gse::value::Object, GSE_CALL, ( gse::value::object_properties_t{
		{ "simulation_id", VALUE( gse::value::Int,, participant.simulation_id ) },
		{ "faction_id", VALUE( gse::value::String,, participant.faction_id ) },
		{ "phase", VALUE( gse::value::String,, state.phase == faction::SmacFactionSocialState::Phase::Early ? "early" : "refreshed" ) },
		{ "selected_models", array( state.selected ) }, { "pending_models", array( state.pending ) },
		{ "current", array( state.current ) }, { "intrinsic", optional( state.intrinsic ) }, { "retained", optional( state.retained ) }
	} ) );
}

gse::Value* Game::CopyStartupDesigns( GSE_CALLABLE, const Player* player ) const {
	const auto& participant = RequireSimulationParticipant( player );
	const auto& state = m_simulation->value.designs;
	const auto& owner = *state.Find( participant.simulation_id );
	gse::value::array_elements_t designs;
	for ( const auto& d : owner.designs ) {
		designs.push_back( VALUEEXT( gse::value::Object, GSE_CALL, ( gse::value::object_properties_t{
			{ "id", VALUE( gse::value::Int,, d.id ) }, { "chassis", VALUE( gse::value::Int,, d.chassis ) },
			{ "weapon", VALUE( gse::value::Int,, d.weapon ) }, { "defense", VALUE( gse::value::Int,, d.defense ) },
			{ "reactor", VALUE( gse::value::Int,, d.reactor ) }, { "abilities", VALUE( gse::value::Int,, d.abilities ) },
			{ "cost", VALUE( gse::value::Int,, d.cost ) }, { "plan", VALUE( gse::value::Int,, d.plan ) }, { "carry", VALUE( gse::value::Int,, d.carry ) },
			{ "active", VALUE( gse::value::Bool,, d.active ) }, { "prototyped", VALUE( gse::value::Bool,, d.prototyped ) },
			{ "faction_exclusion", VALUE( gse::value::Int,, d.faction_exclusion ) }
		} ) ) );
	}
	return VALUEEXT( gse::value::Object, GSE_CALL, ( gse::value::object_properties_t{
		{ "simulation_id", VALUE( gse::value::Int,, participant.simulation_id ) },
		{ "global_difficulty", VALUE( gse::value::Int,, state.global_difficulty ) },
		{ "starting_scout", VALUE( gse::value::Int,, owner.starting_scout ) },
		{ "designs", VALUE( gse::value::Array,, designs ) }
	} ) );
}

}
