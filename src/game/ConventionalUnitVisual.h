#pragma once
#include <cstdint>
#include <string>
#include <tuple>

namespace game {
// Owned frontend projection. Identity, rules and save authority remain in the
// backend retained design; a shared presentation Def is not a component tuple.
struct ConventionalUnitVisual {
    int32_t owner,chassis,weapon,armor,reactor,weapon_power,weapon_mode,armor_strength;
    uint32_t abilities;
    std::string faction_art;
    auto Fields() const {return std::tie(owner,chassis,weapon,armor,reactor,weapon_power,weapon_mode,armor_strength,abilities,faction_art);}
    bool operator==(const ConventionalUnitVisual& other) const {return Fields()==other.Fields();}
    bool operator!=(const ConventionalUnitVisual& other) const {return !(*this==other);}
    std::string Key() const {
        return std::to_string(owner)+":"+std::to_string(chassis)+":"+std::to_string(weapon)+":"+std::to_string(armor)+":"+
            std::to_string(reactor)+":"+std::to_string(weapon_power)+":"+std::to_string(weapon_mode)+":"+
            std::to_string(armor_strength)+":"+std::to_string(abilities)+":"+std::to_string(faction_art.size())+":"+faction_art;
    }
};
}
