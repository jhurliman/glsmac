#pragma once

#include <algorithm>
#include <cstdint>

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

    static constexpr int LOGICAL_WIDTH = 1024, LOGICAL_HEIGHT = 768;

    // The original canvas is fixed. Prefer complete, equally sized source
    // pixels; smaller windows necessarily downsample but never change layout.
    static PresentationViewport Canonical(int drawable_width, int drawable_height) noexcept {
        const int scale = (std::min)(drawable_width / LOGICAL_WIDTH, drawable_height / LOGICAL_HEIGHT);
        if (scale < 1) { return Fit(drawable_width, drawable_height); }
        const int width = scale * LOGICAL_WIDTH, height = scale * LOGICAL_HEIGHT;
        return {(drawable_width-width)/2, (drawable_height-height)/2, width, height};
    }

    static bool MapCanonicalWindowPoint(int& px, int& py, int window_width, int window_height,
        int drawable_width, int drawable_height) noexcept {
        if(window_width<=0 || window_height<=0 || drawable_width<=0 || drawable_height<=0
            || px<0 || py<0 || px>=window_width || py>=window_height) { return false; }
        const auto area=Canonical(drawable_width,drawable_height);
        if(!area.width || !area.height) {return false;}
        // SDL events are window points, not Retina drawable pixels. Keep the
        // rational position through the bar test and inverse presentation map.
        const int64_t x=(int64_t(px)*2+1)*drawable_width-int64_t(area.x)*window_width*2;
        const int64_t y=(int64_t(py)*2+1)*drawable_height-int64_t(area.y)*window_height*2;
        const int64_t width=int64_t(area.width)*window_width*2,height=int64_t(area.height)*window_height*2;
        if(x<0 || y<0 || x>=width || y>=height) {return false;}
        px=static_cast<int>(x*LOGICAL_WIDTH/width);py=static_cast<int>(y*LOGICAL_HEIGHT/height);
        return true;
    }

    bool MapPoint(int& px, int& py) const noexcept {
        px -= x; py -= y;
        return px >= 0 && py >= 0 && px < width && py < height;
    }
};

}
