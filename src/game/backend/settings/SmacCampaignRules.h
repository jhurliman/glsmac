#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include "types/Buffer.h"

namespace game::backend::settings {

// Active save words and remembered setup preferences have distinct lifetimes.
// Absent GlobalSettings authority represents the older explicit port campaign;
// it must not be relabeled original Standard Rules during migration.
struct SmacCampaignRules {
    static constexpr uint32_t STANDARD = 0x1a0e;
    static constexpr uint32_t RANDOMIZATION = 0x01800000;
    static constexpr size_t MAX_WIRE_BYTES = 128;
    uint32_t primary=0,secondary=0,remembered_primary=0,remembered_secondary=0;
    int32_t menu_choice=-1;
    bool ordinary_new_game=false;
    int32_t objective_facility=0;

    void BeginOrdinary();
    void SelectStandard();
    void SelectCurrent(bool citizen);
    bool BlindResearch() const {return (primary&0x0200)!=0;}
    bool Stagnation() const {return (primary&0x0020)!=0;}
    bool AutomaticLandResources() const {return (primary&0x2000)!=0;}
    bool CooperativeVictory() const {return (primary&0x1000)!=0;}
    bool RestartsAllowed() const {return (primary&0x0001)==0;}
    bool IsOrdinary() const;
    void Validate() const;
    bool operator==(const SmacCampaignRules& other) const;
    bool operator!=(const SmacCampaignRules& other) const { return !(*this==other); }
    std::string Serialize() const;
    static SmacCampaignRules Deserialize(const std::string& wire);
};
}
