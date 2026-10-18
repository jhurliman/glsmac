#pragma once
// Literal outputs from RE-SMAC-HURRY-001; excludes5 composed reset/switch cases.
namespace gse::tests::fixtures {
struct HurryCase {
    const char* id; int item,stored,cost,factor,exemption,percent; bool voice;
    int treasury,reserved; bool reserve_payment,marker,riots,blocked; int choice,entered;
    int quote,stored_after,baseline_after,baseline_item_after; bool marker_after; int treasury_after,reserved_after;
};
inline constexpr HurryCase hurry_cases[] = {
    {"HURRY-001", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 0, 30, 60, 20, 7, -1, false, 1000, 0},
    {"HURRY-002", -3, 20, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 0, 30, 40, 20, 7, -1, false, 1000, 0},
    {"HURRY-003", -34, 20, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 0, 30, 40, 20, 7, -1, false, 1000, 0},
    {"HURRY-004", -39, 39, 200, 10, 10, 100, false, 1000, 0, false, false, false, false, 0, 30, 1288, 39, 7, -1, false, 1000, 0},
    {"HURRY-005", -39, 40, 200, 10, 10, 100, false, 1000, 0, false, false, false, false, 0, 30, 640, 40, 7, -1, false, 1000, 0},
    {"HURRY-006", 0, 9, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 0, 30, 220, 9, 7, -1, false, 1000, 0},
    {"HURRY-007", 0, 10, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 0, 30, 105, 10, 7, -1, false, 1000, 0},
    {"HURRY-008", 0, 20, 40, 10, 10, 100, true, 1000, 0, false, false, false, false, 0, 30, 120, 20, 7, -1, false, 1000, 0},
    {"HURRY-009", 0, 20, 40, 10, 10, 67, false, 1000, 0, false, false, false, false, 0, 30, 40, 20, 7, -1, false, 1000, 0},
    {"HURRY-010", 0, 20, 40, 10, 10, 0, false, 1000, 0, false, false, false, false, 0, 30, 0, 20, 7, -1, false, 1000, 0},
    {"HURRY-011", 0, 40, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 0, 30, 0, 40, 7, -1, false, 1000, 0},
    {"HURRY-012", 0, 41, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 0, 30, -2, 41, 7, -1, false, 1000, 0},
    {"HURRY-013", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 1, 30, 60, 40, 40, 0, false, 940, 0},
    {"HURRY-014", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 2, 30, 60, 30, 30, 0, true, 970, 0},
    {"HURRY-015", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 2, 5, 60, 21, 21, 0, true, 995, 0},
    {"HURRY-016", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 2, 2, 60, 20, 20, 0, true, 998, 0},
    {"HURRY-017", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 2, 0, 60, 20, 20, 0, true, 1000, 0},
    {"HURRY-018", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 2, -1, 60, 20, 20, 0, true, 1000, 0},
    {"HURRY-019", 0, 20, 40, 10, 10, 100, false, 25, 20, false, false, false, false, 2, 30, 60, 21, 21, 0, true, 20, 20},
    {"HURRY-020", 0, 20, 40, 10, 10, 100, false, 10, 20, false, false, false, false, 2, 30, 60, 20, 20, 0, true, 10, 20},
    {"HURRY-021", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 2, 100, 60, 53, 53, 0, true, 900, 0},
    {"HURRY-022", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, true, false, false, 2, 30, 60, 20, 7, -1, true, 1000, 0},
    {"HURRY-023", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, true, false, false, 1, 30, 60, 20, 7, -1, true, 1000, 0},
    {"HURRY-024", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, false, false, false, 2, 30, 60, 30, 30, 0, true, 970, 0},
    {"HURRY-028", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, false, true, false, 1, 30, 60, 20, 7, -1, false, 1000, 0},
    {"HURRY-029", -3, 20, 40, 10, 10, 100, false, 1000, 0, false, false, true, false, 1, 30, 40, 40, 40, -3, false, 960, 0},
    {"HURRY-030", -34, 20, 40, 10, 10, 100, false, 1000, 0, false, false, true, false, 0, 30, 40, 20, 7, -1, false, 1000, 0},
    {"HURRY-031", 0, 20, 40, 10, 10, 100, false, 1000, 0, false, false, false, true, 2, 30, 60, 20, 7, -1, false, 1000, 0},
    {"HURRY-034", 0, 75, 30, 10, 10, 100, false, 1000, 0, false, false, false, false, 0, 30, 11, 75, 7, -1, false, 1000, 0},
    {"HURRY-035", 0, 75, 30, 10, 10, 100, false, 1000, 0, false, false, false, false, 2, 5, 11, 55, 55, 0, true, 995, 0},
    {"HURRY-036", 0, 70, 30, 10, 10, 100, false, 1000, 0, false, false, false, false, 0, 30, 0, 70, 7, -1, false, 1000, 0},
    {"HURRY-037", 0, 71, 30, 10, 10, 100, false, 1000, 0, false, false, false, false, 0, 30, 2, 71, 7, -1, false, 1000, 0},
};
}
