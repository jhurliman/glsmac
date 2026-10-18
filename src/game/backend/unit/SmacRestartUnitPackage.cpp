#include "SmacRestartUnitPackage.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace game::backend::unit {
namespace {
int32_t Checked(int64_t value) {
    if(value<INT32_MIN || value>INT32_MAX) {throw std::overflow_error("Restart package score exceeds signed32");}
    return static_cast<int32_t>(value);
}
bool Prerequisite(const settings::SmacTechnologyReference& reference,
    const faction::SmacFactionTechnologyState& owner, const settings::SmacTechnologyRules& technologies) {
    using Kind=settings::SmacTechnologyReference::Kind;
    if(reference.kind==Kind::NONE || reference.kind==Kind::DISABLED) {
        if(reference.technology_id) {throw std::invalid_argument("Restart prerequisite sentinel has an ID");}
        return reference.kind==Kind::NONE;
    }
    if(reference.kind!=Kind::TECHNOLOGY_ID || !reference.technology_id || *reference.technology_id<0 || *reference.technology_id>88) {
        throw std::invalid_argument("Restart prerequisite is unresolved");
    }
    const auto id=*reference.technology_id;
    if(id==88) {return false;}
    const auto resolved=technologies.ResolveProductionPrerequisite(technologies.technologies[id].key);
    using Status=settings::SmacProductionPrerequisite::Status;
    if(resolved.status==Status::UNAVAILABLE || resolved.technology_id!=id) {
        throw std::invalid_argument("Restart prerequisite definition is unresolved");
    }
    return resolved.status==Status::TECHNOLOGY_ID && owner.owned[id];
}
}

SmacRestartUnitPackage SmacRestartUnitPackage::Select(int32_t turn, const SmacStartupDesignState& designs,
    const SmacPredefinedUnitCatalog& catalog, const faction::SmacFactionTechnologyState& owner,
    const settings::SmacTechnologyRules& technologies) {
    if(turn<1 || turn>100) {throw std::invalid_argument("Ordinary restart package requires turn 1..100");}
    technologies.Validate();designs.Validate(catalog);
    const auto id=owner.admission.simulation_id;
    const auto* custom=designs.Find(id);
    if(id<1 || id>7 || !custom) {throw std::invalid_argument("Restart owner has no saved catalog block");}
    if(catalog.source.ruleset_id!="smac" || catalog.chassis.ruleset_id!="smac" || catalog.weapons.ruleset_id!="smac"
        || catalog.defenses.ruleset_id!="smac" || catalog.reactors.ruleset_id!="smac" || catalog.abilities.ruleset_id!="smac"
        || catalog.units.size()<3 || catalog.units.size()>64 || catalog.chassis.chassis.size()!=9
        || catalog.weapons.weapons.size()!=21 || catalog.defenses.defenses.size()!=10 || catalog.reactors.reactors.size()!=4
        || catalog.abilities.abilities.size()!=24) {throw std::invalid_argument("Unsupported restart component catalog");}
    const auto components=[&](uint32_t chassis,uint32_t weapon,uint32_t armor) {
        if(chassis>=catalog.chassis.chassis.size() || weapon>=catalog.weapons.weapons.size() || armor>=catalog.defenses.defenses.size()) {
            throw std::invalid_argument("Restart design component is outside its catalog");
        }
        const auto& c=catalog.chassis.chassis[chassis];
        const auto a=catalog.weapons.weapons[weapon].offense,d=catalog.defenses.defenses[armor].rating;
        // Original component loads are signed bytes; speed is an unsigned byte.
        // Do not silently narrow richer parser metadata or emulate overflow.
        if(a<-128 || a>127 || d<-128 || d>127 || c.speed<0 || c.speed>255
            || static_cast<int32_t>(c.triad)<0 || static_cast<int32_t>(c.triad)>2) {
            throw std::invalid_argument("Restart component exceeds the admitted byte domain");
        }
    };
    for(size_t i=0;i<catalog.units.size();++i) {
        const auto& row=catalog.units[i];
        if(row.source_id!=i || row.reactor<1 || row.reactor>4 || row.abilities>0xffffff) {
            throw std::invalid_argument("Invalid positional restart predefined record");
        }
        components(row.chassis_id,row.weapon_id,row.defense_id);
    }
    using namespace loader::txt;
    if(catalog.weapons.weapons[catalog.units[0].weapon_id].mode!=WeaponMode::Colonist
        || catalog.weapons.weapons[catalog.units[1].weapon_id].mode!=WeaponMode::Terraformer
        || catalog.chassis.chassis[catalog.units[0].chassis_id].triad!=ChassisTriad::Land
        || catalog.chassis.chassis[catalog.units[1].chassis_id].triad!=ChassisTriad::Land) {
        throw std::invalid_argument("Restart grants do not resolve to ground Pod and Former");
    }
    SmacRestartUnitPackage result;
    std::array<int32_t,3> best{-1,-1,-1};
    const auto consider=[&](int32_t design,uint32_t chassis,uint32_t weapon,uint32_t armor) {
        components(chassis,weapon,armor);
        const auto& c=catalog.chassis.chassis[chassis];const auto raw=catalog.weapons.weapons[weapon].offense;
        if(c.triad!=ChassisTriad::Land || raw==0) {return;}
        // Nonzero-turn initialization clears the Psi strength caches. Only
        // negative components become zero here; conventional values survive.
        const auto a=std::max(0,raw),d=Checked(2LL*std::max(0,catalog.defenses.defenses[armor].rating)),s=c.speed;
        const std::array<int32_t,3> scores{Checked(8LL*d-a),Checked(2LL*d+4LL*a+s),Checked(4LL*s-d-a)};
        if(scores[0]>best[0]) {best[0]=scores[0];result.defender=design;}
        if(scores[1]>best[1]) {best[1]=scores[1];result.balanced=design;}
        if(s>1 && scores[2]>best[2]) {best[2]=scores[2];result.mobile=design;}
    };
    // Positional predefined slots precede this owner's sorted block. All other
    // slots in the original 512-slot scan are empty or foreign and cannot win.
    for(const auto& row:catalog.units) {
        if(row.active && !(row.faction_exclusion&(uint32_t(1)<<id)) && Prerequisite(row.prerequisite,owner,technologies)) {
            consider(row.source_id,row.chassis_id,row.weapon_id,row.defense_id);
        }
    }
    for(const auto& row:custom->designs) {
        if(row.active && !(row.faction_exclusion&(uint32_t(1)<<id))) {consider(row.id,row.chassis,row.weapon,row.defense);}
    }
    // Pod/Former grants bypass candidate eligibility, including technologies.
    result.creation_order={0,1,0,result.defender};
    if(turn>20 && result.mobile) {result.creation_order.push_back(*result.mobile);}
    if(turn>40 && result.balanced) {result.creation_order.push_back(*result.balanced);}
    if(turn>60) {result.creation_order.push_back(result.defender);}
    if(turn>80 && result.balanced) {result.creation_order.push_back(*result.balanced);}
    return result;
}

}
