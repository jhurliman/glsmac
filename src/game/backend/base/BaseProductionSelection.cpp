#include "Base.h"

#include "BaseManager.h"
#include "FacilityDef.h"
#include "SmacProductionSwitch.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/unit/Def.h"
#include "game/backend/unit/SmacWorkshop.h"
#include "game/backend/State.h"
#include "gc/Space.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Int.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include <type_traits>
#include <algorithm>

namespace game::backend {

bool Game::HandleSmacProductionEvent( GSE_CALLABLE, const std::string& name,
    const gse::value::object_properties_t& args, size_t caller, bool apply ) {
    if(name=="commit_smac_workshop") {
        const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
            const auto at=args.find(key);
            if(at==args.end() || !at->second || at->second->Deref()->type!=type) {THROW("Malformed Workshop command");}
            return at->second->Deref();
        };
        if(args.size()!=7) {THROW("Workshop command requires Base, selected design, expected state, armor, name, Done flag and UI request");}
        const auto* object=static_cast<const gse::value::Object*>(field("base",gse::VT_OBJECT));
        if(object->object_class!=base::Base::WRAP_CLASS) {THROW("Workshop requires an actual Base");}
        auto* base=base::Base::Unwrap(args.at("base"));
        const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
        const auto armor=static_cast<const gse::value::Int*>(field("armor",gse::VT_INT))->value;
        const auto selected=static_cast<const gse::value::Int*>(field("selected_design",gse::VT_INT))->value;
        const auto& supplied=static_cast<const gse::value::String*>(field("name",gse::VT_STRING))->value;
        const auto done=static_cast<const gse::value::Bool*>(field("done",gse::VT_BOOL))->value;
        // Client correlation only: ownership and the complete saved-state fence
        // below authorize the edit. Old acknowledgements must not close a new UI.
        const auto request=static_cast<const gse::value::Int*>(field("request_id",gse::VT_INT))->value;
        if(!base || base->GetGame()!=this || caller!=m_slot_num || expected.size()>unit::SmacStartupDesignState::MAX_WIRE_BYTES+89+2*513
            || selected<0 || selected>511 || request<1 || armor<0 || armor>1 || supplied.size()>31 || supplied.find('\0')!=std::string::npos) {THROW("Invalid Workshop command authority or draft");}
        // Current narrow UI has the same printable-byte rendering boundary as
        // Base Rename. Do not install a name that its renderer cannot display.
        for(const unsigned char c:supplied) {if(c<32 || c>126) {THROW("Workshop name requires printable display bytes");}}
        base->RequireSmacProductionSelection();
        if(apply) {
            CheckRW(GSE_CALL);
            if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Workshop requires its owning accumulation");}
        }
        const bool stale=base->SmacWorkshopState()!=expected;
        bool warning=false,failed=false;int32_t design_id=2;
        if(!stale) {
            auto next=m_simulation->value.designs;
            unit::SmacWorkshopDraft draft;draft.selected_design=static_cast<int32_t>(selected);draft.armor=uint32_t(armor);draft.name=supplied;
            const auto& settings=GetState()->m_settings.global;
            const auto* owner=m_simulation->value.technology.Find(base->GetSmacResourceContext().owner_simulation_id);
            try {
                const auto result=unit::CommitSmacWorkshopFirstDesign(next,draft,settings.GetSmacPredefinedUnitRules()->catalog,*settings.GetSmacTechnologyRules(),*owner);
                warning=result.intrinsic_name_warning;design_id=result.design_id;
            } catch(const std::invalid_argument&) {failed=true;}
            catch(const std::runtime_error&) {failed=true;}
            if(apply && !failed) {
                m_simulation->value.designs.owners.swap(next.owners);
            }
        }
        if(apply) {
            GetState()->TriggerObject(this,"smac_workshop_commit",ARGS_F(this,base,done,stale,warning,design_id,failed,request) {
                {"base",base->Wrap(GSE_CALL)}, {"selection",base->CopySmacWorkshop(GSE_CALL)},
                {"done",VALUE(gse::value::Bool,,done && !stale && !failed)}, {"stale",VALUE(gse::value::Bool,,stale)},
                {"failed",VALUE(gse::value::Bool,,failed)},
                {"request_id",VALUE(gse::value::Int,,request)},
                {"intrinsic_name_warning",VALUE(gse::value::Bool,,warning)}, {"design_id",VALUE(gse::value::Int,,design_id)}
            };});
        }
        return true;
    }
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
    if ( !base || base->GetGame() != this || caller != m_slot_num || id < -33
        || (id<0 && !SupportsSmacFacilityProduction(-static_cast<int32_t>(id))) || id > 511 || expected.size() > 16384 ) {
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
std::string Base::SmacWorkshopState() const {
    RequireSmacProductionSelection();
    const auto* technologies=m_game->m_simulation->value.technology.Find(GetSmacResourceContext().owner_simulation_id);
    const auto& rules=m_game->GetState()->m_settings.global.GetSmacPredefinedUnitRules();
    if(!technologies || !rules || rules->catalog.source.ruleset_id!="smac") {THROW("Workshop requires an owned SMAC catalog and technologies");}
    auto state=m_game->m_simulation->value.designs.Serialize(rules->catalog).ToString();
    for(const auto owned:technologies->owned) {state.push_back(owned ? '1':'0');}
    const auto& economy=m_game->m_simulation->value.economy.at(GetSmacResourceContext().owner_simulation_id);
    for(const auto* counts:{&economy.design_counts,&economy.production_reference_counts}) {
        state.push_back(*counts ? '1':'0');
        if(*counts) {for(const auto count:**counts) {state.push_back(static_cast<char>(count));}}
    }
    return state;
}
gse::Value* Base::CopySmacWorkshop(GSE_CALLABLE) const {
    const auto expected=SmacWorkshopState();
    const auto& settings=m_game->GetState()->m_settings.global;
    const auto& catalog=settings.GetSmacPredefinedUnitRules()->catalog;
    const auto& technology=*m_game->m_simulation->value.technology.Find(GetSmacResourceContext().owner_simulation_id);
    const auto choices=unit::SmacWorkshopChoices(unit::SmacWorkshopComponent::Armor,catalog,*settings.GetSmacTechnologyRules(),technology);
    const auto mineral_factor=m_game->GetSmacProductionMineralFactor(m_owner);
    gse::value::array_elements_t armor;
    for(const auto& choice:choices) {
        bool supported=choice.component_id<=1 && choice.technology_available;
        std::string design_name;
        int32_t cost_rows=0;
        if(supported) {
            auto probe=m_game->m_simulation->value.designs;unit::SmacWorkshopDraft draft;draft.armor=choice.component_id;
            try {
                const auto result=unit::CommitSmacWorkshopFirstDesign(probe,draft,catalog,*settings.GetSmacTechnologyRules(),technology);
                if(result.design_id<64) {design_name=catalog.source.units.at(result.design_id).name;cost_rows=catalog.units.at(result.design_id).cost;}
                else {
                    const auto* bank=probe.Find(technology.admission.simulation_id);
                    if(!bank) {throw std::runtime_error("Workshop preview has no owned bank");}
                    const auto design=std::find_if(bank->designs.begin(),bank->designs.end(),[&](const auto& row){return row.id==result.design_id;});
                    if(design==bank->designs.end()) {throw std::runtime_error("Workshop preview has no retained design name");}
                    design_name=unit::SmacWorkshopDesignName(*design,catalog,technology.admission.simulation_id);
                    cost_rows=design->cost;
                }
            }
            catch(const std::exception&) {supported=false;}
        }
        // Workshop shows ordinary rows times 4f9d60's mineral factor, without
        // the Base's separate first-prototype surcharge.
        const int64_t shown_cost=int64_t(cost_rows)*mineral_factor;
        if(shown_cost<0 || shown_cost>INT32_MAX) {throw std::overflow_error("Workshop displayed cost exceeds signed32");}
        armor.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,{
            {"id",VALUE(gse::value::Int,,choice.component_id)},
            {"name",VALUE(gse::value::String,,catalog.defenses.defenses.at(choice.component_id).name)},
            {"short_name",VALUE(gse::value::String,,catalog.defenses.defenses.at(choice.component_id).short_name)},
            {"rating",VALUE(gse::value::Int,,catalog.defenses.defenses.at(choice.component_id).rating)},
            {"design_name",VALUE(gse::value::String,,design_name)},
            {"cost",VALUE(gse::value::Int,,shown_cost)},
            {"technology_available",VALUE(gse::value::Bool,,choice.technology_available)},
            {"available",VALUE(gse::value::Bool,,supported)}
        }));
    }
    return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
        {"expected_state",VALUE(gse::value::String,,expected)}, {"armor",VALUE(gse::value::Array,,armor)},
        {"chassis",VALUE(gse::value::String,,catalog.chassis.chassis.at(0).names[0].name)},
        {"speed",VALUE(gse::value::Int,,catalog.chassis.chassis.at(0).speed)},
        {"weapon",VALUE(gse::value::String,,catalog.weapons.weapons.at(0).name)},
        {"offense",VALUE(gse::value::Int,,catalog.weapons.weapons.at(0).offense)},
        {"reactor",VALUE(gse::value::String,,catalog.reactors.reactors.at(0).name)},
        {"power",VALUE(gse::value::Int,,catalog.reactors.reactors.at(0).power)},
        {"scout_name",VALUE(gse::value::String,,catalog.source.units.at(2).name)}
    });
}
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
    const bool facility=source_id>=-33 && source_id<0 && Game::SupportsSmacFacilityProduction(-source_id);
    if(facility && !m_game->GetBM()->GetSmacFacilityDef(-source_id)) {THROW("Facility selection has no registered definition");}
    if ( !designs || (!facility && source_id != 0 && source_id != 1 && source_id != 2 && source_id != designs->starting_scout ) ) {
        THROW( "Production selector does not support this design" );
    }
    const auto target = m_game->MakeSmacProductionItem( m_owner, facility ? ProductionItemKind::Facility : ProductionItemKind::Unit, facility ? -source_id : source_id );
    if (!(facility ? m_game->IsSmacFacilityProductionEligible(*this,target) : m_game->IsSmacUnitProductionEligible(*this,target))) {THROW("Production target is no longer eligible");}
    if(!facility) {(void)m_game->ResolveSmacProductionPresentation( m_owner, source_id );}
    (void)m_game->GetSmacProductionCost( m_owner, target, this );
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
        result.push_back( { id, def->m_name, m_game->GetSmacProductionCost( m_owner, item, this ), switched.state.minerals, switched.penalty_prompt_required } );
    }
    for(int32_t id=1;id<=33;++id) {
        if(!Game::SupportsSmacFacilityProduction(id)) {continue;}
        const auto* definition=m_game->GetBM()->GetSmacFacilityDef(id);if(!definition) {continue;}
        const auto item=m_game->MakeSmacProductionItem(m_owner,ProductionItemKind::Facility,id);
        if(!m_game->IsSmacFacilityProductionEligible(*this,item)) {continue;}
        const auto switched=PrepareSmacProductionSelection(-id,true);
        result.push_back({-id,definition->m_name,m_game->GetSmacProductionCost( m_owner, item, this ),switched.state.minerals,switched.penalty_prompt_required});
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
    auto context = GetSmacResourceContext();
    const auto target = m_game->MakeSmacProductionItem( m_owner, source_id<0 ? ProductionItemKind::Facility : ProductionItemKind::Unit, source_id<0 ? -source_id : source_id );
    if ( !( target == m_production->current_item ) && ( context.governor_options & SmacBaseResourceContext::STARTUP_AUTOMATION ) ) {
        context.governor_options &= ~( SmacBaseResourceContext::GOVERNOR_ACTIVE | SmacBaseResourceContext::STARTUP_AUTOMATION );
    }
    // The admitted changed-item prelude precedes the loss decision. Declining
    // keeps production intact, but does not restore marked governor activation.
    // There is no governor display to refresh: a production refresh here would
    // close the still-open loss prompt. Subsequent worker queries read this record.
    static_assert( std::is_nothrow_copy_assignable_v< decltype( context ) > );
    if ( result.state == *m_production ) { *m_resource_context = context; return; }
    std::optional< ProductionState > replacement = result.state;
    auto references=PrepareSmacProductionReferences(replacement);auto* reference_target=SmacProductionReferencesTarget();
    auto updates = bm->m_base_updates;
    BaseManager::QueueBaseUpdateInto( updates, this, BaseManager::BUO_REFRESH );
    std::unique_lock lock( bm->m_updated_bases_mutex );
    auto triggers = bm->m_updated_bases; triggers.insert( this );
    static_assert( std::is_nothrow_swappable_v< decltype( replacement ) > );
    // GetSmacResourceContext authenticated the engaged record before staging.
    // Assign that record directly; libc++ does not annotate optional assignment noexcept.
    static_assert( std::is_nothrow_copy_assignable_v< decltype(context) > );
    m_production.swap( replacement );if(reference_target) {reference_target->swap(references);} *m_resource_context = context; bm->m_base_updates.swap( updates ); bm->m_updated_bases.swap( triggers );
}
}
}
