#pragma once

#include <array>
#include <cstdint>

namespace gse::tests::friendly_hq_fixture {
// Inputs from FRIENDLY-BASE-MOVE-001's public fixture (SHA256
// 0cec28996e4efad56b373f3a0a61c702295d307a27c24155d5298ad0a7a33b20).
// Outputs are literal guest-memory results from the independent retained report
// afd61ed5b9dc2aea531d5309180534780a4f93d148dffb8c287bb6eb768e398c.
// Only the70 Home::None cases are projected; no original implementation is used.
struct Case {
    const char* id;
    int endpoint, spent, source_flags, destination_flags;
    uint32_t seed;
    int spent_after;
    uint32_t shared_after;
    bool moved, failed;
};
inline constexpr std::array< Case, 70 > CASES{{
    { "FRIENDLY-BASE-MOVE-002", 1, 0, 0, 0, 3u, 3, 3u, true, false },
    { "FRIENDLY-BASE-MOVE-006", 1, 1, 0, 0, 3u, 4, 1018897798u, true, false },
    { "FRIENDLY-BASE-MOVE-007", 1, 1, 0, 0, 1u, 4, 1015568748u, true, false },
    { "FRIENDLY-BASE-MOVE-008", 1, 1, 0, 0, 0u, 3, 1013904223u, false, true },
    { "FRIENDLY-BASE-MOVE-012", 1, 2, 0, 0, 3u, 5, 1018897798u, true, false },
    { "FRIENDLY-BASE-MOVE-013", 1, 2, 0, 0, 1u, 3, 1015568748u, false, true },
    { "FRIENDLY-BASE-MOVE-014", 1, 2, 0, 0, 0u, 3, 1013904223u, false, true },
    { "FRIENDLY-BASE-MOVE-016", 1, 3, 0, 0, 3u, 3, 3u, false, false },
    { "FRIENDLY-BASE-MOVE-018", 1, 4, 0, 0, 3u, 4, 3u, false, false },
    { "FRIENDLY-BASE-MOVE-020", 2, 0, 0, 0, 3u, 3, 3u, true, false },
    { "FRIENDLY-BASE-MOVE-024", 2, 1, 0, 0, 3u, 4, 1018897798u, true, false },
    { "FRIENDLY-BASE-MOVE-025", 2, 1, 0, 0, 1u, 4, 1015568748u, true, false },
    { "FRIENDLY-BASE-MOVE-026", 2, 1, 0, 0, 0u, 3, 1013904223u, false, true },
    { "FRIENDLY-BASE-MOVE-030", 2, 2, 0, 0, 3u, 5, 1018897798u, true, false },
    { "FRIENDLY-BASE-MOVE-031", 2, 2, 0, 0, 1u, 3, 1015568748u, false, true },
    { "FRIENDLY-BASE-MOVE-032", 2, 2, 0, 0, 0u, 3, 1013904223u, false, true },
    { "FRIENDLY-BASE-MOVE-034", 2, 3, 0, 0, 3u, 3, 3u, false, false },
    { "FRIENDLY-BASE-MOVE-036", 2, 4, 0, 0, 3u, 4, 3u, false, false },
    { "FRIENDLY-BASE-MOVE-038", 0, 0, 0, 0, 3u, 3, 3u, true, false },
    { "FRIENDLY-BASE-MOVE-042", 0, 1, 0, 0, 3u, 4, 1018897798u, true, false },
    { "FRIENDLY-BASE-MOVE-043", 0, 1, 0, 0, 1u, 4, 1015568748u, true, false },
    { "FRIENDLY-BASE-MOVE-044", 0, 1, 0, 0, 0u, 3, 1013904223u, false, true },
    { "FRIENDLY-BASE-MOVE-048", 0, 2, 0, 0, 3u, 5, 1018897798u, true, false },
    { "FRIENDLY-BASE-MOVE-049", 0, 2, 0, 0, 1u, 3, 1015568748u, false, true },
    { "FRIENDLY-BASE-MOVE-050", 0, 2, 0, 0, 0u, 3, 1013904223u, false, true },
    { "FRIENDLY-BASE-MOVE-052", 0, 3, 0, 0, 3u, 3, 3u, false, false },
    { "FRIENDLY-BASE-MOVE-054", 0, 4, 0, 0, 3u, 4, 3u, false, false },
    { "FRIENDLY-BASE-MOVE-056", 1, 0, 0, 4, 3u, 1, 3u, true, false },
    { "FRIENDLY-BASE-MOVE-058", 1, 2, 0, 4, 3u, 3, 3u, true, false },
    { "FRIENDLY-BASE-MOVE-060", 1, 3, 0, 4, 3u, 3, 3u, false, false },
    { "FRIENDLY-BASE-MOVE-062", 1, 0, 0, 8, 3u, 0, 3u, true, false },
    { "FRIENDLY-BASE-MOVE-064", 1, 2, 0, 8, 3u, 2, 3u, true, false },
    { "FRIENDLY-BASE-MOVE-066", 1, 3, 0, 8, 3u, 3, 3u, false, false },
    { "FRIENDLY-BASE-MOVE-068", 2, 0, 4, 0, 3u, 1, 3u, true, false },
    { "FRIENDLY-BASE-MOVE-070", 2, 2, 4, 0, 3u, 3, 3u, true, false },
    { "FRIENDLY-BASE-MOVE-072", 2, 3, 4, 0, 3u, 3, 3u, false, false },
    { "FRIENDLY-BASE-MOVE-074", 2, 0, 8, 0, 3u, 0, 3u, true, false },
    { "FRIENDLY-BASE-MOVE-076", 2, 2, 8, 0, 3u, 2, 3u, true, false },
    { "FRIENDLY-BASE-MOVE-078", 2, 3, 8, 0, 3u, 3, 3u, false, false },
    { "FRIENDLY-BASE-MOVE-088", 1, 0, 0, 32, 33u, 9, 1068833548u, true, false },
    { "FRIENDLY-BASE-MOVE-089", 1, 0, 0, 32, 3u, 9, 1018897798u, true, false },
    { "FRIENDLY-BASE-MOVE-090", 1, 0, 0, 32, 16u, 9, 1040536623u, true, false },
    { "FRIENDLY-BASE-MOVE-091", 1, 0, 0, 32, 1u, 3, 1015568748u, false, true },
    { "FRIENDLY-BASE-MOVE-092", 1, 0, 0, 32, 4u, 3, 1020562323u, false, true },
    { "FRIENDLY-BASE-MOVE-093", 1, 0, 0, 32, 62u, 3, 1117104773u, false, true },
    { "FRIENDLY-BASE-MOVE-094", 1, 0, 0, 32, 2u, 3, 1017233273u, false, true },
    { "FRIENDLY-BASE-MOVE-095", 1, 0, 0, 32, 50u, 3, 1097130473u, false, true },
    { "FRIENDLY-BASE-MOVE-096", 1, 0, 0, 32, 0u, 3, 1013904223u, false, true },
    { "FRIENDLY-BASE-MOVE-106", 1, 1, 0, 32, 33u, 10, 1068833548u, true, false },
    { "FRIENDLY-BASE-MOVE-107", 1, 1, 0, 32, 3u, 10, 1018897798u, true, false },
    { "FRIENDLY-BASE-MOVE-108", 1, 1, 0, 32, 16u, 3, 1040536623u, false, true },
    { "FRIENDLY-BASE-MOVE-109", 1, 1, 0, 32, 1u, 3, 1015568748u, false, true },
    { "FRIENDLY-BASE-MOVE-110", 1, 1, 0, 32, 4u, 3, 1020562323u, false, true },
    { "FRIENDLY-BASE-MOVE-111", 1, 1, 0, 32, 62u, 3, 1117104773u, false, true },
    { "FRIENDLY-BASE-MOVE-112", 1, 1, 0, 32, 2u, 3, 1017233273u, false, true },
    { "FRIENDLY-BASE-MOVE-113", 1, 1, 0, 32, 50u, 3, 1097130473u, false, true },
    { "FRIENDLY-BASE-MOVE-114", 1, 1, 0, 32, 0u, 3, 1013904223u, false, true },
    { "FRIENDLY-BASE-MOVE-124", 1, 2, 0, 32, 33u, 11, 1068833548u, true, false },
    { "FRIENDLY-BASE-MOVE-125", 1, 2, 0, 32, 3u, 3, 1018897798u, false, true },
    { "FRIENDLY-BASE-MOVE-126", 1, 2, 0, 32, 16u, 3, 1040536623u, false, true },
    { "FRIENDLY-BASE-MOVE-127", 1, 2, 0, 32, 1u, 3, 1015568748u, false, true },
    { "FRIENDLY-BASE-MOVE-128", 1, 2, 0, 32, 4u, 3, 1020562323u, false, true },
    { "FRIENDLY-BASE-MOVE-129", 1, 2, 0, 32, 62u, 3, 1117104773u, false, true },
    { "FRIENDLY-BASE-MOVE-130", 1, 2, 0, 32, 2u, 3, 1017233273u, false, true },
    { "FRIENDLY-BASE-MOVE-131", 1, 2, 0, 32, 50u, 3, 1097130473u, false, true },
    { "FRIENDLY-BASE-MOVE-132", 1, 2, 0, 32, 0u, 3, 1013904223u, false, true },
    { "FRIENDLY-BASE-MOVE-134", 2, 0, 32, 0, 3u, 3, 3u, true, false },
    { "FRIENDLY-BASE-MOVE-138", 2, 2, 32, 0, 3u, 5, 1018897798u, true, false },
    { "FRIENDLY-BASE-MOVE-139", 2, 2, 32, 0, 1u, 3, 1015568748u, false, true },
    { "FRIENDLY-BASE-MOVE-140", 2, 2, 32, 0, 0u, 3, 1013904223u, false, true },
}};
}
