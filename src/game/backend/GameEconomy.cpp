#include "Game.h"

#include "Player.h"
#include "base/Base.h"
#include "base/BaseManager.h"
#include "base/FacilityDef.h"
#include "base/SmacBaseEnergy.h"
#include "faction/Faction.h"
#include "map/Map.h"
#include "map/SmacTerritoryRules.h"
#include "slot/Slot.h"
#include "unit/Unit.h"
#include "unit/UnitManager.h"
#include "gc/Space.h"
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <set>

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
	const base::SmacBaseResourceCalculation& resources, const base::SmacEnergyRates* candidate_rates, const base::SmacBaseAllocationState* candidate_allocation, const base::FacilityDef* granted ) const {
	using namespace base;
	if ( evaluating_base.GetGame() != this || !m_bm || m_bm->GetBase( evaluating_base.m_id ) != &evaluating_base || !m_map ) {
		throw std::runtime_error( "Energy calculation requires a registered Base in this world" );
	}
	const auto economy = GetCanonicalEconomy( evaluating_base.m_owner );
	const auto rates=candidate_rates ? *candidate_rates : economy.rates;rates.Validate();
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
	std::vector< std::pair< int32_t, int32_t > > maintenance_fees;
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
			maintenance_fees.emplace_back( id, fee );
		}
	}
    if(granted) {
        if(m_bm->GetFacilityDef(granted->m_id)!=granted || !granted->m_smac_source_id || !SupportsSmacFacilityProduction(*granted->m_smac_source_id)) {throw std::runtime_error("Energy staged facility is not admitted");}
        m_bm->ValidateSmacFacilityDef(granted);const auto id=*granted->m_smac_source_id;
        if(!local.at(id) && !free_facility.at(id)) {
            const auto fee=facilities->facilities.at(id-1).maintenance;
            if(fee<0) {throw std::runtime_error("Negative staged facility maintenance is outside ordinary posting");}
            maintenance=Checked(int64_t(maintenance)+fee);maintenance_fees.emplace_back(id,fee);
        }
        local.at(id)=true;
        if(id==1) {const auto* center=evaluating_base.GetTile();headquarters={{Checked(center->coord.x),Checked(center->coord.y)}};}
    }
	const auto allocation = candidate_allocation ? std::optional<SmacBaseAllocationState>(*candidate_allocation) : evaluating_base.GetSmacAllocation();
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
	const auto categories = AllocateSmacEnergy( net, rates, outputs );
	// The definition constructor clears flat labs. The fully validated admitted
	// keyword set has no direct RESEARCH writer; current social RESEARCH is a
	// separate later submission multiplier and must not be substituted here.
	SmacEnergyTailInputs inputs{ categories.after_specialists, rates, social->current[ 1 ], 0 };
	inputs.facilities = local;
	inputs.current_stockpile = production->current_item.kind == ProductionItemKind::Stockpile;
	inputs.baseline_stockpile = production->baseline_item.kind == ProductionItemKind::Stockpile;
	inputs.mineral_surplus = resources.minerals.surplus;
	std::sort( maintenance_fees.begin(), maintenance_fees.end() );
	return { raw, distance, waste, net, maintenance, categories, FinishSmacEnergy( inputs ), std::move(maintenance_fees) };
}

