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
SmacWorkshopAbilitySelection CleanSmacWorkshopRetainedAbilities(const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner) {
    RequireOwner(owner);RequireComponents(draft,catalog);
    std::array<int32_t,2> slots{-1,-1};size_t count=0;
    for(int32_t bit=0;bit<24;++bit) {if(draft.abilities&(1u<<bit)) {if(count<2) {slots[count]=bit;}++count;}}
    return CleanSmacWorkshopAbilities(draft,slots,count<2 || Available(Prerequisite(technologies.ResolveToken("Neural")),technologies,owner),catalog);
}
void ValidateSmacWorkshopEditorAbilities(const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner) {
    RequireOwner(owner);RequireComponents(draft,catalog);
    const auto known=[&](const std::string& token) {return Available(Prerequisite(technologies.ResolveToken(token)),technologies,owner);};
    std::array<int32_t,2> slots{-1,-1};size_t count=0;
    for(uint32_t bit=0;bit<24;++bit) {if(draft.abilities&(1u<<bit)) {
        if(!known(catalog.abilities.abilities.at(bit).prerequisite) || count>=2) {throw std::invalid_argument("Workshop ability is unavailable");}
        slots[count++]=int32_t(bit);
    }}
    if(CleanSmacWorkshopAbilities(draft,slots,count<2 || known("Neural"),catalog).mask!=draft.abilities) {throw std::invalid_argument("Workshop abilities require editor cleanup");}
}
SmacWorkshopStatus QuerySmacWorkshopStatus(const SmacOriginalDesignPool& pool,const SmacWorkshopDraft& draft,
    int32_t owner,const std::optional<faction::SmacDesignCounts>& units,
    const std::optional<faction::SmacDesignCounts>& production) {
    if(owner<1 || owner>7 || draft.selected_design<0 || draft.selected_design>=512
        || (draft.selected_design>=64 && draft.selected_design/64!=owner)) {throw std::invalid_argument("Invalid Workshop status selection");}
    const auto& current=pool.at(draft.selected_design);
    bool same=current[36]==draft.chassis && current[37]==draft.weapon && current[38]==draft.armor && current[39]==draft.reactor;
    for(unsigned i=0;i<4;++i) {same=same && current[32+i]==uint8_t(draft.abilities>>(8*i));}
    using Kind=SmacWorkshopStatusKind;
    if(draft.selected_design<64) {return {draft.selected_design,same ? Kind::Basic:Kind::BasedOnOld};}
    const auto excluded=[&](int32_t id){return (pool.at(id)[44]&(1u<<owner))!=0;};
    if(!(current[48]&1) || excluded(draft.selected_design)) {return {draft.selected_design,Kind::New};}
    if(same) {return {draft.selected_design,Kind::Previous};}
    int32_t selected=draft.selected_design;bool accepted=false;
    for(int32_t id=owner*64;id<(owner+1)*64;++id) {
        const auto& row=pool.at(id);
        if(!(row[48]&1)) {return {id,Kind::BasedOnOld};}
        if(!excluded(id)) {continue;}
        if(!units) {throw std::invalid_argument("Workshop status requires retained Unit counts");}
        if(units->at(id)!=0) {continue;}
        if(!production) {throw std::invalid_argument("Workshop status requires retained production counts");}
        if(production->at(id)!=0 && (accepted || row[43]!=pool.at(selected)[43])) {continue;}
        selected=id;accepted=true;
    }
    return {selected,accepted ? Kind::BasedOnOld:Kind::Modifying};
}
int32_t SmacWorkshopMovement(const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog) {
    RequireComponents(draft,catalog);
    const auto& chassis=catalog.chassis.chassis.at(draft.chassis);const auto& weapon=catalog.weapons.weapons.at(draft.weapon);
    const auto mask=draft.abilities;
    const bool air=chassis.triad==loader::txt::ChassisTriad::Air,transport=weapon.mode==loader::txt::WeaponMode::Transport;
    int64_t movement=int64_t(chassis.speed)+(air ? 2*int64_t(draft.reactor):0);
    if(transport) {--movement;}if(mask&0x800000) {--movement;}
    if(mask&0x400) {movement+=air ? 2*int64_t(draft.reactor):1;}
    if(air && (mask&0x20)) {movement=4*movement/5;}if(air && transport) {movement/=2;}
    return int32_t(std::clamp<int64_t>(movement,1,99));
}
std::string FormatSmacWorkshopStats(const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog,bool main_caption) {
    RequireComponents(draft,catalog);
    const auto& chassis=catalog.chassis.chassis.at(draft.chassis);const auto& weapon=catalog.weapons.weapons.at(draft.weapon);
    const auto strength=catalog.defenses.defenses.at(draft.armor).rating;const auto mask=draft.abilities;
    using Domain=loader::txt::ChassisTriad;
    std::string attack;
    if(weapon.offense<0) {attack="?";}
    else if(weapon.offense>=99) {attack="*";}
    else {
        attack=std::to_string(chassis.missile ? int64_t(weapon.offense)*3/2:weapon.offense);
        for(const auto& decoration:std::array<std::pair<uint32_t,char>,5>{{{0x10,'^'},{8,'~'},{0x100000,'x'},{0x800,'e'},{0x20000,'!'}}}) {
            if(mask&decoration.first) {attack+=decoration.second;}
        }
        if(weapon.offense>0 && strength>=0 && (chassis.triad==Domain::Sea || (chassis.triad==Domain::Land && (mask&0x8000)))) {attack='('+attack+')';}
        if(mask&0x20) {attack='<'+attack+'>';}
    }
    std::string armor="?";
    if(strength>=0) {
        armor=std::to_string(std::clamp(strength,1,9999));
        if(mask&0x200) {armor+='+';}if(mask&0x40000) {armor+='t';}if(mask&0x100) {armor='<'+armor+'>';}
    }
    std::string result=attack+'-'+armor+'-'+std::to_string(SmacWorkshopMovement(draft,catalog));
    if(draft.reactor>1) {result+='*'+std::to_string(draft.reactor);}
    if(main_caption) {
        result+=' ';bool first=true;
        for(size_t bit=0;bit<24;++bit) {const auto& label=catalog.abilities.abilities[bit].abbreviation;
            if((mask&(1u<<bit)) && !label.empty()) {if(!first) {result+=", ";}result+=label;first=false;}
        }
    }return result;
}
SmacWorkshopEditContext QuerySmacWorkshopEditContext(const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner,bool automatic_suggestions) {
    RequireOwner(owner);if(catalog.abilities.abilities.size()!=24) {throw std::invalid_argument("Workshop edits require24 ability definitions");}
    SmacWorkshopEditContext result{automatic_suggestions,Available(Prerequisite(technologies.ResolveToken("Neural")),technologies,owner),{}};
    for(size_t bit=0;bit<24;++bit) {result.available_abilities[bit]=Available(Prerequisite(technologies.ResolveToken(catalog.abilities.abilities[bit].prerequisite)),technologies,owner);}
    return result;
}
namespace {
void RequireEditor(const SmacWorkshopEditor& editor,const SmacPredefinedUnitCatalog& catalog) {
    RequireComponents(editor.draft,catalog);
    for(const auto slot:editor.ability_slots) {if(slot< -1 || slot>=24) {throw std::invalid_argument("Invalid Workshop ability slot");}}
}
std::array<int32_t,2> WorkshopSlots(uint32_t mask) {
    std::array<int32_t,2> result{-1,-1};size_t next=0;
    for(int32_t bit=0;bit<24 && next<2;++bit) {if(mask&(1u<<bit)) {result[next++]=bit;}}
    return result;
}
}
SmacWorkshopAbilityMenu QuerySmacWorkshopAbilities(const SmacWorkshopEditor& editor,uint32_t slot,
    const SmacWorkshopEditContext& context,const SmacPredefinedUnitCatalog& catalog) {
    RequireEditor(editor,catalog);if(slot>=2) {throw std::invalid_argument("Invalid Workshop ability slot");}
    using Guard=SmacWorkshopAbilityMenu::Guard;
    if(slot==1 && !context.second_ability) {return {Guard::SecondTechnology,{}};}
    if(catalog.chassis.chassis.at(editor.draft.chassis).missile) {return {Guard::Missile,{}};}
    SmacWorkshopAbilityMenu result{Guard::None,{24}};
    for(uint32_t bit=0;bit<24;++bit) {
        if(context.available_abilities[bit] && editor.ability_slots[1-slot]!=int32_t(bit)
            && SmacWorkshopAbilityCompatible(bit,editor.draft,catalog)) {result.choices.push_back(bit);}
    }return result;
}
std::optional<SmacWorkshopAbilityCost> QuerySmacWorkshopAbilityCost(int32_t code,uint32_t flags,
    int32_t triad,uint32_t weapon_cost,uint32_t armor_cost,uint32_t chassis_speed) {
    if(triad<0 || triad>2 || weapon_cost>255 || armor_cost>255 || chassis_speed>255) {throw std::invalid_argument("Ability cost requires raw component bytes");}
    const int64_t w=weapon_cost,a=armor_cost,s=chassis_speed;
    if(code< -7) {return std::nullopt;}
    SmacWorkshopAbilityCost result{code,code<0?code:0,-1};
    switch(code) {
        case -1:if(!a) {throw std::domain_error("Ability ratio has zero armor cost");}result.quantity=std::clamp<int64_t>(w/a,0,2);break;
        case -2:result.quantity=w-1;break;case -3:result.quantity=a-1;break;case -4:result.quantity=s-1;break;
        case -5:result.quantity=w+a-2;break;case -6:result.quantity=w+s-2;break;case -7:result.quantity=a+s-2;break;
        default:if(flags&0x400) {result.domain=triad;if(triad==0) {++result.quantity;}}break;
    }return result;
}
SmacWorkshopEditor ChangeSmacWorkshopComponent(const SmacWorkshopEditor& before,SmacWorkshopComponent kind,
    uint32_t accepted,const SmacWorkshopEditContext& context,const SmacPredefinedUnitCatalog& catalog) {
    RequireEditor(before,catalog);auto result=before;auto& draft=result.draft;
    const auto cleanup=[&]() {const auto value=CleanSmacWorkshopAbilities(draft,result.ability_slots,context.second_ability,catalog);
        draft.abilities=value.mask;result.ability_slots=value.slots;};
    const auto suggest=[&](uint32_t bit) {
        cleanup();if(!context.automatic_suggestions || !context.available_abilities.at(bit) || !SmacWorkshopAbilityCompatible(bit,draft,catalog)) {return;}
        unsigned count=0;for(auto mask=draft.abilities;mask;mask>>=1) {count+=mask&1;}
        if(count>1 || (count==1 && !context.second_ability)) {return;}
        draft.abilities|=1u<<bit;result.ability_slots=WorkshopSlots(draft.abilities);
    };
    using Domain=loader::txt::ChassisTriad;
    const auto weapon=[&]()->const loader::txt::WeaponComponent& {return catalog.weapons.weapons.at(draft.weapon);};
    switch(kind) {
        case SmacWorkshopComponent::Chassis: {
            if(accepted>=catalog.chassis.chassis.size()) {throw std::invalid_argument("Invalid Workshop chassis choice");}
            const auto old_domain=catalog.chassis.chassis.at(draft.chassis).triad;draft.chassis=accepted;
            const auto& chassis=catalog.chassis.chassis.at(accepted);
            if(chassis.missile) {draft.armor=0;if(int32_t(weapon().mode)>=2) {draft.weapon=0;}if(draft.weapon!=13) {draft.weapon=20;}}
            else if(weapon().offense>=99 || draft.weapon==20) {draft.weapon=0;}
            if(chassis.triad!=old_domain) {
                if(chassis.triad!=Domain::Land) {
                    suggest(1);
                    if(chassis.triad==Domain::Air && (int32_t(weapon().mode)==11 || (int32_t(weapon().mode)==9 && chassis.range!=0))) {draft.weapon=0;}
                } else if(draft.abilities&2) {draft.abilities&=~2u;result.ability_slots=WorkshopSlots(draft.abilities);}
            }break;
        }
        case SmacWorkshopComponent::Weapon: {
            if(accepted>=catalog.weapons.weapons.size()) {throw std::invalid_argument("Invalid Workshop weapon choice");}
            const auto old=draft.weapon;draft.weapon=accepted;const auto& chassis=catalog.chassis.chassis.at(draft.chassis);
            if(chassis.triad==Domain::Air) {
                if(int32_t(weapon().mode)==11 || (int32_t(weapon().mode)==9 && chassis.range!=0)) {draft.weapon=old;}
                if(chassis.missile) {if(draft.weapon!=13) {draft.weapon=20;}}
                else if(draft.weapon==13 || draft.weapon==20) {draft.weapon=old;}
            } else if(draft.weapon==13 || draft.weapon==20) {draft.weapon=old;}
            if(weapon().offense>=99) {draft.abilities=0;result.ability_slots={-1,-1};}
            // The source's empty-mask gate is before cleanup, including when
            // the old mask will subsequently lose every incompatible ability.
            if(!draft.abilities && draft.weapon!=old) {
                if(int32_t(weapon().mode)==7) {suggest(19);}
                if(int32_t(weapon().mode)==9) {suggest(0);suggest(13);}
                if(int64_t(weapon().offense)>=2*int64_t(catalog.defenses.defenses.at(draft.armor).rating)) {suggest(17);}
            }break;
        }
        case SmacWorkshopComponent::Armor:
            if(accepted>=catalog.defenses.defenses.size()) {throw std::invalid_argument("Invalid Workshop shielding choice");}
            draft.armor=accepted;if(catalog.defenses.defenses.at(accepted).rating>1 && weapon().offense<=1) {suggest(9);}break;
        case SmacWorkshopComponent::Reactor:
            if(accepted<1 || accepted>4 || accepted>catalog.reactors.reactors.size()) {throw std::invalid_argument("Invalid Workshop reactor choice");}
            draft.reactor=uint8_t(accepted);break;
        default: throw std::invalid_argument("Unknown Workshop component edit");
    }
    cleanup();return result;
}
SmacWorkshopAbilityEdit SetSmacWorkshopAbilitySlot(const SmacWorkshopEditor& before,uint32_t slot,int32_t accepted,
    const SmacWorkshopEditContext& context,const SmacPredefinedUnitCatalog& catalog) {
    RequireEditor(before,catalog);if(slot>=2 || accepted< -1) {throw std::invalid_argument("Invalid Workshop ability choice");}
    SmacWorkshopAbilityEdit result{before,false};
    if(catalog.chassis.chassis.at(before.draft.chassis).missile || (slot==1 && !context.second_ability)) {return result;}
    result.admitted=true;result.editor.ability_slots[slot]=accepted<24?accepted:-1;result.editor.draft.abilities=0;
    for(const auto bit:result.editor.ability_slots) {if(bit>=0) {result.editor.draft.abilities|=1u<<bit;}}
    return result;
}
int32_t ResolveSmacWorkshopPickerResult(int32_t old_choice,int32_t mapped_hover,bool valid,SmacWorkshopComponent kind) {
    if(old_choice<0 || kind<SmacWorkshopComponent::Chassis || kind>SmacWorkshopComponent::Reactor) {
        throw std::invalid_argument("Invalid Workshop modal context");
    }
    if(mapped_hover<0 || !valid) {return old_choice;}
    if(kind==SmacWorkshopComponent::Reactor) {
        if(mapped_hover==INT32_MAX) {throw std::overflow_error("Workshop reactor rating overflows signed32");}
        return mapped_hover+1;
    }return mapped_hover;
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
    const auto domain=int32_t(chassis.triad);
    if(!catalog.workshop_name_labels || owner<1 || owner>7 || index>=512 || (index>=64 && int32_t(index/64)!=owner)
        || domain<0 || domain>2) {throw std::invalid_argument("Workshop naming requires admitted conventional text and context");}
    const auto& current=pool[index];const auto& labels=*catalog.workshop_name_labels;const auto& names=chassis.names;
    for(size_t i=0;i<8;++i) {const auto& label=labels[i];if(i==7 && label.empty()) {continue;}if(label.empty() || label.size()>128 || label.find('\0')!=std::string::npos) {throw std::invalid_argument("Invalid Workshop naming text");}}
    std::string prefix;for(unsigned bit=0;bit<24;++bit) {
        const auto& text=catalog.abilities.abilities.at(bit).abbreviation;
        if(!(draft.abilities&(1u<<bit)) || text.empty() || bit==15 || (bit==5 && domain==2)) {continue;}
        if(bit==3 && speed==1 && offense>=defense && !(draft.abilities&0x8000)) {continue;}
        prefix=text+' ';break;
    }
    std::string full=prefix,short_name=prefix;bool compact=false,elite=false;unsigned variant=((draft.reactor-1)^owner)&1;
    if(mode<3) {int threshold=defense<=offense ? 3:6;if(domain==1) {++threshold;}else if(domain==2) {threshold+=threshold/2;}
        elite=std::min(defense,offense)>threshold && !names[4].name.empty() && !names[5].name.empty();}
    if(domain==0 && offense>=0 && speed>=std::max(offense,defense) && mode<3) {
        compact=true;variant=speed>1;
        if(draft.abilities&0x400000) {
            if(!full.empty()) {full.pop_back();}short_name.resize(std::min(short_name.size(),full.empty()?size_t(0):full.size()-1));
        }else {const auto& label=labels[offense>=2?1:0];full+=label;short_name+=label;}
    }
    // This scan reads retained selected metadata, not the proposed tuple. A
    // completed active record normally clears the flag itself; draft callers
    // can legitimately select inactive rows with older plan/chassis/reactor.
    bool reactor_prefix=false;
    if(!compact) {for(size_t i=0;i<512;++i) {
        if(i>=64 && (i<size_t(owner)*64 || i>=size_t(owner+1)*64)) {continue;}const auto& row=pool[i];
        if(!(row[48]&1) || row[42]!=current[42] || row[36]!=current[36]) {continue;}
        if(row[39]>=current[39]) {reactor_prefix=false;break;}reactor_prefix=true;
    }}
    const auto& reactor=catalog.reactors.reactors.at(draft.reactor-1).short_name;
    const bool defensive=offense>=0 && defense>offense && mode<3 && !(draft.abilities&0x8000),interceptor=offense>=0 && domain==2 && (draft.abilities&32) && mode<3;
    if(defensive || interceptor) {
        if(!compact) {const auto& text=reactor_prefix ? reactor:defensive ? catalog.defenses.defenses.at(draft.armor).short_name:weapon.short_name;
            full+=text;short_name+=text;if(defensive) {variant^=unsigned(defense)&1;}}
        full+=' '+names[2].name;
        short_name+=' '+(compact && (index&1) ? names[elite && domain!=1 ? 5:2].name:names[elite && domain==1 ? 5:3].name);
    } else if(chassis.missile && offense>=99) {
        full+=reactor+' '+weapon.short_name;short_name+=reactor+' '+weapon.short_name;
    } else {
        if(mode>=3 && draft.chassis!=0) {
            if(draft.reactor>1) {full+=reactor+' ';short_name+=reactor+' ';}
            if(!(mode==7 && draft.chassis==3) && !(mode==9 && domain==0 && full.find(' ')!=std::string::npos)) {
                if((mode==8 || mode==9) && draft.chassis==3) {
                    if(labels[7].empty()) {throw std::runtime_error("Retained Workshop profile has no Sea naming label");}
                    full+=labels[7]+' ';short_name+=labels[7]+' ';
                }else {full+=names[0].name+' ';short_name+=names[1].name+' ';}
            }
        }
        if(!compact) {full+=weapon.short_name;short_name+=weapon.short_name;variant^=unsigned(offense)&1;}
        if(mode<3) {
            if(draft.abilities&0x8000) {full+=' '+labels[2];short_name+=' '+labels[3];}
            else if((draft.abilities&8) && speed==1 && offense>defense) {full+=' '+labels[4];short_name+=' '+labels[5];}
            else if(reactor_prefix) {full+=' '+reactor;short_name+=' '+reactor;}
            else {full+=' '+names[elite && domain!=1 ? 4:0].name;short_name+=' '+names[elite && domain==1 ? 4:1].name;}
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
            const auto& row=pool[i];bool same=row[36]==draft.chassis && row[37]==draft.weapon && row[38]==draft.armor && row[39]==draft.reactor;
            for(unsigned k=0;k<4;++k) {same=same && row[32+k]==uint8_t(draft.abilities>>(8*k));}if(same) {continue;}
            const std::string name(row.begin(),std::find(row.begin(),row.begin()+32,0));
            if(name==full) {if(!identical) {variant=1;}bump(full);changed=true;break;}
            if(!identical && name==short_name) {variant=0;bump(short_name);changed=true;break;}
        }
        if(!changed) {return variant ? short_name:full;}
    }
}

