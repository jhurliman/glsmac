#pragma once

#include "FriendlyHQMovement.h"

namespace gse::tests::spartan_hq_fixture {
// Ordered inputs: 3b2991051b1d4f724f4403f65a4e9ed9c0b4b4f0a74036d20d2dded39f183669.
// Literal checked original outcomes: 60156db4944b3793c25c9d6d4b627cea54a552756566e2f2b1d930d78f0faa24.
// All21 actual startup-selected Rover requests; no original implementation.
inline constexpr std::array< friendly_hq_fixture::Case, 21 > CASES{{
    { "SPARTAN-HQ-MOVE-01", 1, 0, 0, 0, 3u, 3, 3u, true, false },
    { "SPARTAN-HQ-MOVE-02", 1, 5, 0, 0, 3u, 8, 1018897798u, true, false },
    { "SPARTAN-HQ-MOVE-03", 1, 5, 0, 0, 1u, 6, 1015568748u, false, true },
    { "SPARTAN-HQ-MOVE-04", 1, 6, 0, 0, 3u, 6, 3u, false, false },
    { "SPARTAN-HQ-MOVE-05", 1, 5, 0, 4, 3u, 6, 3u, true, false },
    { "SPARTAN-HQ-MOVE-06", 1, 5, 0, 8, 3u, 5, 3u, true, false },
    { "SPARTAN-HQ-MOVE-07", 1, 6, 0, 8, 3u, 6, 3u, false, false },
    { "SPARTAN-HQ-MOVE-08", 2, 0, 0, 0, 3u, 3, 3u, true, false },
    { "SPARTAN-HQ-MOVE-09", 2, 5, 0, 0, 3u, 8, 1018897798u, true, false },
    { "SPARTAN-HQ-MOVE-10", 2, 5, 0, 0, 1u, 6, 1015568748u, false, true },
    { "SPARTAN-HQ-MOVE-11", 2, 6, 0, 0, 3u, 6, 3u, false, false },
    { "SPARTAN-HQ-MOVE-12", 2, 5, 4, 0, 3u, 6, 3u, true, false },
    { "SPARTAN-HQ-MOVE-13", 2, 5, 8, 0, 3u, 5, 3u, true, false },
    { "SPARTAN-HQ-MOVE-14", 2, 6, 8, 0, 3u, 6, 3u, false, false },
    { "SPARTAN-HQ-MOVE-15", 1, 0, 0, 32, 62u, 9, 1117104773u, true, false },
    { "SPARTAN-HQ-MOVE-16", 1, 0, 0, 32, 2u, 6, 1017233273u, false, true },
    { "SPARTAN-HQ-MOVE-17", 1, 4, 0, 32, 3u, 13, 1018897798u, true, false },
    { "SPARTAN-HQ-MOVE-18", 1, 4, 0, 32, 16u, 6, 1040536623u, false, true },
    { "SPARTAN-HQ-MOVE-19", 1, 6, 0, 32, 3u, 6, 3u, false, false },
    { "SPARTAN-HQ-MOVE-20", 2, 5, 32, 0, 3u, 8, 1018897798u, true, false },
    { "SPARTAN-HQ-MOVE-21", 2, 5, 32, 0, 1u, 6, 1015568748u, false, true },
}};
}
