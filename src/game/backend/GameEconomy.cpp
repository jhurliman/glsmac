#include "Game.h"

#include "Player.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/FacilityDef.h"
#include "base/SmacBaseEnergy.h"
#include "faction/Faction.h"
#include "map/Map.h"
#include "slot/Slot.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
#include "gc/Space.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend {
namespace {
int32_t Checked( int64_t value ) {
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::overflow_error( "Canonical economy exceeds signed32" ); }
	return static_cast< int32_t >( value );
}
}

faction::SmacEconomyState Game::GetCanonicalEconomy( const slot::Slot* owner ) const {
	RequireSmacLocalHumanOwner( owner );
	for ( const auto& participant : m_simulation->value.roster ) {
		if ( participant.slot == owner->GetIndex() ) {
			const auto& result = m_simulation->value.economy.at( participant.simulation_id );
			result.Validate(); return result;
		}
	}
	throw std::runtime_error( "Canonical economy owner is not admitted" );
}

base::SmacBaseEnergyCalculation Game::GetCanonicalBaseEnergy( const base::Base& evaluating_base ) const {
	return CalculateCanonicalBaseEnergy( evaluating_base, evaluating_base.PrepareSmacResourceCalculation() );
}

base::SmacBaseEnergyCalculation Game::CalculateCanonicalBaseEnergy( const base::Base& evaluating_base,
	const base::SmacBaseResourceCalculation& resources ) const {
	using namespace base;
	if ( evaluating_base.GetGame() != this || !m_bm || m_bm->GetBase( evaluating_base.m_id ) != &evaluating_base || !m_map ) {
		throw std::runtime_error( "Energy calculation requires a registered Base in this world" );
	}
	const auto economy = GetCanonicalEconomy( evaluating_base.m_owner );
	const auto context = evaluating_base.GetSmacResourceContext();
	if ( m_simulation->value.movement_capability != faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain ) {
		throw std::runtime_error( "Energy calculation requires the explicit no-project campaign" );
	}
	for ( const auto& owner : m_simulation->value.economy ) {
		owner.second.Validate();
		if ( ( owner.first == context.owner_simulation_id && owner.second.treaty_partners )
			|| ( owner.second.treaty_partners & ( 1u << context.owner_simulation_id ) ) ) {
			throw std::runtime_error( "Treaty commerce pairing is not connected" );
		}
	}
	const auto* social = m_simulation->value.social.Find( context.owner_simulation_id );
	if ( !social || social->phase != faction::SmacFactionSocialState::Phase::Refreshed ) {
		throw std::runtime_error( "Energy calculation requires current refreshed EFFIC" );
	}
	const auto& global = m_state->m_settings.global;
	const auto& profiles = global.GetSmacFactionStartupRules();
	const auto& technologies = global.GetSmacTechnologyRules();
	const auto& facilities = global.GetSmacProductionRules();
	const auto& specialists = global.GetSmacSpecialistRules();
	if ( !profiles || !technologies || !facilities || !specialists || !evaluating_base.m_faction ) {
		throw std::runtime_error( "Energy calculation requires complete frozen profiles" );
	}
	profiles->Validate( *technologies ); facilities->Validate(); specialists->Validate();
	const auto* profile = profiles->Find( evaluating_base.m_faction->m_id );
	if ( !profile ) { throw std::runtime_error( "Energy faction profile is unavailable" ); }
	std::array< bool, 34 > local{}, free_facility{};
	for ( const auto& record : profile->Records() ) {
		if ( record.kind == settings::SmacFactionStartupBonus::Kind::FACILITY ) {
			if ( record.parameter < 1 || record.parameter > 33 ) { throw std::runtime_error( "Free facility maintenance requires an ordinary local facility" ); }
			free_facility[ record.parameter ] = true;
		}
	}
	std::vector< SmacEnergyCoordinate > headquarters;
	int32_t maintenance = 0;
	for ( const auto& item : m_bm->GetBases() ) {
		const auto* candidate = item.second;
		if ( !candidate || candidate->m_id != item.first || candidate->GetGame() != this ) { throw std::runtime_error( "Invalid energy Base registry" ); }
		const auto candidate_context = candidate->GetSmacResourceContext();
		std::array< bool, 34 > seen{};
		for ( const auto& entry : candidate->m_facilities ) {
			if ( !entry.second || m_bm->GetFacilityDef( entry.first ) != entry.second ) { throw std::runtime_error( "Energy facility is not registered" ); }
			m_bm->ValidateSmacFacilityDef( entry.second );
			if ( !entry.second->m_smac_source_id || *entry.second->m_smac_source_id < 1 || *entry.second->m_smac_source_id > 33 ) {
				throw std::runtime_error( "Energy calculation does not admit unresolved orbital/project facilities" );
			}
			const auto id = *entry.second->m_smac_source_id;
			if ( seen[ id ] ) { throw std::runtime_error( "Duplicate semantic energy facility" ); }
			seen[ id ] = true;
			if ( id == 1 && candidate_context.owner_simulation_id == context.owner_simulation_id ) {
				const auto* tile = candidate->GetTile();
				if ( !tile || tile->tiles != m_map->GetTilesPtr() || tile->base != candidate ) { throw std::runtime_error( "Energy headquarters membership is invalid" ); }
				headquarters.push_back( { Checked( tile->coord.x ), Checked( tile->coord.y ) } );
			}
			if ( candidate != &evaluating_base ) { continue; }
			local[ id ] = true;
			if ( free_facility[ id ] ) { continue; }
			if ( id == 27 ) { throw std::runtime_error( "Command Center maintenance pricing is not connected" ); }
			const auto fee = facilities->facilities.at( id - 1 ).maintenance;
			if ( fee < 0 ) { throw std::runtime_error( "Negative facility maintenance is outside ordinary posting" ); }
			maintenance = Checked( int64_t( maintenance ) + fee );
		}
	}
	const auto allocation = evaluating_base.GetSmacAllocation();
	const auto& production = evaluating_base.GetProductionState();
	if ( !allocation || !production ) { throw std::runtime_error( "Energy calculation requires exact allocation and production authority" ); }
	ValidateSmacProductionState( evaluating_base.m_owner, *production );
	std::vector< SmacEnergyCategories > outputs;
	if ( allocation->specialists.total_count < 0 || allocation->specialists.total_count > 16 ) { throw std::runtime_error( "Energy specialist count exceeds stored slots" ); }
	for ( int32_t i = 0; i < allocation->specialists.total_count; ++i ) {
		const auto& row = specialists->specialists.at( allocation->specialists.types[ i ] );
		outputs.push_back( { row.economy, row.psych, row.labs } );
	}
	const auto* tile = evaluating_base.GetTile();
	if ( !tile || tile->tiles != m_map->GetTilesPtr() || tile->base != &evaluating_base ) { throw std::runtime_error( "Energy Base membership is invalid" ); }
	const auto raw = Checked( resources.raw_intake[ 2 ] );
	const auto distance = SmacHeadquartersDistance( Checked( m_map->GetWidth() ), true /* actual GLSMAC tile grid wraps X */,
		{ Checked( tile->coord.x ), Checked( tile->coord.y ) }, headquarters );
	const auto waste = CalculateSmacEnergyWaste( raw, distance, social->current[ 1 ], local[ 2 ] );
	const auto net = Checked( int64_t( raw ) - waste );
	const auto categories = AllocateSmacEnergy( net, economy.rates, outputs );
	// The definition constructor clears flat labs. The fully validated admitted
	// keyword set has no direct RESEARCH writer; current social RESEARCH is a
	// separate later submission multiplier and must not be substituted here.
	SmacEnergyTailInputs inputs{ categories.after_specialists, economy.rates, social->current[ 1 ], 0 };
	inputs.facilities = local;
	inputs.current_stockpile = production->current_item.kind == ProductionItemKind::Stockpile;
	inputs.baseline_stockpile = production->baseline_item.kind == ProductionItemKind::Stockpile;
	inputs.mineral_surplus = resources.minerals.surplus;
	return { raw, distance, waste, net, maintenance, categories, FinishSmacEnergy( inputs ) };
}

