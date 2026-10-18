#include "SmacHurry.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace game::backend::base {
namespace {
int32_t Checked(int64_t value) {
    if(value<INT32_MIN || value>INT32_MAX) {throw std::invalid_argument("Hurry arithmetic exceeds its checked signed32 domain");}
    return static_cast<int32_t>(value);
}
}
int32_t CalculateSmacHurryQuote(const ProductionState& state,const SmacHurryContext& context) {
    state.Validate();
    if(context.total_cost<0 || context.mineral_factor<1) {throw std::invalid_argument("Hurry requires an actual nonnegative cost and mineral factor");}
    const auto stored=Checked(state.minerals);const auto missing=Checked(int64_t(context.total_cost)-stored);
    auto quote=Checked(2*int64_t(missing));
    if(state.current_item.kind==ProductionItemKind::Unit) {quote=Checked(int64_t(quote)+Checked(int64_t(missing)*missing)/20);}
    if(state.current_item.kind==ProductionItemKind::Project) {
        quote=Checked(2*int64_t(quote));
        if(stored<Checked(4*int64_t(context.mineral_factor))) {quote=Checked(2*int64_t(quote));}
    }
    if(stored<context.retool_exemption) {quote=Checked(2*int64_t(quote));}
    if(context.voice_of_planet) {quote=Checked(2*int64_t(quote));}
    return Checked(int64_t(Checked(int64_t(quote)*context.hurry_percent))/100);
}
SmacHurryResult PrepareSmacHurryPayment(const ProductionState& state,const SmacHurryContext& context,
    int32_t treasury,int32_t reserved,bool reserve_payment,SmacHurryChoice choice,int32_t entered) {
    if(choice!=SmacHurryChoice::None && choice!=SmacHurryChoice::Full && choice!=SmacHurryChoice::Partial) {throw std::invalid_argument("Unknown Hurry choice");}
    SmacHurryResult result{state,CalculateSmacHurryQuote(state,context),0,treasury,reserved,false};
    if(choice==SmacHurryChoice::None || result.quote<=0 || context.blocking || state.hurry_partial_paid
        || (context.drone_riots && state.current_item.kind!=ProductionItemKind::Facility)) {return result;}
    if(choice==SmacHurryChoice::Full) {result.paid=result.quote;result.production.minerals=context.total_cost;}
    else {
        const auto available=std::max(Checked(int64_t(treasury)-reserved),0);
        result.paid=std::min(std::max(entered,0),available);
        const auto missing=Checked(int64_t(context.total_cost)-Checked(state.minerals));
        result.production.minerals=Checked(int64_t(Checked(state.minerals))+Checked(int64_t(missing)*result.paid)/result.quote);
        result.production.hurry_partial_paid=true;
    }
    result.production.baseline_item=result.production.current_item;
    result.production.baseline_minerals=result.production.minerals;
    if(reserve_payment) {result.reserved=Checked(int64_t(reserved)+result.paid);}
    else {result.treasury=Checked(int64_t(treasury)-result.paid);}
    result.purchased=true;result.production.Validate();return result;
}
}
