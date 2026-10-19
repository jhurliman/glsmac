#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace game::frontend::smac_terrain_raster {

using Point = std::array<int32_t, 2>;
struct Vertex { Point screen, uv; int32_t light; };
using Triangle = std::array<Vertex, 3>;
// Left/top inclusive, right/bottom exclusive, in the target bitmap.
using Clip = std::array<int32_t, 4>;
struct Span {
    int32_t y, left, right, u, v, du, dv;
    uint16_t shade, dshade;
    bool operator==(const Span& other) const;
};
struct Bitmap {
    int32_t width, height;
    std::vector<uint8_t> pixels;
};
struct Result { Bitmap bitmap; uint64_t writes = 0; };

// Ordinary integer triangles only; preserves native winding, tie selection,
// fixed-point interpolation and x87 chop, independent of host FP controls.
// Unsupported signed32 intermediates are rejected rather than overflowing.
std::vector<Span> GenerateSpans(const Triangle& triangle, const Clip& clip);

// Loaded indexed texture bytes (including the original PCX index rebase).
// Shade addition wraps in the index domain; palette conversion is downstream.
// Copies the destination once for the complete ordered batch. No partial
// output escapes if any triangle or texture address is invalid.
Result Draw(const Bitmap& initial, const Bitmap& texture,
    const std::vector<Triangle>& triangles, const Clip& clip);

}
