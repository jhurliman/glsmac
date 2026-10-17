#pragma once

#include "FriendlyHQMovement.h"

namespace gse::tests::gaian_hq_fixture {
// Ordered inputs: 1026ea4f056acb9597c48ac754fefc09373603eb929c48b04b5cbb00c04f3a93.
// Checked original outcomes: 128c064b7becb7ed4ef1600888f14042b9d1999f04c8aec790e5a62e3518532a.
inline constexpr std::array< friendly_hq_fixture::Case, 8 > CASES{{
    { "GAIAN-HQ-MOVE-01", 1, 2, 0, 0, 3u, 5, 1018897798u, true, false },
    { "GAIAN-HQ-MOVE-02", 1, 2, 0, 0, 1u, 3, 1015568748u, false, true },
    { "GAIAN-HQ-MOVE-03", 2, 2, 0, 0, 3u, 5, 1018897798u, true, false },
    { "GAIAN-HQ-MOVE-04", 2, 2, 0, 0, 1u, 3, 1015568748u, false, true },
    { "GAIAN-HQ-MOVE-05", 1, 0, 0, 32, 3u, 3, 3u, true, false },
    { "GAIAN-HQ-MOVE-06", 1, 1, 0, 32, 1u, 4, 1015568748u, true, false },
    { "GAIAN-HQ-MOVE-07", 1, 1, 0, 32, 0u, 3, 1013904223u, false, true },
    { "GAIAN-HQ-MOVE-08", 1, 3, 0, 8, 3u, 3, 3u, false, false },
}};
}
