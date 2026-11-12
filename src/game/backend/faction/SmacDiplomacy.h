#pragma once
#include "SmacHostileState.h"
#include <stdexcept>
#include <optional>

namespace game::backend::faction {
// Raw directed source DWORDs, with independent lifetimes. Absence of this
// block on migrated campaigns is unavailable history, not a guessed zero.
struct SmacWarHistory {
    using Rows=std::array<std::array<std::optional<uint32_t>,8>,8>;
    Rows field240{},field26c{},field2f4{},field314{};
    void Initialize(int32_t owner,int32_t turn) {
        if(owner<1 || owner>7 || turn<0) {throw std::invalid_argument("War history initialization requires a positive owner and turn");}
        field26c[owner].fill(uint32_t(0));
        if(!turn) {field240[owner].fill(uint32_t(0));field2f4[owner].fill(uint32_t(0));field314[owner].fill(uint32_t(0));}
    }

    bool operator==(const SmacWarHistory& other) const noexcept {
        return field240==other.field240 && field26c==other.field26c && field2f4==other.field2f4 && field314==other.field314;
    }
};
struct SmacWarBreakInputs {
    int32_t actor,target,turn,actor_rank,target_rank,actor_difficulty,target_major_atrocities;
    bool dominance;
};
enum class SmacDiplomaticWarning {None,PactBlocked,Treaty,Truce,BeginVendetta};
//575c00 permission only: raw forward relation, caller guards and player answer.
// Neither current turn nor transition date participates in this decision.
inline SmacDiplomaticWarning SelectSmacDiplomaticWarning(uint32_t relation,uint32_t guards) noexcept {
    if((relation&16u) || !(relation&guards)) {return SmacDiplomaticWarning::None;}
    if(relation&1u) {return SmacDiplomaticWarning::PactBlocked;}
    if(relation&2u) {return SmacDiplomaticWarning::Treaty;}
    if((relation&4u) && !(relation&0x10000u)) {return SmacDiplomaticWarning::Truce;}
    return (guards&0x10000u)?SmacDiplomaticWarning::None:SmacDiplomaticWarning::BeginVendetta;
}
inline bool PassSmacDiplomaticWarning(SmacDiplomaticWarning warning,bool proceed) noexcept {
    return warning!=SmacDiplomaticWarning::PactBlocked && (warning==SmacDiplomaticWarning::None || proceed);
}
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
//51ff10(A,B,-1), human A/computer B. Caller proves the no-propagation
// predicate over living third parties before invoking this bounded recipe.
inline void ApplySmacOrdinaryVendetta(SmacHostileState& state,SmacWarHistory& history,const SmacWarBreakInputs& in) {
    if(in.actor<1 || in.actor>7 || in.target<1 || in.target>7 || in.actor==in.target || in.turn<0
        || in.actor_rank<0 || in.actor_rank>7 || in.target_rank<0 || in.target_rank>7 || in.actor_difficulty<0 || in.actor_difficulty>5
        || !state.owners.count(in.actor) || !state.owners.count(in.target)) {throw std::invalid_argument("Invalid ordinary Vendetta context");}
    auto& own=state.owners.at(in.actor);auto& peer=state.owners.at(in.target);
    auto& forward=own.directions[in.target];auto& reverse=peer.directions[in.actor];
    if(forward.primary&1u) {throw std::invalid_argument("Pact breakup requires its independent transaction");}
    const bool peace=(forward.primary&6u)!=0,exempt=(forward.primary&0x10000u)!=0,skip=(forward.secondary&0x20u)!=0;
    const bool justified=(forward.primary&0x40000u)!=0 || in.target_major_atrocities!=0;
    const bool first_blemish=!exempt && (in.dominance || (in.actor_rank>=6 && in.target_rank<=4)) && !justified;
    const bool second_blemish=in.actor_difficulty && !justified;
    if(!skip && peace && (!history.field2f4[in.actor][in.target] || ((first_blemish || second_blemish) && !history.field314[in.target][in.actor]))) {
        throw std::runtime_error("Vendetta cannot increment unavailable grievance history");
    }
    history.field26c[in.target][in.actor]=0;
    if(!skip && peace) {
        const auto blemish=[&]() {++own.integrity;++*history.field314[in.target][in.actor];};
        if(!exempt) {
            if(own.bookkeeping) {--own.bookkeeping;}
            if((in.dominance || (in.actor_rank>=6 && in.target_rank<=4)) && !justified) {blemish();}
        }
        if(in.actor_difficulty && !justified) {blemish();}
        ++*history.field2f4[in.actor][in.target];
        if(reverse.secondary&0x400u) {reverse.secondary|=0x800u;}
        if(reverse.primary&0x20u) {reverse.secondary|=0x400u;}
        reverse.primary|=0x20u;
    }
    GrantSmacRelation(state,in.actor,in.target,16u);
    if(!skip) {forward.primary|=0x40u;history.field240[in.actor][in.target]=50;forward.transition_turn=in.turn;}
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
