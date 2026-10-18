#include "Game.h"

#include <stdexcept>
#include <algorithm>
#include <utility>

#include "Player.h"
#include "slot/Slot.h"
#include "slot/Slots.h"
#include "unit/UnitManager.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/FacilityDef.h"
#include "map/Map.h"
#include "map/SmacPlanetMap.h"
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
	if ( simulation.value.movement_history ) {
		const auto& selected = m_state->m_settings.global.map.GetSmacWorldSize();
		if ( !selected ) { throw std::runtime_error( "Movement history requires an accepted world geometry" ); }
		simulation.value.movement_history->ValidateDimensions( selected->width, selected->height );
	}
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
	faction::SmacCapabilityState capabilities;
	faction::SmacHostileState hostility;
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
		hostility.owners.emplace(id,faction::SmacHostileOwner::NewWorld(m_smac_secondary_relations.at(id)));
	}
	try {
		technology = faction::SmacTechnologyState::NewWorld( admissions, *settings.GetSmacTechnologyRules(), *settings.GetSmacFactionStartupRules() );
		social = faction::SmacSocialState::NewWorld( admissions, *settings.GetSmacTechnologyRules(), *settings.GetSmacFactionStartupRules() );
		if(!settings.GetSmacCombatRules() || (!settings.GetSmacCampaignRules() && !settings.IsOrdinarySmacCampaign())) {
			throw std::runtime_error("New capability state requires frozen combat and campaign rules");
		}
		// The existing legacy generated-campaign contract explicitly admits no
		// restriction flags without a campaign record. Other missing contexts reject.
		const bool no_colony=settings.GetSmacCampaignRules() && (settings.GetSmacCampaignRules()->primary&0x00400000u)!=0;
		designs = unit::SmacStartupDesignState::NewWorld( technology, *settings.GetSmacTechnologyRules(), *settings.GetSmacPredefinedUnitRules(), OriginalGlobalDifficulty( settings ),
			[&](const auto& owner) {
				std::array<uint32_t,8> relations{};
				for(size_t i=0;i<relations.size();++i) {relations[i]=hostility.owners.at(owner.admission.simulation_id).directions[i].primary;}
				// Actual per-owner generation side effect before MarkAndSelect. The
				// admitted definition constructor has PSI0 and this new campaign
				// explicitly excludes projects; neither is inferred from Unit counts.
				capabilities.Refresh(owner.admission.simulation_id,owner,*settings.GetSmacTechnologyRules(),settings.GetSmacPredefinedUnitRules()->catalog,
					*settings.GetSmacCombatRules(),relations,0,false,false,no_colony);
			});
	}
	catch ( const std::invalid_argument& error ) { throw std::runtime_error( error.what() ); }
	// GLSMAC policy, not an original startup seed reconstruction. Each word
	// is sampled exactly once, in this order, without constructing/seeding JSF.
	// A second-sample failure publishes neither word nor the staged simulation.
	std::optional<faction::SmacEcologyWorld> ecology_world;
	if(m_map && m_map->GetResourcePolicy()) {
		if(const auto* planet=m_map->GetResourcePolicy()->OriginalPlanet()) {
			ecology_world=faction::SmacEcologyWorld{planet->NativeLifeCategory(),planet->CloudCoverCategory(),planet->CumulativeSeaLevelOffset()};
			ecology_world->Validate();
		}
	}
	const auto shared = m_gameplay_entropy_word_source();
	const auto crt = m_gameplay_entropy_word_source();
	// Explicit GLSMAC supported campaign: project effects are not activated.
	// This is not a claim about original setup or a missing-project fallback.
	faction::SmacSimulationState value( SmacGameplayRandomState( shared, crt ), faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain,
		faction::SmacGlobalResourceModifiers::NewWorld() );
    // Every occupied controller admitted by the current startup path is an
    // explicitly selected human Player. AI admission must supply its own
    // control authority; it must not inherit a transport-role heuristic.
    for(const auto& participant:roster) {value.human_membership|=static_cast<uint8_t>(1u<<participant.simulation_id);}
	value.roster = std::move( roster ); value.technology = std::move( technology ); value.social = std::move( social ); value.designs = std::move( designs );
	value.capabilities=capabilities;value.hostility=std::move(hostility);
	value.regional_military.emplace(); // explicit fresh-world ushort counters, all positive slots
	// Whole-new-world reset follows setup terrain generation in the native
	// lifecycle. Do not derive the retained status from the generated map here.
	value.retained_land_region_concentration=false;
	value.retained_base_totals=faction::SmacBaseTotals{};
	value.regional_strategy=m_smac_cross_world_strategy;
	value.ranking_scores=m_smac_cross_world_scores;
	value.ecology_history=m_smac_cross_world_ecology;
	value.ecology_world=ecology_world;
	value.perihelion=false; // accepted fresh-world reset, never replayed on restore
	value.victory_history.emplace(); // measured fresh economic/history words; no Voice project
	for(int32_t owner=0;owner<=7;++owner) {value.victory_history->Initialize(owner,0);}
	for(auto& owner:value.regional_military->factions) {owner.unit_phase_flags.emplace();owner.base_phase_flags.emplace();}
	value.strategy.emplace();
	for ( const auto& participant : value.roster ) {
		if(value.ecology_history) {value.ecology_history->Initialize(participant.simulation_id,0);}
		value.satellites.emplace( participant.simulation_id, faction::SmacSatelliteCounts::NewWorld() );
		const auto& resources = settings.GetSmacResourceRules();
		const auto* profile = settings.GetSmacFactionStartupRules()->Find( participant.faction_id );
		if ( !resources || !profile ) { throw std::runtime_error( "New-world economy requires frozen resource and faction inputs" ); }
		value.strategy->emplace(participant.simulation_id,faction::SmacFactionStrategy::NewWorld(*profile));
		int32_t bonus = 0;
		// ENERGY is a scalar assignment, independent of the shared eight-record
		// list (original definition+4e8); later supported entries replace it.
		for ( const auto& entry : profile->bonuses ) {
			if ( entry.kind == settings::SmacFactionStartupBonus::Kind::ENERGY ) { bonus = entry.parameter; }
		}
		const int64_t credits = int64_t( std::clamp( resources->initial_energy, 0, 1000 ) ) + bonus;
		if ( credits < INT32_MIN || credits > INT32_MAX ) { throw std::overflow_error( "Starting treasury exceeds signed32" ); }
		// STARTUP-ECONOMY001 supplies sliders, treasury, debit and no treaties.
		// Original5c8d0e explicitly clears the same fractional research word
		// used by submission. Restore retains its saved value without replay.
		value.economy.emplace( participant.simulation_id, faction::SmacEconomyState::NewWorld( { 0, 5 }, static_cast< int32_t >( credits ), 0 ) );
	}
	if ( m_active_admission && (m_active_admission->generated_map || m_active_admission->installed_planet) ) {
		const auto& selected = settings.map.GetSmacWorldSize();
		if ( !selected ) { throw std::runtime_error( "Generated movement history requires accepted world geometry" ); }
		value.movement_history = SmacMovementHistory::NewGeneratedWorld( selected->width, selected->height );
	}
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

