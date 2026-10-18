#include "Base.h"

#include "BaseManager.h"
#include "FacilityDef.h"
#include "SmacPopulationStep.h"
#include "SmacUnitSupport.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/map/Map.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/SmacStartupDesignState.h"
#include "gc/Space.h"
#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace game::backend::base {
namespace {
int32_t Checked( int64_t value ) {
	if ( value < INT32_MIN || value > INT32_MAX ) { throw std::overflow_error( "Production accounting exceeds signed32" ); }
	return static_cast< int32_t >( value );
}
}

const std::optional<SmacBaseUpkeepCache>& Base::GetSmacUpkeepCache() const {
	GetSmacResourceContext();
	return m_smac_upkeep_cache;
}

Base::SmacProductionResult Base::ProcessSmacProduction( GSE_CALLABLE, const SmacAllocationRoles& roles,
	const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode ) {
	return ProcessSmacProductionAndPopulation( GSE_CALL, roles, definition, mode, false );
}
Base::SmacProductionResult Base::ProcessSmacUpkeep( GSE_CALLABLE, const SmacAllocationRoles& roles,
	const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode ) {
	return ProcessSmacProductionAndPopulation( GSE_CALL, roles, definition, mode, true );
}
Base::SmacProductionResult Base::BeginSmacUpkeep( GSE_CALLABLE, const SmacAllocationRoles& roles,
	const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode ) {
	return ProcessSmacProductionAndPopulation( GSE_CALL, roles, definition, mode, true, true );
}

void Base::ValidateSmacColonyPause( const SmacColonyUpkeepPause& pause ) const {
	const auto context = GetSmacResourceContext();
	if ( !m_allocation || !m_production || !m_production->queue || m_pops.size() != 1
		|| m_production->current_item.kind != ProductionItemKind::Unit || !m_production->current_item.smac_source_id
		|| pause.first_calculation.nutrients.stored_before != m_accumulated_nutrients
		|| pause.first_calculation.nutrients.normalized_stored < 0
		|| ( pause.morale_mode != unit::SmacMoraleMode::Mode0 && pause.morale_mode != unit::SmacMoraleMode::Mode1 )
		|| unit::SmacStartupDesignState::OriginalDifficulty( m_owner->GetPlayer()->GetDifficultyLevel() ) <= 1
		|| ( context.GovernorActive() && ( context.governor_options & 1u ) ) ) {
		THROW( "Colony continuation no longer matches its size-one question" );
	}
	m_game->RequireSmacLocalHumanOwner( m_owner );
	m_game->ValidateSmacProductionState( m_owner, *m_production );
	const unit::ConventionalUnitState identity{ *m_production->current_item.smac_source_id, context.owner_simulation_id,
		1, 0, 0, false, unit::ConventionalUnitState::Home::Base, pause.morale_mode, m_id };
	if ( m_game->ResolveConventionalUnit( identity, m_owner ).package != unit::ConventionalUnitContext::Package::Colony
		|| m_production->minerals < m_game->GetSmacProductionCost( m_owner, m_production->current_item ) ) {
		THROW( "Colony continuation lost its completed owned target" );
	}
	// A saved question belongs to this exact first pass. Read-only validation
	// never replaces its retained result or posts its mineral surplus again.
	const auto actual = CalculateSmacResourceCalculation( SmacBaseResourcePhase::Upkeep, &*m_allocation );
	const auto& expected = pause.first_calculation;
	const auto same = []( const SmacBaseResourceCalculation& a, const SmacBaseResourceCalculation& b ) {
		return a.raw_intake == b.raw_intake
			&& std::tie( a.nutrients.intake, a.nutrients.demand, a.nutrients.surplus, a.nutrients.stored_before, a.nutrients.normalized_stored,
				a.minerals.multiplier_steps, a.minerals.intake, a.minerals.demand, a.minerals.surplus, a.minerals.pre_orbital_surplus )
			== std::tie( b.nutrients.intake, b.nutrients.demand, b.nutrients.surplus, b.nutrients.stored_before, b.nutrients.normalized_stored,
				b.minerals.multiplier_steps, b.minerals.intake, b.minerals.demand, b.minerals.surplus, b.minerals.pre_orbital_surplus );
	};
	if ( !same( actual, expected ) ) { THROW( "Colony continuation resource inputs changed while suspended" ); }
}

