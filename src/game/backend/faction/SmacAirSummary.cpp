#include "SmacAirSummary.h"
#include "game/backend/unit/SmacStartupDesignState.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::faction {
namespace {
int32_t SignedWord(int64_t value) noexcept {
    const auto word=static_cast<uint32_t>(value);
    return word>INT32_MAX ? static_cast<int32_t>(int64_t(word)-INT64_C(4294967296)):static_cast<int32_t>(word);
}
}
int32_t SmacAirAttackEstimate(int32_t offense,int32_t peer_defense,
    const std::optional<std::array<int32_t,2>>& ratio,int32_t psi_percent,bool dream_twister) {
    if(offense>=0) {return offense;}
    if(!ratio || (*ratio)[0]<=0 || (*ratio)[1]<=0) {throw std::invalid_argument("PSI air evaluation requires retained positive land ratios");}
    int32_t value=SignedWord(int64_t(peer_defense)*(*ratio)[0])/(*ratio)[1];
    if(psi_percent) {value=SignedWord(int64_t(value)*(int64_t(100)+psi_percent))/100;}
    if(dream_twister) {value=SignedWord(int64_t(value)+value/2);}
    return value;
}
SmacAirSummary CountSmacAirSummary(const std::vector<SmacAirSummaryUnit>& units,const SmacAirSummaryContext& context) {
    if(context.owner<1 || context.owner>7 || units.size()>2048) {throw std::invalid_argument("Air summary requires an owning faction and bounded live Unit pool");}
    SmacAirSummary result;
    for(const auto& unit:units) {
        if(unit.owner<0 || unit.owner>7 || unit.domain<0 || unit.domain>2 || unit.plan<0 || unit.plan>255) {
            throw std::invalid_argument("Air summary contains an invalid source Unit projection");
        }
        if(unit.owner==context.owner) {
            if(unit.domain==2) {result.own_plan0+=unit.plan==0;result.own_plan4+=unit.plan==4;}
        } else {
            const auto relation=context.relations.at(unit.owner);
            if((relation&8u) && (!(relation&3u) || (relation&0xc20u)) && unit.domain==2 && unit.plan==0 && !unit.special
                && SmacAirAttackEstimate(unit.offense,context.peer_defense,context.land_psi_ratio,context.psi_percent,context.dream_twister)>=context.peer_defense) {
                ++result.foreign_threats;
            }
        }
    }
    return result;
}
SmacAirSummaryUnit ProjectSmacAirSummaryUnit(const unit::SmacStartupDesignState& designs,
    const unit::SmacPredefinedUnitCatalog& catalog,int32_t design,int32_t owner) {
    if(design<0 || design>=512 || owner<0 || owner>7) {throw std::invalid_argument("Air summary Unit has no valid source identity");}
    uint32_t chassis,weapon;int32_t plan;
    if(designs.native_registry) {
        const auto& row=designs.native_registry->at(design);chassis=row[36];weapon=row[37];plan=row[42];
    } else if(design<64) {
        const auto found=std::find_if(catalog.units.begin(),catalog.units.end(),[&](const auto& row){return row.source_id==uint32_t(design);});
        if(found==catalog.units.end()) {throw std::invalid_argument("Air summary intrinsic has no frozen definition");}
        const auto row=designs.EffectiveIntrinsic(*found);chassis=row.chassis_id;weapon=row.weapon_id;plan=row.plan;
    } else {
        const auto row=designs.RetainedCustomDesign(design,catalog);chassis=row.chassis;weapon=row.weapon;plan=row.plan;
    }
    const auto& row=catalog.chassis.chassis.at(chassis);
    const auto power=static_cast<uint32_t>(catalog.weapons.weapons.at(weapon).offense)&255u;
    return {owner,static_cast<int32_t>(row.triad),plan,power<128 ? int32_t(power):int32_t(power)-256,row.missile};
}
}
