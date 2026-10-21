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
#include "game/backend/unit/SmacProductionDesigns.h"
#include "game/backend/unit/SmacWorkshopUpgrade.h"
#include "game/backend/unit/SmacConventionalStats.h"
#include "game/backend/State.h"
#include "game/backend/faction/Faction.h"
#include "gc/Space.h"
#include "util/SHA256.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Int.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include <type_traits>
#include <algorithm>

namespace game::backend {

namespace {
// A real Base caller keeps its production guard; global entry carries the
// actual local Player. This view never constructs or registers a Base.
struct WorkshopTarget {
    slot::Slot* slot;
    gse::Wrappable* subject;
    base::Base* caller_base=nullptr;
};
WorkshopTarget ResolveWorkshopTarget(Game* game,gse::Value* value) {
    auto* object=static_cast<gse::value::Object*>(value->Deref());
    if(object->object_class==base::Base::WRAP_CLASS) {
        auto* base=base::Base::Unwrap(value);
        if(!base || base->GetGame()!=game) {THROW("Workshop Base belongs to another game");}
        return {base->m_owner,base,base};
    }
    if(object->object_class==Player::WRAP_CLASS) {
        auto* player=Player::Unwrap(value);
        if(!player || player!=game->GetPlayer()) {THROW("Workshop requires the actual local Player");}
        game->RequireSmacWorkshopOwner(player->GetSlot());
        return {player->GetSlot(),player};
    }
    THROW("Workshop requires an actual Base or local Player");
}

// Echoed opaque fence: no new opponent capability/strategy data in the UI.
std::string WorkshopPresetState(const std::string& design_state,const faction::SmacSimulationState& state,const settings::GlobalSettings& settings,int32_t owner) {
    types::Buffer wire;wire.WriteString(design_state);wire.WriteInt(owner);
    wire.WriteBool(settings.GetSmacCombatRules().has_value());
    if(settings.GetSmacCombatRules()) {wire.WriteString(settings.GetSmacCombatRules()->Serialize().ToString());}
    wire.WriteBool(settings.GetSmacCampaignRules() && (settings.GetSmacCampaignRules()->primary&0x00400000u)!=0);
    wire.WriteInt(static_cast<int32_t>(state.movement_capability));wire.WriteInt(state.human_membership);
    wire.WriteBool(state.capabilities.has_value());if(state.capabilities) {for(const auto& row:state.capabilities->factions) {
        for(const auto& values:{row.available,row.foreign}) {for(const auto value:values) {wire.WriteInt(value);}}
    }}
    wire.WriteBool(state.rank_order.has_value());if(state.rank_order) {for(const auto id:state.rank_order->faction_at_rank) {wire.WriteInt(id);}}
    wire.WriteBool(state.retained_base_totals.has_value());if(state.retained_base_totals) {wire.WriteInt(state.retained_base_totals->at(owner));}
    wire.WriteBool(state.strategy.has_value());if(state.strategy) {
        const auto& policy=state.strategy->at(owner);for(const auto value:policy.priorities) {wire.WriteInt(value);}
        wire.WriteBool(policy.fight.has_value());if(policy.fight) {wire.WriteInt(*policy.fight);}
    }
    const auto* social=state.social.Find(owner);wire.WriteBool(social!=nullptr);
    if(social) {wire.WriteInt(social->current.at(9));wire.WriteInt(static_cast<int32_t>(social->phase));}
    const auto& air=state.air_summary.at(owner);wire.WriteBool(air.has_value());
    if(air) {wire.WriteInt(air->foreign_threats);wire.WriteInt(air->own_plan0);wire.WriteInt(air->own_plan4);}
    for(const auto& direction:state.hostility.owners.at(owner).directions) {wire.WriteInt(direction.primary);}
    return util::SHA256(wire.ToString());
}
unit::SmacWorkshopHumanContext WorkshopHumanContext(const faction::SmacSimulationState& state,int32_t owner) {
    const auto* social=state.social.Find(owner);
    if(!state.IsHuman(owner) || !state.capabilities || !state.rank_order || !state.retained_base_totals
        || !state.strategy || !state.strategy->at(owner).fight || !state.air_summary.at(owner) || !social
        || social->phase!=faction::SmacFactionSocialState::Phase::Refreshed) {
        throw std::invalid_argument("Workshop recommendations require current retained faction authority");
    }
    int32_t rank=-1;for(size_t i=1;i<8;++i) {if(state.rank_order->faction_at_rank[i]==owner) {rank=static_cast<int32_t>(i);break;}}
    if(rank<0) {throw std::invalid_argument("Workshop owner has no retained rank position");}
    const auto& policy=state.strategy->at(owner);const auto& cap=state.capabilities->factions.at(owner-1);const auto& air=*state.air_summary.at(owner);
    // The admitted faction keyword domain has no direct MORALE scalar writer;
    // its native definition constructor supplies0. SOCIAL MORALE is separate.
    return {{{0x95d1ac,rank},{0x95d584,*policy.fight},{0x95d588,policy.priorities[2]},
        {0x95d58c,policy.priorities[3]},{0x95d590,policy.priorities[0]},{0x95d594,policy.priorities[1]},
        {0x95e200,state.retained_base_totals->at(owner)},{0x95d9d8,cap.foreign.at(faction::SmacCapabilityState::LandSpeed)},
        {0x95d9d0,cap.foreign.at(faction::SmacCapabilityState::Weapon)},{0x95d9c8,cap.available.at(faction::SmacCapabilityState::Armor)},
        {0x95d418,social->current.at(9)},{0x95d9ec,air.foreign_threats},{0x95f22c,air.own_plan0},{0x95f230,air.own_plan4}},0};
}
}

bool Game::HandleSmacProductionEvent( GSE_CALLABLE, const std::string& name,
    const gse::value::object_properties_t& args, size_t caller, bool apply ) {
    using WorkshopBaseUpdates=decltype(GetBM()->m_base_updates);
    const auto stage_retirement=[&](int32_t retired,bool custom,
        std::optional<unit::UnitManager::PreparedConventionalRemoval>& removal,
        std::vector<std::pair<base::Base*,std::optional<base::ProductionState>>>& queues,
        std::optional<WorkshopBaseUpdates>& base_updates) {
        auto* manager=GetUM();std::vector<size_t> ids;base_updates=GetBM()->m_base_updates;
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
        // Intrinsic retirement leaves every Base queue unchanged.
        // Custom retirement has no Base-owner filter or cache recount.
        if(custom) for(const auto& entry:GetBM()->GetBases()) {
            auto* affected=entry.second;if(!affected->m_production) {continue;}
            auto production=affected->m_production;
            const auto matches=[&](const base::ProductionItem& item) {
                if(item.kind!=base::ProductionItemKind::Unit) {return false;}
                if(!item.smac_source_id) {throw std::runtime_error("Workshop retirement requires canonical queue identities");}
                return *item.smac_source_id==retired;
            };
            if(matches(production->current_item)) {production->current_item=MakeSmacProductionItem(affected->m_owner,base::ProductionItemKind::Stockpile,38);}
            if(production->queue) {
                auto& pending=*production->queue;
                // Original595ac0 advances even after shifting a removed
                // entry. Keep the newly shifted neighbor when adjacent
                // entries name the same retired design.
                for(size_t i=0;i<pending.size();++i) {
                    if(matches(pending[i])) {pending.erase(pending.begin()+i);}
                }
            }
            production->Validate();
            if(!(*production==*affected->m_production)) {
                queues.emplace_back(affected,std::move(production));
                base::BaseManager::QueueBaseUpdateInto(*base_updates,affected,base::BaseManager::BUO_REFRESH);
            }
        }
        removal.emplace(manager->PrepareWorldUnitRemoval(GSE_CALL,ids));
    };
    if(name=="open_smac_workshop") {
        const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
            const auto at=args.find(key);if(at==args.end() || !at->second || at->second->Deref()->type!=type) {THROW("Malformed Workshop opening");}
            return at->second->Deref();
        };
        if(args.size()!=4) {THROW("Workshop opening requires Base, selection, request and fence");}
        (void)field("base",gse::VT_OBJECT);
        const auto workshop=ResolveWorkshopTarget(this,args.at("base"));
        if(workshop.caller_base) {workshop.caller_base->RequireSmacProductionSelection();}
        const auto selected=static_cast<const gse::value::Int*>(field("selected_design",gse::VT_INT))->value;
        const auto request=static_cast<const gse::value::Int*>(field("request_id",gse::VT_INT))->value;
        const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
        if(caller!=m_slot_num || selected< -1 || selected>=512 || selected==63 || request<1 || expected.size()!=64) {THROW("Invalid Workshop opening authority");}
        const auto owner=RequireSmacWorkshopOwner(workshop.slot);
        if(selected>=64 && selected/64!=owner) {THROW("Workshop opening selected a foreign bank");}
        if(apply) {CheckRW(GSE_CALL);if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Workshop opening requires its owning accumulation");}}
        const auto& settings=GetState()->m_settings.global;
        const bool stale=WorkshopPresetState(SmacWorkshopState(workshop.slot),m_simulation->value,settings,owner)!=expected;
        bool failed=false;unit::SmacWorkshopDraft draft;
        if(!stale) {try {
            if(!settings.GetSmacCombatRules() || (!settings.GetSmacCampaignRules() && !settings.IsOrdinarySmacCampaign())) {throw std::invalid_argument("Workshop opening requires retained campaign rules");}
            const auto& catalog=settings.GetSmacPredefinedUnitRules()->catalog;auto next=m_simulation->value;
            const auto refresh=[&] {next.RefreshCapabilities(owner,*settings.GetSmacTechnologyRules(),*settings.GetSmacPredefinedUnitRules(),*settings.GetSmacCombatRules(),
                settings.GetSmacCampaignRules() && (settings.GetSmacCampaignRules()->primary&0x00400000u)!=0);};
            refresh(); //4496fe, before mineral-factor capture and selector entry
            (void)GetSmacProductionMineralFactor(workshop.slot);
            auto entry=unit::PrepareSmacWorkshopEntry(next.designs,int32_t(selected),catalog,*settings.GetSmacTechnologyRules(),*next.technology.Find(owner));
            draft=entry.draft;next.designs=std::move(entry.state);
            if(!(next.designs.native_registry->at(draft.selected_design)[48]&1)) {
                const auto chosen=draft.selected_design;const auto reactor=draft.reactor;
                refresh(); //597ede, only the initializeFresh inactive branch
                const auto context=WorkshopHumanContext(next,owner);const auto& economy=next.economy.at(owner);
                if(!economy.design_counts || !economy.production_reference_counts) {throw std::invalid_argument("Workshop opening requires retained allocation counts");}
                auto result=unit::PrepareSmacWorkshopHumanPreview(next.designs,draft,catalog,*settings.GetSmacTechnologyRules(),*next.technology.Find(owner),
                    context,*economy.design_counts,*economy.production_reference_counts);
                draft=result.draft;draft.selected_design=chosen;draft.reactor=reactor;draft.name.clear();
                auto pool=*result.state.native_registry;pool.at(chosen)[48]&=uint8_t(~0x80u);result.state.RetainNativeRegistry(pool,catalog);
                next.designs=std::move(result.state);
            }
            if(apply) {static_assert(std::is_nothrow_swappable_v<unit::SmacStartupDesignState> && std::is_nothrow_swappable_v<decltype(next.capabilities)>);
                std::swap(m_simulation->value.designs,next.designs);m_simulation->value.capabilities.swap(next.capabilities);}
        }catch(const std::invalid_argument&) {failed=true;}catch(const std::runtime_error&) {failed=true;}}
        if(apply) {GetState()->TriggerObject(this,"smac_workshop_open",_ARGS_F(this,workshop,request,stale,failed,draft) {
            out_args={{"base",workshop.subject->Wrap(GSE_CALL)},{"request_id",VALUE(gse::value::Int,,request)},
                {"stale",VALUE(gse::value::Bool,,stale)},{"failed",VALUE(gse::value::Bool,,failed)},
                {"selection",!stale && !failed ? CopySmacWorkshop(GSE_CALL,workshop.slot,&draft):VALUE(gse::value::Undefined)}};
        });}return true;
    }
    if(name=="preset_smac_workshop") {
        const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
            const auto at=args.find(key);if(at==args.end() || !at->second || at->second->Deref()->type!=type) {THROW("Malformed Workshop preset command");}
            return at->second->Deref();
        };
        if(args.size()!=11) {THROW("Workshop preset requires its complete current draft, action and fence");}
        const auto integer=[&](const char* key){return static_cast<const gse::value::Int*>(field(key,gse::VT_INT))->value;};
        const auto text=[&](const char* key)->const std::string& {return static_cast<const gse::value::String*>(field(key,gse::VT_STRING))->value;};
        (void)field("base",gse::VT_OBJECT);
        const auto workshop=ResolveWorkshopTarget(this,args.at("base"));
        if(workshop.caller_base) {workshop.caller_base->RequireSmacProductionSelection();}
        const auto selected=integer("selected_design"),chassis=integer("chassis"),weapon=integer("weapon"),armor=integer("armor"),reactor=integer("reactor"),abilities=integer("abilities");
        const auto action=integer("action"),request=integer("request_id");const auto& expected=text("expected_state");const auto& retained=text("name");
        if(caller!=m_slot_num || action<9 || action>22 || request<1 || selected<0 || selected>511 || selected==63
            || chassis<0 || chassis>8 || weapon<0 || weapon>20 || armor<0 || armor>9 || reactor<1 || reactor>4 || abilities<0 || abilities>0xffffff
            || expected.size()!=64 || retained.size()>31 || retained.find('\0')!=std::string::npos) {THROW("Invalid Workshop preset authority or draft");}
        for(const unsigned char c:retained) {if(c<32 || c>126) {THROW("Workshop preset name requires printable display bytes");}}
        const auto owner=RequireSmacWorkshopOwner(workshop.slot);
        if(selected>=64 && selected/64!=owner) {THROW("Workshop preset selected a foreign bank");}
        if(apply) {CheckRW(GSE_CALL);if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Workshop preset requires its owning accumulation");}}
        const bool stale=WorkshopPresetState(SmacWorkshopState(workshop.slot),m_simulation->value,GetState()->m_settings.global,owner)!=expected;
        bool failed=false;int32_t mode=1;
        unit::SmacWorkshopDraft draft{int32_t(selected),uint32_t(chassis),uint32_t(weapon),uint32_t(armor),uint32_t(abilities),uint8_t(reactor),retained};
        if(!stale) {try {
            const auto& settings=GetState()->m_settings.global;
            if(!settings.GetSmacCombatRules() || (!settings.GetSmacCampaignRules() && !settings.IsOrdinarySmacCampaign())) {throw std::invalid_argument("Workshop preset requires retained combat and campaign rules");}
            auto next=m_simulation->value;
            // Original597ede runs even if this role has no admitted proposal.
            next.RefreshCapabilities(owner,*settings.GetSmacTechnologyRules(),*settings.GetSmacPredefinedUnitRules(),*settings.GetSmacCombatRules(),
                settings.GetSmacCampaignRules() && (settings.GetSmacCampaignRules()->primary&0x00400000u)!=0);
            const auto context=WorkshopHumanContext(next,owner);const auto& economy=next.economy.at(owner);
            if(!economy.design_counts || !economy.production_reference_counts) {throw std::invalid_argument("Workshop preset requires retained allocation counts");}
            constexpr std::array<int32_t,14> filters{{2,3,1,4,10,6,7,8,9,17,13,14,11,12}};
            auto result=unit::PrepareSmacWorkshopHumanPreview(next.designs,draft,settings.GetSmacPredefinedUnitRules()->catalog,
                *settings.GetSmacTechnologyRules(),*next.technology.Find(owner),context,*economy.design_counts,*economy.production_reference_counts,filters.at(size_t(action-9)));
            draft=result.draft;mode=result.mode;
            if(apply) {
                static_assert(std::is_nothrow_swappable_v<unit::SmacStartupDesignState> && std::is_nothrow_swappable_v<decltype(next.capabilities)>);
                std::swap(m_simulation->value.designs,result.state);m_simulation->value.capabilities.swap(next.capabilities);
            }
        }catch(const std::invalid_argument&) {failed=true;}catch(const std::runtime_error&) {failed=true;}}
        if(apply) {GetState()->TriggerObject(this,"smac_workshop_preset",_ARGS_F(this,workshop,request,stale,failed,mode,draft) {
            out_args={
                {"base",workshop.subject->Wrap(GSE_CALL)},{"request_id",VALUE(gse::value::Int,,request)},
                {"stale",VALUE(gse::value::Bool,,stale)},{"failed",VALUE(gse::value::Bool,,failed)},
                {"matched",VALUE(gse::value::Bool,,!stale && !failed && mode>1)},
                {"feedback",VALUE(gse::value::Int,,!stale && !failed ? (mode>1 ? 43:39):0)},
                {"selection",!stale && !failed ? CopySmacWorkshop(GSE_CALL,workshop.slot,&draft):VALUE(gse::value::Undefined)}
            };
        });}return true;
    }
    if(name=="upgrade_smac_workshop") {
        const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
            const auto at=args.find(key);if(at==args.end() || !at->second || at->second->Deref()->type!=type) {THROW("Malformed Workshop Upgrade command");}
            return at->second->Deref();
        };
        if(args.size()!=8) {THROW("Workshop Upgrade requires source, target, retained name, answer and fence");}
        const auto integer=[&](const char* key){return static_cast<const gse::value::Int*>(field(key,gse::VT_INT))->value;};
        (void)field("base",gse::VT_OBJECT);
        const auto workshop=ResolveWorkshopTarget(this,args.at("base"));
        if(workshop.caller_base) {workshop.caller_base->RequireSmacProductionSelection();}const auto source=integer("selected_design"),requested=integer("target_design"),answer=integer("confirm"),request=integer("request_id"),offset=integer("strip_offset");
        const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
        const auto& retained=static_cast<const gse::value::String*>(field("name",gse::VT_STRING))->value;
        if(caller!=m_slot_num || source<0 || source>511 || requested< -1 || requested>511 || answer< -1 || answer>1 || request<1 || offset<0 || offset>512
            || expected.size()>unit::SmacStartupDesignState::MAX_WIRE_BYTES+89+2*513 || retained.size()>31 || retained.find('\0')!=std::string::npos) {THROW("Invalid Workshop Upgrade authority or selection");}
        const auto owner=RequireSmacWorkshopOwner(workshop.slot);
        if((source>=64 && source/64!=owner) || (requested>=0 && (requested<64 || requested/64!=owner))) {THROW("Workshop Upgrade selected a foreign bank or intrinsic target");}
        if(apply) {CheckRW(GSE_CALL);if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Workshop Upgrade requires its owning accumulation");}}
        const bool stale=SmacWorkshopState(workshop.slot)!=expected;bool failed=false,upgraded=false,confirmation=false,affordable=true,unprototyped=false;
        int32_t count=0,total=0,available=0,target=int32_t(requested);size_t strip_offset=size_t(offset);std::string source_name,target_name,illustration;
        struct DesignText {std::string name,stats,tokens;int32_t domain=0,reactor=1;uint32_t abilities=0;};DesignText source_text,target_text;
        struct Choice {int32_t id;std::string name,stats;bool prototype;};std::vector<Choice> choices;
        unit::SmacWorkshopDraft response_draft;
        if(!stale) {try {
            const auto& settings=GetState()->m_settings.global;const auto& catalog=settings.GetSmacPredefinedUnitRules()->catalog;
            const auto& state=m_simulation->value.designs;auto& economy=m_simulation->value.economy.at(owner);
            // This saved campaign capability establishes absence of all project
            // hosts. Do not infer a missing Nano Factory cache as an unowned one.
            if(!settings.IsOrdinarySmacCampaign() || m_simulation->value.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain
                || !state.native_registry || !economy.design_counts) {throw std::runtime_error("Workshop Upgrade requires retained registry/count and project authority");}
            const auto& pool=*state.native_registry;const auto& old=pool.at(source);
            const auto name_of=[](const auto& row){return std::string(row.begin(),std::find(row.begin(),row.begin()+32,uint8_t(0)));};
            const auto text_of=[&](int32_t id){const auto& row=pool.at(id);const uint32_t mask=uint32_t(row[32])|(uint32_t(row[33])<<8)|(uint32_t(row[34])<<16)|(uint32_t(row[35])<<24);
                std::string tokens;for(size_t bit=0;bit<24;++bit) {if(mask&(1u<<bit)) {tokens+=catalog.abilities.abilities.at(bit).abbreviation;}}
                return DesignText{name_of(row),unit::FormatSmacWorkshopStats({id,row[36],row[37],row[38],mask,row[39],{}},catalog),tokens,int32_t(catalog.chassis.chassis.at(row[36]).triad),row[39],mask};};
            source_text=text_of(int32_t(source));
            const auto triad=catalog.chassis.chassis.at(old[36]).triad;
            illustration=triad==loader::txt::ChassisTriad::Land ? "unit_sm.pcx":triad==loader::txt::ChassisTriad::Sea ? "navun_sm.pcx":"air_sm.pcx";
            source_name=name_of(old);count=economy.design_counts->at(source);available=economy.credits;
            const auto candidates=unit::SmacWorkshopUpgradeCandidates(pool,owner,int32_t(source),catalog);
            for(const auto id:candidates) {const auto& row=pool.at(id);
                choices.push_back({id,name_of(row),unit::FormatSmacWorkshopStats({id,row[36],row[37],row[38],
                    uint32_t(row[32])|(uint32_t(row[33])<<8)|(uint32_t(row[34])<<16)|(uint32_t(row[35])<<24),row[39],{}},catalog),bool((old[48]&4) && !(row[48]&4))});
            }
            if(target<0 && candidates.size()==1) {target=candidates.front();}
            if(target>=0) {
                if(std::find(candidates.begin(),candidates.end(),target)==candidates.end()) {throw std::runtime_error("Workshop Upgrade target is no longer eligible");}
                target_name=name_of(pool.at(target));target_text=text_of(target);unprototyped=!(pool.at(target)[48]&4);
                auto prepared=unit::PrepareSmacWorkshopUpgrade(state,owner,target,int32_t(source),catalog,uint8_t(count),false);
                total=prepared.total_energy;affordable=total==0 || available>=total;
                confirmation=answer<0 && total>0 && affordable;
                if(answer!=0 && affordable && !confirmation) {
                    auto selection=unit::PrepareSmacWorkshopReselection(prepared.state,target,retained,catalog,owner);
                    const auto& technology=*m_simulation->value.technology.Find(owner);
                    selection.draft.abilities=unit::CleanSmacWorkshopRetainedAbilities(selection.draft,catalog,*settings.GetSmacTechnologyRules(),technology).mask;
                    response_draft=selection.draft;
                    // The browse order and caption are prepared before any live
                    // mutation. Count deltas can affect excluded ordering only;
                    // the selected included target is still recentered below.
                    auto counts=*economy.design_counts;std::vector<std::pair<size_t,unit::ConventionalUnitState>> changes;
                    for(const auto id:GetUM()->GetUnitRecordOrder()) {
                        const auto* actor=GetUM()->GetUnit(id);if(!actor) {throw std::runtime_error("Workshop Upgrade has a missing Unit identity");}
                        if(actor->IsNative()) {if(actor->Native().owner==owner && actor->Native().design==source) {throw std::runtime_error("Native Unit Upgrade conversion is not bound");}continue;}
                        if(!actor->IsConventional()) {throw std::runtime_error("Workshop Upgrade requires canonical Unit identities");}
                        const auto& before=actor->Conventional();if(before.owner_simulation_id!=owner || before.design_id!=source) {continue;}
                        auto next=before;next.design_id=target;next.stored_morale=uint8_t(std::clamp(int(before.stored_morale)+prepared.morale_delta,0,6));
                        changes.emplace_back(id,next);counts.at(source)=uint8_t(int(counts.at(source))-1);counts.at(target)=uint8_t(int(counts.at(target))+1);
                    }
                    const auto list=unit::BrowseSmacWorkshopDesigns(selection.state,catalog,*settings.GetSmacTechnologyRules(),technology,counts);
                    const auto at=std::find(list.begin(),list.end(),target);if(at!=list.end()) {strip_offset=(size_t(at-list.begin())+list.size()-5)%list.size();}
                    if(apply) {
                        std::vector<std::pair<base::Base*,std::optional<base::ProductionState>>> queues;auto updates=GetBM()->m_base_updates;
                        const auto replacement=MakeSmacProductionItem(workshop.slot,base::ProductionItemKind::Unit,target);
                        for(const auto& entry:GetBM()->GetBases()) {
                            auto* affected=entry.second;if(affected->m_owner!=workshop.slot || !affected->m_production || !affected->m_production->queue) {continue;}
                            auto production=affected->m_production;
                            const auto convert=[&](base::ProductionItem& item) {
                                if(item.kind==base::ProductionItemKind::Unit) {if(!item.smac_source_id) {throw std::runtime_error("Workshop Upgrade requires canonical queue identities");}
                                    if(*item.smac_source_id==source) {item=replacement;}}
                            };
                            convert(production->current_item);for(auto& item:*production->queue) {convert(item);}production->Validate();
                            if(!(*production==*affected->m_production)) {queues.emplace_back(affected,std::move(production));base::BaseManager::QueueBaseUpdateInto(updates,affected,base::BaseManager::BUO_REFRESH);}
                        }
                        for(const auto& change:changes) {if(change.second.home_base_id) {auto* home=GetBM()->GetBase(*change.second.home_base_id);
                            if(!home) {throw std::runtime_error("Upgrading Unit has no registered home");}base::BaseManager::QueueBaseUpdateInto(updates,home,base::BaseManager::BUO_REFRESH);}}
                        auto units=GetUM()->PrepareDefectionUnits(GSE_CALL,workshop.slot,changes);
                        static_assert(std::is_nothrow_swappable_v<unit::SmacStartupDesignState> && std::is_nothrow_swappable_v<std::optional<base::ProductionState>>);
                        // All allocations/resolution precede these nonthrowing
                        // publications. Unit publication owns byte count deltas.
                        units.Commit();for(auto& change:queues) {change.first->m_production.swap(change.second);}
                        GetBM()->m_base_updates.swap(updates);std::swap(m_simulation->value.designs,selection.state);economy.credits-=total;upgraded=true;
                    }
                }
            }
        }catch(const std::invalid_argument&) {failed=true;}catch(const std::runtime_error&) {failed=true;}}
        if(apply) {
            GetState()->TriggerObject(this,"smac_workshop_upgrade",_ARGS_F(this,workshop,request,answer,stale,failed,upgraded,confirmation,affordable,unprototyped,count,total,available,target,source_name,target_name,source_text,target_text,illustration,choices,strip_offset,response_draft) {
                gse::value::array_elements_t rows;for(const auto& choice:choices) {rows.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,{
                    {"id",VALUE(gse::value::Int,,choice.id)}, {"stored_name",VALUE(gse::value::String,,choice.name)}, {"stat_line",VALUE(gse::value::String,,choice.stats)}, {"prototype",VALUE(gse::value::Bool,,choice.prototype)}
                }));}
                const auto wrap_text=[&](const DesignText& text){return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
                    {"name",VALUE(gse::value::String,,text.name)}, {"stats",VALUE(gse::value::String,,text.stats)}, {"ability_tokens",VALUE(gse::value::String,,text.tokens)},
                    {"domain",VALUE(gse::value::Int,,text.domain)}, {"reactor",VALUE(gse::value::Int,,text.reactor)}, {"abilities",VALUE(gse::value::Int,,text.abilities)}
                });};
                out_args={
                    {"base",workshop.subject->Wrap(GSE_CALL)}, {"request_id",VALUE(gse::value::Int,,request)}, {"stale",VALUE(gse::value::Bool,,stale)}, {"failed",VALUE(gse::value::Bool,,failed)},
                    {"upgraded",VALUE(gse::value::Bool,,upgraded)}, {"confirmation",VALUE(gse::value::Bool,,confirmation && !failed)}, {"affordable",VALUE(gse::value::Bool,,affordable)},
                    {"declined",VALUE(gse::value::Bool,,answer==0 && !stale && !failed)}, {"unprototyped",VALUE(gse::value::Bool,,unprototyped)}, {"count",VALUE(gse::value::Int,,count)},
                    {"total",VALUE(gse::value::Int,,total)}, {"available",VALUE(gse::value::Int,,available)}, {"target_design",VALUE(gse::value::Int,,target)},
                    {"source_text",wrap_text(source_text)}, {"target_text",wrap_text(target_text)}, {"illustration",VALUE(gse::value::String,,illustration)},
                    {"source_name",VALUE(gse::value::String,,source_name)}, {"target_name",VALUE(gse::value::String,,target_name)}, {"choices",VALUE(gse::value::Array,,rows)},
                    {"zero_count_notice",VALUE(gse::value::Bool,,upgraded && count==0)}, {"expected_state",VALUE(gse::value::String,,SmacWorkshopState(workshop.slot))},
                    {"strip_offset",VALUE(gse::value::Int,,strip_offset)}, {"selection",upgraded ? CopySmacWorkshop(GSE_CALL,workshop.slot,&response_draft):VALUE(gse::value::Undefined)}
                };
            });
        }return true;
    }
    if(name=="retire_smac_workshop") {
        const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
            const auto at=args.find(key);if(at==args.end() || !at->second || at->second->Deref()->type!=type) {THROW("Malformed Workshop Retire command");}
            return at->second->Deref();
        };
        if(args.size()!=7) {THROW("Workshop Retire requires selection, retained name, answer and fence");}
        const auto integer=[&](const char* key){return static_cast<const gse::value::Int*>(field(key,gse::VT_INT))->value;};
        (void)field("base",gse::VT_OBJECT);
        const auto workshop=ResolveWorkshopTarget(this,args.at("base"));
        if(workshop.caller_base) {workshop.caller_base->RequireSmacProductionSelection();}const auto selected=integer("selected_design"),answer=integer("confirm"),request=integer("request_id"),offset=integer("strip_offset");
        const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
        const auto& retained=static_cast<const gse::value::String*>(field("name",gse::VT_STRING))->value;
        if(caller!=m_slot_num || selected<0 || selected>511 || answer< -1 || answer>1 || request<1 || offset<0 || offset>512
            || expected.size()>unit::SmacStartupDesignState::MAX_WIRE_BYTES+89+2*513 || retained.size()>31 || retained.find('\0')!=std::string::npos) {THROW("Invalid Workshop Retire authority or selection");}
        const auto owner=RequireSmacWorkshopOwner(workshop.slot);
        if(selected>=64 && selected/64!=owner) {THROW("Workshop Retire selected a foreign bank");}
        if(apply) {CheckRW(GSE_CALL);if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Workshop Retire requires its owning accumulation");}}
        const bool stale=SmacWorkshopState(workshop.slot)!=expected;bool failed=false,retired=false;int32_t count=0;size_t strip_offset=size_t(offset);std::string stored_name,illustration;
        unit::SmacWorkshopDraft response_draft;
        std::optional<unit::UnitManager::PreparedConventionalRemoval> removal;
        std::vector<std::pair<base::Base*,std::optional<base::ProductionState>>> queues;std::optional<WorkshopBaseUpdates> base_updates;
        if(!stale) {try {
            const auto& settings=GetState()->m_settings.global;const auto& catalog=settings.GetSmacPredefinedUnitRules()->catalog;
            const auto& state=m_simulation->value.designs;const auto& economy=m_simulation->value.economy.at(owner);
            if(!state.native_registry || !economy.design_counts) {throw std::runtime_error("Workshop Retire requires retained design and Unit-count authority");}
            const auto& row=state.native_registry->at(selected);stored_name=std::string(row.begin(),std::find(row.begin(),row.begin()+32,uint8_t(0)));count=economy.design_counts->at(selected);
            const auto triad=catalog.chassis.chassis.at(row[36]).triad;
            illustration=triad==loader::txt::ChassisTriad::Land ? "unit_sm.pcx":triad==loader::txt::ChassisTriad::Sea ? "navun_sm.pcx":"air_sm.pcx";
            if(answer==1) {
                auto next=unit::RetireSmacWorkshopDesign(state,int32_t(selected),catalog,owner);
                auto selection=unit::PrepareSmacWorkshopReselection(next,int32_t(selected),retained,catalog,owner);
                const auto& technology=*m_simulation->value.technology.Find(owner);
                selection.draft.abilities=unit::CleanSmacWorkshopRetainedAbilities(selection.draft,catalog,*settings.GetSmacTechnologyRules(),technology).mask;
                const auto list=unit::BrowseSmacWorkshopDesigns(selection.state,catalog,*settings.GetSmacTechnologyRules(),technology,economy.design_counts);
                const auto at=std::find(list.begin(),list.end(),selection.draft.selected_design);
                if(at!=list.end()) {strip_offset=(size_t(at-list.begin())+list.size()-5)%list.size();}
                response_draft=selection.draft;
                if(apply) {
                    stage_retirement(int32_t(selected),selected>=64,removal,queues,base_updates);
                    static_assert(std::is_nothrow_swappable_v<unit::SmacStartupDesignState> && std::is_nothrow_swappable_v<std::optional<base::ProductionState>>);
                    removal->Commit();for(auto& change:queues) {change.first->m_production.swap(change.second);}
                    GetBM()->m_base_updates.swap(*base_updates);std::swap(m_simulation->value.designs,selection.state);retired=true;
                }
            }
        }catch(const std::invalid_argument&) {failed=true;}catch(const std::runtime_error&) {failed=true;}}
        if(apply) {
            if(retired) {removal->Notify(GSE_CALL);}
            GetState()->TriggerObject(this,"smac_workshop_retire",ARGS_F(this,workshop,request,answer,stale,failed,retired,count,stored_name,illustration,strip_offset,response_draft) {
                {"base",workshop.subject->Wrap(GSE_CALL)}, {"request_id",VALUE(gse::value::Int,,request)}, {"stale",VALUE(gse::value::Bool,,stale)}, {"failed",VALUE(gse::value::Bool,,failed)},
                {"confirmation",VALUE(gse::value::Bool,,answer<0 && !stale && !failed)}, {"declined",VALUE(gse::value::Bool,,answer==0 && !stale && !failed)},
                {"retired",VALUE(gse::value::Bool,,retired)}, {"count",VALUE(gse::value::Int,,count)}, {"stored_name",VALUE(gse::value::String,,stored_name)},
                {"illustration",VALUE(gse::value::String,,illustration)},
                {"expected_state",VALUE(gse::value::String,,SmacWorkshopState(workshop.slot))}, {"strip_offset",VALUE(gse::value::Int,,strip_offset)},
                {"selection",retired ? CopySmacWorkshop(GSE_CALL,workshop.slot,&response_draft):VALUE(gse::value::Undefined)}
            };});
        }return true;
    }
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
        (void)field("base",gse::VT_OBJECT);
        const auto workshop=ResolveWorkshopTarget(this,args.at("base"));
        if(workshop.caller_base) {workshop.caller_base->RequireSmacProductionSelection();}
        const auto selected=integer("selected_design"),chassis=integer("chassis"),weapon=integer("weapon"),armor=integer("armor"),reactor=integer("reactor"),abilities=integer("abilities"),request=integer("request_id");
        const auto& retained=text("name");const auto& expected=text("expected_state");
        const auto displayed=answer ? text("displayed_default"):std::string();const auto submitted=answer ? text("submitted"):std::string();
        if(caller!=m_slot_num || selected<0 || selected>511 || chassis<0 || chassis>8 || weapon<0 || weapon>20 || armor<0 || armor>9
            || reactor<1 || reactor>4 || abilities<0 || abilities>0xffffff || request<1 || expected.size()>unit::SmacStartupDesignState::MAX_WIRE_BYTES+89+2*513) {THROW("Invalid Workshop Rename authority or draft");}
        for(const auto* item:{&retained,&displayed,&submitted}) {
            if(item->size()>31 || item->find('\0')!=std::string::npos) {THROW("Workshop Rename exceeds31 display bytes");}
            for(const unsigned char c:*item) {if(c<32 || c>126) {THROW("Workshop Rename requires printable display bytes");}}
        }
        const auto owner=RequireSmacWorkshopOwner(workshop.slot);
        if(selected>=64 && selected/64!=owner) {THROW("Workshop Rename selected a foreign bank");}
        if(apply) {CheckRW(GSE_CALL);if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Workshop Rename requires its owning accumulation");}}
        const bool stale=SmacWorkshopState(workshop.slot)!=expected;bool failed=false;std::string default_name,display_name,result_name=retained;
        if(!stale) {try {
            const auto& catalog=GetState()->m_settings.global.GetSmacPredefinedUnitRules()->catalog;
            const unit::SmacWorkshopDraft draft{int32_t(selected),uint32_t(chassis),uint32_t(weapon),uint32_t(armor),uint32_t(abilities),uint8_t(reactor),retained};
            auto result=answer ? unit::AnswerSmacWorkshopRename(m_simulation->value.designs,draft,displayed,true,submitted,catalog,owner)
                :unit::OpenSmacWorkshopRename(m_simulation->value.designs,draft,catalog,owner);
            default_name=result.default_name;result_name=result.draft.name;
            display_name=unit::DisplaySmacWorkshopName(*result.state.native_registry,result.draft,catalog,owner);
            if(apply) {static_assert(std::is_nothrow_swappable_v<unit::SmacStartupDesignState>);std::swap(m_simulation->value.designs,result.state); }
        }catch(const std::invalid_argument&) {failed=true;}catch(const std::runtime_error&) {failed=true;}}
        if(apply) {GetState()->TriggerObject(this,"smac_workshop_rename",ARGS_F(this,workshop,request,answer,stale,failed,default_name,display_name,result_name) {
            {"base",workshop.subject->Wrap(GSE_CALL)}, {"request_id",VALUE(gse::value::Int,,request)}, {"answer",VALUE(gse::value::Bool,,answer)},
            {"stale",VALUE(gse::value::Bool,,stale)}, {"failed",VALUE(gse::value::Bool,,failed)},
            {"expected_state",VALUE(gse::value::String,,SmacWorkshopState(workshop.slot))}, {"default_name",VALUE(gse::value::String,,default_name)},
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
        (void)field("base",gse::VT_OBJECT);
        const auto workshop=ResolveWorkshopTarget(this,args.at("base"));
        if(workshop.caller_base) {workshop.caller_base->RequireSmacProductionSelection();}
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
        if(caller!=m_slot_num || selected<0 || selected>511 || offset<0 || offset>512 || request<1
            || expected.size()>unit::SmacStartupDesignState::MAX_WIRE_BYTES+89+2*513 || retained.size()>31 || retained.find('\0')!=std::string::npos) {
            THROW("Invalid Workshop Obsolete authority or fields");
        }
        for(const unsigned char c:retained) {if(c<32 || c>126) {THROW("Workshop name requires printable display bytes");}}
        const auto owner_id=RequireSmacWorkshopOwner(workshop.slot);
        if(selected>=64 && selected/64!=owner_id) {THROW("Workshop Obsolete selected a foreign bank");}
        if(apply) {CheckRW(GSE_CALL);if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Workshop Obsolete requires its owning accumulation");}}
        const bool stale=SmacWorkshopState(workshop.slot)!=expected;bool failed=false;
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
        if(apply) {GetState()->TriggerObject(this,"smac_workshop_obsolete",_ARGS_F(this,workshop,response,slots,first,request,stale,failed) {
            auto* view=static_cast<gse::value::Object*>(CopySmacWorkshop(GSE_CALL,workshop.slot,stale || failed ? nullptr:&response));
            view->Assign("ability_slots",VALUE(gse::value::Array,,gse::value::array_elements_t{VALUE(gse::value::Int,,slots[0]),VALUE(gse::value::Int,,slots[1])}));
            out_args={
                {"base",workshop.subject->Wrap(GSE_CALL)}, {"selection",view}, {"strip_offset",VALUE(gse::value::Int,,first)},
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
        (void)field("base",gse::VT_OBJECT);
        const auto workshop=ResolveWorkshopTarget(this,args.at("base"));
        if(workshop.caller_base) {workshop.caller_base->RequireSmacProductionSelection();}
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
        if(caller!=m_slot_num || expected.size()>unit::SmacStartupDesignState::MAX_WIRE_BYTES+89+2*513
            || selected<0 || selected>511 || request<1 || armor<0 || armor>9 || chassis<0 || chassis>8 || weapon<0 || weapon>20
            || reactor<1 || reactor>4 || abilities<0 || abilities>0xffffff || supplied.size()>31 || supplied.find('\0')!=std::string::npos) {THROW("Invalid Workshop command authority or draft");}
        // Current narrow UI has the same printable-byte rendering boundary as
        // Base Rename. Do not install a name that its renderer cannot display.
        for(const unsigned char c:supplied) {if(c<32 || c>126) {THROW("Workshop name requires printable display bytes");}}
        (void)RequireSmacWorkshopOwner(workshop.slot);
        if(selected>=64 && selected/64!=RequireSmacWorkshopOwner(workshop.slot)) {THROW("Workshop selected a foreign bank");}
        if(apply) {
            CheckRW(GSE_CALL);
            if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Workshop requires its owning accumulation");}
        }
        const bool stale=SmacWorkshopState(workshop.slot)!=expected;
        bool warning=false,failed=false,confirmation=false,declined=false,reselected=false,retired=false;
        auto notice=unit::SmacWorkshopNotice::None;std::string notice_name;
        int32_t design_id=int32_t(selected),overwrite_count=0,restored_armor=0;std::string overwrite_name,restored_name;
        unit::SmacWorkshopDraft response_draft{int32_t(selected),uint32_t(chassis),uint32_t(weapon),uint32_t(armor),uint32_t(abilities),uint8_t(reactor),supplied};
        std::optional<unit::UnitManager::PreparedConventionalRemoval> removal;
        std::vector<std::pair<base::Base*,std::optional<base::ProductionState>>> queues;
        std::optional<decltype(GetBM()->m_base_updates)> base_updates;
        if(!stale) {
            auto next=m_simulation->value.designs;
            const auto draft=response_draft;
            const auto& settings=GetState()->m_settings.global;
            const auto* owner=m_simulation->value.technology.Find(RequireSmacWorkshopOwner(workshop.slot));
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
                        } else if(!prepared.success) {
                            if(prepared.notice==unit::SmacWorkshopNotice::None) {throw std::runtime_error("Workshop allocation failed");}
                            failed=true;notice=prepared.notice;
                        }
                        if(apply && prepared.retirement) {stage_retirement(*prepared.retirement,true,removal,queues,base_updates);}
                        next=std::move(prepared.state);
                    }
                } else {result=unit::CommitSmacWorkshopFirstDesign(next,draft,settings.GetSmacPredefinedUnitRules()->catalog,*settings.GetSmacTechnologyRules(),*owner);}
                warning=result.intrinsic_name_warning;design_id=result.design_id;
                if(warning && notice==unit::SmacWorkshopNotice::None) {notice=unit::SmacWorkshopNotice::IntrinsicName;}
                if(notice==unit::SmacWorkshopNotice::IntrinsicName || notice==unit::SmacWorkshopNotice::IntrinsicType) {
                    if(next.native_registry) {const auto& row=next.native_registry->at(design_id);notice_name.assign(row.begin(),std::find(row.begin(),row.begin()+32,uint8_t(0)));}
                    else {notice_name=settings.GetSmacPredefinedUnitRules()->catalog.source.units.at(design_id).name;}
                }
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
                auto* manager=GetUM();auto updates=manager->m_unit_updates;
                if(m_simulation->value.designs.native_registry && next.native_registry) {
                    for(const auto& entry:manager->GetUnits()) {
                        const auto* actor=entry.second;if(!actor->IsConventional()) {continue;}
                        const auto id=actor->Conventional().design_id;
                        const auto& before=m_simulation->value.designs.native_registry->at(id);const auto& after=next.native_registry->at(id);
                        if(std::equal(before.begin(),before.begin()+32,after.begin())) {continue;}
                        if(removal && !failed && !declined) {removal->RefreshSurvivor(actor);}
                        else {
                            auto& update=updates.try_emplace(actor->m_id,unit::UnitManager::unit_update_t{unit::UnitManager::UUO_NONE,actor}).first->second;
                            update.ops=static_cast<unit::UnitManager::unit_update_op_t>(update.ops|unit::UnitManager::UUO_REFRESH);
                        }
                    }
                }
                if(!failed && !declined) {
                    if(removal) {removal->Commit();retired=true;}
                    for(auto& change:queues) {change.first->m_production.swap(change.second);}
                    if(base_updates) {GetBM()->m_base_updates.swap(*base_updates);}
                }
                std::swap(m_simulation->value.designs,next);
                if(!retired) {manager->m_unit_updates.swap(updates);}
            }
        }
        if(apply) {
            if(retired) {removal->Notify(GSE_CALL);}
            GetState()->TriggerObject(this,"smac_workshop_commit",ARGS_F(this,workshop,done,stale,warning,design_id,failed,request,confirmation,declined,overwrite_count,overwrite_name,restored_armor,restored_name,reselected,response_draft,notice,notice_name) {
                {"base",workshop.subject->Wrap(GSE_CALL)}, {"selection",CopySmacWorkshop(GSE_CALL,workshop.slot,&response_draft)},
                {"done",VALUE(gse::value::Bool,,done && !stale && !failed && !confirmation && !declined)}, {"stale",VALUE(gse::value::Bool,,stale)},
                {"failed",VALUE(gse::value::Bool,,failed)},
                {"confirmation",VALUE(gse::value::Bool,,confirmation)}, {"declined",VALUE(gse::value::Bool,,declined)},
                {"overwrite_name",VALUE(gse::value::String,,overwrite_name)}, {"overwrite_count",VALUE(gse::value::Int,,overwrite_count)},
                {"restore_draft",VALUE(gse::value::Bool,,reselected)},
                {"restored_armor",VALUE(gse::value::Int,,restored_armor)}, {"restored_name",VALUE(gse::value::String,,restored_name)},
                {"request_id",VALUE(gse::value::Int,,request)},
                {"intrinsic_notice",VALUE(gse::value::Int,,int32_t(notice))}, {"intrinsic_notice_name",VALUE(gse::value::String,,notice_name)},
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

gse::Value* Game::QuerySmacWorkshop(GSE_CALLABLE,const slot::Slot* owner_slot,const std::string& method,
    const gse::value::function_arguments_t& arguments) const {
    const auto owner_id=RequireSmacWorkshopOwner(owner_slot);
    if(method=="get_smac_workshop") {
			N_EXPECT_ARGS_MAX(7);
            if(!arguments.empty() && arguments.size()!=7) {THROW("Workshop preview requires the complete draft tuple");}
			if(arguments.empty()) {return CopySmacWorkshop(GSE_CALL,owner_slot);}
            N_GETVALUE(selected,0,Int);N_GETVALUE(chassis,1,Int);N_GETVALUE(weapon,2,Int);N_GETVALUE(armor,3,Int);
            N_GETVALUE(reactor,4,Int);N_GETVALUE(abilities,5,Int);N_GETVALUE(name,6,String);
            if(selected<0 || selected>511 || chassis<0 || chassis>8 || weapon<0 || weapon>20 || armor<0 || armor>9
                || reactor<1 || reactor>4 || abilities<0 || abilities>0xffffff) {THROW("Invalid Workshop preview tuple");}
            const unit::SmacWorkshopDraft draft{int32_t(selected),uint32_t(chassis),uint32_t(weapon),uint32_t(armor),uint32_t(abilities),uint8_t(reactor),name};
            return CopySmacWorkshop(GSE_CALL,owner_slot,&draft);

    }
    if(method=="get_smac_workshop_abilities") {
            N_EXPECT_ARGS(10);
            N_GETVALUE(selected,0,Int);N_GETVALUE(chassis,1,Int);N_GETVALUE(weapon,2,Int);N_GETVALUE(armor,3,Int);
            N_GETVALUE(reactor,4,Int);N_GETVALUE(abilities,5,Int);N_GETVALUE(name,6,String);
            N_GETVALUE(first,7,Int);N_GETVALUE(second,8,Int);N_GETVALUE(slot,9,Int);
            if(selected<0 || selected>511 || chassis<0 || chassis>8 || weapon<0 || weapon>20 || armor<0 || armor>9
                || reactor<1 || reactor>4 || abilities<0 || abilities>0xffffff || first< -1 || first>=24 || second< -1 || second>=24
                || slot<0 || slot>1 || name.size()>31 || name.find('\0')!=std::string::npos) {THROW("Invalid Workshop ability menu request");}
            const auto& settings=GetState()->m_settings.global;const auto& rules=settings.GetSmacPredefinedUnitRules();
            const auto* owner=m_simulation->value.technology.Find(owner_id);
            if(!owner || (selected>=64 && selected/64!=owner->admission.simulation_id)) {THROW("Workshop ability menu selected a foreign bank");}
            const auto& catalog=rules->catalog;const auto& technologies=*settings.GetSmacTechnologyRules();
            const unit::SmacWorkshopEditor editor{{int32_t(selected),uint32_t(chassis),uint32_t(weapon),uint32_t(armor),uint32_t(abilities),uint8_t(reactor),name},{int32_t(first),int32_t(second)}};
            const auto context=unit::QuerySmacWorkshopEditContext(catalog,technologies,*owner,false);
            const auto menu=unit::QuerySmacWorkshopAbilities(editor,uint32_t(slot),context,catalog);
            using Guard=unit::SmacWorkshopAbilityMenu::Guard;
            gse::value::array_elements_t rows;
            for(const auto id:menu.choices) {
                gse::Value* cost=VALUE(gse::value::Undefined);std::string label,description;
                if(id<24) {
                    const auto& ability=catalog.abilities.abilities.at(id);label=ability.name;description=ability.description;
                    uint32_t flags=0;for(const auto bit:ability.flags) {flags=(flags<<1)|uint32_t(bit-'0');}
                    const auto price=unit::QuerySmacWorkshopAbilityCost(ability.cost,flags,int32_t(catalog.chassis.chassis.at(chassis).triad),
                        uint8_t(catalog.weapons.weapons.at(weapon).cost),uint8_t(catalog.defenses.defenses.at(armor).cost),uint8_t(catalog.chassis.chassis.at(chassis).speed));
                    if(price) {cost=VALUE(gse::value::Object,,GSE_CALL_NOGC,{
                        {"quantity",VALUE(gse::value::Int,,price->quantity)}, {"formula",VALUE(gse::value::Int,,price->formula)}, {"domain",VALUE(gse::value::Int,,price->domain)}
                    });}
                }
                rows.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,{
                    {"id",VALUE(gse::value::Int,,id)}, {"name",VALUE(gse::value::String,,label)},
                    {"description",VALUE(gse::value::String,,description)}, {"cost",cost}
                }));
            }
            std::string required;
            const auto neural=technologies.ResolveToken("Neural");if(neural.technology_id) {required=technologies.technologies.at(*neural.technology_id).name;}
            return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
                {"guard",VALUE(gse::value::String,,menu.guard==Guard::None?"":menu.guard==Guard::SecondTechnology?"second_technology":"missile")},
                {"required_technology",VALUE(gse::value::String,,required)}, {"rows",VALUE(gse::value::Array,,rows)}
            });

    }
    if(method=="get_smac_workshop_edit") {
            N_EXPECT_ARGS(12);
            N_GETVALUE(selected,0,Int);N_GETVALUE(chassis,1,Int);N_GETVALUE(weapon,2,Int);N_GETVALUE(armor,3,Int);
            N_GETVALUE(reactor,4,Int);N_GETVALUE(abilities,5,Int);N_GETVALUE(name,6,String);
            N_GETVALUE(first,7,Int);N_GETVALUE(second,8,Int);N_GETVALUE(kind,9,Int);N_GETVALUE(choice,10,Int);N_GETVALUE(automatic,11,Bool);
            if(selected<0 || selected>511 || chassis<0 || chassis>8 || weapon<0 || weapon>20 || armor<0 || armor>9
                || reactor<1 || reactor>4 || abilities<0 || abilities>0xffffff || first< -1 || first>=24 || second< -1 || second>=24
                || kind<0 || kind>5 || choice< -1 || choice>255 || name.size()>31 || name.find('\0')!=std::string::npos) {THROW("Invalid Workshop editor request");}
            const auto& settings=GetState()->m_settings.global;const auto& rules=settings.GetSmacPredefinedUnitRules();
            const auto* owner=m_simulation->value.technology.Find(owner_id);
            if(!owner || (selected>=64 && selected/64!=owner->admission.simulation_id)) {THROW("Workshop editor selected a foreign bank");}
            const auto& catalog=rules->catalog;const auto& technologies=*settings.GetSmacTechnologyRules();
            const unit::SmacWorkshopEditor before{{int32_t(selected),uint32_t(chassis),uint32_t(weapon),uint32_t(armor),uint32_t(abilities),uint8_t(reactor),name},{int32_t(first),int32_t(second)}};
            const auto context=unit::QuerySmacWorkshopEditContext(catalog,technologies,*owner,automatic);
            auto editor=before;bool admitted=true;
            if(kind<4) {
                const auto component=static_cast<unit::SmacWorkshopComponent>(kind);
                const auto old=std::array<uint32_t,4>{before.draft.chassis,before.draft.weapon,before.draft.armor,before.draft.reactor}.at(size_t(kind));
                auto accepted=old;
                if(choice>=0) {for(const auto& row:unit::SmacWorkshopChoices(component,catalog,technologies,*owner)) {
                    if(row.component_id==uint32_t(choice) && row.technology_available) {accepted=row.component_id;break;}
                }}
                editor=unit::ChangeSmacWorkshopComponent(before,component,accepted,context,catalog);
            } else {
                const auto slot=uint32_t(kind-4);auto accepted=before.ability_slots[slot];
                if(choice>=24) {accepted=24;}
                else if(choice>=0 && context.available_abilities.at(size_t(choice))
                    && before.ability_slots[1-slot]!=choice && unit::SmacWorkshopAbilityCompatible(uint32_t(choice),before.draft,catalog)) {accepted=int32_t(choice);}
                auto result=unit::SetSmacWorkshopAbilitySlot(before,slot,accepted,context,catalog);editor=std::move(result.editor);admitted=result.admitted;
            }
            auto* result=static_cast<gse::value::Object*>(CopySmacWorkshop(GSE_CALL,owner_slot,&editor.draft));
            result->Assign("ability_slots",VALUE(gse::value::Array,,gse::value::array_elements_t{
                VALUE(gse::value::Int,,editor.ability_slots[0]),VALUE(gse::value::Int,,editor.ability_slots[1])}));
            result->Assign("ability_entry_allowed",VALUE(gse::value::Bool,,admitted));return result;

    }
    THROW("Unknown Workshop query");
}

int32_t Game::RequireSmacWorkshopOwner(const slot::Slot* owner) const {
    RequireSmacLocalHumanOwner(owner);
    RequireSmacGameplayActive();
    if(!IsRunning() || m_canonical_turn_failed || m_canonical_turn_in_progress
        || !owner || owner->GetPlayer()!=GetPlayer()) {
        THROW("Workshop requires the ready local owner");
    }
    const auto& settings=GetState()->m_settings.global;
    if(!settings.GetSmacPredefinedUnitRules() || settings.GetSmacPredefinedUnitRules()->catalog.source.ruleset_id!="smac"
        || !settings.GetSmacTechnologyRules()) {THROW("Workshop requires owned SMAC catalogs");}
    const auto id=RequireSmacProductionOwner(owner).simulation_id;
    const auto* social=m_simulation->value.social.Find(id);
    if(!social || social->phase!=faction::SmacFactionSocialState::Phase::Refreshed) {
        THROW("Workshop requires refreshed social authority");
    }
    return id;
}

std::string Game::SmacWorkshopState(const slot::Slot* owner_slot) const {
    const auto owner=RequireSmacWorkshopOwner(owner_slot);
    const auto* technologies=m_simulation->value.technology.Find(owner);
    const auto& rules=GetState()->m_settings.global.GetSmacPredefinedUnitRules();
    if(!technologies || !rules || rules->catalog.source.ruleset_id!="smac") {THROW("Workshop requires an owned SMAC catalog and technologies");}
    auto state=m_simulation->value.designs.Serialize(rules->catalog).ToString();
    for(const auto owned:technologies->owned) {state.push_back(owned ? '1':'0');}
    const auto& economy=m_simulation->value.economy.at(owner);
    for(const auto* counts:{&economy.design_counts,&economy.production_reference_counts}) {
        state.push_back(*counts ? '1':'0');
        if(*counts) {for(const auto count:**counts) {state.push_back(static_cast<char>(count));}}
    }
    return state;
}
gse::Value* Game::CopySmacWorkshop(GSE_CALLABLE,const slot::Slot* owner_slot,const unit::SmacWorkshopDraft* supplied) const {
    const auto owner=RequireSmacWorkshopOwner(owner_slot);
    const auto expected=SmacWorkshopState(owner_slot);
    const auto& settings=GetState()->m_settings.global;
    const auto& catalog=settings.GetSmacPredefinedUnitRules()->catalog;
    const auto& technology=*m_simulation->value.technology.Find(owner);
    auto choices=unit::SmacWorkshopChoices(unit::SmacWorkshopComponent::Armor,catalog,*settings.GetSmacTechnologyRules(),technology);
    unit::SmacWorkshopDraft draft=supplied ? *supplied:unit::SmacWorkshopDraft{};
    if(draft.selected_design<0 || draft.selected_design>511 || (draft.selected_design>=64 && draft.selected_design/64!=technology.admission.simulation_id)
        || draft.chassis>=catalog.chassis.chassis.size() || draft.weapon>=catalog.weapons.weapons.size() || draft.armor>=catalog.defenses.defenses.size()
        || draft.reactor<1 || draft.reactor>catalog.reactors.reactors.size() || draft.abilities>0xffffff || draft.name.size()>31 || draft.name.find('\0')!=std::string::npos) {throw std::invalid_argument("Invalid Workshop preview draft");}
    if(catalog.chassis.chassis.at(draft.chassis).missile) {
        choices.erase(std::remove_if(choices.begin(),choices.end(),[](const auto& row){return row.component_id!=0;}),choices.end());
    }
    const auto mineral_factor=GetSmacProductionMineralFactor(owner_slot);
    const auto& registry=m_simulation->value.designs;
    gse::Value* status=VALUE(gse::value::Undefined);
    if(registry.native_registry) {
        const auto& economy=m_simulation->value.economy.at(technology.admission.simulation_id);
        const auto result=unit::QuerySmacWorkshopStatus(*registry.native_registry,draft,technology.admission.simulation_id,
            economy.design_counts,economy.production_reference_counts);
        draft.selected_design=result.selected;
        status=VALUE(gse::value::Int,,int32_t(result.kind));
    }
    const auto preview_icon=unit::SmacWorkshopPreviewIcon(registry,draft,catalog);int preview_variant=0;
    if(preview_icon==-2) {throw std::invalid_argument("Reserved scratch cached-idle preview is outside the ordinary Workshop profile");}
    if(preview_icon==3) {
        if(!registry.native_registry) {throw std::invalid_argument("Ranked Workshop preview requires retained scratch metadata");}
        // The admitted startup faction profile has no direct MORALE scalar or
        // direct unit-immunity flag. SOCIAL/IMMUNITY axes are not those fields.
        const auto* social=m_simulation->value.social.Find(technology.admission.simulation_id);
        if(!social || !social->retained) {throw std::invalid_argument("Ranked Workshop preview requires owned retained social ratings");}
        const auto plan=registry.native_registry->at(63)[42];int32_t probe_technologies=0;
        if(plan==11) {
            if(m_simulation->value.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {throw std::invalid_argument("Probe preview requires explicit Telepathic Matrix authority");}
            probe_technologies=unit::SmacWorkshopProbeTechnologyCount(*settings.GetSmacTechnologyRules(),technology);
        }
        preview_variant=unit::SmacWorkshopPreviewRank({plan,int8_t(uint8_t(catalog.weapons.weapons.at(draft.weapon).offense)),0,
            social->retained->at(4),social->retained->at(8),false,false,probe_technologies});
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
    const auto& counts=m_simulation->value.economy.at(technology.admission.simulation_id).design_counts;
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
                    {"abilities",VALUE(gse::value::Int,,uint32_t(raw[32])|(uint32_t(raw[33])<<8)|(uint32_t(raw[34])<<16)|(uint32_t(raw[35])<<24))},
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
            auto probe=m_simulation->value.designs;auto candidate=draft;candidate.armor=choice.component_id;
            try {
                unit::SmacWorkshopCommit result;
                if(probe.native_registry) {
                    if(const auto alias=unit::ReuseSmacWorkshopDesign(probe,candidate,catalog,*settings.GetSmacTechnologyRules(),technology)) {result=*alias;}
                    else {
                        unit::ValidateSmacWorkshopEditorAbilities(candidate,catalog,*settings.GetSmacTechnologyRules(),technology);
                        const auto& economy=m_simulation->value.economy.at(technology.admission.simulation_id);
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
    gse::value::array_elements_t ability_names,ability_descriptions;
    for(const auto& row:catalog.abilities.abilities) {
        ability_names.push_back(VALUE(gse::value::String,,row.name));
        ability_descriptions.push_back(VALUE(gse::value::String,,row.description));
    }
    return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
        {"expected_state",VALUE(gse::value::String,,expected)},
        {"preset_state",VALUE(gse::value::String,,WorkshopPresetState(expected,m_simulation->value,settings,technology.admission.simulation_id))}, {"armor",VALUE(gse::value::Array,,armor)},
        {"mineral_factor",VALUE(gse::value::Int,,mineral_factor)},
        {"movement",VALUE(gse::value::Int,,unit::SmacWorkshopMovement(draft,catalog))},
        {"weapon_cost",VALUE(gse::value::Int,,uint8_t(catalog.weapons.weapons.at(draft.weapon).cost))},
        {"chassis_choices",component_choices(unit::SmacWorkshopComponent::Chassis)},
        {"weapon_choices",component_choices(unit::SmacWorkshopComponent::Weapon)},
        {"reactor_choices",component_choices(unit::SmacWorkshopComponent::Reactor)},
        {"ability_names",VALUE(gse::value::Array,,ability_names)},
        {"ability_descriptions",VALUE(gse::value::Array,,ability_descriptions)},
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
        {"preview_icon",VALUE(gse::value::Int,,preview_icon)}, {"preview_variant",VALUE(gse::value::Int,,preview_variant)},
        {"reactor",VALUE(gse::value::String,,catalog.reactors.reactors.at(draft.reactor-1).name)},
        {"power",VALUE(gse::value::Int,,catalog.reactors.reactors.at(draft.reactor-1).power)},
        {"scout_name",VALUE(gse::value::String,,catalog.source.units.at(2).name)},
        {"faction_art",VALUE(gse::value::String,,owner_slot->GetPlayer()->GetFaction()->m_bases_render.file)}
    });
}

namespace base {
std::string Base::SmacWorkshopState() const {
    RequireSmacProductionSelection();
    return m_game->SmacWorkshopState(m_owner);
}
gse::Value* Base::CopySmacWorkshop(GSE_CALLABLE,const unit::SmacWorkshopDraft* supplied) const {
    RequireSmacProductionSelection();
    return m_game->CopySmacWorkshop(GSE_CALL,m_owner,supplied);
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
    if ( !designs || (!facility && (source_id<0 || source_id>=512
        || (!m_game->m_simulation->value.designs.native_registry && source_id!=0 && source_id!=1 && source_id!=2 && source_id!=designs->starting_scout))) ) {
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
    const auto& state=m_game->m_simulation->value;const auto& settings=m_game->GetState()->m_settings.global;
    std::vector<int32_t> ids;
    if(state.designs.native_registry) {
        if(state.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {THROW("Production ordering requires admitted project authority");}
        unit::SmacProductionDesignContext context{designs->simulation_id,std::nullopt,std::nullopt,std::nullopt,0,false,false};
        if(state.capabilities) {const auto& peer=state.capabilities->factions.at(designs->simulation_id-1).foreign;
            context.peer_attack=peer[faction::SmacCapabilityState::Weapon];context.peer_defense=peer[faction::SmacCapabilityState::Armor];}
        if(settings.GetSmacCombatRules() && settings.GetSmacCombatRules()->psi_ratios) {context.land_psi_ratio=settings.GetSmacCombatRules()->psi_ratios->at(0);}
        const auto list=unit::BuildSmacProductionDesignList(*state.designs.native_registry,settings.GetSmacPredefinedUnitRules()->catalog,
            *settings.GetSmacTechnologyRules(),*state.technology.Find(designs->simulation_id),context);
        ids.assign(list.order.begin(),list.order.begin()+list.count);
    } else {
        // Old port saves lack raw candidate/name authority. Keep their prior
        // limited projection; do not fabricate native records or ordering.
        ids={2,1,0};if(designs->starting_scout!=2) {ids.push_back(designs->starting_scout);}
    }
    std::vector< ProductionChoice > result;
    for ( const auto id : ids ) {
        if(state.designs.native_registry && !(state.designs.native_registry->at(id)[48]&1)) {continue;}
        const auto item = m_game->MakeSmacProductionItem( m_owner, ProductionItemKind::Unit, id );
        if ( !m_game->IsSmacUnitProductionEligible( *this, item ) ) { continue; }
        const auto switched = PrepareSmacProductionSelection( id, true );
        const auto* def = m_game->ResolveSmacProductionPresentation( m_owner, id );
        const auto name=state.designs.native_registry ? *unit::ReadSmacProductionDesign(state.designs.native_registry->at(id),id).name:def->m_name;
        result.push_back( { id, name, m_game->GetSmacProductionCost( m_owner, item, this ), switched.state.minerals, switched.penalty_prompt_required } );
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
