#pragma once

#include "SmacResourceBonus.h"
#include <cstdint>

namespace game::backend::resource::smac {

// Decoded native query inputs. Class0/1/2 is retained without inventing a
// pickup outcome. Coefficients are explicit; the installed stock profile
// retains11/61 through its original map loader.
struct SiteSignalInputs {
    int32_t x, y;
    AltitudeBand altitude_band;
    uint32_t tile_flags;
    bool automatic_land_resources;
    int32_t pattern_offset, coefficient_u, coefficient_v;
};
uint8_t ResolveSiteSignal(const SiteSignalInputs& input);

}