void Game::PostCanonicalBaseEconomy( const base::Base& evaluating_base, const base::SmacBaseResourceCalculation& resources ) {
	if ( !GetGCSpace()->IsAccumulating() || !IsRunning() ) { throw std::runtime_error( "Economy posting requires the owning native upkeep" ); }
	const auto result = CalculateCanonicalBaseEnergy( evaluating_base, resources );
	const auto owner = evaluating_base.GetSmacResourceContext().owner_simulation_id;
	auto& saved = m_simulation->value.economy.at( owner );
	auto next = saved;
	const auto* social = m_simulation->value.social.Find( owner );
	auto technology = m_simulation->value.technology.factions.begin();
	for ( ; technology != m_simulation->value.technology.factions.end(); ++technology ) { if ( technology->admission.simulation_id == owner ) { break; } }
	if ( !social || technology == m_simulation->value.technology.factions.end() || GetTurnId() > INT32_MAX ) { throw std::runtime_error( "Research posting owner/turn is invalid" ); }
	bool punishment_sphere = false;
	for ( const auto& entry : evaluating_base.m_facilities ) { if ( entry.second->m_smac_source_id == 23 ) { punishment_sphere = true; } }
	const auto difficulty = unit::SmacStartupDesignState::OriginalDifficulty( evaluating_base.m_owner->GetPlayer()->GetDifficultyLevel() );
	const auto submission = faction::ScaleSmacResearchSubmission( result.tail.final.labs, social->current[ 10 ],
		static_cast< int32_t >( GetTurnId() ), difficulty, punishment_sphere, saved.research_fraction );
	auto progress = technology->labs;
	if ( submission.increment > 0 ) {
		if ( !technology->target || !technology->cached_threshold || technology->owned.at( *technology->target ) ) {
			throw std::runtime_error( "Positive research requires a selected unowned target and cached cost" );
		}
		progress = Checked( int64_t( progress ) + submission.increment );
		if ( progress >= *technology->cached_threshold ) { throw std::runtime_error( "Research completion and Secrets processing are not connected" ); }
	}
	next.research_fraction = submission.fraction;
	next = next.CreditBase( result.tail.final.economy, result.maintenance );
	// No callback or throwing operation between the research and balance writes.
	technology->labs = progress; saved = next;
}

