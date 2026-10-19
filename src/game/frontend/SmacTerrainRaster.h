#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace game::frontend::smac_terrain_raster {

using Point = std::array<int32_t, 2>;
struct Vertex { Point screen, uv; int32_t light; };
using Triangle = std::array<Vertex, 3>;
struct Polygon {std::array<Vertex,4> vertices{};uint8_t count=0;};
enum class Shader {OpaqueShaded,KeyedShaded,KeyedUnlit,WaterDepth16,CoastMapped};
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
struct Result {
    Bitmap bitmap;
    uint64_t writes = 0, attempts = 0;
    // Optional actual-store mask. A non-key index shaded to9 is still opaque.
    std::vector<uint8_t> coverage;
};
struct Command {
    Polygon polygon;
    // Synchronous borrowed source; must remain alive and unchanged for DrawBatch.
    const Bitmap* texture=nullptr;
    Shader shader=Shader::OpaqueShaded;
    Clip clip{{0,0,32767,32767}};
    std::array<int32_t,2> shade_limits{{-4,4}};
    const Bitmap* mask=nullptr; // synchronous borrowed56x56 coast class source
    std::array<uint8_t,53> mask_mapping{};
};

// Ordinary integer triangles only; preserves native winding, tie selection,
// fixed-point interpolation and x87 chop, independent of host FP controls.
// Unsupported signed32 intermediates are rejected rather than overflowing.
std::vector<Span> GenerateSpans(const Triangle& triangle, const Clip& clip);
// Native convex ordinary 3/4-vertex submissions. Unlit U/V omit the shaded
// path's half-texel edge bias. Unsupported polygon profiles remain unproved.
std::vector<Span> GeneratePolygonSpans(const Polygon& polygon,const Clip& clip,
    bool shaded=true,std::array<int32_t,2> shade_limits={{-4,4}});

// Keeps caller order across textures/shaders; copies the destination once.
// Key9 is compared before shading. No borrowed source escapes the result.
Result DrawBatch(const Bitmap& initial,const std::vector<Command>& commands,const Clip& clip,bool track_coverage=false);

// Loaded indexed texture bytes (including the original PCX index rebase).
// Shade addition wraps in the index domain; palette conversion is downstream.
// Copies the destination once for the complete ordered batch. No partial
// output escapes if any triangle or texture address is invalid.
Result Draw(const Bitmap& initial, const Bitmap& texture,
    const std::vector<Triangle>& triangles, const Clip& clip);

}
