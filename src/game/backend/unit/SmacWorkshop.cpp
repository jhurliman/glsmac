#include "SmacWorkshop.h"
#include "SmacPredefinedDesigns.h"
#include "game/backend/faction/SmacTechnologyQuery.h"
#include <algorithm>
#include <stdexcept>
#include <utility>

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
namespace {
void RequireComponents(const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog) {
    if(draft.chassis>=catalog.chassis.chassis.size() || draft.weapon>=catalog.weapons.weapons.size()
        || draft.armor>=catalog.defenses.defenses.size() || draft.reactor<1 || draft.reactor>catalog.reactors.reactors.size()
        || draft.chassis>255 || draft.weapon>255 || draft.armor>255 || draft.abilities>0xffffff || catalog.abilities.abilities.size()!=24) {throw std::invalid_argument("Invalid Workshop components");}
}
uint32_t AbilityFlags(const std::string& text) {
    if(text.empty() || text.size()>32 || text.find_first_not_of("01")!=std::string::npos) {throw std::invalid_argument("Invalid Workshop ability flags");}
    uint32_t flags=0;for(const char bit:text) {flags=(flags<<1)|uint32_t(bit-'0');}return flags;
}
std::pair<uint8_t,uint8_t> Classify(const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog) {
    const auto& ch=catalog.chassis.chassis.at(draft.chassis);const auto& we=catalog.weapons.weapons.at(draft.weapon);
    const int32_t offense=we.offense,defense=catalog.defenses.defenses.at(draft.armor).rating,speed=ch.speed,mode=int32_t(we.mode);
    if(ch.missile) {return offense>=99 ? std::pair<uint8_t,uint8_t>{5,14}:std::pair<uint8_t,uint8_t>{0,13};}
    if(mode>=7) {return {uint8_t(mode),uint8_t(mode+32)};}
    if(ch.triad==loader::txt::ChassisTriad::Sea) {return {6,6};}
    if(ch.triad==loader::txt::ChassisTriad::Air) {return (draft.abilities&32) ? std::pair<uint8_t,uint8_t>{4,9}:std::pair<uint8_t,uint8_t>{0,8};}
    uint8_t plan=defense>1 ? 2:0;
    if(offense>=defense && plan==2) {plan=1;}
    if((speed>1 && offense>defense) || (draft.abilities&0x8018)) {plan=0;}
    uint8_t category=3;
    if(defense>offense) {category=2;}if(int64_t(offense)>2*int64_t(defense)) {category=1;}
    if(speed>=2) {category=4;}if(speed>=3) {category=5;}
    if(offense==1 && defense==1) {category=10;if(speed>1) {plan=3;}if(draft.abilities&0x400000) {plan=2;}}
    return {plan,category};
}
}
bool SmacWorkshopAbilityCompatible(uint32_t ability,const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog) {
    RequireComponents(draft,catalog);if(ability>=24) {throw std::invalid_argument("Workshop ability outside its24-bit mask");}
    const auto& ch=catalog.chassis.chassis.at(draft.chassis);const auto& we=catalog.weapons.weapons.at(draft.weapon);
    const auto defense=catalog.defenses.defenses.at(draft.armor).rating;
    const auto flags=AbilityFlags(catalog.abilities.abilities.at(ability).flags);
    const auto domain=int32_t(ch.triad);if(domain<0 || domain>2) {throw std::invalid_argument("Invalid Workshop domain");}
    if(ch.missile || !(flags&(1u<<domain))) {return false;}
    const auto kind=we.offense!=0 ? 8u:we.mode==loader::txt::WeaponMode::Terraformer ? 16u:32u;
    if(!(flags&kind)) {return false;}
    if(we.mode==loader::txt::WeaponMode::InfoWar && (flags&64)) {return false;}
    if((we.offense<0 || defense<0) && (flags&128)) {return false;}
    if((flags&256) && we.mode!=loader::txt::WeaponMode::Transport) {return false;}
    return !(flags&512) || ch.speed<=1;
}
SmacWorkshopAbilitySelection CleanSmacWorkshopAbilities(const SmacWorkshopDraft& draft,
    std::array<int32_t,2> slots,bool second_ability,const SmacPredefinedUnitCatalog& catalog) {
    RequireComponents(draft,catalog);if(catalog.chassis.chassis.at(draft.chassis).missile) {return {0,{-1,-1}};}
    auto cleaned=draft;for(uint32_t bit=0;bit<24;++bit) {
        const uint32_t flag=1u<<bit;if(!(cleaned.abilities&flag)) {continue;}
        unsigned count=0;for(auto mask=cleaned.abilities;mask;mask>>=1) {count+=mask&1;}
        if((flag==128 && (cleaned.abilities&64)) || !SmacWorkshopAbilityCompatible(bit,cleaned,catalog)
            || count>2 || (count==2 && !second_ability)) {
            cleaned.abilities&=~flag;slots={-1,-1};size_t next=0;
            for(uint32_t selected=0;selected<24 && next<2;++selected) {if(cleaned.abilities&(1u<<selected)) {slots[next++]=int32_t(selected);}}
        }
    }return {cleaned.abilities,slots};
}
void BuildSmacWorkshopRecord(SmacOriginalDesignPool& pool,size_t index,const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner) {
    RequireOwner(owner);RequireComponents(draft,catalog);
    if(index<64 || index>=512 || int32_t(index/64)!=owner.admission.simulation_id) {throw std::invalid_argument("Workshop record requires its owner's custom slot");}
    const auto& chassis=catalog.chassis.chassis.at(draft.chassis);const auto& weapon=catalog.weapons.weapons.at(draft.weapon);
    const auto& armor=catalog.defenses.defenses.at(draft.armor);
    const auto cost=SmacPredefinedUnitCatalog::CalculateCost(chassis,weapon,armor,draft.reactor,draft.abilities,catalog.abilities);
    const auto carry=weapon.mode==loader::txt::WeaponMode::Transport ? SmacPredefinedUnitCatalog::CalculateCapacity(chassis,draft.reactor,draft.abilities):0;
    const auto [plan,category]=Classify(draft,catalog);
    std::array<bool,3> covered{};const size_t first=size_t(owner.admission.simulation_id)*64;
    for(size_t i=0;i<512;++i) {
        if(i>=64 && (i<first || i>=first+64)) {continue;}
        const auto& row=pool[i];const uint16_t flags=uint16_t(row[48])|(uint16_t(row[49])<<8);
        if(!(flags&1)) {continue;}
        if(i<64) {const int16_t prerequisite=int16_t(uint16_t(row[50])|(uint16_t(row[51])<<8));if(!Available(prerequisite,technologies,owner)) {continue;}}
        else if(!(flags&4)) {continue;}
        const auto& prior_chassis=catalog.chassis.chassis.at(row[36]);
        const auto& prior_weapon=catalog.weapons.weapons.at(row[37]);const auto& prior_armor=catalog.defenses.defenses.at(row[38]);
        covered[0]=covered[0] || row[37]==draft.weapon || (weapon.offense>0 && prior_weapon.offense>=weapon.offense);
        covered[1]=covered[1] || row[38]==draft.armor || (armor.rating>0 && prior_armor.rating>=armor.rating);
        covered[2]=covered[2] || row[36]==draft.chassis || (prior_chassis.triad==chassis.triad && prior_chassis.speed>=chassis.speed);
    }
    const uint16_t flags=std::all_of(covered.begin(),covered.end(),[](bool known){return known;}) ? (pool[first+63][48]&16 ? 0x115:0x105):1;
    auto record=pool[index];for(unsigned i=0;i<4;++i) {record[32+i]=uint8_t(draft.abilities>>(8*i));}
    record[36]=uint8_t(draft.chassis);record[37]=uint8_t(draft.weapon);record[38]=uint8_t(draft.armor);record[39]=draft.reactor;
    record[40]=uint8_t(carry);record[41]=uint8_t(cost);record[42]=plan;record[43]=category;
    record[44]=0;record[45]=uint8_t(1u<<owner.admission.simulation_id);record[46]=255;record[48]=uint8_t(flags);record[49]=uint8_t(flags>>8);
    pool[index]=record;
}
std::string NameSmacWorkshopDesign(const SmacOriginalDesignPool& pool,size_t index,const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,int32_t owner) {
    RequireComponents(draft,catalog);
    const auto& chassis=catalog.chassis.chassis.at(draft.chassis);const auto& weapon=catalog.weapons.weapons.at(draft.weapon);
    const auto defense=catalog.defenses.defenses.at(draft.armor).rating;const auto offense=weapon.offense,speed=chassis.speed,mode=int32_t(weapon.mode);
    if(!catalog.workshop_name_labels || owner<1 || owner>7 || index<64 || index>=512 || int32_t(index/64)!=owner
        || chassis.triad!=loader::txt::ChassisTriad::Land || chassis.missile || offense<0) {throw std::invalid_argument("Workshop naming requires initialized conventional land text and context");}
    const auto& current=pool[index];bool same_tuple=(current[48]&1) && current[36]==draft.chassis && current[37]==draft.weapon && current[38]==draft.armor && current[39]==draft.reactor;
    for(unsigned i=0;i<4;++i) {same_tuple=same_tuple && current[32+i]==uint8_t(draft.abilities>>(8*i));}
    if(!same_tuple) {throw std::invalid_argument("Workshop naming requires its completed design record");}
    for(const auto& label:*catalog.workshop_name_labels) {if(label.empty() || label.size()>128 || label.find('\0')!=std::string::npos) {throw std::invalid_argument("Invalid Workshop naming text");}}
    const auto& labels=*catalog.workshop_name_labels;const auto& names=chassis.names;
    std::string prefix;for(unsigned bit=0;bit<24;++bit) {
        const auto& text=catalog.abilities.abilities.at(bit).abbreviation;
        if(!(draft.abilities&(1u<<bit)) || text.empty() || bit==15) {continue;}
        if(bit==3 && speed==1 && offense>=defense && !(draft.abilities&0x8000)) {continue;}
        prefix=text+' ';break;
    }
    std::string full=prefix,short_name=prefix;bool compact=false,elite=false;unsigned variant=((draft.reactor-1)^owner)&1;
    if(mode<3) {const auto threshold=defense<=offense ? 3:6;elite=std::min(defense,offense)>threshold && !names[4].name.empty() && !names[5].name.empty();}
    if(speed>=std::max(offense,defense) && mode<3) {
        compact=true;variant=speed>1;
        if(draft.abilities&0x400000) {
            if(!full.empty()) {full.pop_back();}short_name.resize(std::min(short_name.size(),full.empty()?size_t(0):full.size()-1));
        }else {const auto& label=labels[offense>=2?1:0];full+=label;short_name+=label;}
    }
    if(defense>offense && mode<3 && !(draft.abilities&0x8000)) {
        if(!compact) {const auto& text=catalog.defenses.defenses.at(draft.armor).short_name;full+=text;short_name+=text;variant^=unsigned(defense)&1;}
        full+=' '+names[2].name;
        short_name+=' '+(compact && (index&1) ? names[elite?5:2].name:names[3].name);
    } else {
        if(mode>=3 && draft.chassis!=0) {
            if(draft.reactor>1) {const auto& text=catalog.reactors.reactors.at(draft.reactor-1).short_name;full+=text+' ';short_name+=text+' ';}
            full+=names[0].name+' ';short_name+=names[1].name+' ';
        }
        if(!compact) {full+=weapon.short_name;short_name+=weapon.short_name;variant^=unsigned(offense)&1;}
        if(mode<3) {
            if(draft.abilities&0x8000) {full+=' '+labels[2];short_name+=' '+labels[3];}
            else if((draft.abilities&8) && speed==1 && offense>defense) {full+=' '+labels[4];short_name+=' '+labels[5];}
            else {full+=' '+names[elite?4:0].name;short_name+=' '+names[1].name;}
        }
    }
    const bool identical=full==short_name;const auto& marker=labels[6];
    const auto bump=[&](std::string& text) {
        const auto at=text.rfind(marker);
        if(at!=std::string::npos && text.size()-at<=marker.size()+1 && text.back()>='2' && text.back()<='8') {++text.back();}
        else {text+=' '+marker+'2';}
    };
    for(;;) {
        bool changed=false;
        for(size_t i=0;i<512;++i) {
            if(i==index || (i>=64 && (i<size_t(owner)*64 || i>=size_t(owner+1)*64))) {continue;}
            const auto& row=pool[i];if(std::equal(current.begin()+32,current.begin()+40,row.begin()+32)) {continue;}
            const std::string name(row.begin(),std::find(row.begin(),row.begin()+32,0));
            if(name==full) {if(!identical) {variant=1;}bump(full);changed=true;break;}
            if(!identical && name==short_name) {variant=0;bump(short_name);changed=true;break;}
        }
        if(!changed) {return variant ? short_name:full;}
    }
}
SmacWorkshopSlot ChooseSmacWorkshopSlot(const SmacOriginalDesignPool& pool,const SmacWorkshopDraft& draft,
    const faction::SmacDesignCounts& units,const faction::SmacDesignCounts& production,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner) {
    RequireOwner(owner);const int32_t first=owner.admission.simulation_id*64,limit=first+64;
    const int32_t original=draft.selected_design;
    if(original<0 || original>=512 || (original>=64 && (original<first || original>=limit))
        || draft.chassis>255 || draft.weapon>255 || draft.armor>255 || draft.reactor<1 || draft.reactor>4 || draft.abilities>0xffffff) {
        throw std::invalid_argument("Invalid Workshop slot selection or tuple");
    }
    const auto active=[&](int32_t i){return (pool.at(i)[48]&1)!=0;};
    const auto matches=[&](int32_t i) {
        const auto& row=pool.at(i);
        if(row[36]!=draft.chassis || row[37]!=draft.weapon || row[38]!=draft.armor || row[39]!=draft.reactor) {return false;}
        for(unsigned k=0;k<4;++k) {if(row[32+k]!=uint8_t(draft.abilities>>(8*k))) {return false;}}return true;
    };
    const uint8_t mask=uint8_t(1u<<owner.admission.simulation_id);
    const auto excluded=[&](int32_t i){return (pool.at(i)[44]&mask)!=0;};
    for(int32_t i=0;i<limit;++i) {
        if(i>=64 && i<first) {continue;}if(!active(i) || !matches(i)) {continue;}
        if(i<64) {const auto& row=pool.at(i);const auto prerequisite=int16_t(uint16_t(row[50])|(uint16_t(row[51])<<8));
            if(!Available(prerequisite,technologies,owner)) {continue;}}
        return {i,SmacWorkshopSlotKind::Alias,false};
    }
    int32_t selected=original;
    if(selected<64) {
        if(matches(selected)) {return {selected,SmacWorkshopSlotKind::IntrinsicFailure,false};}
        for(int32_t i=first;i<limit;++i) {
            if(excluded(i) && units.at(i)==0 && production.at(i)==0) {selected=i;break;}
            if(!active(i)) {selected=i;}
        }
        if(selected<64) {return {selected,SmacWorkshopSlotKind::IntrinsicFailure,false};}
    }
    bool unused=false,empty=false;
    if(active(selected) && !matches(selected)) {
        for(int32_t i=first;i<limit;++i) {
            if(!active(i)) {selected=i;empty=true;break;}
            if(excluded(i) && units.at(i)==0) {
                if(production.at(i)==0) {selected=i;unused=true;}
                else if(!unused && pool.at(i)[43]==pool.at(selected)[43]) {selected=i;}
            }
        }
    }
    return {selected,SmacWorkshopSlotKind::Build,!empty && !unused && active(selected) && units.at(selected)>0};
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
std::string SmacWorkshopDesignName(const SmacCustomDesign& design,const SmacPredefinedUnitCatalog& catalog,int32_t owner) {
    if(design.name) {return *design.name;}
    if(owner<1 || owner>7 || design.chassis || design.weapon || design.defense>1 || design.reactor!=1 || design.abilities) {
        throw std::invalid_argument("Unnamed Workshop design requires its installed-language naming producer");
    }
    if(design.defense==0) {return catalog.source.units.at(2).name;}
    const auto& armor=catalog.defenses.defenses.at(1);const auto& chassis=catalog.chassis.chassis.at(0);
    const SmacEarlyDesignNames names{armor.short_name,chassis.names[2].name,chassis.names[3].name,"","",armor.rating};
    auto result=NameSmacOpeningDesign(names,owner,0,1);result.resize(std::min<size_t>(result.size(),31));return result;
}
std::optional<SmacWorkshopCommit> ReuseSmacWorkshopDesign(SmacStartupDesignState& state,
    const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner) {
    RequireOwner(owner);state.Validate(catalog);
    if(draft.name.size()>31 || draft.name.find('\0')!=std::string::npos || draft.chassis>=catalog.chassis.chassis.size()
        || draft.weapon>=catalog.weapons.weapons.size() || draft.armor>=catalog.defenses.defenses.size()
        || draft.reactor<1 || draft.reactor>catalog.reactors.reactors.size() || draft.abilities>0xffffff) {
        throw std::invalid_argument("Invalid Workshop alias draft");
    }
    const auto* bank=state.Find(owner.admission.simulation_id);
    if(!bank) {throw std::invalid_argument("Workshop alias has no owner bank");}
    bool selected_named=false;
    if(draft.selected_design<64) {
        const auto selected=std::find_if(catalog.units.begin(),catalog.units.end(),[&](const auto& row){return int32_t(row.source_id)==draft.selected_design;});
        if(selected==catalog.units.end() || !state.EffectiveIntrinsic(*selected).active) {throw std::invalid_argument("Workshop alias has no active selected intrinsic");}
        selected_named=(state.IntrinsicState(*selected).flags&2)!=0;
    } else {
        const auto selected=std::find_if(bank->designs.begin(),bank->designs.end(),[&](const auto& row){return row.id==draft.selected_design;});
        if(selected==bank->designs.end() || !selected->active) {throw std::invalid_argument("Workshop alias selected a foreign or inactive design");}
        selected_named=selected->explicitly_named;
    }
    const auto matches=[&](uint32_t ch,uint32_t we,uint32_t ar,uint8_t reactor,uint32_t abilities) {
        return ch==draft.chassis && we==draft.weapon && ar==draft.armor && reactor==draft.reactor && abilities==draft.abilities;
    };
    for(const auto& source:catalog.units) {
        const auto design=state.EffectiveIntrinsic(source);
        if(design.source_id<64 && design.active && matches(design.chassis_id,design.weapon_id,design.defense_id,design.reactor,design.abilities)
            && Available(Prerequisite(design.prerequisite),technologies,owner)) {
            if(int32_t(design.source_id)!=draft.selected_design && (design.faction_exclusion&(1u<<owner.admission.simulation_id))) {
                auto next=state;auto overlay=state.IntrinsicState(source);overlay.flags|=8;overlay.exclusion&=uint8_t(~(1u<<owner.admission.simulation_id));
                next.intrinsic_overrides.at(design.source_id)=overlay;next.SynchronizeNativeRegistry();next.Validate(catalog);state=std::move(next);
            }
            return SmacWorkshopCommit{int32_t(design.source_id),!draft.name.empty() && draft.name!=catalog.source.units.at(design.source_id).name};
        }
    }
    const auto match=std::find_if(bank->designs.begin(),bank->designs.end(),[&](const auto& design){
        return design.active && matches(design.chassis,design.weapon,design.defense,design.reactor,design.abilities);
    });
    if(match==bank->designs.end()) {return std::nullopt;}
    const auto id=match->id;auto next=state;
    for(auto& target:next.owners) {if(target.simulation_id==owner.admission.simulation_id) {
        auto& design=*std::find_if(target.designs.begin(),target.designs.end(),[&](const auto& row){return row.id==id;});
        if(id!=draft.selected_design) {
            if(design.faction_exclusion&(1u<<owner.admission.simulation_id)) {design.auxiliary_flags|=8;}
            design.faction_exclusion&=~(1u<<owner.admission.simulation_id);
        }
        if(selected_named && !draft.name.empty()) {design.name=draft.name;design.explicitly_named=true;}
    }}
    next.SynchronizeNativeRegistry();next.Validate(catalog);state=std::move(next);return SmacWorkshopCommit{id,false};
}

SmacWorkshopCommit CommitSmacWorkshopFirstDesign(SmacStartupDesignState& state,const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner) {
    RequireOwner(owner);state.Validate(catalog);
    const auto* bank=state.Find(owner.admission.simulation_id);
    if(!bank || catalog.source.ruleset_id!="smac" || draft.chassis || draft.weapon || draft.armor>1
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
    if(const auto reused=ReuseSmacWorkshopDesign(state,draft,catalog,technologies,owner)) {return *reused;}
    if(draft.selected_design!=2 || !bank->designs.empty() || draft.armor!=1) {throw std::runtime_error("Workshop occupied-bank allocation requires its replacement policy");}
    SmacOriginalDesignPool pool=state.native_registry.value_or(SmacOriginalDesignPool{});
    if(!state.native_registry) {for(const auto& builtin:catalog.units) {
        auto& row=pool.at(builtin.source_id);const auto& name=catalog.source.units.at(builtin.source_id).name;
        std::copy_n(name.begin(),std::min<size_t>(31,name.size()),row.begin());row[36]=uint8_t(builtin.chassis_id);row[37]=uint8_t(builtin.weapon_id);row[38]=uint8_t(builtin.defense_id);
        const auto intrinsic=state.IntrinsicState(builtin);row[39]=builtin.reactor;row[44]=intrinsic.exclusion;row[48]=uint8_t(intrinsic.flags);row[49]=uint8_t(intrinsic.flags>>8);
        for(unsigned i=0;i<4;++i) {row[32+i]=uint8_t(builtin.abilities>>(8*i));}const auto prerequisite=uint16_t(Prerequisite(builtin.prerequisite));
        row[50]=uint8_t(prerequisite);row[51]=uint8_t(prerequisite>>8);
    }
    }
    const faction::SmacDesignCounts zero{};
    const auto allocation=ChooseSmacWorkshopSlot(pool,draft,zero,zero,technologies,owner);
    if(allocation.kind!=SmacWorkshopSlotKind::Build || allocation.confirmation) {throw std::runtime_error("First Workshop allocation requires an unused custom slot");}
    const size_t slot=size_t(allocation.selected);
    BuildSmacWorkshopRecord(pool,slot,draft,catalog,technologies,owner);
    const auto& chassis=catalog.chassis.chassis.at(0);
    const auto& armor=catalog.defenses.defenses.at(1);
    const SmacEarlyDesignNames names{armor.short_name,chassis.names[2].name,chassis.names[3].name,"","",armor.rating};
    auto automatic=catalog.workshop_name_labels ? NameSmacWorkshopDesign(pool,slot,draft,catalog,owner.admission.simulation_id):NameSmacOpeningDesign(names,owner.admission.simulation_id,0,1);automatic.resize(std::min<size_t>(automatic.size(),31));
    SmacCustomDesign design;design.id=static_cast<int32_t>(slot);design.defense=1;
    design.cost=pool[slot][41];design.plan=pool[slot][42];design.carry=pool[slot][40];
    design.prototyped=(pool[slot][48]&4)!=0;design.auxiliary_flags=uint16_t((uint16_t(pool[slot][48])|(uint16_t(pool[slot][49])<<8))&~7u);
    design.name=draft.name.empty()?automatic:draft.name;design.explicitly_named=*design.name!=automatic;
    auto next=state;
    for(auto& target:next.owners) {if(target.simulation_id==owner.admission.simulation_id) {target.designs.push_back(design);}}
    if(next.native_registry) {next.native_registry=std::move(pool);}
    next.SynchronizeNativeRegistry();next.Validate(catalog);state=std::move(next);
    return {design.id,false};
}
}
