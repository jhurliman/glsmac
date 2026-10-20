#include "Base.h"
#include "SmacNutrientStorage.h"
#include "SmacMineralArithmetic.h"

#include "BaseManager.h"
#include "FacilityDef.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "gse/value/Object.h"
#include "gse/value/Int.h"
#include "gse/value/Null.h"
#include "gse/value/Array.h"
#include "gse/value/String.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/Player.h"
#include "gse/value/Bool.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend::base {
namespace {
int32_t Checked( const int64_t value ) {
    if ( value < INT32_MIN || value > INT32_MAX ) { throw std::overflow_error( "SMAC base resource processing exceeds signed32" ); }
    return static_cast< int32_t >( value );
}
int64_t AddIntake( const int64_t a, const int64_t b ) {
    if ( ( b > 0 && a > INT64_MAX - b ) || ( b < 0 && a < INT64_MIN - b ) ) {
        throw std::overflow_error( "SMAC base raw intake exceeds signed64" );
    }
    return a + b;
}
}

SmacBaseResourceCalculation FinishSmacResourceIntake( const std::array<int64_t,3>& raw_intake,
    int64_t nutrient_demand, int64_t stored_nutrients, int32_t mineral_support, int32_t mineral_multiplier_steps ) {
    SmacBaseResourceCalculation result{}; result.raw_intake=raw_intake;
    auto& n=result.nutrients;
    n.intake=Checked(raw_intake[0]); n.demand=Checked(nutrient_demand);
    n.surplus=Checked(int64_t(n.intake)-n.demand);
    n.stored_before=n.normalized_stored=stored_nutrients;
    n.normalized_stored=NormalizeSmacNutrientStorage(n.surplus,n.stored_before);
    auto& m=result.minerals; m.multiplier_steps=mineral_multiplier_steps;
    // Add half-bonuses, multiply once, then signed division toward zero.
    const auto amounts=CalculateSmacMineralAmounts(Checked(raw_intake[1]),mineral_support,mineral_multiplier_steps,[](int64_t v){return Checked(v);});
    m.intake=amounts.intake;m.demand=amounts.demand;
    m.pre_orbital_surplus=m.surplus=amounts.surplus;
    return result;
}

SmacBaseResourceCalculation Base::PrepareSmacResourceCalculation( const SmacBaseResourcePhase phase ) const {
    return CalculateSmacResourceCalculation( phase, nullptr );
}

SmacBaseResourceCalculation Base::CalculateSmacResourceCalculation( const SmacBaseResourcePhase phase, const SmacBaseAllocationState* allocation, std::optional< size_t > candidate_population, std::optional< int64_t > stored, const unit::Unit* prepared_unit, std::optional<int32_t> candidate_support, const FacilityDef* granted, const std::vector<size_t>& removed ) const {
    const auto population = candidate_population.value_or( GetPopulation() );
    // A fresh read-only query must model the same repaired allocation as its
    // owning calculation, rather than mixing old workers with the reduced demand.
    std::optional<SmacBaseAllocationState> repaired;
    if(!allocation && m_retained_population) {
        if(!m_allocation) {THROW("Retained population requires saved allocation authority");}
        repaired=CalculateSmacWorkers(*m_allocation,GetSmacResourceContext(),phase,population,prepared_unit,nullptr,candidate_support,granted);
        allocation=&*repaired;
    }
    GetSmacResourceContext(); // registered Base, bound owner and saved Pop projection
    Game::ResourceQueryPhase query_phase;
    switch ( phase ) {
        case SmacBaseResourcePhase::Ordinary: query_phase = Game::ResourceQueryPhase::Ordinary; break;
        case SmacBaseResourcePhase::Upkeep: query_phase = Game::ResourceQueryPhase::Upkeep; break;
        default: throw std::invalid_argument( "Unknown SMAC base resource phase" );
    }
    if ( m_production ) { m_production->Validate(); }
    // The orbital query authenticates the saved NoProjectsOrdinaryTerrain
    // campaign and every installed local facility, including remote Bases.
    // Thus Singularity, Bulk Matter and Space Elevator are absent authorities,
    // not defaults inferred from a missing field or an English facility name.
    const auto orbital = m_game->CalculateCanonicalOrbitalIntake( *this, population );
    // The support query authenticates every actual Unit as an admitted Combat/Former
    // design with its saved owner/home. There is no Supply/convoy package in
    // this live domain; incoming/outgoing convoy effects are therefore absent.
    const auto support = candidate_support ? *candidate_support : m_game->CalculateCanonicalMineralSupport( *this, population, prepared_unit );
    SmacBaseResourceCalculation result{};
    for ( size_t i = 0; i < orbital.size(); ++i ) { result.raw_intake[ i ] = orbital[ i ]; }
    const auto add = [ & ]( const map::tile::Tile& tile ) {
        const auto values = m_game->CalculateCanonicalTileResources( tile, *this, query_phase, false, false, granted );
        for ( size_t i = 0; i < values.size(); ++i ) { result.raw_intake[ i ] = AddIntake( result.raw_intake[ i ], values[ i ] ); }
    };
    add( *m_tile );
    if ( allocation ) {
        for ( const auto& coordinate : AllocationCoordinates( *allocation, population ) ) { add( *m_game->GetMap()->GetTile( coordinate.x, coordinate.y ) ); }
    }
    else { for ( const auto& entry : m_pops ) { if ( auto* tile = entry.second.GetWorkedTile() ) { add( *tile ); } } }

    int32_t multiplier_steps=0;
    for ( const auto& entry : m_facilities ) {
        // Orbital intake checked registry/catalog identity before this read.
        switch ( *entry.second->m_smac_source_id ) {
            case 19: // Robotic Assembly Plant
            case 20: // Nanoreplicator
            case 21: // Quantum Converter
            case 22: // Genejack Factory
                ++multiplier_steps; break;
            default: break;
        }
    }
    // No Space Elevator: every target has the same final and pre-orbital
    // surplus. Target/buildability does not change this gross mineral input.
    result = FinishSmacResourceIntake(result.raw_intake,m_game->CalculateCanonicalNutrientDemand(*this,population),
        stored.value_or(m_accumulated_nutrients),support,multiplier_steps);
    result.eco_damage = CalculateSmacEcologyDamage(result, allocation, population, granted);
    result.citizens = m_game->CalculateCanonicalCitizens(*this,result,allocation,population,prepared_unit,removed,granted);
    return result;
}

