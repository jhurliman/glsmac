#include "Base.h"

#include "BaseManager.h"
#include "FacilityDef.h"
#include "game/backend/Game.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tile.h"
#include "gse/value/Object.h"
#include "gse/value/Int.h"
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

SmacBaseResourceCalculation Base::PrepareSmacResourceCalculation( const SmacBaseResourcePhase phase ) const {
    return CalculateSmacResourceCalculation( phase, nullptr );
}

SmacBaseResourceCalculation Base::CalculateSmacResourceCalculation( const SmacBaseResourcePhase phase, const SmacBaseAllocationState* allocation, std::optional< size_t > candidate_population, std::optional< int64_t > stored, const unit::Unit* prepared_unit ) const {
    const auto population = candidate_population.value_or( m_pops.size() );
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
    const auto support = m_game->CalculateCanonicalMineralSupport( *this, population, prepared_unit );
    SmacBaseResourceCalculation result{};
    for ( size_t i = 0; i < orbital.size(); ++i ) { result.raw_intake[ i ] = orbital[ i ]; }
    const auto add = [ & ]( const map::tile::Tile& tile ) {
        const auto values = m_game->GetCanonicalTileResources( tile, *this, query_phase );
        for ( size_t i = 0; i < values.size(); ++i ) { result.raw_intake[ i ] = AddIntake( result.raw_intake[ i ], values[ i ] ); }
    };
    add( *m_tile );
    if ( allocation ) {
        for ( const auto& coordinate : AllocationCoordinates( *allocation, population ) ) { add( *m_game->GetMap()->GetTile( coordinate.x, coordinate.y ) ); }
    }
    else { for ( const auto& entry : m_pops ) { if ( auto* tile = entry.second.GetWorkedTile() ) { add( *tile ); } } }

    auto& n = result.nutrients;
    n.intake = Checked( result.raw_intake[ 0 ] );
    n.demand = Checked( m_game->CalculateCanonicalNutrientDemand( *this, population ) );
    n.surplus = Checked( int64_t( n.intake ) - n.demand );
    n.stored_before = n.normalized_stored = stored.value_or( m_accumulated_nutrients );
    if ( n.surplus < 0 && n.stored_before == 0 ) { n.normalized_stored = -1; }
    else if ( n.surplus >= 0 && n.stored_before < 0 ) { n.normalized_stored = 0; }

    auto& m = result.minerals;
    for ( const auto& entry : m_facilities ) {
        // Orbital intake checked registry/catalog identity before this read.
        switch ( *entry.second->m_smac_source_id ) {
            case 19: // Robotic Assembly Plant
            case 20: // Nanoreplicator
            case 21: // Quantum Converter
            case 22: // Genejack Factory
                ++m.multiplier_steps; break;
            default: break;
        }
    }
    // Add half-bonuses, multiply once, then signed division toward zero.
    // Unsupported overflow rejects rather than inventing original wrap rules.
    m.intake = Checked( int64_t( Checked( result.raw_intake[ 1 ] ) ) * ( 2 + m.multiplier_steps ) ) / 2;
    m.demand = support;
    m.pre_orbital_surplus = m.surplus = Checked( int64_t( m.intake ) - m.demand );
    // No Space Elevator: every target has the same final and pre-orbital
    // surplus. No target/buildability lookup or ecology result is invented.
    return result;
}

gse::Value* Base::CopySmacResourceCalculation( GSE_CALLABLE ) const {
    const auto result = PrepareSmacResourceCalculation();
    return VALUEEXT( gse::value::Object, GSE_CALL, ( gse::value::object_properties_t{
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
