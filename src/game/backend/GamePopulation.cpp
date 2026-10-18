#include "Game.h"

#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/FacilityDef.h"
#include "base/SmacPopulationStep.h"
#include "faction/Faction.h"
#include <limits>
#include <stdexcept>

namespace game::backend {

base::SmacPopulationRules Game::GetCanonicalPopulationRules( const base::Base& evaluating_base ) const {
	return CalculateCanonicalPopulationRules( evaluating_base, evaluating_base.GetPopulation() );
}
base::SmacPopulationRules Game::CalculateCanonicalPopulationRules( const base::Base& evaluating_base, size_t population, const base::FacilityDef* granted ) const {
	if ( evaluating_base.GetGame() != this || !m_bm || m_bm->GetBase( evaluating_base.m_id ) != &evaluating_base ) {
		throw std::runtime_error( "Population rules require a registered Base in this world" );
	}
	RequireSmacLocalHumanOwner( evaluating_base.m_owner );
	const auto context = evaluating_base.GetSmacResourceContext();
	if ( m_simulation->value.movement_capability != faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain ) {
		throw std::runtime_error( "Population rules require the explicit no-project campaign" );
	}
	const auto* social = m_simulation->value.social.Find( context.owner_simulation_id );
	if ( !social || social->phase != faction::SmacFactionSocialState::Phase::Refreshed ) {
		throw std::runtime_error( "Population rules require current refreshed GROWTH" );
	}
	const auto& global = m_state->m_settings.global;
	const auto& rules = global.GetSmacResourceRules();
	const auto& profiles = global.GetSmacFactionStartupRules();
	const auto& technologies = global.GetSmacTechnologyRules();
	const auto& selection = global.map.GetSmacWorldSize();
	if ( !rules || !profiles || !technologies || !selection || !evaluating_base.m_faction ) {
		throw std::runtime_error( "Population rules require frozen resources, faction and accepted world category" );
	}
	rules->Validate( *technologies );
	selection->Validate();
	const auto* profile = profiles->Find( evaluating_base.m_faction->m_id );
	if ( !profile ) { throw std::runtime_error( "Population faction profile is unavailable" ); }
	int32_t population_penalty = 0;
	// POPULATION assigns a scalar, including entries after the eight-record
	// budget. Repetition replaces the previous value; it does not add penalties.
	for ( const auto& bonus : profile->bonuses ) {
		if ( bonus.kind == settings::SmacFactionStartupBonus::Kind::POPULATION ) { population_penalty = bonus.parameter; }
	}
	bool creche = false, hab_complex = false, hab_dome = false;
	for ( const auto& registered : m_bm->GetBases() ) {
		const auto* base = registered.second;
		if ( !base || base->m_id != registered.first ) { throw std::runtime_error( "Invalid population Base registry" ); }
		for ( const auto& entry : base->m_facilities ) {
			if ( !entry.second || m_bm->GetFacilityDef( entry.first ) != entry.second ) {
				throw std::runtime_error( "Population facility is not registered" );
			}
			m_bm->ValidateSmacFacilityDef( entry.second );
			if ( !entry.second->m_smac_source_id || *entry.second->m_smac_source_id < 1 || *entry.second->m_smac_source_id > 33 ) {
				throw std::runtime_error( "Population rules do not admit unresolved orbital/project facilities" );
			}
			if ( base != &evaluating_base ) { continue; }
			switch ( *entry.second->m_smac_source_id ) {
				case 2: creche = true; break;
				case 24: hab_complex = true; break;
				case 25: hab_dome = true; break;
				default: break;
			}
		}
	}
	// Completion grants precede housing evaluation, while the transaction is
	// still private. Earlier nutrient intake remains the caller's input.
	if(granted) {
		if(m_bm->GetFacilityDef(granted->m_id)!=granted || !granted->m_smac_source_id || !SupportsSmacFacilityProduction(*granted->m_smac_source_id)) {
			throw std::runtime_error("Population staged grant is not a supported registered facility");
		}
		m_bm->ValidateSmacFacilityDef(granted);
		hab_complex = hab_complex || granted->m_smac_source_id==24;
		hab_dome = hab_dome || granted->m_smac_source_id==25;
	}
	const int64_t growth = int64_t( social->current[ 6 ] ) + ( creche ? 2 : 0 ) + ( context.golden_age ? 2 : 0 );
	if ( growth < INT32_MIN || growth > INT32_MAX || population < 1 || population > 127 ) {
		throw std::overflow_error( "Population growth or population exceeds admitted signed32/byte domain" );
	}
	const auto effective_growth = static_cast< int32_t >( growth );
	// Human cost does not read AI leader/dominance inputs. Cloning/Ascetic are
	// absent by saved campaign authority, checked against all installed facilities.
	return {
		base::CalculateSmacNutrientCostFactor( { true, m_simulation->value.designs.global_difficulty, false, false, false,
			selection->category, rules->nutrient_multiplier, effective_growth } ),
		effective_growth, rules->nutrients_per_citizen,
		base::SmacHousingAllowsGrowth( static_cast< int32_t >( population ), population_penalty,
			rules->hab_complex_limit, rules->hab_dome_limit, hab_complex, hab_dome, false ), false
	};
}

}
