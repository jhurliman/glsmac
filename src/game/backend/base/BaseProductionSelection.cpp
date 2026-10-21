#include "Base.h"

#include "BaseManager.h"
#include "FacilityDef.h"
#include "SmacProductionSwitch.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/slot/Slot.h"
#include "game/backend/unit/Def.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/SmacWorkshop.h"
#include "game/backend/State.h"
#include "game/backend/faction/Faction.h"
#include "gc/Space.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Int.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include <type_traits>
#include <algorithm>

namespace game::backend {

bool Game::HandleSmacProductionEvent( GSE_CALLABLE, const std::string& name,
    const gse::value::object_properties_t& args, size_t caller, bool apply ) {
    if(name=="open_smac_workshop_rename" || name=="answer_smac_workshop_rename") {
        const bool answer=name=="answer_smac_workshop_rename";
        const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
            const auto at=args.find(key);
            if(at==args.end() || !at->second || at->second->Deref()->type!=type) {THROW("Malformed Workshop Rename command");}
            return at->second->Deref();
        };
        if(args.size()!=(answer ? 12u:10u)) {THROW("Workshop Rename requires its complete draft/fence and answer text");}
        const auto integer=[&](const char* key){return static_cast<const gse::value::Int*>(field(key,gse::VT_INT))->value;};
        const auto text=[&](const char* key)->const std::string& {return static_cast<const gse::value::String*>(field(key,gse::VT_STRING))->value;};
        const auto* object=static_cast<const gse::value::Object*>(field("base",gse::VT_OBJECT));
        if(object->object_class!=base::Base::WRAP_CLASS) {THROW("Workshop Rename requires an actual Base");}
        auto* base=base::Base::Unwrap(args.at("base"));
        const auto selected=integer("selected_design"),chassis=integer("chassis"),weapon=integer("weapon"),armor=integer("armor"),reactor=integer("reactor"),abilities=integer("abilities"),request=integer("request_id");
        const auto& retained=text("name");const auto& expected=text("expected_state");
        const auto displayed=answer ? text("displayed_default"):std::string();const auto submitted=answer ? text("submitted"):std::string();
        if(!base || base->GetGame()!=this || caller!=m_slot_num || selected<0 || selected>511 || chassis<0 || chassis>8 || weapon<0 || weapon>20 || armor<0 || armor>9
            || reactor<1 || reactor>4 || abilities<0 || abilities>0xffffff || request<1 || expected.size()>unit::SmacStartupDesignState::MAX_WIRE_BYTES+89+2*513) {THROW("Invalid Workshop Rename authority or draft");}
        for(const auto* item:{&retained,&displayed,&submitted}) {
            if(item->size()>31 || item->find('\0')!=std::string::npos) {THROW("Workshop Rename exceeds31 display bytes");}
            for(const unsigned char c:*item) {if(c<32 || c>126) {THROW("Workshop Rename requires printable display bytes");}}
        }
        base->RequireSmacProductionSelection();const auto owner=base->GetSmacResourceContext().owner_simulation_id;
        if(selected>=64 && selected/64!=owner) {THROW("Workshop Rename selected a foreign bank");}
        if(apply) {CheckRW(GSE_CALL);if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Workshop Rename requires its owning accumulation");}}
        const bool stale=base->SmacWorkshopState()!=expected;bool failed=false;std::string default_name,display_name,result_name=retained;
        if(!stale) {try {
            const auto& catalog=GetState()->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
            const unit::SmacWorkshopDraft draft{int32_t(selected),uint32_t(chassis),uint32_t(weapon),uint32_t(armor),uint32_t(abilities),uint8_t(reactor),retained};
            auto result=answer ? unit::AnswerSmacWorkshopRename(m_simulation->value.designs,draft,displayed,true,submitted,catalog,owner)
                :unit::OpenSmacWorkshopRename(m_simulation->value.designs,draft,catalog,owner);
            default_name=result.default_name;result_name=result.draft.name;
            display_name=unit::DisplaySmacWorkshopName(*result.state.native_registry,result.draft,catalog,owner);
            if(apply) {static_assert(std::is_nothrow_swappable_v<unit::SmacStartupDesignState>);std::swap(m_simulation->value.designs,result.state);}
        }catch(const std::invalid_argument&) {failed=true;}catch(const std::runtime_error&) {failed=true;}}
        if(apply) {GetState()->TriggerObject(this,"smac_workshop_rename",ARGS_F(this,base,request,answer,stale,failed,default_name,display_name,result_name) {
            {"base",base->Wrap(GSE_CALL)}, {"request_id",VALUE(gse::value::Int,,request)}, {"answer",VALUE(gse::value::Bool,,answer)},
            {"stale",VALUE(gse::value::Bool,,stale)}, {"failed",VALUE(gse::value::Bool,,failed)},
            {"expected_state",VALUE(gse::value::String,,base->SmacWorkshopState())}, {"default_name",VALUE(gse::value::String,,default_name)},
            {"display_name",VALUE(gse::value::String,,display_name)}, {"name",VALUE(gse::value::String,,result_name)}
        };});}
        return true;
    }
    if(name=="obsolete_smac_workshop") {
        const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
            const auto at=args.find(key);
            if(at==args.end() || !at->second || at->second->Deref()->type!=type) {THROW("Malformed Workshop Obsolete command");}
            return at->second->Deref();
        };
        if(args.size()!=7) {THROW("Workshop Obsolete requires Base, selection, name, old list, strip offset, state and request");}
        const auto* object=static_cast<const gse::value::Object*>(field("base",gse::VT_OBJECT));
        if(object->object_class!=base::Base::WRAP_CLASS) {THROW("Workshop Obsolete requires an actual Base");}
        auto* base=base::Base::Unwrap(args.at("base"));
        const auto selected=static_cast<const gse::value::Int*>(field("selected_design",gse::VT_INT))->value;
        const auto offset=static_cast<const gse::value::Int*>(field("strip_offset",gse::VT_INT))->value;
        const auto request=static_cast<const gse::value::Int*>(field("request_id",gse::VT_INT))->value;
        const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
        const auto& retained=static_cast<const gse::value::String*>(field("name",gse::VT_STRING))->value;
        const auto& list=static_cast<const gse::value::Array*>(field("old_list",gse::VT_ARRAY))->value;
        if(list.size()>128) {THROW("Workshop Obsolete old list exceeds owner domain");}
        std::vector<int32_t> old_list;
        for(const auto* item:list) {
            if(!item || item->type!=gse::VT_INT || static_cast<const gse::value::Int*>(item)->value<0 || static_cast<const gse::value::Int*>(item)->value>511) {THROW("Invalid Workshop old browse identity");}
            old_list.push_back(int32_t(static_cast<const gse::value::Int*>(item)->value));
        }
        if(!base || base->GetGame()!=this || caller!=m_slot_num || selected<0 || selected>511 || offset<0 || offset>512 || request<1
            || expected.size()>unit::SmacStartupDesignState::MAX_WIRE_BYTES+89+2*513 || retained.size()>31 || retained.find('\0')!=std::string::npos) {
            THROW("Invalid Workshop Obsolete authority or fields");
        }
        for(const unsigned char c:retained) {if(c<32 || c>126) {THROW("Workshop name requires printable display bytes");}}
        base->RequireSmacProductionSelection();const auto owner_id=base->GetSmacResourceContext().owner_simulation_id;
        if(selected>=64 && selected/64!=owner_id) {THROW("Workshop Obsolete selected a foreign bank");}
        if(apply) {CheckRW(GSE_CALL);if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Workshop Obsolete requires its owning accumulation");}}
        const bool stale=base->SmacWorkshopState()!=expected;bool failed=false;
        unit::SmacWorkshopDraft response{int32_t(selected),0,0,0,0,1,retained};size_t first=size_t(offset);
        std::array<int32_t,2> slots{-1,-1};
        if(!stale) {try {
            const auto& settings=GetState()->m_settings.global;const auto& catalog=settings.GetSmacPredefinedUnitRules()->catalog;
            const auto& owner=*m_simulation->value.technology.Find(owner_id);const auto& economy=m_simulation->value.economy.at(owner_id);
            auto result=unit::PrepareSmacWorkshopObsolete(m_simulation->value.designs,int32_t(selected),retained,old_list,size_t(offset),catalog,
                *settings.GetSmacTechnologyRules(),owner,economy.design_counts);
            const auto clean=unit::CleanSmacWorkshopRetainedAbilities(result.selection.draft,catalog,*settings.GetSmacTechnologyRules(),owner);
            result.selection.draft.abilities=clean.mask;response=result.selection.draft;slots=clean.slots;first=result.strip_offset;
            if(apply) {static_assert(std::is_nothrow_swappable_v<unit::SmacStartupDesignState>);std::swap(m_simulation->value.designs,result.selection.state);}
        } catch(const std::invalid_argument&) {failed=true;}catch(const std::runtime_error&) {failed=true;}}
        if(apply) {GetState()->TriggerObject(this,"smac_workshop_obsolete",_ARGS_F(this,base,response,slots,first,request,stale,failed) {
            auto* view=static_cast<gse::value::Object*>(base->CopySmacWorkshop(GSE_CALL,stale || failed ? nullptr:&response));
            view->Assign("ability_slots",VALUE(gse::value::Array,,gse::value::array_elements_t{VALUE(gse::value::Int,,slots[0]),VALUE(gse::value::Int,,slots[1])}));
            out_args={
                {"base",base->Wrap(GSE_CALL)}, {"selection",view}, {"strip_offset",VALUE(gse::value::Int,,first)},
                {"request_id",VALUE(gse::value::Int,,request)}, {"stale",VALUE(gse::value::Bool,,stale)}, {"failed",VALUE(gse::value::Bool,,failed)}
            };
        });}
        return true;
    }
    if(name=="commit_smac_workshop" || name=="select_smac_workshop") {
        const bool selecting=name=="select_smac_workshop";
        const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
            const auto at=args.find(key);
            if(at==args.end() || !at->second || at->second->Deref()->type!=type) {THROW("Malformed Workshop command");}
            return at->second->Deref();
        };
        const bool complete=args.find("chassis")!=args.end() || args.find("weapon")!=args.end() || args.find("reactor")!=args.end() || args.find("abilities")!=args.end();
        const bool answered=args.find("overwrite")!=args.end();
        if((selecting && (complete || answered || args.size()!=5))
            || (!selecting && args.size()!=(complete ? 11u:7u)+(answered ? 1u:0u))) {THROW("Workshop command requires its complete draft/fence and optional overwrite answer");}
        const auto* object=static_cast<const gse::value::Object*>(field("base",gse::VT_OBJECT));
        if(object->object_class!=base::Base::WRAP_CLASS) {THROW("Workshop requires an actual Base");}
        auto* base=base::Base::Unwrap(args.at("base"));
        const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
        const auto armor=selecting ? 0:static_cast<const gse::value::Int*>(field("armor",gse::VT_INT))->value;
        const auto selected=static_cast<const gse::value::Int*>(field("selected_design",gse::VT_INT))->value;
        const auto& supplied=static_cast<const gse::value::String*>(field("name",gse::VT_STRING))->value;
        const auto done=selecting ? false:static_cast<const gse::value::Bool*>(field("done",gse::VT_BOOL))->value;
        const auto chassis=complete ? static_cast<const gse::value::Int*>(field("chassis",gse::VT_INT))->value:0;
        const auto weapon=complete ? static_cast<const gse::value::Int*>(field("weapon",gse::VT_INT))->value:0;
        const auto reactor=complete ? static_cast<const gse::value::Int*>(field("reactor",gse::VT_INT))->value:1;
        const auto abilities=complete ? static_cast<const gse::value::Int*>(field("abilities",gse::VT_INT))->value:0;
        const auto overwrite=answered ? static_cast<const gse::value::Int*>(field("overwrite",gse::VT_INT))->value:-1;
        if(overwrite< -1 || overwrite>1 || (answered && overwrite<0)) {THROW("Invalid Workshop overwrite answer");}
        // Client correlation only: ownership and the complete saved-state fence
        // below authorize the edit. Old acknowledgements must not close a new UI.
        const auto request=static_cast<const gse::value::Int*>(field("request_id",gse::VT_INT))->value;
        if(!base || base->GetGame()!=this || caller!=m_slot_num || expected.size()>unit::SmacStartupDesignState::MAX_WIRE_BYTES+89+2*513
            || selected<0 || selected>511 || request<1 || armor<0 || armor>9 || chassis<0 || chassis>8 || weapon<0 || weapon>20
            || reactor<1 || reactor>4 || abilities<0 || abilities>0xffffff || supplied.size()>31 || supplied.find('\0')!=std::string::npos) {THROW("Invalid Workshop command authority or draft");}
        // Current narrow UI has the same printable-byte rendering boundary as
        // Base Rename. Do not install a name that its renderer cannot display.
        for(const unsigned char c:supplied) {if(c<32 || c>126) {THROW("Workshop name requires printable display bytes");}}
        base->RequireSmacProductionSelection();
        if(selected>=64 && selected/64!=base->GetSmacResourceContext().owner_simulation_id) {THROW("Workshop selected a foreign bank");}
        if(apply) {
            CheckRW(GSE_CALL);
            if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Workshop requires its owning accumulation");}
        }
        const bool stale=base->SmacWorkshopState()!=expected;
        bool warning=false,failed=false,confirmation=false,declined=false,reselected=false,retired=false;
        int32_t design_id=int32_t(selected),overwrite_count=0,restored_armor=0;std::string overwrite_name,restored_name;
        unit::SmacWorkshopDraft response_draft{int32_t(selected),uint32_t(chassis),uint32_t(weapon),uint32_t(armor),uint32_t(abilities),uint8_t(reactor),supplied};
        std::optional<unit::UnitManager::PreparedConventionalRemoval> removal;
        std::vector<std::pair<base::Base*,std::optional<base::ProductionState>>> queues;
        std::optional<decltype(GetBM()->m_base_updates)> base_updates;
        if(!stale) {
            auto next=m_simulation->value.designs;
            const auto draft=response_draft;
            const auto& settings=GetState()->m_settings.global;
            const auto* owner=m_simulation->value.technology.Find(base->GetSmacResourceContext().owner_simulation_id);
            try {
                const auto& economy=m_simulation->value.economy.at(owner->admission.simulation_id);
                unit::SmacWorkshopCommit result;
                if(selecting) {result={int32_t(selected),false};}
                else if(next.native_registry) {
                    const auto& catalog=settings.GetSmacPredefinedUnitRules()->catalog;
                    if(const auto alias=unit::ReuseSmacWorkshopDesign(next,draft,catalog,*settings.GetSmacTechnologyRules(),*owner)) {result=*alias;}
                    else {
                        unit::ValidateSmacWorkshopEditorAbilities(draft,catalog,*settings.GetSmacTechnologyRules(),*owner);
                        // A wholly inactive, included bank makes both cached rows
                        // irrelevant to the original provisional-slot scan. Do
                        // not initialize missing saved caches as a side effect.
                        const auto first=size_t(64*owner->admission.simulation_id);bool independent=true;
                        for(size_t id=first;id<first+64;++id) {const auto& row=next.native_registry->at(id);
                            independent=independent && !(row[48]&1) && !(row[44]&(1u<<owner->admission.simulation_id));}
                        if((!economy.design_counts || !economy.production_reference_counts) && !independent) {throw std::runtime_error("Workshop allocation requires retained unit and production counts");}
                        const faction::SmacDesignCounts zero{};
                        auto prepared=unit::PrepareSmacWorkshopCommit(next,draft,catalog,*settings.GetSmacTechnologyRules(),*owner,
                            economy.design_counts.value_or(zero),economy.production_reference_counts.value_or(zero),overwrite==1);
                        result=prepared.result;
                        if(prepared.confirmation && !prepared.success) {
                            confirmation=overwrite<0;declined=overwrite==0;
                            const auto& old=next.native_registry->at(result.design_id);
                            overwrite_name=std::string(old.begin(),std::find(old.begin(),old.begin()+32,uint8_t(0)));
                            overwrite_count=economy.design_counts->at(result.design_id);
                        } else if(!prepared.success) {throw std::runtime_error("Workshop allocation failed");}
                        if(apply && prepared.retirement) {
                            const auto retired=*prepared.retirement;auto* manager=GetUM();std::vector<size_t> ids;
                            base_updates=GetBM()->m_base_updates;
                            // 595ac0 removes raw-design matches in descending physical order.
                            const auto& order=manager->GetUnitRecordOrder();
                            for(auto it=order.rbegin();it!=order.rend();++it) {
                                const auto* actor=manager->GetUnit(*it);
                                if(!actor || (!actor->IsConventional() && !actor->IsNative())) {throw std::runtime_error("Workshop retirement requires canonical Unit identities");}
                                const auto design=actor->IsConventional() ? actor->Conventional().design_id:actor->Native().design;
                                if(design==retired) {
                                    ids.push_back(*it);
                                    if(actor->IsConventional() && actor->Conventional().home_base_id) {
                                        auto* home=GetBM()->GetBase(*actor->Conventional().home_base_id);
                                        if(!home) {throw std::runtime_error("Retiring Unit has no registered home");}
                                        base::BaseManager::QueueBaseUpdateInto(*base_updates,home,base::BaseManager::BUO_REFRESH);
                                    }
                                }
                            }
                            // No Base-owner filter, no queue promotion, and no cache recount.
                            for(const auto& entry:GetBM()->GetBases()) {
                                auto* affected=entry.second;if(!affected->m_production) {continue;}
                                auto production=affected->m_production;
                                const auto matches=[&](const base::ProductionItem& item) {
                                    if(item.kind!=base::ProductionItemKind::Unit) {return false;}
                                    if(!item.smac_source_id) {throw std::runtime_error("Workshop retirement requires canonical queue identities");}
                                    return *item.smac_source_id==retired;
                                };
                                if(matches(production->current_item)) {production->current_item=MakeSmacProductionItem(affected->m_owner,base::ProductionItemKind::Stockpile,38);}
                                if(production->queue) {auto& pending=*production->queue;pending.erase(std::remove_if(pending.begin(),pending.end(),matches),pending.end());}
                                production->Validate();
                                if(!(*production==*affected->m_production)) {
                                    queues.emplace_back(affected,std::move(production));
                                    base::BaseManager::QueueBaseUpdateInto(*base_updates,affected,base::BaseManager::BUO_REFRESH);
                                }
                            }
                            removal.emplace(manager->PrepareWorldUnitRemoval(GSE_CALL,ids));
                        }
                        next=std::move(prepared.state);
                    }
                } else {result=unit::CommitSmacWorkshopFirstDesign(next,draft,settings.GetSmacPredefinedUnitRules()->catalog,*settings.GetSmacTechnologyRules(),*owner);}
                warning=result.intrinsic_name_warning;design_id=result.design_id;
            } catch(const std::invalid_argument&) {failed=true;}
            catch(const std::runtime_error&) {failed=true;}
            if(apply && !done && !confirmation && next.native_registry) {
                try {
                    // Apply always reselects with argument0, even after a failed
                    // edit or declined overwrite. Failed preparation never leaks
                    // a tentative record into the reselect source.
                    const auto& source=failed || declined ? m_simulation->value.designs:next;
                    auto selection=unit::PrepareSmacWorkshopReselection(source,design_id,supplied,settings.GetSmacPredefinedUnitRules()->catalog,owner->admission.simulation_id);
                    selection.draft.abilities=unit::CleanSmacWorkshopRetainedAbilities(selection.draft,settings.GetSmacPredefinedUnitRules()->catalog,
                        *settings.GetSmacTechnologyRules(),*owner).mask;
                    next=std::move(selection.state);design_id=selection.draft.selected_design;
                    response_draft=selection.draft;restored_armor=int32_t(selection.draft.armor);restored_name=std::move(selection.draft.name);reselected=true;
                } catch(const std::exception&) {failed=true;}
            }
            if(apply && !confirmation && ((!failed && !declined) || reselected)) {
                static_assert(std::is_nothrow_swappable_v<unit::SmacStartupDesignState> && std::is_nothrow_swappable_v<std::optional<base::ProductionState>>);
                if(!failed && !declined) {
                    if(removal) {removal->Commit();retired=true;}
                    for(auto& change:queues) {change.first->m_production.swap(change.second);}
                    if(base_updates) {GetBM()->m_base_updates.swap(*base_updates);}
                }
                std::swap(m_simulation->value.designs,next);
            }
        }
        if(apply) {
            if(retired) {removal->Notify(GSE_CALL);}
            GetState()->TriggerObject(this,"smac_workshop_commit",ARGS_F(this,base,done,stale,warning,design_id,failed,request,confirmation,declined,overwrite_count,overwrite_name,restored_armor,restored_name,reselected,response_draft) {
                {"base",base->Wrap(GSE_CALL)}, {"selection",base->CopySmacWorkshop(GSE_CALL,&response_draft)},
                {"done",VALUE(gse::value::Bool,,done && !stale && !failed && !confirmation && !declined)}, {"stale",VALUE(gse::value::Bool,,stale)},
                {"failed",VALUE(gse::value::Bool,,failed)},
                {"confirmation",VALUE(gse::value::Bool,,confirmation)}, {"declined",VALUE(gse::value::Bool,,declined)},
                {"overwrite_name",VALUE(gse::value::String,,overwrite_name)}, {"overwrite_count",VALUE(gse::value::Int,,overwrite_count)},
                {"restore_draft",VALUE(gse::value::Bool,,reselected)},
                {"restored_armor",VALUE(gse::value::Int,,restored_armor)}, {"restored_name",VALUE(gse::value::String,,restored_name)},
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
gse::Value* Base::CopySmacWorkshop(GSE_CALLABLE,const unit::SmacWorkshopDraft* supplied) const {
    const auto expected=SmacWorkshopState();
    const auto& settings=m_game->GetState()->m_settings.global;
    const auto& catalog=settings.GetSmacPredefinedUnitRules()->catalog;
    const auto& technology=*m_game->m_simulation->value.technology.Find(GetSmacResourceContext().owner_simulation_id);
    auto choices=unit::SmacWorkshopChoices(unit::SmacWorkshopComponent::Armor,catalog,*settings.GetSmacTechnologyRules(),technology);
    unit::SmacWorkshopDraft draft=supplied ? *supplied:unit::SmacWorkshopDraft{};
    if(draft.selected_design<0 || draft.selected_design>511 || (draft.selected_design>=64 && draft.selected_design/64!=technology.admission.simulation_id)
        || draft.chassis>=catalog.chassis.chassis.size() || draft.weapon>=catalog.weapons.weapons.size() || draft.armor>=catalog.defenses.defenses.size()
        || draft.reactor<1 || draft.reactor>catalog.reactors.reactors.size() || draft.abilities>0xffffff || draft.name.size()>31 || draft.name.find('\0')!=std::string::npos) {throw std::invalid_argument("Invalid Workshop preview draft");}
    if(catalog.chassis.chassis.at(draft.chassis).missile) {
        choices.erase(std::remove_if(choices.begin(),choices.end(),[](const auto& row){return row.component_id!=0;}),choices.end());
    }
    const auto mineral_factor=m_game->GetSmacProductionMineralFactor(m_owner);
    const auto& registry=m_game->m_simulation->value.designs;
    gse::Value* status=VALUE(gse::value::Undefined);
    if(registry.native_registry) {
        const auto& economy=m_game->m_simulation->value.economy.at(technology.admission.simulation_id);
        const auto result=unit::QuerySmacWorkshopStatus(*registry.native_registry,draft,technology.admission.simulation_id,
            economy.design_counts,economy.production_reference_counts);
        draft.selected_design=result.selected;
        status=VALUE(gse::value::Int,,int32_t(result.kind));
    }
    const auto component_choices=[&](unit::SmacWorkshopComponent kind) {
        gse::value::array_elements_t rows;
        for(const auto& choice:unit::SmacWorkshopChoices(kind,catalog,*settings.GetSmacTechnologyRules(),technology)) {
            const bool chassis=kind==unit::SmacWorkshopComponent::Chassis;
            const bool weapon=kind==unit::SmacWorkshopComponent::Weapon;
            const auto id=choice.component_id;
            const auto* equipment=weapon?&catalog.weapons.weapons.at(id):nullptr;
            int sprite=-1;
            if(weapon) {
                sprite=int8_t(uint8_t(equipment->icon));
                if(equipment->mode==loader::txt::WeaponMode::InfoWar) {
                    // Retained intrinsic Probe6 wins; V1 has only its checked
                    // predefined initializer, with no mutable raw registry.
                    sprite=int8_t(registry.native_registry ? registry.native_registry->at(6)[46]:catalog.units.at(6).icon);
                }
            }
            rows.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,{
                {"id",VALUE(gse::value::Int,,id)}, {"technology_available",VALUE(gse::value::Bool,,choice.technology_available)},
                {"name",VALUE(gse::value::String,,chassis ? catalog.chassis.chassis.at(id).names[0].name:weapon ? equipment->name:catalog.reactors.reactors.at(id-1).name)},
                {"value",VALUE(gse::value::Int,,chassis ? catalog.chassis.chassis.at(id).speed:weapon ? uint8_t(int(equipment->mode)<3?equipment->offense:equipment->cost):int32_t(id))},
                {"domain",VALUE(gse::value::Int,,chassis ? int(catalog.chassis.chassis.at(id).triad):-1)},
                {"power",VALUE(gse::value::Int,,weapon ? int8_t(uint8_t(equipment->offense)):0)},
                {"mode",VALUE(gse::value::Int,,weapon ? int(equipment->mode):0)},
                {"psi",VALUE(gse::value::Bool,,weapon && uint8_t(equipment->offense)==255)},
                {"sprite",VALUE(gse::value::Int,,sprite)}
            }));
        }return VALUE(gse::value::Array,,rows);
    };
    gse::Value* designs=VALUE(gse::value::Undefined);
    const auto& counts=m_game->m_simulation->value.economy.at(technology.admission.simulation_id).design_counts;
    if(registry.native_registry) {
        const auto first=64*technology.admission.simulation_id;bool ready=true;
        if(!counts) {for(int id=first;id<first+64;++id) {const auto& row=registry.native_registry->at(id);
            if((row[48]&1) && (row[44]&(1u<<technology.admission.simulation_id))) {ready=false;break;}}}
        if(ready) {
            gse::value::array_elements_t rows;
            for(const auto id:unit::BrowseSmacWorkshopDesigns(registry,catalog,*settings.GetSmacTechnologyRules(),technology,counts)) {
                const auto& raw=registry.native_registry->at(id);const bool active=(raw[48]&1)!=0;
                // Stored metadata is not the selector's editable draft or a
                // generated display caption. Inactive records supply neither.
                gse::Value* components=VALUE(gse::value::Undefined);
                if(active) {components=VALUE(gse::value::Object,,GSE_CALL_NOGC,{
                    {"chassis",VALUE(gse::value::Int,,raw[36])}, {"weapon",VALUE(gse::value::Int,,raw[37])},
                    {"armor",VALUE(gse::value::Int,,raw[38])}, {"reactor",VALUE(gse::value::Int,,raw[39])},
                    {"offense",VALUE(gse::value::Int,,catalog.weapons.weapons.at(raw[37]).offense)},
                    {"weapon_mode",VALUE(gse::value::Int,,int(catalog.weapons.weapons.at(raw[37]).mode))},
                    {"strength",VALUE(gse::value::Int,,catalog.defenses.defenses.at(raw[38]).rating)},
                    {"speed",VALUE(gse::value::Int,,catalog.chassis.chassis.at(raw[36]).speed)},
                    {"stat_line",VALUE(gse::value::String,,unit::FormatSmacWorkshopStats({id,raw[36],raw[37],raw[38],
                        uint32_t(raw[32])|(uint32_t(raw[33])<<8)|(uint32_t(raw[34])<<16)|(uint32_t(raw[35])<<24),raw[39],{}},catalog))}
                });}
                rows.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,{
                    {"id",VALUE(gse::value::Int,,id)}, {"active",VALUE(gse::value::Bool,,active)},
                    {"excluded",VALUE(gse::value::Bool,,(raw[44]&(1u<<technology.admission.simulation_id))!=0)},
                    {"flags",VALUE(gse::value::Int,,uint16_t(uint16_t(raw[48])|(uint16_t(raw[49])<<8)))},
                    {"stored_name",VALUE(gse::value::String,,active ? std::string(raw.begin(),std::find(raw.begin(),raw.begin()+32,uint8_t(0))):std::string())},
                    {"components",components},
                    {"unit_count",counts ? static_cast<gse::Value*>(VALUE(gse::value::Int,,counts->at(id))):VALUE(gse::value::Undefined)}
                }));
            }designs=VALUE(gse::value::Array,,rows);
        }
    }
    gse::value::array_elements_t armor;
    for(const auto& choice:choices) {
        bool supported=choice.technology_available;
        std::string design_name;
        const auto& chassis=catalog.chassis.chassis.at(draft.chassis);
        const auto& weapon=catalog.weapons.weapons.at(draft.weapon);
        const auto& defense=catalog.defenses.defenses.at(choice.component_id);
        const auto cost_rows=unit::SmacPredefinedUnitCatalog::CalculateCost(chassis,weapon,defense,draft.reactor,draft.abilities,catalog.abilities);
        const bool fast_land=chassis.triad==loader::txt::ChassisTriad::Land && uint8_t(chassis.speed)>1
            && int8_t(uint8_t(defense.rating))>1 && uint8_t(weapon.cost)>1;
        if(supported) {
            auto probe=m_game->m_simulation->value.designs;auto candidate=draft;candidate.armor=choice.component_id;
            try {
                unit::SmacWorkshopCommit result;
                if(probe.native_registry) {
                    if(const auto alias=unit::ReuseSmacWorkshopDesign(probe,candidate,catalog,*settings.GetSmacTechnologyRules(),technology)) {result=*alias;}
                    else {
                        unit::ValidateSmacWorkshopEditorAbilities(candidate,catalog,*settings.GetSmacTechnologyRules(),technology);
                        const auto& economy=m_game->m_simulation->value.economy.at(technology.admission.simulation_id);
                        bool independent=true;for(int id=64*technology.admission.simulation_id;id<64*(technology.admission.simulation_id+1);++id) {const auto& row=probe.native_registry->at(id);
                            independent=independent && !(row[48]&1) && !(row[44]&(1u<<technology.admission.simulation_id));}
                        if((!economy.design_counts || !economy.production_reference_counts) && !independent) {throw std::runtime_error("Workshop quote requires retained counts");}
                        const faction::SmacDesignCounts zero{};
                        auto prepared=unit::PrepareSmacWorkshopCommit(probe,candidate,catalog,*settings.GetSmacTechnologyRules(),technology,
                            economy.design_counts.value_or(zero),economy.production_reference_counts.value_or(zero));
                        result=prepared.result;
                        if(!prepared.success && !prepared.confirmation) {throw std::runtime_error("Workshop preview allocation failed");}
                        if(prepared.confirmation) {
                            // Quote the replacement without retiring it or
                            // consuming saved history; the live command owns consent.
                            auto pool=*probe.native_registry;pool.at(result.design_id)[48]&=uint8_t(~1u);
                            unit::BuildSmacWorkshopRecord(pool,size_t(result.design_id),candidate,catalog,*settings.GetSmacTechnologyRules(),technology);
                            design_name=unit::NameSmacWorkshopDesign(pool,size_t(result.design_id),candidate,catalog,technology.admission.simulation_id);
                        } else {probe=std::move(prepared.state);}
                    }
                } else {result=unit::CommitSmacWorkshopFirstDesign(probe,candidate,catalog,*settings.GetSmacTechnologyRules(),technology);}
                if(result.design_id<64) {design_name=catalog.source.units.at(result.design_id).name;}
                else if(design_name.empty()) {
                    const auto* bank=probe.Find(technology.admission.simulation_id);
                    if(!bank) {throw std::runtime_error("Workshop preview has no owned bank");}
                    const auto design=std::find_if(bank->designs.begin(),bank->designs.end(),[&](const auto& row){return row.id==result.design_id;});
                    if(design==bank->designs.end()) {throw std::runtime_error("Workshop preview has no retained design name");}
                    design_name=unit::SmacWorkshopDesignName(*design,catalog,technology.admission.simulation_id);
                }
            }
            catch(const std::exception&) {supported=false;}
        }
        // 43f8c0 recomputes draft rows; a reused record's stored cost byte is
        // not the main COST source. The UI caches its factor at modal open.
        const int64_t shown_cost=int64_t(std::max(cost_rows,int32_t(draft.reactor)))*mineral_factor;
        if(shown_cost<0 || shown_cost>INT32_MAX) {throw std::overflow_error("Workshop displayed cost exceeds signed32");}
        auto displayed=draft;displayed.armor=choice.component_id;
        armor.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,{
            {"id",VALUE(gse::value::Int,,choice.component_id)},
            {"name",VALUE(gse::value::String,,catalog.defenses.defenses.at(choice.component_id).name)},
            {"short_name",VALUE(gse::value::String,,catalog.defenses.defenses.at(choice.component_id).short_name)},
            {"rating",VALUE(gse::value::Int,,catalog.defenses.defenses.at(choice.component_id).rating)},
            {"armor_class",VALUE(gse::value::Int,,int32_t(catalog.defenses.defenses.at(choice.component_id).mode))},
            {"stat_line",VALUE(gse::value::String,,unit::FormatSmacWorkshopStats(displayed,catalog,true))},
            {"design_name",VALUE(gse::value::String,,design_name)},
            {"cost",VALUE(gse::value::Int,,shown_cost)},
            {"cost_rows",VALUE(gse::value::Int,,cost_rows)},
            {"fast_land_warning",VALUE(gse::value::Bool,,fast_land)},
            {"technology_available",VALUE(gse::value::Bool,,choice.technology_available)},
            {"available",VALUE(gse::value::Bool,,supported)}
        }));
    }
    gse::value::array_elements_t ability_names;
    for(const auto& row:catalog.abilities.abilities) {ability_names.push_back(VALUE(gse::value::String,,row.name));}
    return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
        {"expected_state",VALUE(gse::value::String,,expected)}, {"armor",VALUE(gse::value::Array,,armor)},
        {"mineral_factor",VALUE(gse::value::Int,,mineral_factor)},
        {"movement",VALUE(gse::value::Int,,unit::SmacWorkshopMovement(draft,catalog))},
        {"weapon_cost",VALUE(gse::value::Int,,uint8_t(catalog.weapons.weapons.at(draft.weapon).cost))},
        {"chassis_choices",component_choices(unit::SmacWorkshopComponent::Chassis)},
        {"weapon_choices",component_choices(unit::SmacWorkshopComponent::Weapon)},
        {"reactor_choices",component_choices(unit::SmacWorkshopComponent::Reactor)},
        {"ability_names",VALUE(gse::value::Array,,ability_names)},
        {"designs",designs}, {"status",status},
        {"draft",VALUE(gse::value::Object,,GSE_CALL_NOGC,{
            {"selected_design",VALUE(gse::value::Int,,draft.selected_design)}, {"chassis",VALUE(gse::value::Int,,draft.chassis)},
            {"weapon",VALUE(gse::value::Int,,draft.weapon)}, {"armor",VALUE(gse::value::Int,,draft.armor)},
            {"reactor",VALUE(gse::value::Int,,draft.reactor)}, {"abilities",VALUE(gse::value::Int,,draft.abilities)}, {"name",VALUE(gse::value::String,,draft.name)}
        })},
        {"chassis",VALUE(gse::value::String,,catalog.chassis.chassis.at(draft.chassis).names[0].name)},
        {"speed",VALUE(gse::value::Int,,catalog.chassis.chassis.at(draft.chassis).speed)},
        {"weapon",VALUE(gse::value::String,,catalog.weapons.weapons.at(draft.weapon).name)},
        {"offense",VALUE(gse::value::Int,,catalog.weapons.weapons.at(draft.weapon).offense)},
        {"weapon_mode",VALUE(gse::value::Int,,int32_t(catalog.weapons.weapons.at(draft.weapon).mode))},
        {"reactor",VALUE(gse::value::String,,catalog.reactors.reactors.at(draft.reactor-1).name)},
        {"power",VALUE(gse::value::Int,,catalog.reactors.reactors.at(draft.reactor-1).power)},
        {"scout_name",VALUE(gse::value::String,,catalog.source.units.at(2).name)},
        {"faction_art",VALUE(gse::value::String,,m_faction->m_bases_render.file)}
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
