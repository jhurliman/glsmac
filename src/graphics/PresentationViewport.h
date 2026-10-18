#pragma once

#include <algorithm>

namespace graphics {

// Window coordinates use a top-left origin. Multiples of 8x6 retain the
// renderer's even-dimension requirement while preserving exactly 4:3.
struct PresentationViewport {
    int x = 0, y = 0, width = 0, height = 0;

    static PresentationViewport Fit(int window_width, int window_height) noexcept {
        const int scale = (std::max)(0, (std::min)(window_width / 8, window_height / 6));
        const int width = scale * 8, height = scale * 6;
        return {(window_width - width) / 2, (window_height - height) / 2, width, height};
    }

    bool MapPoint(int& px, int& py) const noexcept {
        px -= x; py -= y;
        return px >= 0 && py >= 0 && px < width && py < height;
    }
};

}
