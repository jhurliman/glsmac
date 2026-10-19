#pragma once
#include "SmacFactionStartupRules.h"
#include <optional>
#include <cstdint>

namespace game::backend::settings {
// Original badge colors belong to the loaded definition slot, never the port's
// dense player/controller slot. Only our frozen canonical source order is
// admitted here; reordered/custom catalogs retain the legacy presentation.
inline std::array<uint8_t,7> SmacBadgeSlots(const SmacFactionStartupRules& profiles,
    const std::array<std::optional<std::string>,7>& players,
    const std::optional<std::array<std::string,8>>& retained = {}) {
    std::array<uint8_t,7> unavailable;unavailable.fill(255);
    const auto& bindings=SmacFactionStartupRules::Bindings();
    for(size_t i=0;i<7;++i) {
        const auto& profile=profiles.factions[i];const auto& source=bindings[i];
        if(profile.port_id!=source.port_id || profile.filename!=source.filename || profile.section!=source.section
            || (retained && retained->at(i+1)!=profile.section)) {return unavailable;}
    }
    if(retained && !retained->at(0).empty()) {return unavailable;}
    auto result=unavailable;std::array<bool,8> assigned{};
    for(size_t player=0;player<players.size();++player) {
        if(!players[player]) {continue;}
        size_t source=0;while(source<7 && profiles.factions[source].port_id!=*players[player]) {++source;}
        if(source==7 || assigned[source+1]) {return unavailable;}
        assigned[source+1]=true;result[player]=static_cast<uint8_t>(source+1);
    }
    return result;
}
}
