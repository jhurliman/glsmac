#pragma once

#include "FlicReader.h"

namespace loader::flic {

struct MenuImage {
    size_t width, height;
    std::vector< uint8_t > rgba;
};

// Native opaque PCX background indices are common-palette indices (0..235).
// FLC indices are game indices: 0 applies blue lookup, 9 leaves the background.
MenuImage ComposeMenu( const Frame& frame, const std::vector< uint8_t >& background,
    size_t background_width, size_t background_height,
    const std::array< std::array< uint8_t, 3 >, 256 >& common_palette, size_t rows );

}