faction::SmacEconomyState Game::PrepareSmacNonzeroInitialization( int32_t owner ) const {
	if (!m_simulation || GetTurnId()==0) {throw std::runtime_error("Faction reinitialization requires a running nonzero turn");}
	const auto& state=m_simulation->value;
	const auto participant=std::find_if(state.roster.begin(),state.roster.end(),[&](const auto& row){return row.simulation_id==owner;});
	const auto& global=m_state->m_settings.global;
	const auto* profile=participant==state.roster.end() ? nullptr : global.GetSmacFactionStartupRules()->Find(participant->faction_id);
	if (!profile || !global.GetSmacResourceRules()) {throw std::runtime_error("Faction reinitialization has no frozen inputs");}
	int32_t bonus=0;
	for (const auto& row:profile->bonuses) {if(row.kind==settings::SmacFactionStartupBonus::Kind::ENERGY) {bonus=row.parameter;}}
	auto candidate=state.economy.at(owner);
	const int64_t credits=int64_t(candidate.credits)+std::clamp(global.GetSmacResourceRules()->initial_energy,0,1000)+bonus;
	if (credits<INT32_MIN || credits>INT32_MAX) {throw std::overflow_error("Reinitialized treasury exceeds signed32");}
	candidate.credits=static_cast<int32_t>(credits);candidate.pending_debit=0;candidate.Validate();return candidate;
}

void Game::SettleCanonicalEconomy( int32_t owner ) {
	ValidateCurrentSimulation();
	if ( !GetGCSpace()->IsAccumulating() || !IsRunning() ) { throw std::runtime_error( "Economy settlement requires the owning native upkeep" ); }
	auto& saved = m_simulation->value.economy.at( owner );
	const bool has_base=std::any_of(m_bm->GetBases().begin(),m_bm->GetBases().end(),[&](const auto& row){return row.second->GetSmacResourceContext().owner_simulation_id==owner;});
	const bool has_unit=std::any_of(m_um->GetUnits().begin(),m_um->GetUnits().end(),[&](const auto& row){return row.second->Conventional().owner_simulation_id==owner;});
	if (!has_base && !has_unit) {
		const auto initialized=PrepareSmacNonzeroInitialization(owner);
		auto technology=std::find_if(m_simulation->value.technology.factions.begin(),m_simulation->value.technology.factions.end(),[&](const auto& row){return row.admission.simulation_id==owner;});
		if(technology==m_simulation->value.technology.factions.end()) {throw std::runtime_error("Empty faction has no technology authority");}
		saved=initialized;technology->cached_threshold.reset();
	}
	const auto next = saved.SettleDebit(); saved = next;
}

}