SmacWorkshopHumanPreview PrepareSmacWorkshopHumanPreview(const SmacStartupDesignState& state,
    const SmacWorkshopDraft& provisional,const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner,
    const SmacWorkshopHumanContext& context,const faction::SmacDesignCounts& units,
    const faction::SmacDesignCounts& production) {
    RequireOwner(owner);RequireComponents(provisional,catalog);state.Validate(catalog);
    if(!state.native_registry || !state.Find(owner.admission.simulation_id) || context.threat>3
        || catalog.chassis.chassis.size()!=9 || catalog.weapons.weapons.size()!=21
        || catalog.defenses.defenses.size()!=10 || catalog.reactors.reactors.size()!=4) {
        throw std::invalid_argument("Human Workshop preview requires retained stock records and explicit entry state");
    }
    // Address identities are evidence contracts, not reads from native memory.
    const auto f=[&](uint32_t address)->int64_t {const auto found=context.fields.find(address);
        if(found==context.fields.end()) {throw std::invalid_argument("Missing human Workshop faction authority");}return found->second;};
    for(const auto address:{0x95d1acu,0x95d584u,0x95d588u,0x95d58cu,0x95d590u,0x95d594u,0x95e200u,
        0x95d9d8u,0x95d9d0u,0x95d9c8u,0x95d418u,0x95d9ecu,0x95f230u,0x95f22cu}) {(void)f(address);}
    const auto& CH=catalog.chassis.chassis;const auto& WE=catalog.weapons.weapons;const auto& AR=catalog.defenses.defenses;
    const auto known=[&](const std::string& token) {return Available(Prerequisite(technologies.ResolveToken(token)),technologies,owner);};
    const auto ability=[&](int bit) {return known(catalog.abilities.abilities.at(bit).prerequisite);};
    const bool neural=known("Neural");int reactor=1;
    for(int r=4;r>=1;--r) {if(known(catalog.reactors.reactors.at(r-1).prerequisite)) {reactor=r;break;}}
    const int64_t power=2<<reactor;const int first=64*owner.admission.simulation_id;
    const uint8_t excluded=uint8_t(1u<<owner.admission.simulation_id);
    auto pool=*state.native_registry;SmacWorkshopHumanPreview result{state,provisional,1,-1,{}};
    const auto read=[&](int id) {const auto& row=pool.at(id);SmacWorkshopDraft draft=provisional;
        draft.chassis=row[36];draft.weapon=row[37];draft.armor=row[38];draft.reactor=row[39];draft.abilities=0;
        for(unsigned k=0;k<4;++k) {draft.abilities|=uint32_t(row[32+k])<<(8*k);}RequireComponents(draft,catalog);return draft;};
    const auto intrinsic_known=[&](int id) {const auto& row=pool.at(id);
        return Available(int16_t(uint16_t(row[50])|(uint16_t(row[51])<<8)),technologies,owner);};
    const auto eligible=[&](int id) {return (pool.at(id)[48]&1) && !(pool.at(id)[44]&excluded) && (id>=64 || intrinsic_known(id));};
    for(int id=first;id<first+64;++id) {if(pool[id][48]&1) {const auto d=read(id);
        if(WE.at(d.weapon).offense<0 || AR.at(d.armor).rating<0) {throw std::invalid_argument("Human Workshop PSI custom evaluation is not admitted");}}}
    const auto price=[&](int ch,int we,int ar,uint32_t mask=0) {return SmacPredefinedUnitCatalog::CalculateCost(CH.at(ch),WE.at(we),AR.at(ar),reactor,mask,catalog.abilities);};
    const auto submit=[&](const SmacWorkshopDraft& draft,int category,int source) {
        int alias=-1;
        if(result.filter<0 || result.filter==category) {
            bool candidate=false;
            for(int id=0;id<first+64;++id) {
                if(id>=64 && id<first) {continue;}auto& row=pool[id];const bool own=id>=64,obsolete=(row[44]&excluded)!=0;
                if(own && (row[48]&1) && obsolete && !units[id] && !production[id]) {candidate=true;continue;}
                if(!(row[48]&1)) {candidate|=own;continue;}if(!own && !intrinsic_known(id)) {continue;}
                const auto stored=read(id);
                if(stored.chassis==draft.chassis && stored.weapon==draft.weapon && stored.armor==draft.armor && stored.reactor==draft.reactor
                    && !((stored.abilities^draft.abilities)&0x44ae1b) && (stored.abilities&draft.abilities)==stored.abilities) {
                    if(result.mode<=2) {result.draft=draft;result.mode=2;}row[44]&=uint8_t(~excluded);alias=id;break;
                }
                if(own) {candidate|=(obsolete && !units[id]) || row[43]==category || (!units[id] && !production[id]);}
            }
            if(alias<0 && candidate && result.mode<=2) {result.draft=draft;result.mode=3;result.filter=category;}
        }
        result.steps.push_back({draft,result.draft,category,source,alias,result.mode,result.filter});
    };
    const auto emit=[&](int ch,int we,int ar,uint32_t mask,int category) {auto draft=provisional;
        draft.chassis=ch;draft.weapon=we;draft.armor=ar;draft.reactor=uint8_t(reactor);draft.abilities=mask;submit(draft,category,-1);};
    // Upgrade prefix: the initial equipment scan is frozen, but each own-row
    // admission below sees exclusion changes from the preceding submission.
    std::vector<int> active;
    for(int id=0;id<first+64;++id) {if(id>=64 && id<first) {continue;}if(eligible(id)) {active.push_back(id);}}
    int new_weapon=-1,new_armor=-1,best=-1;
    for(int w=0;w<21;++w) {const int strength=WE[w].offense;
        if(!known(WE[w].prerequisite) || strength<=0 || strength>=99 || w==20 || strength<best) {continue;}
        for(const auto id:active) {const auto draft=read(id);if(CH.at(draft.chassis).missile) {continue;}
            if(WE.at(draft.weapon).offense>=strength) {break;}new_weapon=w;best=strength;}
    }
    best=-1;for(int a=0;a<10;++a) {const int strength=AR[a].rating;
        if(!known(AR[a].prerequisite) || strength<=1 || strength<best) {continue;}
        for(const auto id:active) {if(AR.at(read(id).armor).rating>=strength) {break;}new_armor=a;best=strength;}
    }
    int64_t offense_factor=10;
    if(new_weapon>=0 || new_armor>=0) {for(int id=first;id<first+64;++id) {
        if(!eligible(id)) {continue;}auto draft=read(id);const int plan=pool[id][42];
        if(CH.at(draft.chassis).missile || plan>6 || plan==3 || plan==5) {continue;}
        if(plan==0 || plan==4) {if(new_weapon<0) {continue;}draft.weapon=new_weapon;}
        else if(plan==2) {if(new_armor<0) {continue;}draft.armor=new_armor;}
        else if(plan==1) {
            if(new_armor>=0) {offense_factor=reactor==1 ? 2:1;}
            const int candidate=new_weapon>=0 ? new_weapon:int(draft.weapon);const int64_t strength=WE.at(candidate).offense;
            if(strength<=0) {throw std::invalid_argument("Human Workshop upgrade requires positive offense");}
            int64_t limit=(reactor==1 ? 2:1)*power*4/strength;limit=limit>=2 && strength>=2 ? std::min(limit,strength):2;
            if(new_armor>=0 && AR.at(new_armor).rating<=limit) {draft.armor=new_armor;}else if(new_weapon<0) {continue;}
            draft.weapon=candidate;
        } else {
            const auto limit=f(0x95d594)+f(0x95d588)+std::max(5,reactor*reactor);
            if(new_weapon>=0 && price(draft.chassis,new_weapon,draft.armor,draft.abilities)<=limit) {draft.weapon=new_weapon;}
            bool changed=false;if(new_armor>=0 && price(draft.chassis,draft.weapon,new_armor,draft.abilities)<=limit) {draft.armor=new_armor;changed=true;}
            if(!changed && int(draft.weapon)!=new_weapon) {continue;}
        }
        draft.reactor=uint8_t(reactor);submit(draft,pool[id][43],id);
    }}
    const auto chassis=[&](int domain,int minimum) {int selected=-1;int64_t score=0;
        for(int id=0;id<9;++id) {const auto& row=CH[id];
            if(!known(row.prerequisite) || (domain>=0 && int(row.triad)!=domain) || (row.missile && minimum!=-1)) {continue;}
            int64_t current;
            if(minimum>0) {if(row.speed<minimum) {continue;}current=row.speed==minimum ? 2:1;}
            else {if(domain==2 && ((minimum==-1 && !row.missile) || (minimum==-2 && row.range!=1))) {continue;}current=int64_t(row.speed)+1;}
            if(current>score) {selected=id;score=current;}
        }return selected;
    };
    const auto weapon=[&](int64_t budget) {int selected=0;int64_t best_strength=-1;
        for(int id=0;id<21;++id) {const auto& row=WE[id];if(!known(row.prerequisite)) {continue;}
            if(budget<0) {if(int(row.mode)==-budget) {return id;}continue;}
            if(row.cost>budget || int(row.mode)>=3 || row.offense>=99 || id==20) {continue;}
            if(2*int64_t(row.offense)>=best_strength) {selected=id;best_strength=2*int64_t(row.offense);}
        }return selected;
    };
    const auto armor=[&](int64_t budget) {int selected=0;int64_t best_strength=-1;
        for(int id=0;id<10;++id) {const auto& row=AR[id];if(known(row.prerequisite) && row.cost<=budget && row.rating>=0 && 2*int64_t(row.rating)>=best_strength) {selected=id;best_strength=2*int64_t(row.rating);}}
        return selected;
    };
    const auto clean=[&] {return f(0x95d1ac)>=6 && (f(0x95d590)!=0 || f(0x95d58c)!=0) && (f(0x95d594)==0 || f(0x95d584)<1) && ability(16);};
    int ch=chassis(0,1),we,ar;uint32_t mask;
    if(ch>=0) {
        ar=armor(99);mask=ability(9) ? 512:0;
        if(!mask && (f(0x95d584)<0 || context.innate_morale<0) && ability(14)) {mask=16384;}
        we=weapon(1);int budget=2;
        for(;;) {if(budget>10000) {throw std::invalid_argument("Unbounded human Workshop defensive budget");}
            const int candidate=weapon(budget);if(candidate==we || price(ch,candidate,ar,mask)>price(ch,we,ar,mask)) {break;}we=candidate;++budget;}
        emit(ch,we,ar,mask,2);
    }
    ch=chassis(0,1);
    if(ch>=0) {
        int64_t n=2,budget=1,limit=f(0x95d594)+5;
        if(2/(2*power)<=limit) {for(;;) {++budget;++n;if((n*(n+1)/2)/(2*power)>limit) {break;}
            if(n>=10000) {throw std::invalid_argument("Unbounded human Workshop armor budget");}}}
        const int strongest=weapon(99);int64_t p=WE.at(strongest).offense;
        if(p<=0) {throw std::invalid_argument("Human Workshop attack requires positive offense");}
        int64_t q=offense_factor*power*4/p;q=q>=2 && p>=2 ? std::min(q,p):2;
        ar=armor(std::max<int64_t>(1,std::min<int64_t>(99,std::max(q,budget/2))));const int64_t d=AR.at(ar).rating;
        while((p+1)*d/(2*power)<=limit) {if(++p>=100000) {throw std::invalid_argument("Unbounded human Workshop weapon budget");}}
        we=weapon(p);mask=0;
        if(f(0x95e200)>=8 && f(0x95d1ac)>=6 && (f(0x95d590)!=0 || f(0x95d58c)!=0) && ability(16)) {mask=65536;}
        if(context.innate_morale<0 && !mask && ability(14)) {mask=16384;}
        if(d>1 && !mask && f(0x95d58c)>0 && f(0x95d9d8)>1 && f(0x95d9d0)>=2*f(0x95d9c8) && ability(9)) {mask=512;}
        if(!mask && f(0x95d584)<0 && ability(14)) {mask=16384;}
        if(ability(10) && (!mask || neural)) {mask|=1024;}
        emit(ch,we,ar,mask,3);const auto extra=neural ? mask&~1024u:0;
        if(ability(4)) {emit(ch,we,ar,extra|16,11);}if(ability(3)) {emit(ch,we,ar,extra|8,12);}
    }
    const int strongest=weapon(99);ch=chassis(0,1);
    if(ability(15) && ch>=0) {
        ar=armor(1);
        for(int iteration=0;iteration<100;++iteration) {bool changed=false;const int faster=chassis(0,2);
            if(CH.at(ch).speed==1 && faster>=0 && faster!=ch) {const int old=price(ch,strongest,ar),next=price(faster,strongest,ar);
                if((next<=reactor+5 && next<=old+(owner.admission.simulation_id&1)+1) || next<=old) {ch=faster;changed=true;}}
            const int candidate=armor(int64_t(AR.at(ar).rating)+1);
            if(AR.at(candidate).rating>AR.at(ar).rating && price(ch,strongest,candidate)<=price(ch,strongest,ar)) {ar=candidate;changed=true;}
            if(!changed) {break;}
        }
        mask=0;if(neural) {for(int bit:{17,10,1}) {if(ability(bit)) {mask=1u<<bit;break;}}}emit(ch,strongest,ar,mask|32768,1);
    }
    ch=chassis(0,3);if(ch<0) {ch=chassis(0,2);}
    if(ch>=0) {
        we=weapon(99);ar=armor(1);
        for(int iteration=0;iteration<100;++iteration) {const int64_t p=WE.at(we).offense,d=AR.at(ar).rating;if(d>=p-1) {break;}
            const int candidate=armor(std::max(p/2,d+1));if(AR.at(candidate).rating<=d || price(ch,we,candidate)>f(0x95d594)+f(0x95d584)+reactor+3) {break;}ar=candidate;
        }
        mask=0;if((context.innate_morale<0 || (f(0x95d584)>0 && f(0x95d418)>=0)) && ability(14)) {mask=16384;}
        if(ability(10) && (!mask || neural)) {mask|=1024;}emit(ch,we,ar,mask,4);
    }
    ch=chassis(0,3);mask=0;if(ch<0) {ch=chassis(0,2);}if(ch<0) {ch=chassis(0,1);if(ability(10)) {mask=1024;}}
    if(ch>=0) {
        if(!mask) {for(int bit:{1,2}) {if(ability(bit)) {mask=1u<<bit;break;}}}
        if(ability(10)) {mask=neural ? mask|1024:1024;}emit(ch,weapon(1),armor(1),mask,10);
    }
    ch=chassis(1,0);
    if(ch>=0) {
        mask=0;for(int bit:{1,8}) {if(ability(bit)) {mask=1u<<bit;break;}}
        if((context.threat&1) && ability(18) && (!mask || neural)) {mask|=1u<<18;}
        for(int bit:{8,1}) {unsigned count=0;for(auto bits=mask;bits;bits>>=1) {count+=bits&1;}
            if(ability(bit) && !(mask&(1u<<bit)) && count<2 && (!mask || neural)) {mask|=1u<<bit;}}
        we=weapon(1);ar=armor(1);
        for(int iteration=0;iteration<100;++iteration) {bool changed=false;int candidate=we;
            for(int delta=1;delta<=3;++delta) {candidate=weapon(int64_t(WE.at(we).offense)+delta);if(candidate!=we) {break;}}
            const auto limit=f(0x95d594)+f(0x95d588)+std::max(5,2*reactor-reactor/2);
            if(candidate!=we && price(ch,candidate,ar)<=limit) {we=candidate;changed=true;}
            candidate=ar;for(int delta=1;delta<=3;++delta) {candidate=armor(int64_t(AR.at(ar).rating)+delta);if(candidate!=ar) {break;}}
            if(AR.at(candidate).rating>AR.at(ar).rating && price(ch,we,candidate,mask)<=limit) {ar=candidate;changed=true;}
            if(!changed) {break;}
        }
        emit(ch,weapon(99),ar,mask,6);
    }
    ch=chassis(1,0);
    if(ch>=0) {
        ar=armor(f(0x95d9ec)>f(0x95f230) && f(0x95d9ec)>f(0x95f22c) ? 99:1);emit(ch,weapon(-7),ar,0,7);
        if(ch!=3 && known(CH[3].prerequisite)) {ch=3;}ar=armor(1);we=weapon(-9);
        mask=ability(0) ? 1:ability(13) ? 8192:0;if(mask==1 && ability(13) && neural) {mask=8193;}emit(ch,we,ar,mask,15);
        emit(ch,weapon(-8),ar,0,16);
    }
    for(const auto minimum:{0,-2}) {
        ch=chassis(2,minimum);if(ch<0) {continue;}ar=armor(1);mask=clean() ? 65536:0;
        if((context.threat&2) && !mask && ability(11)) {mask=2048;}if(ability(1) && (!mask || neural)) {mask|=2;}
        const int category=minimum==0 ? 8:17;emit(ch,weapon(99),ar,mask,category);
        if(category==8) {const uint32_t intercept=ability(1) && neural ? 34:(context.threat&1) && ability(18) && neural ? 262176:32;emit(ch,weapon(99),ar,intercept,9);}
    }
    ch=chassis(2,-1);
    if(ch>=0) {ar=armor(1);if(known(WE[20].prerequisite)) {emit(ch,20,ar,0,13);}
        for(int id=0;id<21;++id) {if(known(WE[id].prerequisite) && WE[id].offense>=99) {emit(ch,id,ar,0,14);break;}}
    }
    if(reactor<=1 && !ability(0)) {if(ability(13)) {ch=chassis(0,1);if(ch>=0) {emit(ch,15,armor(1),8192,-1);}}}
    else {ch=chassis(0,2);if(ch>=0) {mask=ability(0) ? 1:0;if(ability(13) && (!mask || neural)) {mask|=8192;}emit(ch,15,armor(1),mask,-1);}}
    result.state.RetainNativeRegistry(pool,catalog);return result;
}

