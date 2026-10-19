#include "SmacMinimap.h"
#include <stdexcept>

namespace game::backend::map::smac_minimap {
std::optional<Color> Resolve(const Input& in,const MapColors& colors) {
    const auto display=in.display_faction.value_or(in.viewer);
    if(in.band<0 || in.band>7 || in.moisture<0 || in.moisture>3 || in.viewer<1 || in.viewer>7
        || display<1 || display>7 || in.claimant< -1 || in.claimant>7 || !(in.flags&0x80000000u)
        || (in.flags&~0x80010010u) || (in.flags&0x10010u)==0x10010u) {
        throw std::invalid_argument("Unsupported coarse minimap context");
    }
    if(!in.explored && !in.initial_survey && !in.revealed) {return {};}
    if(in.band<3) {return Color{190,{{14,37,75}}};}
    if((in.flags&0x10000u) && (in.explored || in.revealed) && in.claimant>0
        && (in.claimant==display || in.editor || (in.relation&0x08000008u)==0x08000008u)) {
        const auto& color=colors.at(static_cast<size_t>(in.claimant));
        if(!color) {throw std::invalid_argument("Political minimap lacks assigned map color");}
        return color;
    }
    if(in.flags&0x10u) {
        static constexpr std::array<Color,4> rainfall{{
            {149,{{212,208,116}}},{151,{{152,176,76}}},{153,{{88,140,44}}},{155,{{216,224,235}}}
        }};
        return rainfall.at(static_cast<size_t>(in.moisture));
    }
    static constexpr std::array<Color,5> physical{{
        {22,{{80,65,51}}},{20,{{93,76,59}}},{18,{{106,87,67}}},{16,{{120,98,74}}},{14,{{133,109,82}}}
    }};
    return physical.at(static_cast<size_t>(in.band-3));
}
}
