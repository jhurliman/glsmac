#include "Base.h"
#include "BaseManager.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "gc/Space.h"
#include "gse/value/Bool.h"
#include "gse/value/Int.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include <type_traits>

namespace game::backend {
bool Game::HandleSmacHurryEvent(GSE_CALLABLE,const std::string& name,
    const gse::value::object_properties_t& args,size_t caller,bool apply) {
    if(name!="hurry_smac_production") {return false;}
    const auto field=[&](const char* key,gse::value_type_t type)->const gse::Value* {
        const auto found=args.find(key);
        if(found==args.end() || !found->second || found->second->Deref()->type!=type) {THROW("Malformed Hurry command");}
        return found->second->Deref();
    };
    if(args.size()!=4) {THROW("Hurry requires Base, expected state, full/partial choice and credits");}
    const auto target=args.find("base");
    const auto* object=target!=args.end() && target->second
        ? dynamic_cast<const gse::value::Object*>(target->second->Deref()) : nullptr;
    if(!object || object->object_class!=base::Base::WRAP_CLASS || caller!=m_slot_num) {THROW("Hurry requires an actual local Base");}
    // The wrapper owns its cached scalar identity, not the C++ Base lifetime.
    // Retirement can unlink it between enqueue and dispatch; never dereference
    // its former pointer to discover whether the registry still owns it.
    const auto id=object->value.find("id");
    if(id==object->value.end() || !id->second || id->second->type!=gse::VT_INT) {THROW("Hurry requires a stable Base identity");}
    const auto number=static_cast<const gse::value::Int*>(id->second)->value;
    if(number<0 || static_cast<uint64_t>(number)>SIZE_MAX) {THROW("Invalid Hurry Base identity");}
    auto* base=m_bm->GetBase(static_cast<size_t>(number));
    if(!base || object->wrapobj!=base) {return true;}
    const auto& expected=static_cast<const gse::value::String*>(field("expected_state",gse::VT_STRING))->value;
    const bool full=static_cast<const gse::value::Bool*>(field("full",gse::VT_BOOL))->value;
    const auto amount=static_cast<const gse::value::Int*>(field("amount",gse::VT_INT))->value;
    if(!base || base->GetGame()!=this || caller!=m_slot_num || expected.size()>2*1024*1024
        || amount<0 || amount>INT32_MAX || (full && amount!=0)) {THROW("Hurry has invalid authority or credit amount");}
    const auto offer=base->GetSmacHurryOffer();
    if(apply) {
        CheckRW(GSE_CALL);
        if(gc_space!=GetGCSpace() || !gc_space->IsAccumulating()) {THROW("Hurry requires its owning writable accumulation");}
    }
    // Treasury/reservations and actual quote inputs participate in the native
    // snapshot. A purchase at another Base invalidates this view too.
    if(expected!=offer.expected) {return true;}
    if(!offer.context || !offer.reason.empty()) {THROW(offer.reason);}
    const auto quote=base::CalculateSmacHurryQuote(*base->m_production,*offer.context);
    const auto available=int64_t(offer.credits)-offer.reserved;
    if(full && available<quote) {THROW("Not enough available credits for full Hurry");}
    // Deliberate port numeric admission, not a claim about the original control.
    if(!full && amount>quote) {THROW("Partial Hurry currently admits credits from zero through the shown quote");}
    const auto result=base::PrepareSmacHurryPayment(*base->m_production,*offer.context,offer.credits,offer.reserved,false,
        full ? base::SmacHurryChoice::Full : base::SmacHurryChoice::Partial,static_cast<int32_t>(amount));
    if(!result.purchased) {THROW("Hurry is no longer available");}
    if(!apply) {return true;}
    std::optional<base::ProductionState> replacement=result.production;
    auto& economy=m_simulation->value.economy.at(base->m_resource_context->owner_simulation_id);
    auto money=economy;money.credits=result.treasury;money.pending_debit=result.reserved;money.Validate();
    auto updates=m_bm->m_base_updates;base::BaseManager::QueueBaseUpdateInto(updates,base,base::BaseManager::BUO_REFRESH);
    std::unique_lock lock(m_bm->m_updated_bases_mutex);auto triggers=m_bm->m_updated_bases;triggers.insert(base);
    static_assert(std::is_nothrow_swappable_v<decltype(replacement)> && std::is_nothrow_swappable_v<decltype(money)>);
    base->m_production.swap(replacement);std::swap(economy,money);m_bm->m_base_updates.swap(updates);m_bm->m_updated_bases.swap(triggers);
    return true;
}

void Game::ResetCanonicalHurryMarkers(int32_t owner) {
    if(!IsRunning() || !GetGCSpace()->IsAccumulating() || !m_canonical_turn_in_progress
        || !m_simulation || !m_simulation->value.economy.count(owner)) {THROW("Hurry reset requires the current faction upkeep tail");}
    // The original owner reset follows faction upkeep, before reservation settlement.
    // All throwing authentication precedes this allocation-free publication.
    // Existing per-Base upkeep refreshes read this current state when delivered.
    for(const auto& row:m_bm->GetBases()) {
        auto* base=row.second;
        if(base->m_resource_context && base->m_resource_context->owner_simulation_id==owner && base->m_production) {
            base->m_production->hurry_partial_paid=false;base->m_resource_context->SyncKnownHurryBit(false);
        }
    }
}
namespace base {
Base::HurryOffer Base::GetSmacHurryOffer() const {
    RequireSmacProductionSelection();
    const auto money=m_game->GetCanonicalEconomy(m_owner);
    HurryOffer offer{std::nullopt,money.credits,money.pending_debit,{}, {}};
    const auto& item=m_production->current_item;
    const auto& rules=*m_game->GetState()->m_settings.global.GetSmacProductionRules();
    if(m_game->m_simulation->value.movement_capability!=faction::SmacSimulationState::MovementCapability::NoProjectsOrdinaryTerrain) {
        offer.reason="Hurry requires the saved ordinary no-project campaign authority";
    } else if(item.kind!=ProductionItemKind::Unit && item.kind!=ProductionItemKind::Facility) {
        offer.reason="Hurry for this production category is not connected";
    } else if(!(item.kind==ProductionItemKind::Unit ? m_game->IsSmacUnitProductionEligible(*this,item) : m_game->IsSmacFacilityProductionEligible(*this,item))) {
        offer.reason="Current production is not eligible in this Base";
    } else if(rules.retool_exemption<0 || rules.retool_exemption>1000) {
        offer.reason="Hurry requires an active retool exemption in the original supported range";
    } else {
        // Strict admitted profiles reject HURRY overrides, establishing100%.
        // Saved NoProjectsOrdinaryTerrain establishes absent Voice. The actual
        // local ready/Screen controller has no network advance/input lock; the
        // ordinary-local producer and continuation closure are measured in v2.
        offer.context=SmacHurryContext{m_game->GetSmacProductionCost(m_owner,item),m_game->GetSmacProductionMineralFactor(m_owner),
            rules.retool_exemption,100,false,false,m_resource_context->drone_riots};
        if(m_production->hurry_partial_paid) {offer.reason="A partial Hurry was already used this turn";}
        else if(m_production->minerals<0 || m_production->minerals>=offer.context->total_cost) {offer.reason="Hurry requires positive missing minerals";}
        else if(offer.context->drone_riots && item.kind!=ProductionItemKind::Facility) {offer.reason="Drone Riots prevent hurrying this production";}
        else if(money.credits<0 || money.pending_debit<0 || money.credits<money.pending_debit) {offer.reason="Hurry requires a nonnegative available local budget";}
        else if(CalculateSmacHurryQuote(*m_production,*offer.context)<=0) {offer.reason="This production has no positive Hurry quote";}
    }
    types::Buffer snapshot;snapshot.WriteString(SmacSpecialistSelectionState());snapshot.WriteInt(money.credits);snapshot.WriteInt(money.pending_debit);
    snapshot.WriteBool(offer.context.has_value());
    if(offer.context) {
        const auto& c=*offer.context;snapshot.WriteInt(c.total_cost);snapshot.WriteInt(c.mineral_factor);snapshot.WriteInt(c.retool_exemption);
        snapshot.WriteInt(c.hurry_percent);snapshot.WriteBool(c.voice_of_planet);snapshot.WriteBool(c.blocking);snapshot.WriteBool(c.drone_riots);
    }
    offer.expected=snapshot.ToString();return offer;
}
gse::Value* Base::CopySmacHurryPreview(GSE_CALLABLE,int32_t amount) const {
    const auto offer=GetSmacHurryOffer();
    gse::value::object_properties_t fields{
        {"expected_state",VALUE(gse::value::String,,offer.expected)}, {"available",VALUE(gse::value::Bool,,offer.reason.empty() && offer.context.has_value())},
        {"reason",VALUE(gse::value::String,,offer.reason)}, {"credits",VALUE(gse::value::Int,,offer.credits)}, {"reserved",VALUE(gse::value::Int,,offer.reserved)}
    };
    if(offer.reason.empty() && offer.context) {
        const auto quote=CalculateSmacHurryQuote(*m_production,*offer.context);
        if(amount>quote) {THROW("Partial Hurry credits exceed the shown quote");}
        const auto partial=PrepareSmacHurryPayment(*m_production,*offer.context,offer.credits,offer.reserved,false,SmacHurryChoice::Partial,amount);
        fields.emplace("quote",VALUE(gse::value::Int,,quote));fields.emplace("stored",VALUE(gse::value::Int,,m_production->minerals));
        fields.emplace("cost",VALUE(gse::value::Int,,offer.context->total_cost));
        fields.emplace("can_full",VALUE(gse::value::Bool,,int64_t(offer.credits)-offer.reserved>=quote));
        fields.emplace("paid",VALUE(gse::value::Int,,partial.paid));fields.emplace("minerals_after",VALUE(gse::value::Int,,partial.production.minerals));
    }
    return VALUEEXT(gse::value::Object,GSE_CALL,fields);
}
}
}