std::vector<int32_t> BrowseSmacWorkshopDesigns(const SmacStartupDesignState& state,
    const SmacPredefinedUnitCatalog& catalog,const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner,const std::optional<faction::SmacDesignCounts>& units) {
    RequireOwner(owner);state.Validate(catalog);
    if(!state.native_registry || !state.Find(owner.admission.simulation_id)) {throw std::invalid_argument("Workshop browse requires its retained owner registry");}
    const auto& pool=*state.native_registry;const int first=owner.admission.simulation_id*64;
    const uint8_t mask=uint8_t(1u<<owner.admission.simulation_id);std::vector<std::pair<int32_t,int32_t>> order;
    for(int id=0;id<512;++id) {
        if(id>=64 && (id<first || id>=first+64)) {continue;}
        const auto& row=pool[id];const bool active=(row[48]&1)!=0;
        if(id<64) {
            if(!active || catalog.weapons.weapons.at(row[37]).offense<0) {continue;}
            const auto prerequisite=int16_t(uint16_t(row[50])|(uint16_t(row[51])<<8));
            if(!Available(prerequisite,technologies,owner)) {continue;}
        }
        int32_t key=id;
        if(!active) {key+=0x400;}
        else if(row[44]&mask) {
            if(id<64) {key+=0x2000;}
            else {if(!units) {throw std::runtime_error("Workshop browse requires the retained Unit-count row");}key+=units->at(id)>0 ? 0x800:0x1000;}
        }
        order.emplace_back(key,id);
    }
    std::sort(order.begin(),order.end());std::vector<int32_t> ids;ids.reserve(order.size());
    for(const auto& row:order) {ids.push_back(row.second);}return ids;
}

