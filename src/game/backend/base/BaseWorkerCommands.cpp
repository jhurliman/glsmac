#include "Base.h"

#include "BaseManager.h"
#include "PopDef.h"
#include "game/backend/faction/Faction.h"
#include <algorithm>
#include "SmacWorkedSquareCollection.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"
#include "gc/Space.h"
#include "gse/value/Object.h"

namespace game::backend {

bool Game::HandleSmacWorkerEvent( GSE_CALLABLE, const std::string& name,
    const gse::value::object_properties_t& args, size_t caller, bool apply ) {
    if ( name != "work_base_tile" && name != "unwork_base_tile" ) { return false; }
    const auto base_arg = args.find( "base" );
    if ( base_arg == args.end() || !base_arg->second ) { return false; }
    const auto* object = base_arg->second->Deref();
    if ( object->type != gse::VT_OBJECT || static_cast< const gse::value::Object* >( object )->object_class != base::Base::WRAP_CLASS ) { return false; }
    auto* base = base::Base::Unwrap( base_arg->second );
    if ( !base->HasSmacResourceContext() ) { return false; }
    // This is a native local command, never an exemption for registered script
    // callbacks, optimistic network application, or user-supplied caller data.
    base->GetSmacResourceContext();
    if ( base->GetGame() != this || !IsRunning() || m_state->GetConnection()
        || caller != m_slot_num || base->m_owner->GetIndex() != caller
        || base->m_owner->GetPlayer() != GetPlayer() || GetPlayer()->GetRole() != Player::PR_SINGLE ) {
        THROW( "SMAC worker command requires the authenticated local Base owner" );
    }
    const auto tile_arg = args.find( "tile" );
    if ( args.size() != 2 || tile_arg == args.end() || !tile_arg->second ) { THROW( "SMAC worker command requires only Base and tile" ); }
    auto* tile = map::tile::Tile::Unwrap( tile_arg->second );
    if ( !tile || tile->tiles != GetMap()->GetTilesPtr() ) { THROW( "SMAC worker command tile is not in the current map" ); }
    if ( apply ) { base->CommandSmacWorkerTile( GSE_CALL, tile ); }
    return true;
}

namespace base {
namespace {
int32_t Checked( int64_t value ) {
    if ( value < INT32_MIN || value > INT32_MAX ) { THROW( "Manual worker arithmetic exceeds signed32" ); }
    return static_cast< int32_t >( value );
}
int32_t Mul( int32_t a, int32_t b ) { return Checked( int64_t( a ) * b ); }
int32_t Clamp( int32_t value ) { return std::max( 1, std::min( 99, value ) ); }
}
void Base::CommandSmacWorkerTile( GSE_CALLABLE, map::tile::Tile* tile ) {
    m_game->CheckRW( GSE_CALL );
    if ( gc_space != m_game->GetGCSpace() || !gc_space->IsAccumulating() ) { THROW( "SMAC worker command requires its owning accumulation" ); }
    auto* bm = m_game->GetBM();
    bm->RequireSmacWorkerRecalculation();
    auto context = GetSmacResourceContext();
    if ( m_game->GetState()->GetConnection() || !m_game->GetPlayer()
        || m_game->GetPlayer()->GetRole() != Player::PR_SINGLE || m_owner->GetPlayer() != m_game->GetPlayer() ) {
        THROW( "SMAC manual worker command requires the local human Base owner" );
    }
    const auto allocation = GetSmacAllocation();
    if ( !allocation ) { THROW( "SMAC worker command requires complete allocation authority" ); }
    const auto* map = m_game->GetMap();
    if ( !tile || tile->tiles != map->GetTilesPtr() || map->GetWidth() > INT32_MAX || map->GetHeight() > INT32_MAX ) {
        THROW( "SMAC worker command has unsupported map geometry" );
    }
    const SmacWorkerRadiusMap radius{ { static_cast< int32_t >( m_tile->coord.x ), static_cast< int32_t >( m_tile->coord.y ) },
        static_cast< int32_t >( map->GetWidth() ), static_cast< int32_t >( map->GetHeight() ), true };
    int32_t clicked = -1;
    for ( int32_t i = 0; i <= 20; ++i ) {
        const auto xy = SmacWorkedSquareCollection::Resolve( radius, i );
        if ( xy && map->GetTile( ( *xy )[ 0 ], ( *xy )[ 1 ] ) == tile ) {
            if ( clicked != -1 ) { THROW( "SMAC worker command does not admit aliased radius coordinates" ); }
            clicked = i;
        }
    }
    if ( clicked < 0 ) { THROW( "SMAC worker command tile is outside the Base radius" ); }
    // The real handler's preliminary ordinary intake can repair existing
    // allocation before the click. Keep that result private until final publish.
    auto next = CalculateSmacWorkers( *allocation, context );
    context.governor_options &= ~SmacBaseResourceContext::MANAGE_CITIZENS;
    const uint32_t bit = uint32_t( 1 ) << clicked;
    if ( clicked == 0 ) {
        next.selected = 0;
        next.specialists.total_count = 0;
    }
    else if ( next.selected & bit ) {
        if ( next.specialists.total_count >= 16 ) { THROW( "Live worker command specialist storage is full" ); }
        next.selected &= ~bit;
        next.specialists.types[ next.specialists.total_count++ ] = SelectAutomaticSmacSpecialist();
    }
    else if ( !( m_game->GetSmacWorkerAvailability( *this ).unavailable & bit ) ) {
        if ( next.specialists.total_count ) {
            // Ordinary click consumes the last counted slot without shifting
            // padding; a specialist drag has a different, unexposed contract.
            --next.specialists.total_count;
            next.selected |= bit;
        }
        else {
            const auto& global = m_game->GetState()->m_settings.global;
            const auto& factions = global.GetSmacFactionStartupRules();
            const auto* faction = factions ? factions->Find( m_faction->m_id ) : nullptr;
            if ( !faction ) { THROW( "Manual replacement requires frozen faction priorities" ); }
            const auto& priorities = faction->worker_priorities;
            const int32_t population = static_cast< int32_t >( m_pops.size() );
            const auto query = [ & ]( int32_t index, const SmacWorkerStepState& ) -> std::optional< SmacWorkerTileYield > {
                const auto xy = SmacWorkedSquareCollection::Resolve( radius, index );
                if ( !xy ) { return std::nullopt; }
                const auto* candidate = map->GetTile( ( *xy )[ 0 ], ( *xy )[ 1 ] );
                const auto values = m_game->GetCanonicalTileResources( *candidate, *this, Game::ResourceQueryPhase::Ordinary );
                return SmacWorkerTileYield{ { Checked( values[ 0 ] ), Checked( values[ 1 ] ), Checked( values[ 2 ] ) }, candidate->is_water_tile };
            };
            const auto collected = SmacWorkedSquareCollection::Collect( { population, next.selected, 0,
                m_game->GetCanonicalOrbitalIntake( *this ), false, 0, 0 }, query );
            const auto& intake = collected.state.intake;
            const auto demand = Mul( population, global.GetSmacResourceRules()->nutrients_per_citizen );
            // Manual replacement consumes actual homed-unit support and the
            // frozen faction priorities; automatic governor-off allocation
            // has its separate zero-priority branch.
            const int32_t support = m_game->GetCanonicalMineralSupport( *this );
            int32_t minimum = 999, remove = 0;
            for ( int32_t i = 1; i <= 20; ++i ) {
                if ( !( next.selected & ( uint32_t( 1 ) << i ) ) ) { continue; }
                const auto candidate = query( i, collected.state );
                if ( !candidate ) { continue; }
                const auto& yield = candidate->resources;
                const auto food = Mul( Mul( yield[ 0 ], Checked( int64_t( priorities[ 1 ] ) + 3 ) ), 4 ) / Clamp( Checked( int64_t( intake[ 0 ] ) - demand ) );
                const auto minerals = Mul( Mul( yield[ 1 ], Checked( int64_t( priorities[ 0 ] ) + priorities[ 4 ] + 4 ) ), population ) / Clamp( Checked( int64_t( intake[ 1 ] ) - support ) );
                const auto energy = Mul( Mul( yield[ 2 ], population ), Checked( int64_t( priorities[ 3 ] ) + priorities[ 2 ] + 2 ) ) / Clamp( intake[ 2 ] );
                const auto score = Checked( int64_t( Checked( int64_t( food ) + minerals ) ) + energy );
                if ( score < minimum ) { minimum = score; remove = i; }
            }
            if ( remove ) { next.selected = ( next.selected & ~( uint32_t( 1 ) << remove ) ) | bit; }
        }
    }
    next = CalculateSmacWorkers( next, context );
    SmacAllocationRoles roles;
    if (m_game->HasSmacBasePresentations()) {
        const auto& configured=m_game->GetSmacBasePresentations(); roles.worker=configured[0];
        for(size_t i=0;i<7;++i) {roles.specialists[i]=configured[i+1];}
    } else for ( const auto& item : bm->GetBasePopDefs() ) {
        const auto* def = item.second;
        if ( def->m_flags & PopDef::PF_TILE_WORKER ) {
            if ( !roles.worker.empty() ) { THROW( "SMAC worker command requires one registered worker presentation role" ); }
            roles.worker = item.first;
        }
        else if ( def->m_smac_specialist_id ) { roles.specialists[ *def->m_smac_specialist_id ] = item.first; }
    }
    PublishSmacCalculatedWorkers( GSE_CALL, next, roles, context, SmacBaseResourcePhase::Ordinary );
}
}
}