std::optional<bool> Game::PrepareSmacPerihelionForTurn(const size_t turn) const {
	if(m_state->m_settings.global.GetRulesetId()!="smac") {return {};}
	ValidateCurrentSimulation();
	const auto active=m_simulation->value.perihelion;
	if(!active) {return {};}
	// Ordinary campaigns currently use the same2100 origin as the turn event.
	// Scenarios/custom calendars require their own admitted saved authority.
	if(turn>static_cast<size_t>(INT32_MAX-2100)) {throw std::overflow_error("Ordinary mission year exceeds signed32");}
	return faction::AdvanceSmacPerihelion(*active,static_cast<int32_t>(turn),2100+static_cast<int32_t>(turn),true).active;
}

void Game::RefreshSimulationForTurn( const size_t turn ) {
	if ( m_state->m_settings.global.GetRulesetId() != "smac" ) { return; }
	ValidateCurrentSimulation();
	const auto& settings = m_state->m_settings.global;
	try {
		m_simulation->value.Validate( *settings.GetSmacTechnologyRules(), *settings.GetSmacFactionStartupRules(), *settings.GetSmacPredefinedUnitRules() );
		auto staged = *m_simulation;
		if ( m_current_turn.GetId() == 0 ) {
			if ( turn != 1 ) { throw std::runtime_error( "SMAC early social state requires the first turn transition" ); }
			staged.value.social.RefreshForFirstCommand( *settings.GetSmacTechnologyRules(), *settings.GetSmacFactionStartupRules() );
		}
		staged.value.social.ValidateTurn( turn );
		if ( m_um->HasPendingConventionalUnits() ) {
			RequireConventionalStartupClaim();
			m_um->PreflightConventionalStartup( [ & ]( const auto& unit, const auto* owner ) {
				return ResolveConventionalUnit( unit, owner, staged );
			} );
		}
		// All validation/allocation is complete. These transfers have no callbacks;
		// AdvanceTurn publishes the scalar turn immediately afterward, before hooks.
		m_simulation->value.social = std::move( staged.value.social );
		m_um->SealConventionalStartup();
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

unit::SmacCustomDesign Game::ResolveConventionalDesign( const unit::ConventionalUnitState& state, const BoundSimulation& simulation ) const {
	const auto& catalog = m_state->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
	unit::SmacCustomDesign design;

	if ( state.design_id < 64 ) {
		for ( const auto& row : catalog.units ) {
			if ( row.source_id == static_cast< uint32_t >( state.design_id ) ) {
				design = { state.design_id, row.chassis_id, row.weapon_id, row.defense_id, row.reactor, row.cost, row.plan,
					row.carry, row.abilities, row.faction_exclusion, row.active, row.prototyped };
				return design;
			}
		}
	}
	else if ( const auto* designs = simulation.value.designs.Find( state.owner_simulation_id ) ) {
		for ( const auto& row : designs->designs ) { if ( row.id == state.design_id ) { return row; } }
	}
	throw std::runtime_error( "Conventional design not found in its owner's catalog" );
}

void Game::RequireConventionalStartupClaim() const {
	std::lock_guard< std::mutex > lock( m_admission_mutex );
	if ( !m_state || !m_state->IsMaster() || m_game_state != GS_RUNNING || !m_owner_transition
		|| m_owner_transition->phase != OwnerTransition::Phase::Running
		|| m_owner_transition->thread != std::this_thread::get_id()
		|| m_owner_transition->stage != StartupStage::Start
		|| !m_active_admission || m_owner_transition->admission != m_active_admission
		|| m_active_admission->cancelled || m_active_admission->kind != WorldAdmission::Kind::INITIALIZATION
		|| m_active_admission->previous || m_active_admission->map_path || m_active_admission->campaign_world
		|| m_active_admission->state != m_state || !m_state->IsGameSessionActive( m_active_admission->admitted_binding )
		|| m_current_turn.GetId() != 0 || !m_simulation || m_pending_simulation ) {
		throw std::runtime_error( "Early conventional admission requires the claimed new-world Start at turn zero" );
	}
}

unit::ConventionalUnitState Game::PrepareStartingConventionalUnit( const slot::Slot* owner, unit::SmacMoraleMode mode ) const {
	RequireConventionalStartupClaim();
	ValidateCurrentSimulation();
	// The raw initializer has no retained-social dependency. It does not query
	// effective morale or full movement, and it accepts no caller float stats.
	for ( const auto& participant : m_simulation->value.roster ) {
		if ( &m_state->m_slots->GetSlot( participant.slot ) != owner ) { continue; }
		const auto* social = m_simulation->value.social.Find( participant.simulation_id );
		const auto* designs = m_simulation->value.designs.Find( participant.simulation_id );
		if ( !social || social->phase != faction::SmacFactionSocialState::Phase::Early || social->retained || !designs ) {
			throw std::runtime_error( "Starting conventional unit requires unrefreshed admitted startup state" );
		}
		unit::ConventionalUnitState result{ designs->starting_scout, participant.simulation_id, 1, 0, 0, false,
			unit::ConventionalUnitState::Home::None, mode };
		ResolveConventionalIdentity( result, owner, *m_simulation );
		return result;
	}
	throw std::runtime_error( "Starting conventional owner is not an admitted participant" );
}

unit::ConventionalUnitContext Game::ResolveConventionalUnit( const unit::ConventionalUnitState& state, const slot::Slot* owner ) const {
	ValidateCurrentSimulation();
	if ( !m_simulation ) { throw std::runtime_error( "Conventional units require an admitted SMAC simulation" ); }
	return ResolveConventionalUnit( state, owner, *m_simulation );
}

unit::ConventionalUnitContext Game::ResolveConventionalUnit( const unit::ConventionalUnitState& state, const slot::Slot* owner, const BoundSimulation& simulation, bool defer_home ) const {
	auto result = ResolveConventionalIdentity( state, owner, simulation );
	const auto* social = simulation.value.social.Find( state.owner_simulation_id );
	if ( !social || social->phase != faction::SmacFactionSocialState::Phase::Refreshed || !social->retained ) {
		throw std::runtime_error( "Conventional units require refreshed retained social state" );
	}
	result.retained_morale = social->retained->at( 4 );
	if ( state.home == unit::ConventionalUnitState::Home::Base ) {
		// The complete downloaded Base records load after Units. Do not bind a
		// saved ID into the destination's old world or fabricate home morale.
		if ( defer_home ) { return result; }
		const auto* home = m_bm->GetBase( static_cast< size_t >( *state.home_base_id ) );
		if ( !home || home->m_owner != owner || home->GetGame() != this
			|| home->GetSmacResourceContext().owner_simulation_id != state.owner_simulation_id ) {
			throw std::runtime_error( "Conventional home must be an existing same-owner canonical Base" );
		}
		bool creche = false;
		for ( const auto& item : home->m_facilities ) {
			if ( !item.second || m_bm->GetFacilityDef( item.first ) != item.second ) { throw std::runtime_error( "Unregistered conventional home facility" ); }
			m_bm->ValidateSmacFacilityDef( item.second );
			if ( !item.second->m_smac_source_id ) { throw std::runtime_error( "Conventional home facility lacks positional identity" ); }
			if ( *item.second->m_smac_source_id == 2 ) { creche = true; }
		}
		result.home = unit::SmacMoraleHome{ creche, home->GetSmacResourceContext().drone_riots };
	}
	unit::QueryConventionalUnit( state, result, state.render_morale_mode );
	return result;
}

unit::ConventionalUnitContext Game::ResolveConventionalIdentity( const unit::ConventionalUnitState& state, const slot::Slot* owner, const BoundSimulation& simulation ) const {
	state.Validate();
	ValidateSimulation( simulation );
	const auto& global = m_state->m_settings.global;
	const auto& movement = global.GetSmacMovementRules();
	const auto& profiles = global.GetSmacFactionStartupRules();
	const auto& technologies = global.GetSmacTechnologyRules();
	const auto& units = global.GetSmacPredefinedUnitRules();
	if ( !global.IsRulesetLocked() || !movement || !profiles || !technologies || !units ) {
		throw std::runtime_error( "Conventional units require complete frozen source profiles" );
	}
	// Validate every raw bonus, including records after the shared cap. The
	// supported profile cannot contain direct MORALE, so absence means 0/OFF.
	profiles->Validate( *technologies );
	const faction::SmacSimulationParticipant* participant = nullptr;
	for ( const auto& candidate : simulation.value.roster ) {
		if ( &m_state->m_slots->GetSlot( candidate.slot ) == owner && candidate.simulation_id == state.owner_simulation_id ) {
			participant = &candidate; break;
		}
	}
	if ( !participant || !participant->active || !profiles->Find( participant->faction_id ) ) { throw std::runtime_error( "Conventional owner does not match an active simulation participant" ); }
	const auto& catalog = units->catalog;
	simulation.value.designs.Validate( catalog );
	const auto design = ResolveConventionalDesign( state, simulation );
	if ( !design.active || design.chassis > 1 || design.defense != 0 || design.reactor != 1
		|| design.abilities != 0 || design.faction_exclusion != 0
		|| catalog.chassis.chassis.size() <= design.chassis || catalog.weapons.weapons.size() <= design.weapon
		|| catalog.defenses.defenses.empty() || catalog.reactors.reactors.empty() ) {
		throw std::runtime_error( "Unsupported conventional ground design" );
	}
	using namespace loader::txt;
	const auto& chassis = catalog.chassis.chassis[ design.chassis ];
	const auto& weapon = catalog.weapons.weapons[ design.weapon ]; const auto& defense = catalog.defenses.defenses[ 0 ];
	const bool combat = weapon.mode == WeaponMode::Projectile && weapon.offense == 1 && design.plan == 3;
	const bool former = design.chassis == 0 && weapon.mode == WeaponMode::Terraformer && weapon.offense == 0 && design.plan == 9;
	const bool colony = design.chassis == 0 && weapon.mode == WeaponMode::Colonist && weapon.offense == 0 && design.plan == 8;
	if ( chassis.triad != ChassisTriad::Land || chassis.missile || chassis.range != 0 || chassis.cargo != 1
		|| ( !combat && !former && !colony ) || defense.mode != DefenseMode::Projectile
		|| defense.rating != 1 || catalog.reactors.reactors[ 0 ].power != 1 ) {
		throw std::runtime_error( "Unsupported conventional component semantics" );
	}
	if ( !former && ( state.work_order != unit::ConventionalUnitState::WorkOrder::None || state.work_progress != 0 ) ) {
		throw std::runtime_error( "Only admitted Former packages may retain terraforming work" );
	}
	using Ground = settings::SmacMovementRules::GroundChassis;
	const auto ground = design.chassis == 0 ? Ground::Infantry : Ground::Speeder;
	unit::ConventionalUnitContext result{ ground, movement->Quantum(), movement->OrdinaryGroundAllowance( ground, chassis.speed ),
		std::nullopt, design.reactor };
	if ( former ) {
		result.package = unit::ConventionalUnitContext::Package::Former;
		result.presentation = unit::ConventionalUnitContext::Presentation::FormerMarker;
	}
	if ( colony ) {
		result.package = unit::ConventionalUnitContext::Package::Colony;
		result.presentation = unit::ConventionalUnitContext::Presentation::ColonyMarker;
	}
	if ( ground == Ground::Speeder ) {
		const auto* selected = simulation.value.designs.Find( state.owner_simulation_id );
		// This renderer has fixed Spartan source material. An arbitrary Speeder,
		// another faction's design, or a predefined Rover is not interchangeable.
		if ( participant->faction_id != "SPARTANS" || state.design_id < 64 || !selected || selected->starting_scout != state.design_id ) {
			throw std::runtime_error( "Speeder presentation requires the selected owned Spartan startup design" );
		}
		result.presentation = unit::ConventionalUnitContext::Presentation::SelectedSpartanRover;
	}
	return result;
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
