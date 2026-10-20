#pragma once
#include "game/backend/faction/SmacStartupCapability.h"

namespace game::backend::unit {
struct SmacDesignComponents;
using SmacOriginalDesign=std::array<uint8_t,52>;
using SmacOriginalDesignPool=std::array<SmacOriginalDesign,512>;
struct SmacDesignStore {
    enum class Space:uint8_t {Designs,Factions,Global};
    Space space;uint32_t offset;uint8_t size;uint32_t value,caller;
};
using SmacDesignObserver=std::function<void(const SmacDesignStore&)>;
// First Synthmetal defender / Speeder Recon, or basic Scout shortcut. Text is
// already localized single-byte text. Arbitrary workshop/replacement is excluded.
int32_t ConstructSmacOpeningDesign(SmacOriginalDesignPool& designs,int32_t owner,int32_t chassis,int32_t armor,
    int32_t difficulty,const std::string& name,int32_t& first_new_design,bool allow_first_defender=false,
    const SmacDesignObserver& observer={},const SmacDesignComponents* components=nullptr);
// Original caller loop: prototype every active custom design, select last Recon.
int32_t MarkAndSelectSmacStartingScout(SmacOriginalDesignPool& designs,int32_t owner,const SmacDesignObserver& observer={});
using SmacOpeningDesignName=std::function<std::string(int32_t owner,int32_t chassis,int32_t armor)>;
// Stock named-grant opening profile only: no own Bases/Units, custom designs,
// contacts, PSI projects/bonuses or advanced technologies/workshop state.
// Caller supplies owned stock numeric catalog and parsed prerequisites, and the
// original localized name producer. No captured capability/selector outputs.
int32_t GenerateSmacOpeningDesigns(std::array<faction::SmacStartupFaction,8>& actors,SmacOriginalDesignPool& designs,
    const faction::SmacStartupTechnologyOwnership& ownership,const std::array<std::array<int32_t,2>,89>& prerequisites,
    const faction::SmacCapabilityCatalog& catalog,int32_t owner,int32_t difficulty,int32_t& first_new_design,
    const SmacOpeningDesignName& name,const SmacDesignObserver& observer={},const SmacDesignComponents* components=nullptr);
}
