#pragma once

#include "FriendlyHQMovement.h"

namespace gse::tests::remote_hq_fixture {
struct Case { friendly_hq_fixture::Case step; bool same_owner; bool reordered; bool tie; };
// Literal actual results; no original probe/oracle code imported.
// remote-hq-cardinality: fixture 550b61db461d78df2ff4435b1170b30d14bfdd245454fff03a5a258ebc61def4
// report c2718c3721fbc696179f8308218507f8370a2b7e9f0315344539adb37a24f999
// remote-hq-order: fixture baecd2226d82ded06447b79b706d31d58a2001c7dc750cabe28fa81220cdf192
// report 89c9976ea0971b4a181bd4a993e40cf45e95289f6560102e830cfefbc23aa346
inline constexpr std::array< Case, 21 > CASES{{
    { { "REMOTE-HQ-MOVE-01", 1, 0, 0, 0, 3u, 3, 3u, true, false }, false, false, false },
    { { "REMOTE-HQ-MOVE-02", 1, 5, 0, 0, 3u, 8, 1018897798u, true, false }, false, false, false },
    { { "REMOTE-HQ-MOVE-03", 1, 5, 0, 0, 1u, 6, 1015568748u, false, true }, false, false, false },
    { { "REMOTE-HQ-MOVE-04", 2, 0, 0, 0, 3u, 3, 3u, true, false }, false, false, false },
    { { "REMOTE-HQ-MOVE-05", 2, 5, 0, 0, 3u, 8, 1018897798u, true, false }, false, false, false },
    { { "REMOTE-HQ-MOVE-06", 2, 5, 0, 0, 1u, 6, 1015568748u, false, true }, false, false, false },
    { { "REMOTE-HQ-MOVE-07", 1, 0, 0, 32, 62u, 9, 1117104773u, true, false }, false, false, false },
    { { "REMOTE-HQ-MOVE-08", 1, 0, 0, 32, 2u, 6, 1017233273u, false, true }, false, false, false },
    { { "REMOTE-HQ-MOVE-09", 1, 6, 0, 8, 3u, 6, 3u, false, false }, false, false, false },
    { { "REMOTE-HQ-ORDER-01", 1, 0, 0, 0, 3u, 3, 3u, true, false }, true, false, false },
    { { "REMOTE-HQ-ORDER-02", 2, 0, 0, 0, 3u, 3, 3u, true, false }, true, false, false },
    { { "REMOTE-HQ-ORDER-03", 1, 0, 0, 32, 62u, 9, 1117104773u, true, false }, true, false, false },
    { { "REMOTE-HQ-ORDER-04", 1, 0, 0, 32, 2u, 6, 1017233273u, false, true }, true, false, false },
    { { "REMOTE-HQ-ORDER-05", 1, 0, 0, 0, 3u, 3, 3u, true, false }, true, true, false },
    { { "REMOTE-HQ-ORDER-06", 2, 0, 0, 0, 3u, 3, 3u, true, false }, true, true, false },
    { { "REMOTE-HQ-ORDER-07", 1, 0, 0, 32, 62u, 9, 1117104773u, true, false }, true, true, false },
    { { "REMOTE-HQ-ORDER-08", 1, 0, 0, 32, 2u, 6, 1017233273u, false, true }, true, true, false },
    { { "REMOTE-HQ-ORDER-09", 1, 0, 0, 32, 62u, 9, 1117104773u, true, false }, true, false, true },
    { { "REMOTE-HQ-ORDER-10", 1, 0, 0, 32, 2u, 6, 1017233273u, false, true }, true, false, true },
    { { "REMOTE-HQ-ORDER-11", 1, 0, 0, 32, 62u, 9, 1117104773u, true, false }, true, true, true },
    { { "REMOTE-HQ-ORDER-12", 1, 0, 0, 32, 2u, 6, 1017233273u, false, true }, true, true, true },
}};
}
