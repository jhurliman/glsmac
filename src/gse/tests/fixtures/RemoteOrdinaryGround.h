#pragma once

#include <array>
#include <cstdint>

namespace gse::tests::remote_ground_fixture {
// Literal actual original results; no original oracle/probe code imported.
// RE-SMAC-REMOTE-ORDINARY-GROUND-001
// Fixture SHA256: 98d98b69c7ffb3a585a9f2a44b48242d755fcf9b7ca59d9a3a38721d5a1424b9
// Report SHA256: 08ccad0e0b7774bcf5af2db6c383da695563ea543cf3b7fe96c4e5a53b1f194a
struct Case { const char* id; bool rover, remote; int spent, source_flags, destination_flags; uint32_t seed; int allowance, spent_after; uint32_t shared_after, crt_after; bool moved, failed; };
inline constexpr std::array< Case, 24 > CASES{{
    { "REMOTE-GROUND-SPARTANS-01", true, true, 0, 0, 0, 3u, 6, 3, 3u, 2712847316u, true, false },
    { "REMOTE-GROUND-SPARTANS-02", true, true, 5, 0, 0, 3u, 6, 8, 1018897798u, 2712847316u, true, false },
    { "REMOTE-GROUND-SPARTANS-03", true, true, 5, 0, 0, 1u, 6, 6, 1015568748u, 2712847316u, false, true },
    { "REMOTE-GROUND-SPARTANS-04", true, true, 5, 4, 4, 3u, 6, 6, 3u, 2712847316u, true, false },
    { "REMOTE-GROUND-SPARTANS-05", true, true, 5, 8, 8, 3u, 6, 5, 3u, 2712847316u, true, false },
    { "REMOTE-GROUND-SPARTANS-06", true, true, 6, 8, 8, 3u, 6, 6, 3u, 2712847316u, false, false },
    { "REMOTE-GROUND-SPARTANS-07", true, false, 0, 0, 0, 3u, 6, 3, 3u, 2712847316u, true, false },
    { "REMOTE-GROUND-SPARTANS-08", true, false, 5, 0, 0, 3u, 6, 8, 1018897798u, 2712847316u, true, false },
    { "REMOTE-GROUND-SPARTANS-09", true, false, 5, 0, 0, 1u, 6, 6, 1015568748u, 2712847316u, false, true },
    { "REMOTE-GROUND-SPARTANS-10", true, false, 5, 4, 4, 3u, 6, 6, 3u, 2712847316u, true, false },
    { "REMOTE-GROUND-SPARTANS-11", true, false, 5, 8, 8, 3u, 6, 5, 3u, 2712847316u, true, false },
    { "REMOTE-GROUND-SPARTANS-12", true, false, 6, 8, 8, 3u, 6, 6, 3u, 2712847316u, false, false },
    { "REMOTE-GROUND-GAIANS-01", false, true, 0, 0, 0, 3u, 3, 3, 3u, 2712847316u, true, false },
    { "REMOTE-GROUND-GAIANS-02", false, true, 2, 0, 0, 3u, 3, 5, 1018897798u, 2712847316u, true, false },
    { "REMOTE-GROUND-GAIANS-03", false, true, 2, 0, 0, 1u, 3, 3, 1015568748u, 2712847316u, false, true },
    { "REMOTE-GROUND-GAIANS-04", false, true, 2, 4, 4, 3u, 3, 3, 3u, 2712847316u, true, false },
    { "REMOTE-GROUND-GAIANS-05", false, true, 2, 8, 8, 3u, 3, 2, 3u, 2712847316u, true, false },
    { "REMOTE-GROUND-GAIANS-06", false, true, 3, 8, 8, 3u, 3, 3, 3u, 2712847316u, false, false },
    { "REMOTE-GROUND-GAIANS-07", false, false, 0, 0, 0, 3u, 3, 3, 3u, 2712847316u, true, false },
    { "REMOTE-GROUND-GAIANS-08", false, false, 2, 0, 0, 3u, 3, 5, 1018897798u, 2712847316u, true, false },
    { "REMOTE-GROUND-GAIANS-09", false, false, 2, 0, 0, 1u, 3, 3, 1015568748u, 2712847316u, false, true },
    { "REMOTE-GROUND-GAIANS-10", false, false, 2, 4, 4, 3u, 3, 3, 3u, 2712847316u, true, false },
    { "REMOTE-GROUND-GAIANS-11", false, false, 2, 8, 8, 3u, 3, 2, 3u, 2712847316u, true, false },
    { "REMOTE-GROUND-GAIANS-12", false, false, 3, 8, 8, 3u, 3, 3, 3u, 2712847316u, false, false },
}};
}
