#pragma once
#include "SmacOpeningDesigns.h"

namespace game::backend::unit {
struct SmacDesignChassis {int32_t speed,domain;bool missile;int32_t cargo,cost;};
struct SmacDesignWeapon {int32_t power,mode,cost;};
struct SmacDesignArmor {int32_t rating,cost;};
struct SmacDesignAbility {int32_t cost;uint32_t flags;};
struct SmacDesignComponents {
    std::array<SmacDesignChassis,9> chassis;
    std::array<SmacDesignWeapon,21> weapons;
    std::array<SmacDesignArmor,10> armor;
    std::array<SmacDesignAbility,24> abilities;
};
struct SmacPredefinedDesign {
    std::string name;int32_t chassis,weapon,armor,plan,cost,capacity,prerequisite,icon;uint32_t abilities;
};
// Complete stock59dfc0 pool transformation, preserving all unrelated bytes.
// Components and fourteen ordered rows come from parsed installed source.
void LoadSmacPredefinedDesigns(SmacOriginalDesignPool& pool,const SmacDesignComponents& components,
    const std::vector<SmacPredefinedDesign>& rows,const SmacDesignObserver& observer={});
// Shared original Fission component builder used before row/allocation overrides.
void BuildSmacFissionDesign(SmacOriginalDesignPool& pool,size_t index,const SmacDesignComponents& components,
    int32_t chassis,int32_t weapon,int32_t armor,uint32_t abilities,uint8_t owner_mask,const SmacDesignObserver& observer={});
struct SmacEarlyDesignNames {std::string armor_short,infantry_even,infantry_odd,scout,rover;int32_t armor_rating;};
std::string NameSmacOpeningDesign(const SmacEarlyDesignNames& names,int32_t owner,int32_t chassis,int32_t armor);
// Earlier fixed-component controls retain their explicit stock early profile.
SmacDesignComponents SmacEarlyFissionComponents();
int32_t GenerateSmacInstalledOpeningDesigns(std::array<faction::SmacStartupFaction,8>& actors,SmacOriginalDesignPool& designs,
    const faction::SmacStartupTechnologyOwnership& ownership,const std::array<std::array<int32_t,2>,89>& prerequisites,
    const faction::SmacCapabilityCatalog& catalog,const SmacDesignComponents& components,const SmacEarlyDesignNames& names,
    int32_t owner,int32_t difficulty,int32_t& first_new_design,const SmacDesignObserver& observer={});
}