Base::SmacProductionResult Base::ResumeSmacColonyUpkeep( GSE_CALLABLE, const SmacColonyUpkeepPause& pause,
	SmacColonyChoice choice, const SmacAllocationRoles& roles, const std::function< unit::Def*( int32_t ) >& definition ) {
	if ( choice != SmacColonyChoice::Delay && choice != SmacColonyChoice::Screen ) {
		THROW( "Colony acceptance requires its distinct abandonment provider" );
	}
	ValidateSmacColonyPause( pause );
	return ProcessSmacProductionAndPopulation( GSE_CALL, roles, definition, pause.morale_mode, true, false, &pause );
}

Base::SmacProductionResult Base::ResumeSmacAbandonmentTail( GSE_CALLABLE, const SmacBaseUpkeepCache& retained,
	const SmacAllocationRoles& roles, unit::SmacMoraleMode mode ) {
	if ( !GetSmacUpkeepCache() ) { THROW( "Abandonment successor has no retained calculation" ); }
	// The manager returns this owned cache with its authenticated successor ID.
	// Compaction reaches population directly: no fresh mineral production pass.
	return ProcessSmacProductionAndPopulation( GSE_CALL, roles, {}, mode, true, false, nullptr, &retained );
}

Base::SmacProductionResult Base::ProcessSmacProductionAndPopulation( GSE_CALLABLE, const SmacAllocationRoles& roles,
	const std::function< unit::Def*( int32_t ) >& definition, unit::SmacMoraleMode mode, const bool population_step,
	const bool allow_colony_pause, const SmacColonyUpkeepPause* pause, const SmacBaseUpkeepCache* retained ) {
	m_game->CheckRW( GSE_CALL );
	if ( gc_space != m_game->GetGCSpace() || !gc_space->IsAccumulating() ) { THROW( "Production requires its owning accumulation" ); }
	m_game->GetBM()->RequireSmacWorkerRecalculation();
	m_game->RequireSmacLocalHumanOwner( m_owner );
	const auto context = GetSmacResourceContext();
	if ( !m_allocation || !m_production || !m_production->queue ) { THROW( "Production requires complete allocation and target/queue authority" ); }
	m_game->ValidateSmacProductionState( m_owner, *m_production );
	auto production = *m_production;
	auto population = m_pops.size();
	if ( production.current_item.kind != ProductionItemKind::Unit && production.current_item.kind != ProductionItemKind::Stockpile
		&& !(production.current_item.kind==ProductionItemKind::Facility && Game::SupportsSmacFacilityProduction(*production.current_item.smac_source_id)) ) {
		THROW( "Production completion for this target kind is not yet connected" );
	}
	Checked( production.minerals ); Checked( production.baseline_minerals ); Checked( m_accumulated_nutrients );
	auto allocation = pause || retained ? *m_allocation : CalculateSmacWorkers( *m_allocation, context, SmacBaseResourcePhase::Upkeep );
	SmacProductionResult result{ pause ? pause->first_calculation : retained ? retained->calculation : CalculateSmacResourceCalculation( SmacBaseResourcePhase::Upkeep, &allocation ), {}, false };
	const auto& resources = result.first_calculation;
	auto support = retained ? retained->support : m_game->CalculateCanonicalUnitSupport( *this, population, nullptr );
	std::optional< unit::UnitManager::PreparedConventionalUnit > unit;
	std::optional< unit::UnitManager::PreparedConventionalRemoval > removal;
	const FacilityDef* granted=nullptr;
	std::vector<std::pair<Base*,facilities_t>> facility_changes;
	if ( !pause && !retained && resources.nutrients.normalized_stored >= 0 && production.current_item.kind != ProductionItemKind::Stockpile ) {
		if ( resources.minerals.surplus >= 0 && !context.drone_riots ) {
			production.minerals = Checked( production.minerals + resources.minerals.surplus );
		}
		const auto cost = m_game->GetSmacProductionCost( m_owner, production.current_item );
		bool complete = production.minerals >= cost;
		if(production.current_item.kind==ProductionItemKind::Facility) {
			for(const auto& row:m_facilities) {
				if(row.second->m_smac_source_id==production.current_item.smac_source_id
					|| (production.current_item.smac_source_id==3 && row.second->m_smac_source_id==26)) {complete=false;}
			}
		}
		bool colony = false;
		if ( complete && production.current_item.kind==ProductionItemKind::Unit ) {
			// Classify the actual owned package before preparing a factory. A
			// governor deferral neither needs a presentation nor reserves a Unit.
			const unit::ConventionalUnitState identity{ *production.current_item.smac_source_id, context.owner_simulation_id,
				1, 0, 0, false, unit::ConventionalUnitState::Home::Base, mode, m_id };
			colony = m_game->ResolveConventionalUnit( identity, m_owner ).package == unit::ConventionalUnitContext::Package::Colony;
			if ( colony ) {
				if ( !population_step || population == 0 ) { THROW( "Colony completion requires full upkeep and a populated Base" ); }
				if ( population == 1 && unit::SmacStartupDesignState::OriginalDifficulty( m_owner->GetPlayer()->GetDifficultyLevel() ) > 1 ) {
					// Stored easy difficulty bypasses this check. Production
					// management (0x1) is independent of citizen management.
					if ( context.GovernorActive() && ( context.governor_options & 0x1u ) ) { complete = false; }
					else if ( allow_colony_pause ) {
						result.colony_pause = SmacColonyUpkeepPause{ resources, mode };
						// Publish only the question prefix: posted minerals and repaired
						// workers, with the old baseline and food still authoritative.
						auto prepared = PrepareSmacWorkerState( GSE_CALL, allocation, roles, context, production, m_accumulated_nutrients );
						std::optional<SmacBaseUpkeepCache> cache{SmacBaseUpkeepCache{resources,support}};
						static_assert(std::is_nothrow_swappable_v<decltype(cache)>);
						prepared.Commit(); m_smac_upkeep_cache.swap(cache); prepared.Finish();
						return result;
					}
					else { THROW( "Size-one Colony completion requires an explicit player choice" ); }
				}
			}
		}
		if ( complete ) {
			const auto exemption = m_game->GetState()->m_settings.global.GetSmacProductionRules()->retool_exemption;
			production.minerals = std::min( Checked( production.minerals - cost ), std::max( 0, std::min( exemption, resources.minerals.surplus ) ) );
			production.baseline_minerals = production.minerals;
			// Resolve presentation before any authority writes. The factory checks
			// the actual owner/design/home/package and stages sight and every queue.
			if(production.current_item.kind==ProductionItemKind::Facility) {
				granted=m_game->GetBM()->GetSmacFacilityDef(*production.current_item.smac_source_id);
				if(!granted) {THROW("Facility completion has no registered definition");}
				if(granted->m_smac_source_id==8) {
					// No Artifact or Artifact-link command is admitted by this world.
					// Keep the Node reset's empty domain explicit when new packages land.
					m_game->GetUM()->RequireConventionalUnitsReady();
					for(const auto& row:m_game->GetUM()->GetUnits()) {
						const auto package=m_game->ResolveConventionalUnit(row.second->Conventional(),row.second->m_owner).package;
						if(package!=unit::ConventionalUnitContext::Package::Combat && package!=unit::ConventionalUnitContext::Package::Former
							&& package!=unit::ConventionalUnitContext::Package::Colony) {THROW("Node completion requires Artifact-link reset support for this package");}
					}
				}
				if(granted->m_smac_source_id==1) {
					for(const auto& row:m_game->GetBM()->GetBases()) {
						auto* other=row.second;if(other==this || other->m_owner!=m_owner) {continue;}
						auto facilities=other->m_facilities;bool removed=false;
						for(auto it=facilities.begin();it!=facilities.end();) {
							if(it->second->m_smac_source_id==1) {it=facilities.erase(it);removed=true;} else {++it;}
						}
						if(removed) {facility_changes.emplace_back(other,std::move(facilities));}
					}
				}
				auto facilities=m_facilities;facilities.emplace(granted->m_id,granted);facility_changes.emplace_back(this,std::move(facilities));
			} else {
				const auto design_id = *production.current_item.smac_source_id;
				if ( !definition ) { THROW( "Completed unit has no explicit presentation resolver" ); }
				auto* def = definition( design_id );
				unit.emplace( m_game->GetUM()->PrepareManufacturedConventionalUnit( GSE_CALL, *this, design_id, def, mode ) );
			}
			if ( colony && population > 1 ) {
				--population; // factory/home first; debit before queue, baselines and growth
			}
			// Empty queues keep the current target without revalidation. Each
			// popped candidate remains current, including the last rejected one.
			while ( !production.queue->empty() ) {
				production.current_item = production.queue->front();
				production.queue->erase( production.queue->begin() );
				const auto& item=production.current_item;
				const bool eligible=item.kind==ProductionItemKind::Stockpile ? true
					: item.kind==ProductionItemKind::Unit ? m_game->IsSmacUnitProductionEligible(*this,item)
					: m_game->IsSmacFacilityProductionEligible(*this,item,granted);
				if (eligible) { result.queue_advanced = true; break; }
			}
		}
	}
	// Ordinary unmanaged human secondary production leaves the selected target.
	production.baseline_item = production.current_item;
	production.baseline_minerals = production.minerals;
	auto food = retained ? m_accumulated_nutrients : resources.nutrients.normalized_stored;
	if ( population_step ) {
		if ( !m_game->GetState()->m_settings.global.map.IsOrdinaryCampaignMap() ) {
			THROW( "Canonical upkeep requires an ordinary campaign, not scenario/mapfile objectives" );
		}
		// The current generated local campaign has no objective producer. This
		// is explicit GLSMAC non-scenario policy, not an inferred original flag.
		const auto plan = SmacPopulationStep::Prepare( m_game->CalculateCanonicalPopulationRules( *this, population ),
			static_cast< int32_t >( population ), food, resources.nutrients.surplus, false );
		if ( !plan.stored_nutrients ) {
			// Negative normalized food skipped completion above. Keep this explicit
			// so a future producer cannot discard an uncommitted newborn here.
			if ( unit ) { THROW( "Starvation removal cannot retain a prepared completion" ); }
			result.removed_base = true;
			m_game->GetBM()->RemoveStarvedSmacBase( GSE_CALL, *this );
			return result; // this has retired; only the owned local result remains
		}
		population = plan.population; food = *plan.stored_nutrients;
		const auto* newborn = unit ? &unit->Candidate() : nullptr;
		auto gross_minerals = resources.minerals.intake;
		if ( plan.NeedsRecalculation() ) {
			allocation = CalculateSmacWorkers( allocation, context, SmacBaseResourcePhase::Upkeep, population, newborn, &production, {}, granted );
			const auto internal = CalculateSmacResourceCalculation( SmacBaseResourcePhase::Upkeep, &allocation, population, food, newborn, {}, granted );
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
		support = m_game->CalculateCanonicalUnitSupport( *this, population, newborn, result.removed_units );
		const auto remaining_support = support.mineral_support;
		allocation = CalculateSmacWorkers( allocation, context, SmacBaseResourcePhase::Upkeep, population, newborn, &production, remaining_support, granted );
		result.final_calculation = CalculateSmacResourceCalculation( SmacBaseResourcePhase::Upkeep, &allocation, population, food, newborn, remaining_support, granted );
		food = result.final_calculation->nutrients.normalized_stored;
	}
	std::optional<SmacBaseUpkeepCache> cache{SmacBaseUpkeepCache{result.final_calculation.value_or(result.first_calculation),support}};
	static_assert(std::is_nothrow_swappable_v<decltype(cache)>);
	// Both resource passes consume incoming conquest history. Its one tail step
	// publishes with the finished Base state and does not trigger another pass.
	const auto next_context=population_step ? context.AfterUpkeep() : context;
	auto prepared = PrepareSmacWorkerState( GSE_CALL, allocation, roles, next_context, production, food, population, std::move(facility_changes), nullptr, {}, population_step );
	// No callback, allocation or validation between Unit and Base authority.
	if ( unit ) { result.completed_unit = unit->Commit()->m_id; }
	if ( removal ) { removal->Commit(); }
	prepared.Commit();m_smac_upkeep_cache.swap(cache);prepared.Finish();
	if ( unit && ( !removal || !removal->Removes( *result.completed_unit ) ) ) { unit->Notify( GSE_CALL ); }
	if ( removal ) { removal->Notify( GSE_CALL ); }
	return result;
}

}