gse::Value* Base::CopySmacResourceCalculation( GSE_CALLABLE ) const {
    m_game->ValidateCurrentSimulation();
    const auto owner=GetSmacResourceContext().owner_simulation_id;
    const auto& roster=m_game->m_simulation->value.roster;
    const auto participant=std::find_if(roster.begin(),roster.end(),[&](const auto& row){return row.simulation_id==owner;});
    if(participant==roster.end()) {THROW("Resource display owner is outside the admitted roster");}
    const bool retained_only=!participant->active;
    if(retained_only && !m_smac_upkeep_cache) {THROW("Inactive Base has no retained resource calculation");}
    const auto result = retained_only ? m_smac_upkeep_cache->calculation:PrepareSmacResourceCalculation();
    const bool owned=m_owner && m_owner->GetPlayer()==m_game->GetPlayer();
    const auto snapshot=[&](const std::optional<SmacCitizenCalculation>& value)->gse::Value* {
        if(!owned || !value) {return VALUE(gse::value::Null);}
        const auto& c=*value;
        const auto counts=[&](const SmacCitizenCounts& n)->gse::Value* {
            return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
                {"talents",VALUE(gse::value::Int,,n.talents)},{"content",VALUE(gse::value::Int,,c.population-c.excluded-n.talents-n.drones)},
                {"drones",VALUE(gse::value::Int,,n.drones)},{"extra_drone_steps",VALUE(gse::value::Int,,n.extra_drone_steps)}}));
        };
        return VALUEEXT(gse::value::Object,GSE_CALL,(gse::value::object_properties_t{
            {"population",VALUE(gse::value::Int,,c.population)},{"specialists",VALUE(gse::value::Int,,c.excluded)},
            {"psych_points",VALUE(gse::value::Int,,c.psych_points)},{"police",VALUE(gse::value::Int,,c.police)},
            {"initial",counts(c.early.initial)},{"after_psych",counts(c.early.psych)},{"after_facilities",counts(c.early.facilities)},
            {"after_police",counts(c.after_police)},{"final",counts(c.final)}}));
    };
    std::string unavailable;
    if(owned && !retained_only && !result.citizens) {(void)m_game->CalculateCanonicalCitizens(*this,result,nullptr,{},nullptr,{},nullptr,&unavailable);}
    if(retained_only) {unavailable="This faction is inactive; showing its last resource calculation";}
    if(!owned) {unavailable="Citizen details are available for your Bases";}
    const std::optional<SmacCitizenCalculation> retained=m_smac_upkeep_cache ? m_smac_upkeep_cache->calculation.citizens:std::nullopt;

    return VALUEEXT( gse::value::Object, GSE_CALL, ( gse::value::object_properties_t{
        {"retained_only",VALUE(gse::value::Bool,,retained_only)},
        {"citizens",snapshot(retained_only ? std::nullopt:result.citizens)},
        {"retained_citizens",snapshot(retained)},
        {"citizens_unavailable",VALUE(gse::value::String,,unavailable)},
        { "eco_damage", result.eco_damage ? static_cast<gse::Value*>(VALUE(gse::value::Int,, *result.eco_damage)) : static_cast<gse::Value*>(VALUE(gse::value::Null)) },
        { "raw_intake", VALUEEXT( gse::value::Object, GSE_CALL, ( gse::value::object_properties_t{
            { "NUTRIENTS", VALUE( gse::value::Int,, result.raw_intake[ 0 ] ) },
            { "MINERALS", VALUE( gse::value::Int,, result.raw_intake[ 1 ] ) },
            { "ENERGY", VALUE( gse::value::Int,, result.raw_intake[ 2 ] ) }
        } ) ) },
        { "nutrients", VALUEEXT( gse::value::Object, GSE_CALL, ( gse::value::object_properties_t{
            { "intake", VALUE( gse::value::Int,, result.nutrients.intake ) },
            { "demand", VALUE( gse::value::Int,, result.nutrients.demand ) },
            { "surplus", VALUE( gse::value::Int,, result.nutrients.surplus ) },
            { "stored_before", VALUE( gse::value::Int,, result.nutrients.stored_before ) },
            { "normalized_stored", VALUE( gse::value::Int,, result.nutrients.normalized_stored ) }
        } ) ) },
        { "minerals", VALUEEXT( gse::value::Object, GSE_CALL, ( gse::value::object_properties_t{
            { "multiplier_steps", VALUE( gse::value::Int,, result.minerals.multiplier_steps ) },
            { "intake", VALUE( gse::value::Int,, result.minerals.intake ) },
            { "demand", VALUE( gse::value::Int,, result.minerals.demand ) },
            { "surplus", VALUE( gse::value::Int,, result.minerals.surplus ) },
            { "pre_orbital_surplus", VALUE( gse::value::Int,, result.minerals.pre_orbital_surplus ) }
        } ) ) }
    } ) );
}

}
