#pragma once
#include "SmacHostileState.h"
#include <stdexcept>

namespace game::backend::faction {
// Exact mutual primitive572480. Preserve every unnamed raw bit independently.
inline void GrantSmacRelation(SmacHostileState& state,int32_t actor,int32_t target,uint32_t mask) {
    if(actor<1 || actor>7 || target<1 || target>7 || actor==target || !state.owners.count(actor) || !state.owners.count(target)) {
        throw std::invalid_argument("Diplomacy requires two distinct admitted positive owners");
    }
    auto& forward=state.owners.at(actor).directions.at(target);
    auto& reverse=state.owners.at(target).directions.at(actor);
    const auto original=forward.primary;
    if(mask&7u) {forward.primary&=0xffee7fafu;reverse.primary&=0xffee7fafu;}
    if(mask&1u) {mask|=2u;}
    if(mask&0x10u) {
        if(original&7u) {forward.paired_b=reverse.paired_b=0;}
        mask|=0x80000108u;forward.primary&=0xfaeef378u;reverse.primary&=0xfaeef378u;
    }
    if(mask&0x17u) {forward.primary&=0xff7fffffu;reverse.primary&=0xff7fffffu;}
    forward.primary|=mask;reverse.primary|=mask;
}
inline uint32_t SmacPhysicalContactMask(int32_t sunspots,bool valid_coordinates) noexcept {
    return (sunspots>0?0x80000008u:0x88000008u)|(valid_coordinates?0x40000000u:0u);
}
// Ordinary local physical encounter574e80; request dispatch is an independent
// UI/negotiation stage. Positive sunspots deliberately omit recognition08000000.
inline void CompleteSmacPhysicalContact(SmacHostileState& state,int32_t actor,int32_t target,int32_t turn,int32_t sunspots,bool valid_coordinates) {
    if(turn<0) {throw std::invalid_argument("Contact requires a nonnegative turn");}
    if(!turn) {return;}
    GrantSmacRelation(state,actor,target,sunspots>0?0x80000008u:0x88000008u);
    if(valid_coordinates) {GrantSmacRelation(state,actor,target,0x40000000u);}
}
}
