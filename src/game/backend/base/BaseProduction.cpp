#include "Base.h"

#include "BaseManager.h"
#include "SmacPopulationStep.h"
#include "SmacUnitSupport.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
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
	auto support = m_game->CalculateCanonicalUnitSupport( *this, population, nullptr );
	std::optional< unit::UnitManager::PreparedConventionalUnit > unit;
	std::optional< unit::UnitManager::PreparedConventionalRemoval > removal;
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
		auto gross_minerals = resources.minerals.intake;
		if ( plan.NeedsRecalculation() ) {
			allocation = CalculateSmacWorkers( allocation, context, SmacBaseResourcePhase::Upkeep, population, newborn, &production );
			const auto internal = CalculateSmacResourceCalculation( SmacBaseResourcePhase::Upkeep, &allocation, population, food, newborn );
			food = plan.Finish( internal.nutrients.surplus );
			gross_minerals = internal.minerals.intake;
			support = m_game->CalculateCanonicalUnitSupport( *this, population, newborn );
		}
		if ( m_game->GetTurnId() > 1 && support.mineral_support > gross_minerals ) {
			// Paid markings belong to the latest resource pass, not to the newly
			// produced roster. Growth's internal pass can make the newborn paid.
			const auto* map = m_game->GetMap();
			const auto& knowledge = map->GetSmacWorkerKnowledge();
			if ( !knowledge ) { THROW( "Support loss requires the current map geometry" ); }
			const bool wraps = knowledge->CopyData().horizontal_wrap;
			auto demand = support.mineral_support;
			while ( demand > gross_minerals ) {
				std::optional<size_t> selected;
				int64_t best_distance = -1;
				for ( const auto& marking : support.units ) {
					if ( marking.disposition != SmacSupportDisposition::Paid
						|| std::find( result.removed_units.begin(), result.removed_units.end(), marking.id ) != result.removed_units.end() ) { continue; }
					const auto* candidate = newborn && newborn->m_id == marking.id ? newborn : m_game->GetUM()->GetUnit( marking.id );
					if ( !candidate ) { THROW( "Paid support record lost its Unit" ); }
					const auto* position = candidate == newborn ? GetTile() : candidate->GetTile();
					auto dx = std::abs( int64_t( position->coord.x ) - int64_t( GetTile()->coord.x ) );
					const auto dy = std::abs( int64_t( position->coord.y ) - int64_t( GetTile()->coord.y ) );
					if ( wraps ) { dx = std::min( dx, int64_t( map->GetWidth() ) - dx ); }
					const auto distance = std::max( dx, dy ) - ( std::abs( dx - dy ) + 2 ) / 4;
					// Strict replacement preserves live roster order for ties. All
					// admitted packages have ordinary plans 3, 8 or 9; no convoys.
					if ( distance > best_distance ) { selected = marking.id; best_distance = distance; }
				}
				if ( !selected ) { break; }
				result.removed_units.push_back( *selected );
				demand -= support.minerals_per_paid_unit;
			}
			if ( !result.removed_units.empty() ) {
				removal.emplace( m_game->GetUM()->PrepareConventionalRemoval( GSE_CALL, result.removed_units, unit ? &*unit : nullptr ) );
			}
		}
		// The outer forced pass always uses the resulting population and newborn
		// home support. It normalizes food but never posts minerals or collects
		// food again. Surviving Pop nodes are still untouched during these reads.
		const auto remaining_support = m_game->CalculateCanonicalUnitSupport( *this, population, newborn, result.removed_units ).mineral_support;
		allocation = CalculateSmacWorkers( allocation, context, SmacBaseResourcePhase::Upkeep, population, newborn, &production, remaining_support );
		result.final_calculation = CalculateSmacResourceCalculation( SmacBaseResourcePhase::Upkeep, &allocation, population, food, newborn, remaining_support );
		food = result.final_calculation->nutrients.normalized_stored;
	}
	auto prepared = PrepareSmacWorkerState( GSE_CALL, allocation, roles, context, production, food, population );
	// No callback, allocation or validation between Unit and Base authority.
	if ( unit ) { result.completed_unit = unit->Commit()->m_id; }
	if ( removal ) { removal->Commit(); }
	prepared.Commit(); prepared.Finish();
	if ( unit && ( !removal || !removal->Removes( *result.completed_unit ) ) ) { unit->Notify( GSE_CALL ); }
	if ( removal ) { removal->Notify( GSE_CALL ); }
	return result;
}

}
