#pragma once

#include "ProductionState.h"

namespace game::backend::base {

// Resolved inputs of RE-SMAC-HURRY-001. This is the admitted quote/payment
// fragment, not the original dialog's complete admission or network protocol.
struct SmacHurryContext {
    int32_t total_cost;
    int32_t mineral_factor;
    int32_t retool_exemption;
    int32_t hurry_percent;
    bool voice_of_planet;
    bool blocking;
    bool drone_riots;
};
enum class SmacHurryChoice { None, Full, Partial };
struct SmacHurryResult {
    ProductionState production;
    int32_t quote;
    int32_t paid;
    int32_t treasury;
    int32_t reserved;
    bool purchased;
};

int32_t CalculateSmacHurryQuote(const ProductionState&,const SmacHurryContext&);
// Full affordability belongs to admission. Partial arithmetic clamps only to
// available budget; the live port UI separately bounds entered credits to quote.
SmacHurryResult PrepareSmacHurryPayment(const ProductionState&,const SmacHurryContext&,
    int32_t treasury,int32_t reserved,bool reserve_payment,SmacHurryChoice,int32_t entered);

}
