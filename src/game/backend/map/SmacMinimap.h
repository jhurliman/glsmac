#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace game::backend::map::smac_minimap {

struct Color {
    uint8_t index;
    std::array<uint8_t,3> rgb;
    bool operator==(const Color& other) const {return index==other.index && rgb==other.rgb;}
};
// Supplied assigned map-slot colors, not vehicle or faction-text tints. A live
// producer must establish assignment before requesting political output.
using MapColors=std::array<std::optional<Color>,8>;
struct Input {
    int32_t band=3, moisture=0, viewer=3, claimant=-1;
    uint32_t flags=0x80000000u, relation=0;
    bool explored=true, initial_survey=false, revealed=false, editor=false;
    // Native political ownership uses display/focus faction separately from
    // the renderer viewer used to construct explored/revealed admission.
    std::optional<int32_t> display_faction;
};
// Complete coarse tile-color decision, not the original polygon sampler or
// minimap resampling/camera. Absence means no polygon, preserving its backing.
std::optional<Color> Resolve(const Input& input,const MapColors& colors={});

}
