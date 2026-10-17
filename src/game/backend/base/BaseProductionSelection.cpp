#include "Base.h"

#include "BaseManager.h"
#include "FacilityDef.h"
#include "SmacProductionSwitch.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/unit/Def.h"
#include "gc/Space.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Int.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include <type_traits>

namespace game::backend {

bool Game::HandleSmacProductionEvent( GSE_CALLABLE, const std::string& name,
    const gse::value::object_properties_t& args, size_t caller, bool apply ) {
    if ( name != "select_smac_production" ) { return false; }
    const auto field = [ & ]( const char* key, gse::value_type_t type ) -> const gse::Value* {
        const auto at = args.find( key );
        if ( at == args.end() || !at->second || at->second->Deref()->type != type ) { THROW( "Malformed production selection command" ); }
        return at->second->Deref();
    };
    if ( args.size() != 5 ) { THROW( "Production selection requires Base, target, saved state, shown balance and confirmation" ); }
    const auto* object = static_cast< const gse::value::Object* >( field( "base", gse::VT_OBJECT ) );
    if ( object->object_class != base::Base::WRAP_CLASS ) { THROW( "Production selection requires an actual Base" ); }
    auto* base = base::Base::Unwrap( args.at( "base" ) );
    const auto id = static_cast< const gse::value::Int* >( field( "source_id", gse::VT_INT ) )->value;
    const auto& expected = static_cast< const gse::value::String* >( field( "expected_state", gse::VT_STRING ) )->value;
    const auto confirm = static_cast< const gse::value::Bool* >( field( "confirm", gse::VT_BOOL ) )->value;
    const auto shown = static_cast< const gse::value::Int* >( field( "expected_minerals", gse::VT_INT ) )->value;
    if ( !base || base->GetGame() != this || caller != m_slot_num || id < 0 || id > 511 || expected.size() > 16384 ) {
        THROW( "Production selection has invalid authority or arguments" );
    }
    base->RequireSmacProductionSelection();
    // Old clicks are harmless after authentication, including duplicate queued
    // confirmations. No supplied state is decoded or installed.
    if ( base::SerializeProductionState( base->GetProductionState() ).ToString() != expected ) { return true; }
    // Facility changes can change the retool result without changing the saved
    // production record. Never accept a different balance from the one shown.
    if ( base->PrepareSmacProductionSelection( static_cast< int32_t >( id ), true ).state.minerals != shown ) { return true; }
    if ( apply ) { base->SelectSmacProduction( GSE_CALL, static_cast< int32_t >( id ), expected, confirm ); }
    return true;
}

namespace base {
void Base::RequireSmacProductionSelection() const {
    GetSmacResourceContext();
    m_game->RequireSmacLocalHumanOwner( m_owner );
    if ( !m_game->IsRunning() || m_game->m_canonical_turn_failed || m_game->m_canonical_turn_in_progress
        || m_game->GetBM()->GetBase( m_id ) != this || m_owner->GetPlayer() != m_game->GetPlayer()
        || !m_production || !m_production->queue || !m_allocation ) {
        THROW( "Production selection requires the ready local canonical Base" );
    }
    const auto* social = m_game->m_simulation->value.social.Find( GetSmacResourceContext().owner_simulation_id );
    if ( !social || social->phase != faction::SmacFactionSocialState::Phase::Refreshed ) {
        THROW( "Production selection requires refreshed social authority" );
    }
    m_game->ValidateSmacProductionState( m_owner, *m_production );
}

SmacManualSwitchResult Base::PrepareSmacProductionSelection( const int32_t source_id, const bool confirm ) const {
    RequireSmacProductionSelection();
    const auto* designs = m_game->m_simulation->value.designs.Find( GetSmacResourceContext().owner_simulation_id );
    if ( !designs || ( source_id != 0 && source_id != 1 && source_id != 2 && source_id != designs->starting_scout ) ) {
        THROW( "Production selector does not support this design" );
    }
    const auto target = m_game->MakeSmacProductionItem( m_owner, ProductionItemKind::Unit, source_id );
    if ( !m_game->IsSmacUnitProductionEligible( *this, target ) ) { THROW( "Production target is no longer eligible" ); }
    (void)m_game->ResolveSmacProductionPresentation( m_owner, source_id );
    (void)m_game->GetSmacProductionCost( m_owner, target );
    const auto& rules = *m_game->GetState()->m_settings.global.GetSmacProductionRules();
    SmacProductionSwitchContext context{ rules.retool_strictness, rules.retool_loss_percent, rules.retool_exemption,
        true, unit::SmacStartupDesignState::OriginalDifficulty( m_owner->GetPlayer()->GetDifficultyLevel() ), false, {} };
    for ( const auto& entry : m_facilities ) {
        const auto* facility = entry.second;
        if ( !facility || m_game->GetBM()->GetFacilityDef( entry.first ) != facility || !facility->m_smac_source_id ) {
            THROW( "Production selection requires canonical facility roles" );
        }
        m_game->GetBM()->ValidateSmacFacilityDef( facility );
        const auto id = *facility->m_smac_source_id;
        context.skunkworks |= id == 10;
        if ( id <= 33 ) { context.present_local_facilities.insert( m_game->MakeSmacProductionItem( m_owner, ProductionItemKind::Facility, id ).id ); }
    }
    return SmacProductionSwitch::Manual( *m_production, target, confirm ? SmacPenaltyChoice::Accept : SmacPenaltyChoice::Cancel, context );
}

std::vector< Base::ProductionChoice > Base::GetSmacProductionChoices() const {
    RequireSmacProductionSelection();
    const auto* designs = m_game->m_simulation->value.designs.Find( GetSmacResourceContext().owner_simulation_id );
    if ( !designs ) { THROW( "Production choices require the owned design selection" ); }
    std::vector< int32_t > ids{ 2, 1, 0 };
    if ( designs->starting_scout != 2 ) { ids.push_back( designs->starting_scout ); }
    std::vector< ProductionChoice > result;
    for ( const auto id : ids ) {
        const auto item = m_game->MakeSmacProductionItem( m_owner, ProductionItemKind::Unit, id );
        if ( !m_game->IsSmacUnitProductionEligible( *this, item ) ) { continue; }
        const auto switched = PrepareSmacProductionSelection( id, true );
        const auto* def = m_game->ResolveSmacProductionPresentation( m_owner, id );
        result.push_back( { id, def->m_name, m_game->GetSmacProductionCost( m_owner, item ), switched.state.minerals, switched.penalty_prompt_required } );
    }
    return result;
}

gse::Value* Base::CopySmacProductionChoices( GSE_CALLABLE ) const {
    const auto choices = GetSmacProductionChoices();
    gse::value::array_elements_t values;
    for ( const auto& choice : choices ) {
        values.push_back( VALUE( gse::value::Object,, GSE_CALL_NOGC, {
            { "source_id", VALUE( gse::value::Int,, choice.source_id ) },
            { "name", VALUE( gse::value::String,, choice.name ) },
            { "cost", VALUE( gse::value::Int,, choice.cost ) },
            { "minerals", VALUE( gse::value::Int,, choice.minerals ) },
            { "confirmation", VALUE( gse::value::Bool,, choice.confirmation ) },
        } ) );
    }
    return VALUE( gse::value::Object,, GSE_CALL_NOGC, {
        { "expected_state", VALUE( gse::value::String,, SerializeProductionState( m_production ).ToString() ) },
        { "minerals", VALUE( gse::value::Int,, m_production->minerals ) },
        { "choices", VALUE( gse::value::Array,, values ) },
    } );
}

void Base::SelectSmacProduction( GSE_CALLABLE, const int32_t source_id, const std::string& expected, const bool confirm ) {
    m_game->CheckRW( GSE_CALL );
    if ( gc_space != m_game->GetGCSpace() || !gc_space->IsAccumulating() ) { THROW( "Production selection requires its owning accumulation" ); }
    auto* bm = m_game->GetBM(); bm->RequireSmacWorkerRecalculation();
    RequireSmacProductionSelection();
    if ( SerializeProductionState( m_production ).ToString() != expected ) { return; }
    const auto result = PrepareSmacProductionSelection( source_id, confirm );
    if ( result.penalty_cancelled || result.state == *m_production ) { return; }
    std::optional< ProductionState > replacement = result.state;
    auto context = GetSmacResourceContext();
    if (!(result.state.current_item == m_production->current_item) && (context.governor_options & SmacBaseResourceContext::STARTUP_AUTOMATION)) {
        context.governor_options &= ~(SmacBaseResourceContext::GOVERNOR_ACTIVE | SmacBaseResourceContext::STARTUP_AUTOMATION);
    }
    auto updates = bm->m_base_updates;
    BaseManager::QueueBaseUpdateInto( updates, this, BaseManager::BUO_REFRESH );
    std::unique_lock lock( bm->m_updated_bases_mutex );
    auto triggers = bm->m_updated_bases; triggers.insert( this );
    static_assert( std::is_nothrow_swappable_v< decltype( replacement ) > );
    static_assert( std::is_nothrow_assignable_v< decltype(m_resource_context)&, decltype(context) > );
    m_production.swap( replacement ); m_resource_context = context; bm->m_base_updates.swap( updates ); bm->m_updated_bases.swap( triggers );
}
}
}
