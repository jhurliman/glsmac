#include "SmacResearchValue.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace game::backend::faction {
namespace {
int32_t Signed( uint32_t value ) { return value <= INT32_MAX ? int32_t( value ) : int32_t( int64_t( value ) - 0x100000000LL ); }
int32_t Add( int32_t a, int32_t b ) { return Signed( uint32_t( a ) + uint32_t( b ) ); }
int32_t Mul( int32_t a, int32_t b ) { return Signed( uint32_t( a ) * uint32_t( b ) ); }
int32_t Div( int32_t a, int32_t b ) {
    if ( !b || ( a == INT32_MIN && b == -1 ) ) { throw std::invalid_argument( "Research valuation division outside the measured domain" ); }
    return a / b;
}
class Query {
public:
    explicit Query( const SmacHumanResearchInputs& input ) : in( input ) {}
    bool Owns( int32_t id ) const {
        if ( id == -1 ) { return true; }
        if ( id < 0 || id >= 88 ) { return false; }
        const auto& row = in.technologies[ id ];
        return row.prerequisites[ 0 ] != -2 && ( row.prerequisites[ 1 ] != -2 || row.prerequisites[ 0 ] == -1 )
            && ( row.possession & ( uint32_t( 1 ) << in.faction_id ) );
    }
    bool Related( int32_t candidate, int32_t target, int32_t depth ) const {
        if ( candidate < 0 || target < 0 || candidate >= 89 || target >= 89 ) { return false; }
        // Equality is checked before the depth stop. The outer query includes
        // direct prerequisites at depth zero; this is not conventional depth.
        std::vector< std::pair< int32_t, int32_t > > pending{ { target, std::min( depth + 1, 89 ) } };
        std::array< int32_t, 89 > visited;
        visited.fill( -1 );
        while ( !pending.empty() ) {
            const auto [ id, remaining ] = pending.back(); pending.pop_back();
            if ( id < 0 || id >= 89 ) { continue; }
            if ( id == candidate ) { return true; }
            if ( remaining <= 0 || visited[ id ] >= remaining ) { continue; }
            visited[ id ] = remaining;
            for ( const auto prerequisite : in.technologies[ id ].prerequisites ) { pending.emplace_back( prerequisite, remaining - 1 ); }
        }
        return false;
    }
    const SmacHumanResearchInputs& in;
};
}

