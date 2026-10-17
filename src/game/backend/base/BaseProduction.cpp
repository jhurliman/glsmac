#include "Base.h"

#include "BaseManager.h"
#include "SmacPopulationStep.h"
#include "game/backend/Game.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "gc/Space.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::base {
namespace {
int32_t Checked( int64_t value ) {
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::overflow_error( "Production accounting exceeds signed32" ); }
	return static_cast< int32_t >( value );
}
}

Base::SmacProductionResult Base::ProcessSmacProduction( GSE_CALLABLE, const SmacAllocationRoles& roles,
	const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode ) {
	return ProcessSmacProductionAndPopulation( GSE_CALL, roles, definition, mode, false );
}
Base::SmacProductionResult Base::ProcessSmacUpkeep( GSE_CALLABLE, const SmacAllocationRoles& roles,
	const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode ) {
	return ProcessSmacProductionAndPopulation( GSE_CALL, roles, definition, mode, true );
}
Base::SmacProductionResult Base::ProcessSmacProductionAndPopulation( GSE_CALLABLE, const SmacAllocationRoles& roles,
	const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode, const bool population_step ) {
	m_game->CheckRW( GSE_CALL );
	if ( gc_space != m_game->GetGCSpace() || !gc_space->IsAccumulating() ) { THROW( "Production requires its owning accumulation" ); }
	m_game->GetBM()->RequireSmacWorkerRecalculation();
	m_game->RequireSmacLocalHumanOwner( m_owner );
	const auto context = GetSmacResourceContext();
	if ( !m_allocation || !m_production || !m_production->queue ) { THROW( "Production requires complete allocation and target/queue authority" ); }
	m_game->ValidateSmacProductionState( m_owner, *m_production );
	auto production = *m_production;
	auto population = m_pops.size();
	if ( production.current_item.kind != ProductionItemKind::Unit && production.current_item.kind != ProductionItemKind::Stockpile ) {
		THROW( "Production completion for this target kind is not yet connected" );
	}
	Checked( production.minerals ); Checked( production.baseline_minerals ); Checked( m_accumulated_nutrients );
	auto allocation = CalculateSmacWorkers( *m_allocation, context, SmacBaseResourcePhase::Upkeep );
	SmacProductionResult result{ CalculateSmacResourceCalculation( SmacBaseResourcePhase::Upkeep, &allocation ), {}, false };
	const auto& resources = result.first_calculation;
	std::optional< unit::UnitManager::PreparedConventionalUnit > unit;
	if ( resources.nutrients.normalized_stored >= 0 && production.current_item.kind != ProductionItemKind::Stockpile ) {
		if ( resources.minerals.surplus >= 0 && !context.drone_riots ) {
			production.minerals = Checked( production.minerals + resources.minerals.surplus );
		}
		const auto cost = m_game->GetSmacProductionCost( m_owner, production.current_item );
		if ( production.minerals >= cost ) {
			const auto exemption = m_game->GetState()->m_settings.global.GetSmacProductionRules()->retool_exemption;
			production.minerals = std::min( Checked( production.minerals - cost ), std::max( 0, std::min( exemption, resources.minerals.surplus ) ) );
			production.baseline_minerals = production.minerals;
			// Resolve presentation before any authority writes. The factory checks
			// the actual owner/design/home/package and stages sight and every queue.
			const auto design_id = *production.current_item.smac_source_id;
			if ( !definition ) { THROW( "Completed unit has no explicit presentation resolver" ); }
			auto* def = definition( design_id );
			unit.emplace( m_game->GetUM()->PrepareManufacturedConventionalUnit( GSE_CALL, *this, design_id, def, mode ) );
			if ( m_game->ResolveConventionalUnit( unit->Candidate().Conventional(), m_owner ).package == unit::ConventionalUnitContext::Package::Colony ) {
				if ( !population_step || population < 2 ) { THROW( "Colony completion requires full upkeep and size-one choice policy is not connected" ); }
				--population; // factory/home first; debit before queue, baselines and growth
			}
			// Empty queues keep the current target without revalidation. Each
			// popped candidate remains current, including the last rejected one.
			while ( !production.queue->empty() ) {
				production.current_item = production.queue->front();
				production.queue->erase( production.queue->begin() );
				if ( m_game->IsSmacUnitProductionEligible( *this, production.current_item ) ) { result.queue_advanced = true; break; }
			}
		}
	}
	// Ordinary unmanaged human secondary production leaves the selected target.
	production.baseline_item = production.current_item;
	production.baseline_minerals = production.minerals;
	auto food = resources.nutrients.normalized_stored;
	if ( population_step ) {
		const auto map_type = m_game->GetState()->m_settings.global.map.type;
		if ( map_type != settings::MapSettings::MT_RANDOM && map_type != settings::MapSettings::MT_CUSTOM ) {
			THROW( "Canonical upkeep admits ordinary generated maps, not scenario/mapfile objectives" );
		}
		// The current generated local campaign has no objective producer. This
		// is explicit GLSMAC non-scenario policy, not an inferred original flag.
		const auto plan = SmacPopulationStep::Prepare( m_game->CalculateCanonicalPopulationRules( *this, population ),
			static_cast< int32_t >( population ), food, resources.nutrients.surplus, false );
		if ( !plan.stored_nutrients ) { THROW( "Size-one starvation awaits canonical Base removal and home reassignment" ); }
		population = plan.population; food = *plan.stored_nutrients;
		const auto* newborn = unit ? &unit->Candidate() : nullptr;
		auto mineral_surplus = resources.minerals.surplus;
		if ( plan.NeedsRecalculation() ) {
			allocation = CalculateSmacWorkers( allocation, context, SmacBaseResourcePhase::Upkeep, population, newborn, &production );
			const auto internal = CalculateSmacResourceCalculation( SmacBaseResourcePhase::Upkeep, &allocation, population, food, newborn );
			food = plan.Finish( internal.nutrients.surplus );
			mineral_surplus = internal.minerals.surplus;
		}
		if ( m_game->GetTurnId() > 1 && mineral_surplus < 0 ) {
			THROW( "Mineral deficit awaits canonical supported-unit loss processing" );
		}
		// The outer forced pass always uses the resulting population and newborn
		// home support. It normalizes food but never posts minerals or collects
		// food again. Surviving Pop nodes are still untouched during these reads.
		allocation = CalculateSmacWorkers( allocation, context, SmacBaseResourcePhase::Upkeep, population, newborn, &production );
		result.final_calculation = CalculateSmacResourceCalculation( SmacBaseResourcePhase::Upkeep, &allocation, population, food, newborn );
		food = result.final_calculation->nutrients.normalized_stored;
	}
	auto prepared = PrepareSmacWorkerState( GSE_CALL, allocation, roles, context, production, food, population );
	// No callback, allocation or validation between Unit and Base authority.
	if ( unit ) { result.completed_unit = unit->Commit()->m_id; }
	prepared.Commit(); prepared.Finish();
	if ( unit ) { unit->Notify( GSE_CALL ); }
	return result;
}

}