SmacWorkshopReselection PrepareSmacWorkshopReselection(const SmacStartupDesignState& state,int32_t requested,
    const std::string& retained_name,const SmacPredefinedUnitCatalog& catalog,int32_t owner) {
    state.Validate(catalog);
    if(owner<1 || owner>7 || !state.Find(owner) || !state.native_registry || requested>=512
        || (requested>=64 && requested/64!=owner) || retained_name.size()>31 || retained_name.find('\0')!=std::string::npos) {
        throw std::invalid_argument("Workshop reselect requires retained owner records and a bounded draft");
    }
    SmacWorkshopReselection result{state,{}};auto pool=*state.native_registry;
    const auto admissible=[&](int32_t id) {return id>=0 && (pool.at(id)[48]&1)
        && (id>=64 || catalog.weapons.weapons.at(pool.at(id)[37]).offense>=0);};
    int32_t selected=requested;
    if(!admissible(selected)) {
        selected=owner*64;
        // Inactive own records sort by raw ID, before excluded records.
        for(int32_t id=owner*64;id<(owner+1)*64;++id) {if(!(pool[id][48]&1)) {selected=id;break;}}
    }
    auto& row=pool.at(selected);auto& draft=result.draft;draft.selected_design=selected;
    if(!(row[48]&1)) {
        draft.name=retained_name; // New selection resets components, not the draft name or raw flags.
        return result;
    }
    draft.chassis=row[36];draft.weapon=row[37];draft.armor=row[38];draft.reactor=row[39];
    for(unsigned byte=0;byte<4;++byte) {draft.abilities|=uint32_t(row[32+byte])<<(8*byte);}
    RequireComponents(draft,catalog);row[48]&=uint8_t(~0x80u);
    if(selected>=64 && (row[48]&2)) {
        draft.name=std::string(row.begin(),std::find(row.begin(),row.begin()+32,uint8_t(0)));
        if(draft.name==NameSmacWorkshopDesign(pool,size_t(selected),draft,catalog,owner)) {row[48]&=uint8_t(~2u);draft.name.clear();}
    }
    size_t slot=0;for(int32_t bit=0;bit<24 && slot<2;++bit) {if(draft.abilities&(1u<<bit)) {result.ability_slots[slot++]=bit;}}
    result.state.RetainNativeRegistry(pool,catalog);return result;
}