int32_t CalculateSmacHumanResearchValue( const SmacHumanResearchInputs& in, int32_t technology ) {
    if ( in.faction_id < 1 || in.faction_id > 7 || technology < 0 || technology >= 89 || in.base_count < 0 || in.turn < 0 ) {
        throw std::invalid_argument( "Research valuation requires an ordinary positive human and normal technology" );
    }
    Query query( in );
    const auto related = [&]( int32_t root, int32_t depth ) { return query.Related( technology, root, depth ); };
    const auto facility = [&]( size_t id, int32_t depth ) { return related( in.facility_prerequisites.at( id ), depth ); };
    const auto& ratings = in.technologies[ technology ].ratings;
    const auto X = in.priorities[ 0 ], D = in.priorities[ 1 ], B = in.priorities[ 2 ], K = in.priorities[ 3 ];
    const auto P = ratings[ 0 ], T = ratings[ 1 ], W = ratings[ 2 ], G = ratings[ 3 ];
    const int32_t multiplier = in.blind ? 4 : 2;
    int32_t value = 0;
    for ( const auto& axis : { std::pair< int32_t, int32_t >{ X, G }, { D, T }, { B, W }, { K, P } } ) {
        value = Add( value, Mul( Add( Mul( multiplier, axis.first ), 1 ), axis.second ) );
    }
    bool vendetta = false;
    for ( size_t other = 1; other <= 7; ++other ) { if ( other != size_t( in.faction_id ) && ( in.relations[ other ] & 0x10 ) ) { vendetta = true; } }
    if ( !( ( P && ( K || vendetta ) ) || ( T && D ) || ( W && B ) || ( G && ( X || in.base_count < 4 ) ) ) ) { value = Div( Add( value, 1 ), 2 ); }
    if ( in.base_count ) {
        const auto denominator = Mul( 2, in.base_count );
        for ( size_t region = 1; region <= 62; ++region ) {
            const auto count = in.regional_bases[ in.faction_id ][ region ];
            const auto term = [&]( int32_t factor ) { return Div( Mul( Mul( factor, count ), P ), denominator ); };
            const auto strategy = in.regional_strategy[ region ];
            if ( strategy == 2 ) { value = Add( value, term( 4 ) ); }
            else if ( strategy == 0 ) { value = Add( value, term( in.maximum_attack >= in.maximum_defense ? 2 : 4 ) ); }
            else {
                for ( size_t other = 1; other <= 7; ++other ) {
                    const auto relation = in.relations[ other ];
                    if ( other != size_t( in.faction_id ) && count && in.regional_bases[ other ][ region ] && ( relation & 8 ) && ( !( relation & 3 ) || ( relation & 32 ) ) ) {
                        value = Add( value, term( 1 ) );
                    }
                }
            }
        }
    }
    if ( query.Owns( technology ) ) { return value; }
    if ( in.endgame_pressure && facility( 71, 1 ) ) { value = Mul( value, 4 ); }
    if ( in.intrinsic_planet > 0 && X ) {
        if ( related( 47, 9998 ) ) { value = Mul( value, 3 ); }
        if ( related( 61, 9998 ) ) { value = Mul( value, 2 ); }
        if ( related( 64, 2 ) ) { value = Mul( value, 2 ); }
    }
    if ( in.intrinsic_probe <= 0 && facility( 54, 1 + bool( D ) ) ) {
        if ( !K ) { value = Mul( value, 2 ); }
        if ( D ) { value = Mul( value, 2 ); }
    }
    if ( X && related( 35, 1 ) ) { value = Mul( value, 2 ); }
    if ( ( B || in.cloud_cover == 0 ) && related( 59, 9998 ) ) { value = Mul( value, 2 ); }
    const auto& candidate = in.technologies[ technology ];
    if ( ( candidate.flags & 1 ) && !candidate.possession && !in.blind ) { value = Mul( value, Mul( 2, Add( K, 1 ) ) ); }
    if ( in.psi_bonus > 0 ) { if ( facility( 58, 9998 ) ) { value = Mul( value, 2 ); } }
    else {
        if ( technology == in.reactor_prerequisites[ 0 ] ) { value = Mul( value, 2 ); }
        if ( technology == in.reactor_prerequisites[ 1 ] ) { value = Mul( value, 2 ); }
        if ( related( in.reactor_prerequisites[ 0 ], 9998 ) ) { value = Add( value, 1 ); }
        if ( !in.blind && related( in.reactor_prerequisites[ 0 ], 0 ) ) { value = Mul( value, 2 ); }
    }
    const auto ecology = Div( in.ecology_aggregate, std::clamp( in.base_count, 1, 9999 ) );
    if ( ecology > K && ( facility( 13, 9998 ) || facility( 14, 9998 ) || facility( 31, 9998 ) || facility( 32, 9998 ) ) ) { value = Add( value, ecology ); }
    if ( in.population_bonus > 0 ) {
        if ( facility( 24, 9998 ) ) { value = Mul( value, 2 ); }
        else if ( facility( 25, 9998 ) && in.turn > 250 ) { value = Div( Mul( 3, value ), 2 ); }
    }
    if ( K ) {
        for ( const auto& weapon : in.weapons ) {
            if ( !weapon.power ) { continue; }
            if ( technology == weapon.prerequisite ) { value = Mul( value, 4 ); }
            else if ( related( weapon.prerequisite, 0 ) ) { value = Mul( value, 3 ); }
        }
    }
    if ( D || !K ) {
        for ( int32_t id = 0; id < 89; ++id ) {
            const auto& row = in.technologies[ id ];
            if ( !query.Owns( id ) && ( row.flags & 1 ) && !row.possession && related( id, 0 ) ) { value = Mul( value, 3 ); }
        }
    }
    if ( !query.Owns( in.former_prerequisite ) && related( in.former_prerequisite, 9998 ) ) { value = Mul( value, 4 ); }
    if ( !query.Owns( in.foil_prerequisite ) && related( in.foil_prerequisite, 9998 ) ) {
        bool missing = false;
        for ( size_t region = 1; region <= 63; ++region ) {
            if ( !in.regional_bases[ in.faction_id ][ region ] ) { continue; }
            for ( size_t other = 1; other <= 7; ++other ) {
                if ( other != size_t( in.faction_id ) && !in.regional_bases[ other ][ region ] ) { missing = true; }
            }
            if ( missing && in.regional_known_tiles[ region ] >= in.region_area[ region ] ) { value = Mul( value, 6 ); break; }
        }
        if ( missing ) { value = Add( Mul( 2, value ), 4 ); }
    }
    return value;
}

}
