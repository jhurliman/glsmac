#include "SmacWorkshop.h"
#include "SmacPredefinedDesigns.h"
#include "game/backend/faction/SmacTechnologyQuery.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::unit {
namespace {
using Reference = settings::SmacTechnologyReference;
int32_t Prerequisite(const Reference& reference) {
    switch(reference.kind) {
        case Reference::Kind::NONE: return -1;
        case Reference::Kind::DISABLED: return -2;
        case Reference::Kind::TECHNOLOGY_ID:
            if(reference.technology_id && *reference.technology_id>=0 && *reference.technology_id<89) {return *reference.technology_id;}
            break;
        case Reference::Kind::UNKNOWN: break;
    }
    throw std::invalid_argument("Workshop component has an unresolved prerequisite");
}
bool Available(int32_t id,const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner) {
    std::array<int32_t,2> parents{-1,-1};
    if(id>=0 && id<88) {
        const auto& rule=technologies.technologies.at(id);
        for(size_t i=0;i<2;++i) {parents[i]=Prerequisite(technologies.ResolveToken(rule.prerequisites[i]));}
    }
    return faction::QuerySmacTechnology(owner.admission.simulation_id,id,id>=0 && id<88 && owner.owned.at(id) ? uint8_t(1u<<owner.admission.simulation_id):0,parents);
}
void RequireOwner(const faction::SmacFactionTechnologyState& owner) {
    if(owner.admission.simulation_id<1 || owner.admission.simulation_id>7) {throw std::invalid_argument("Workshop requires a player faction");}
}
}
SmacPrototypePrice SmacDesignProductionRows(const SmacCustomDesign& design,
    const SmacPredefinedUnitCatalog& catalog,const SmacPrototypePriceContext& context) {
    if(design.id<0 || design.id>=512) {throw std::invalid_argument("Prototype price requires a canonical design identity");}
    if(design.id<64 || design.prototyped) {return {design.cost,false};}
    if(context.owner_difficulty<0 || context.owner_difficulty>5) {throw std::invalid_argument("Prototype price requires stored owner difficulty");}
    if(context.free_prototype || context.owner_difficulty<=1 || context.queried_base_skunkworks) {return {design.cost,false};}
    if(!context.percentages) {throw std::runtime_error("Legacy production profile has no prototype percentages");}
    const auto& chassis=catalog.chassis.chassis.at(design.chassis);
    const auto domain=static_cast<int32_t>(chassis.triad);
    if(domain<0 || domain>2) {throw std::invalid_argument("Prototype price requires land/sea/air domain");}
    const auto percentage=context.percentages->at(domain);
    if(percentage<0) {throw std::invalid_argument("Negative prototype percentage is outside admitted price domain");}
    const auto basis=SmacPredefinedUnitCatalog::CalculateCost(chassis,catalog.weapons.weapons.at(design.weapon),
        catalog.defenses.defenses.at(design.defense),design.reactor,0,catalog.abilities);
    const int64_t product=int64_t(basis)*percentage;
    if(basis<0 || product>INT32_MAX-50) {throw std::overflow_error("Prototype surcharge exceeds admitted signed32 arithmetic");}
    const auto extra=(product+50)/100;
    if(extra+design.cost>INT32_MAX) {throw std::overflow_error("Prototype rows exceed signed32");}
    return {int32_t(extra+design.cost),extra!=0};
}
std::vector<SmacWorkshopChoice> SmacWorkshopChoices(SmacWorkshopComponent kind,const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner) {
    RequireOwner(owner);
    std::vector<SmacWorkshopChoice> result;
    const auto append=[&](const auto& rows,uint32_t first) {
        for(size_t i=0;i<rows.size();++i) {
            const auto id=Prerequisite(technologies.ResolveToken(rows[i].prerequisite));
            if(id>=-1) {result.push_back({uint32_t(i)+first,Available(id,technologies,owner)});}
        }
    };
    switch(kind) {
        case SmacWorkshopComponent::Chassis: append(catalog.chassis.chassis,0);break;
        case SmacWorkshopComponent::Weapon: append(catalog.weapons.weapons,0);break;
        case SmacWorkshopComponent::Armor: append(catalog.defenses.defenses,0);break;
        case SmacWorkshopComponent::Reactor: append(catalog.reactors.reactors,1);break;
        default: throw std::invalid_argument("Unknown Workshop component picker");
    }
    return result;
}
SmacWorkshopCommit CommitSmacWorkshopFirstDesign(SmacStartupDesignState& state,const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner) {
    RequireOwner(owner);state.Validate(catalog);
    const auto* bank=state.Find(owner.admission.simulation_id);
    if(!bank || catalog.source.ruleset_id!="smac" || draft.selected_design!=2 || draft.chassis || draft.weapon || draft.armor>1
        || draft.reactor!=1 || draft.abilities || draft.name.size()>31 || draft.name.find('\0')!=std::string::npos) {
        throw std::invalid_argument("Workshop first path requires an owned Scout/Infantry Fission draft");
    }
    const auto require=[&](SmacWorkshopComponent kind,uint32_t id) {
        const auto choices=SmacWorkshopChoices(kind,catalog,technologies,owner);
        if(std::none_of(choices.begin(),choices.end(),[&](const auto& row){return row.component_id==id && row.technology_available;})) {
            throw std::invalid_argument("Workshop draft selects unavailable technology");
        }
    };
    require(SmacWorkshopComponent::Chassis,draft.chassis);require(SmacWorkshopComponent::Weapon,draft.weapon);
    require(SmacWorkshopComponent::Armor,draft.armor);require(SmacWorkshopComponent::Reactor,draft.reactor);
    const auto matches=[&](uint32_t ch,uint32_t we,uint32_t ar,uint8_t reactor,uint32_t abilities) {
        return ch==draft.chassis && we==draft.weapon && ar==draft.armor && reactor==draft.reactor && abilities==draft.abilities;
    };
    for(const auto& design:catalog.units) {
        if(design.source_id<64 && design.active && matches(design.chassis_id,design.weapon_id,design.defense_id,design.reactor,design.abilities)
            && Available(Prerequisite(design.prerequisite),technologies,owner)) {
            if(design.faction_exclusion&(1u<<owner.admission.simulation_id)) {throw std::runtime_error("Workshop intrinsic reactivation needs obsolescence bookkeeping");}
            const auto& name=catalog.source.units.at(design.source_id).name;
            return {int32_t(design.source_id),!draft.name.empty() && draft.name!=name};
        }
    }
    // No custom rename/replacement is inferred from the four native controls.
    for(const auto& design:bank->designs) {
        if(design.active && design.name && matches(design.chassis,design.weapon,design.defense,design.reactor,design.abilities)
            && (!draft.name.empty() ? design.name==draft.name : !design.explicitly_named)) {
            if(design.faction_exclusion&(1u<<owner.admission.simulation_id)) {throw std::runtime_error("Workshop reactivation needs obsolescence bookkeeping");}
            return {design.id,false};
        }
    }
    if(!bank->designs.empty() || draft.armor!=1) {throw std::runtime_error("Workshop occupied-bank allocation requires its replacement policy");}
    SmacDesignComponents components{};
    const auto& chassis=catalog.chassis.chassis.at(0);const auto& weapon=catalog.weapons.weapons.at(0);
    components.chassis[0]={chassis.speed,int32_t(chassis.triad),chassis.missile,chassis.cargo,chassis.cost};
    components.weapons[0]={weapon.offense,int32_t(weapon.mode),weapon.cost};
    for(size_t i=0;i<2;++i) {const auto& armor=catalog.defenses.defenses.at(i);components.armor[i]={armor.rating,armor.cost};}
    SmacOriginalDesignPool pool{};BuildSmacFissionDesign(pool,0,components,0,0,1,0,uint8_t(1u<<owner.admission.simulation_id));
    const auto& armor=catalog.defenses.defenses.at(1);
    const SmacEarlyDesignNames names{armor.short_name,chassis.names[2].name,chassis.names[3].name,"","",armor.rating};
    auto automatic=NameSmacOpeningDesign(names,owner.admission.simulation_id,0,1);automatic.resize(std::min<size_t>(automatic.size(),31));
    SmacCustomDesign design;design.id=owner.admission.simulation_id*64+63;design.defense=1;
    design.cost=pool[0][41];design.plan=pool[0][42];design.carry=pool[0][40];
    design.name=draft.name.empty()?automatic:draft.name;design.explicitly_named=*design.name!=automatic;
    auto next=state;
    for(auto& target:next.owners) {if(target.simulation_id==owner.admission.simulation_id) {target.designs.push_back(design);}}
    next.Validate(catalog);state.owners.swap(next.owners);
    return {design.id,false};
}
}