SmacWorkshopObsolete PrepareSmacWorkshopObsolete(const SmacStartupDesignState& state,int32_t selected,
    const std::string& retained_name,const std::vector<int32_t>& old_list,size_t strip_offset,const SmacPredefinedUnitCatalog& catalog,
    const settings::SmacTechnologyRules& technologies,const faction::SmacFactionTechnologyState& owner,
    const std::optional<faction::SmacDesignCounts>& units) {
    RequireOwner(owner);const int32_t faction=owner.admission.simulation_id;
    if(!state.native_registry || selected<0 || selected>=512 || (selected>=64 && selected/64!=faction) || strip_offset>512) {
        throw std::invalid_argument("Invalid Workshop Obsolete selection or strip offset");
    }
    if(old_list.size()>128) {throw std::invalid_argument("Workshop Obsolete old list exceeds its owner domain");}
    std::array<bool,512> seen{};
    for(const auto id:old_list) {
        if(id<0 || id>=512 || (id>=64 && id/64!=faction) || seen.at(id)) {throw std::invalid_argument("Invalid Workshop Obsolete old browse identity");}
        seen[id]=true;
    }
    auto next=state;auto pool=*state.native_registry;auto& row=pool.at(selected);
    uint16_t flags=uint16_t(row[48])|(uint16_t(row[49])<<8);const uint8_t bit=uint8_t(1u<<faction);
    if(row[44]&bit) {if(flags&8) {flags|=0x200;}flags|=8;}
    else {flags&=0xfdf7;}
    row[48]=uint8_t(flags);row[49]=uint8_t(flags>>8);row[44]^=bit;
    next.RetainNativeRegistry(pool,catalog);
    int32_t chosen=selected;
    for(size_t scanned=0;scanned<old_list.size();++scanned) {
        const auto index=(strip_offset+scanned)%old_list.size();
        if(old_list[index]==selected) {chosen=old_list[(index+1)%old_list.size()];break;}
    }
    auto selection=PrepareSmacWorkshopReselection(next,chosen,retained_name,catalog,faction);
    const auto rebuilt=BrowseSmacWorkshopDesigns(selection.state,catalog,technologies,owner,units);
    const auto at=std::find(rebuilt.begin(),rebuilt.end(),selection.draft.selected_design);
    if(at!=rebuilt.end()) {strip_offset=(size_t(at-rebuilt.begin())+rebuilt.size()-5)%rebuilt.size();}
    return {std::move(selection),strip_offset};
}
namespace {
void RequireRename(const SmacStartupDesignState& state,const SmacWorkshopDraft& draft,const SmacPredefinedUnitCatalog& catalog,int32_t owner) {
    state.Validate(catalog);RequireComponents(draft,catalog);
    if(owner<1 || owner>7 || !state.native_registry || !state.Find(owner) || draft.selected_design<0 || draft.selected_design>=512
        || (draft.selected_design>=64 && draft.selected_design/64!=owner) || draft.name.size()>31 || draft.name.find('\0')!=std::string::npos) {
        throw std::invalid_argument("Invalid Workshop Rename draft or owner");
    }
}
}
SmacWorkshopRename OpenSmacWorkshopRename(const SmacStartupDesignState& state,const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,int32_t owner) {
    RequireRename(state,draft,catalog,owner);SmacWorkshopRename result{state,draft,draft.name};
    if(result.default_name.empty()) {
        auto pool=*state.native_registry;result.default_name=NameSmacWorkshopDesign(pool,size_t(draft.selected_design),draft,catalog,owner);
        pool.at(draft.selected_design)[48]&=uint8_t(~2u);result.state.RetainNativeRegistry(pool,catalog);
    }
    result.default_name.resize(std::min(size_t(31),result.default_name.size()));return result;
}
std::string DisplaySmacWorkshopName(const SmacOriginalDesignPool& pool,const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,int32_t owner) {
    RequireComponents(draft,catalog);
    if(draft.selected_design<0 || draft.selected_design>=512 || owner<1 || owner>7
        || (draft.selected_design>=64 && draft.selected_design/64!=owner)) {throw std::invalid_argument("Invalid Workshop title context");}
    const auto& row=pool.at(draft.selected_design);bool same=row[36]==draft.chassis && row[37]==draft.weapon && row[38]==draft.armor && row[39]==draft.reactor;
    for(unsigned i=0;i<4;++i) {same=same && row[32+i]==uint8_t(draft.abilities>>(8*i));}
    if(draft.selected_design<64 && same) {return std::string(row.begin(),std::find(row.begin(),row.begin()+32,uint8_t(0)));}
    if((row[48]&2) && !draft.name.empty()) {return draft.name;}
    return NameSmacWorkshopDesign(pool,size_t(draft.selected_design),draft,catalog,owner);
}
SmacWorkshopRename AnswerSmacWorkshopRename(const SmacStartupDesignState& state,const SmacWorkshopDraft& draft,
    const std::string& displayed_default,bool accepted,const std::string& submitted,
    const SmacPredefinedUnitCatalog& catalog,int32_t owner) {
    RequireRename(state,draft,catalog,owner);
    if(displayed_default.size()>31 || submitted.size()>31 || displayed_default.find('\0')!=std::string::npos || submitted.find('\0')!=std::string::npos) {
        throw std::invalid_argument("Workshop Rename text exceeds its native byte boundary");
    }
    SmacWorkshopRename result{state,draft,displayed_default};if(!accepted) {return result;}
    auto pool=*state.native_registry;auto& flags=pool.at(draft.selected_design)[48];
    if(submitted.empty()) {result.draft.name.clear();flags&=uint8_t(~2u);}
    else if(submitted!=displayed_default) {result.draft.name=submitted;flags|=2;}
    result.state.RetainNativeRegistry(pool,catalog);return result;
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
        if(state.native_registry && draft.selected_design>=64*owner.admission.simulation_id && draft.selected_design<64*(owner.admission.simulation_id+1)) {
            selected_named=((*state.native_registry)[draft.selected_design][48]&2)!=0;
        } else {
            if(selected==bank->designs.end() || !selected->active) {throw std::invalid_argument("Workshop alias selected a foreign or inactive design");}
            selected_named=selected->explicitly_named;
        }
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

SmacWorkshopPreparation PrepareSmacWorkshopCommit(const SmacStartupDesignState& state,const SmacWorkshopDraft& draft,
    const SmacPredefinedUnitCatalog& catalog,const settings::SmacTechnologyRules& technologies,
    const faction::SmacFactionTechnologyState& owner,const faction::SmacDesignCounts& units,
    const faction::SmacDesignCounts& production,bool confirm) {
    RequireOwner(owner);state.Validate(catalog);RequireComponents(draft,catalog);
    if(!state.native_registry || !catalog.workshop_name_labels || !state.Find(owner.admission.simulation_id)
        || draft.name.size()>31 || draft.name.find('\0')!=std::string::npos) {throw std::invalid_argument("Workshop common commit requires retained records, naming text and owned draft");}
    SmacWorkshopPreparation prepared{state,{draft.selected_design,false}};
    if(const auto reused=ReuseSmacWorkshopDesign(prepared.state,draft,catalog,technologies,owner)) {prepared.result=*reused;prepared.success=true;return prepared;}
    const auto require=[&](SmacWorkshopComponent kind,uint32_t id) {
        const auto choices=SmacWorkshopChoices(kind,catalog,technologies,owner);
        if(std::none_of(choices.begin(),choices.end(),[&](const auto& row){return row.component_id==id && row.technology_available;})) {throw std::invalid_argument("Workshop component technology is unavailable");}
    };
    require(SmacWorkshopComponent::Chassis,draft.chassis);require(SmacWorkshopComponent::Weapon,draft.weapon);
    require(SmacWorkshopComponent::Armor,draft.armor);require(SmacWorkshopComponent::Reactor,draft.reactor);
    const auto slot=ChooseSmacWorkshopSlot(*state.native_registry,draft,units,production,technologies,owner);
    prepared.result.design_id=slot.selected;prepared.confirmation=slot.confirmation;
    if(slot.kind==SmacWorkshopSlotKind::IntrinsicFailure) {prepared.result.intrinsic_name_warning=true;return prepared;}
    if(slot.kind!=SmacWorkshopSlotKind::Build) {throw std::runtime_error("Workshop allocation and alias passes disagree");}
    if(slot.confirmation && !confirm) {return prepared;}
    auto pool=*state.native_registry;
    if(slot.confirmation) {
        auto& history=prepared.state.retirement_history.at(owner.admission.simulation_id);
        if(!history) {throw std::runtime_error("Confirmed Workshop retirement requires retained tuple history");}
        SmacRetirementTuple tuple{};std::copy_n(pool.at(slot.selected).begin()+32,8,tuple.begin());history->Record(tuple);prepared.retirement=slot.selected;
        pool.at(slot.selected)[48]&=uint8_t(~1u); // 595d99 precedes the replacement builder/prototype scan
    }
    BuildSmacWorkshopRecord(pool,size_t(slot.selected),draft,catalog,technologies,owner);
    auto automatic=NameSmacWorkshopDesign(pool,size_t(slot.selected),draft,catalog,owner.admission.simulation_id);
    const auto& name=draft.name.empty() ? automatic:draft.name;auto& row=pool.at(slot.selected);
    const auto length=std::min<size_t>(31,name.size());std::copy_n(name.begin(),length,row.begin());row[length]=0;
    auto flags=uint16_t(uint16_t(row[48])|(uint16_t(row[49])<<8));flags&=uint16_t(~2u);
    if(!draft.name.empty() && draft.name!=automatic) {flags|=2;}row[48]=uint8_t(flags);row[49]=uint8_t(flags>>8);
    // This is a new component tuple, not the old riot-copy lineage.
    for(auto& bank:prepared.state.owners) {for(auto& design:bank.designs) {if(design.id==slot.selected) {design.riot.reset();}}}
    prepared.state.RetainNativeRegistry(pool,catalog);prepared.success=true;return prepared;
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