std::vector<std::string> Game::PostCanonicalBaseMaintenance( const base::Base& evaluating_base, const base::SmacBaseEnergyCalculation& energy ) {
	if (!GetGCSpace()->IsAccumulating() || !IsRunning()) {throw std::runtime_error("Maintenance requires the owning native upkeep");}
	auto* current=m_bm->GetBase(evaluating_base.m_id);
	if(current!=&evaluating_base || m_bm->m_dispatching_updates) {throw std::runtime_error("Maintenance Base is not current or update dispatch is active");}
	const auto owner=current->GetSmacResourceContext().owner_simulation_id;
	auto& saved=m_simulation->value.economy.at(owner);
	auto candidate=saved.CreditBase(energy.tail.final.economy,0);
	auto facilities=current->m_facilities;
	std::vector<std::string> notifications;
	int32_t previous=0,total=0;
	for(const auto& charge:energy.maintenance_fees) {
		const auto id=charge.first,fee=charge.second;
		if(id<=previous || id>33 || fee<0) {throw std::runtime_error("Invalid ordered maintenance charge");}
		previous=id;total=Checked(int64_t(total)+fee);
		const auto* definition=m_bm->GetSmacFacilityDef(id);
		if(!definition || !facilities.count(definition->m_id)) {throw std::runtime_error("Charged maintenance facility is no longer present");}
		candidate.credits=Checked(int64_t(candidate.credits)-fee);
		if(candidate.credits>=0) {continue;}
		const auto& target=current->GetProductionState()->current_item;
		const auto difficulty=unit::SmacStartupDesignState::OriginalDifficulty(current->m_owner->GetPlayer()->GetDifficultyLevel());
		// Static original branches: easy difficulties and rebuilding this very
		// facility keep it and floor the balance. Runtime receipt covers the
		// ordinary Librarian, nonmatching-target sale path.
		if(difficulty<=1 || (target.kind==base::ProductionItemKind::Facility && target.smac_source_id==id)) {
			candidate.credits=0;continue;
		}
		candidate.credits=GetSmacProductionCost(current->m_owner,MakeSmacProductionItem(current->m_owner,base::ProductionItemKind::Facility,id));
		facilities.erase(definition->m_id);
		notifications.push_back(current->m_name+": "+definition->m_name+" sold to pay maintenance.");
	}
	if(total!=energy.maintenance) {throw std::runtime_error("Maintenance total differs from ordered charges");}
	// Stage the frontend queue before publishing treasury and inventory. Do not
	// trigger resource/citizen recalculation: this upkeep's outputs remain earned.
	auto updates=m_bm->m_base_updates;
	if(!notifications.empty()) {base::BaseManager::QueueBaseUpdateInto(updates,current,base::BaseManager::BUO_REFRESH);}
	static_assert(std::is_nothrow_swappable_v<base::Base::facilities_t>);
	current->m_facilities.swap(facilities);m_bm->m_base_updates.swap(updates);saved=candidate;
	return notifications; // caller notifies only after its remaining authority writes
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
	// The compatibility entry shares the live controller's individual charges.
	const auto notifications=PostCanonicalBaseMaintenance(evaluating_base,result);
	next.credits = saved.credits;
	// No callback or throwing operation between the research and balance writes.
	technology->labs = progress; saved = next;
	for(const auto& notification:notifications) {Message(notification);}
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

uint8_t Game::GetCanonicalRegion(const map::tile::Tile& tile,const map::GlsmacTerritoryTopology& topology) const {
	if(!m_map || tile.tiles!=m_map->GetTilesPtr() || tile.coord.x>=m_map->GetWidth() || tile.coord.y>=m_map->GetHeight()
		|| ((tile.coord.x^tile.coord.y)&1) || m_map->GetTile(tile.coord.x,tile.coord.y)!=&tile
		|| topology.Width()!=static_cast<int64_t>(m_map->GetWidth()) || topology.Height()!=static_cast<int64_t>(m_map->GetHeight())) {
		throw std::runtime_error("Regional Base count requires its current topology and coordinate");
	}
	return topology.Regions().at(size_t(tile.coord.y)*(m_map->GetWidth()/2)+tile.coord.x/2);
}

std::optional<faction::SmacEconomyState::RegionalBaseCounts> Game::PrepareCanonicalRegionalBaseDelta(
	const base::Base& base,int delta,const std::optional<map::GlsmacTerritoryTopology>& topology) const {
	if(!m_simulation || !base.GetTile() || !base.m_resource_context || (delta!=1 && delta!=-1)) {throw std::runtime_error("Invalid regional Base delta");}
	// Founding supplies a validated private candidate, before registration.
	auto counts=m_simulation->value.economy.at(base.m_resource_context->owner_simulation_id).regional_base_counts;
	if(counts) {
		// The generic canonical creator can run before worker knowledge owns
		// topology. Use the same fresh labeling without changing any map state.
		const auto region=topology ? GetCanonicalRegion(*base.GetTile(),*topology)
			: GetCanonicalRegion(*base.GetTile(),map::GlsmacTerritoryTopology::Build(m_map->GetWidth(),m_map->GetHeight(),true,m_map->TerritorySea()));
		// Original storage is a byte, including wrap on a stale zero or 255.
		(*counts)[region]=static_cast<uint8_t>(int((*counts)[region])+delta);
	}
	return counts;
}

void Game::RefreshCanonicalPopulationAggregate( int32_t owner ) {
	ValidateCurrentSimulation();
	if(!GetGCSpace()->IsAccumulating() || !IsRunning() || !m_simulation->value.economy.count(owner)) {
		throw std::runtime_error("Population refresh requires its current owning faction upkeep");
	}
	int64_t population=0,base_total=0;
	const auto& topology=m_map->GetTerritoryTopology();
	auto military=m_simulation->value.regional_military;
	const bool recount_military=military.has_value() && topology.has_value();
	if(recount_military && military->factions.at(owner-1).base_phase_flags) {
		faction::SmacRegionalMilitaryState::ResetBaseFlags(*military->factions.at(owner-1).base_phase_flags,GetTurnId(),owner);
	}
	std::optional<std::vector<bool>> coastal_sea;
	std::optional<faction::SmacEconomyState::RegionalBaseCounts> counts;
	if(topology) {counts=faction::SmacEconomyState::RegionalBaseCounts{};}
	std::set<size_t> base_records;
	for(const auto id:m_bm->GetBaseRecordOrder()) {
		const auto* base=m_bm->GetBase(id);
		if(!base || base->m_id!=id || !base_records.insert(id).second) {throw std::runtime_error("Population refresh found an invalid Base record");}
		if(base->GetSmacResourceContext().owner_simulation_id==owner) {
			if(base->m_pops.size()>static_cast<uint64_t>(INT32_MAX-population)) {throw std::overflow_error("Population aggregate exceeds signed32");}
			population+=static_cast<int64_t>(base->m_pops.size());
			if(++base_total>INT32_MAX) {throw std::overflow_error("Base total exceeds signed32");}
			if(counts) {
				if(!base->GetTile()) {throw std::runtime_error("Regional summary Base has no center");}
				const auto region=GetCanonicalRegion(*base->GetTile(),*topology);++(*counts)[region];
				if(recount_military && military->factions.at(owner-1).base_phase_flags) {
					if(!coastal_sea) {coastal_sea=m_map->TerritorySea();}
					const auto* square=base->GetTile();
					const auto direction=map::smac_territory::ImmediateSeaNeighbor(topology->Width(),topology->Height(),topology->Wraps(),
						*coastal_sea,{{static_cast<int32_t>(square->coord.x),static_cast<int32_t>(square->coord.y)}});
					faction::SmacRegionalMilitaryState::AccumulateBaseFlag(*military->factions.at(owner-1).base_phase_flags,region,direction!=0);
				}
			}
		}
	}
	if(base_records.size()!=m_bm->GetBases().size()) {throw std::runtime_error("Population refresh omitted a registered Base");}
	if(population>INT32_MAX) {throw std::overflow_error("Population aggregate exceeds signed32");}
	// Founding, removal and individual Base recalculation deliberately do not
	// update this saved field. The native post-Base owner pass owns its refresh.
	const auto& settings=m_state->m_settings.global;
	if(!settings.GetSmacCombatRules() || (!settings.GetSmacCampaignRules() && !settings.IsOrdinarySmacCampaign())) {throw std::runtime_error("Owner summary lacks frozen capability inputs");}
	auto capability_candidate=m_simulation->value;
	capability_candidate.RefreshCapabilities(owner,*settings.GetSmacTechnologyRules(),*settings.GetSmacPredefinedUnitRules(),
		*settings.GetSmacCombatRules(),settings.GetSmacCampaignRules() && (settings.GetSmacCampaignRules()->primary&0x00400000u)!=0);
    faction::SmacDesignCounts design_counts{};
    if(recount_military) {auto& row=military->factions.at(owner-1);row.units={};row.attack={};row.unit_phase_flags.emplace();}
    std::set<size_t> seen;
    const auto& order=m_um->GetUnitRecordOrder();
    for(size_t cursor=order.size();cursor>0;--cursor) {
        const auto id=order[cursor-1];const auto* unit=m_um->GetUnit(id);
        if(!unit || !seen.insert(id).second) {throw std::runtime_error("Faction summary requires complete ordered Unit records");}
        if(unit->IsNative()) {(void)m_um->QueryNativeUnit(unit->Native());continue;}
        if(!unit->IsConventional()) {throw std::runtime_error("Faction summary requires typed Unit authority");}
        const auto& record=unit->Conventional();
        if(record.owner_simulation_id!=owner) {continue;}
        ++design_counts.at(record.design_id);
        if(!recount_military) {continue;}
        const auto context=ResolveConventionalUnit(record,unit->m_owner);
        const auto* square=unit->GetTile();
        if(!square || square->tiles!=m_map->GetTilesPtr() || square->coord.x>=m_map->GetWidth() || square->coord.y>=m_map->GetHeight()
            || ((square->coord.x^square->coord.y)&1) || m_map->GetTile(square->coord.x,square->coord.y)!=square) {
            throw std::runtime_error("Military summary Unit has no current map coordinate");
        }
        const bool valid_base=square->base && m_bm->GetBase(square->base->m_id)==square->base && square->base->GetTile()==square;
        if(square->is_water_tile && !valid_base) {
            throw std::runtime_error("Unbased ground units on water require the native transport/invalid-placement cleanup producer");
        }
        // The current resolver admits land Scout/Rover and Former/Colony
        // designs. Native unit count and attack have distinct role/sea gates.
        const auto design=ResolveConventionalDesign(record,*m_simulation);
        const auto& catalog=m_state->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
        const auto power=catalog.weapons.weapons.at(design.weapon).offense;
        const auto armor=catalog.defenses.defenses.at(design.defense).rating;
        const auto morale=unit::QueryConventionalUnit(record,context,unit::SmacMoraleMode::Mode0).effective_morale;
        const auto region=GetCanonicalRegion(*square,*topology);
        // No Sentry/order2 or special plan5 is admitted by these packages.
        // Civilian A0 yields zero naturally; attack is not filtered by plan.
        faction::SmacRegionalMilitaryState::Accumulate(military->factions.at(owner-1),{
            region,design.plan,0,!square->is_water_tile,power,armor,morale,0,false,valid_base});
    }
    if(seen.size()!=m_um->GetUnits().size()) {throw std::runtime_error("Faction summary omitted a registered Unit");}
    // The final strategy phase sees this owner's newly recounted inputs and
    // every peer's retained row. Prepare it before publishing any summary.
    capability_candidate.regional_military=std::move(military);
    auto& economy=capability_candidate.economy.at(owner);
    economy.design_counts=design_counts;economy.population_aggregate=static_cast<int32_t>(population);economy.regional_base_counts=counts;
    if(capability_candidate.retained_base_totals) {capability_candidate.retained_base_totals->at(owner)=static_cast<int32_t>(base_total);}
    const auto regional=PrepareCanonicalRegionalStrategy(owner,capability_candidate);
    if(regional) {capability_candidate.regional_strategy->at(owner)=*regional;}
    m_simulation->value.capabilities.swap(capability_candidate.capabilities);
    m_simulation->value.regional_military.swap(capability_candidate.regional_military);
    m_simulation->value.regional_strategy.swap(capability_candidate.regional_strategy);
    m_simulation->value.economy.at(owner).design_counts=economy.design_counts;
    m_simulation->value.economy.at(owner).population_aggregate=economy.population_aggregate;
    m_simulation->value.economy.at(owner).regional_base_counts=economy.regional_base_counts;
    m_simulation->value.retained_base_totals=capability_candidate.retained_base_totals;
}

void Game::SettleCanonicalEconomy( int32_t owner ) {
	ValidateCurrentSimulation();
	if ( !GetGCSpace()->IsAccumulating() || !IsRunning() ) { throw std::runtime_error( "Economy settlement requires the owning native upkeep" ); }
	auto& saved = m_simulation->value.economy.at( owner );
	const bool has_base=std::any_of(m_bm->GetBases().begin(),m_bm->GetBases().end(),[&](const auto& row){return row.second->GetSmacResourceContext().owner_simulation_id==owner;});
	const bool has_unit=std::any_of(m_um->GetUnits().begin(),m_um->GetUnits().end(),[&](const auto& row){return row.second->Conventional().owner_simulation_id==owner;});
	if (!has_base && !has_unit) {
		const auto initialized=PrepareSmacNonzeroInitialization(owner);
		auto hostility=m_simulation->value.hostility;hostility.Reinitialize(owner);
		auto victory=m_simulation->value.victory_history;
		if(victory) {
			if(GetTurnId()>INT32_MAX) {throw std::runtime_error("Faction initialization history exceeds signed32");}
			victory->Initialize(owner,static_cast<int32_t>(GetTurnId()));
		}
		auto technology=std::find_if(m_simulation->value.technology.factions.begin(),m_simulation->value.technology.factions.end(),[&](const auto& row){return row.admission.simulation_id==owner;});
		if(technology==m_simulation->value.technology.factions.end()) {throw std::runtime_error("Empty faction has no technology authority");}
		saved=initialized;technology->cached_threshold.reset();
		if(m_simulation->value.retained_base_totals) {m_simulation->value.retained_base_totals->at(owner)=0;}
		if(m_simulation->value.capabilities) {m_simulation->value.capabilities->factions.at(owner-1)={};}
		m_simulation->value.hostility.owners.swap(hostility.owners);
		m_simulation->value.victory_history=std::move(victory);
	}
	const auto next = saved.SettleDebit(); saved = next;
}

}
