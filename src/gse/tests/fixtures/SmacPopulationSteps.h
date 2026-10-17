#pragma once
// Literal self-authored RE-SMAC-GROW-001 and RE-SMAC-NUTRIENT-COST-001 observations.
// Population resource recalculation is supplied by these cases; this is not a full-turn oracle.
#include "game/backend/base/SmacPopulationStep.h"
#include <array>
namespace gse::tests::smac_population_fixture {
struct Case { const char* id; int32_t population, stored, surplus, next_surplus, growth; bool creche, golden; int32_t size, multiplier, demand; bool complex, dome; int32_t complex_limit, dome_limit, penalty; bool ascetic, cloning, objective; int32_t expected_population; std::optional<int32_t> expected_stored; bool recalculated; };
inline const std::array<Case, 36> cases{{
  {"GROW-001", 2, 29, 4, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 2, 33, false},
  {"GROW-002", 2, 30, 4, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 3, 2, true},
  {"GROW-003", 2, 75, 4, 1, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 3, 1, true},
  {"GROW-004", 2, 39, 4, 2, -3, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 2, 36, false},
  {"GROW-005", 2, 35, 4, 2, -3, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 2, 39, false},
  {"GROW-006", 2, 3, 4, 2, 6, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 3, 3, true},
  {"GROW-007", 2, 3, 1, 2, 6, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 2, 4, false},
  {"GROW-008", 2, 3, 2, 2, -3, false, false, 2, 10, 2, false, false, 7, 14, 0, false, true, false, 3, 3, true},
  {"GROW-009", 2, 3, 4, 2, 0, false, false, 2, 10, 0, false, false, 7, 14, 0, false, true, false, 2, 7, false},
  {"GROW-010", 7, 80, 4, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 7, 44, false},
  {"GROW-011", 7, 80, 4, 2, 0, false, false, 2, 10, 2, true, false, 7, 14, 0, false, false, false, 8, 2, true},
  {"GROW-012", 14, 150, 2, 2, 0, false, false, 2, 10, 2, true, false, 7, 14, 0, false, false, false, 14, 77, false},
  {"GROW-013", 4, 50, 4, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 3, false, false, false, 4, 29, false},
  {"GROW-014", 8, 90, 4, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, true, false, false, 9, 2, true},
  {"GROW-015", 9, 100, 4, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, true, false, false, 9, 54, false},
  {"GROW-016", 14, 150, 2, 2, 0, false, false, 2, 10, 2, false, true, 7, 14, 0, false, false, false, 14, 77, false},
  {"GROW-017", 2, 1, -3, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 2, -2, false},
  {"GROW-018", 2, 0, -3, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 1, -3, false},
  {"GROW-019", 1, 0, -3, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 0, std::nullopt, false},
  {"GROW-020", 1, -2, -3, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, true, 1, -3, false},
  {"GROW-021", 2, -2, 0, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 2, 0, false},
  {"GROW-022", 2, -2, 2, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 2, 2, false},
  {"GROW-023", 2, 23, 3, 2, 1, false, false, 0, 10, 2, false, false, 7, 14, 0, false, false, false, 2, 26, false},
  {"GROW-024", 2, 24, 4, 1, 1, false, false, 0, 10, 2, false, false, 7, 14, 0, false, false, false, 3, 1, true},
  {"GROW-025", 2, 24, 4, 1, 1, false, false, 0, 11, 2, false, false, 7, 14, 0, false, false, false, 3, 1, true},
  {"GROW-026", 127, 3, 4, 2, 6, false, false, 2, 10, 2, true, true, 7, 14, 0, false, false, false, 127, 3, true},
  {"GROW-027", 7, 3, 4, 2, 6, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 7, 7, false},
  {"GROW-028", 7, 40, 4, 2, 6, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 7, 24, false},
  {"GROW-029", 2, 25, 4, 2, 6, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 3, 15, true},
  {"GROW-030", 2, 30, 4, -1, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 3, -2, true},
  {"GROW-031", 2, 3, 4, 2, 2, true, true, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 3, 3, true},
  {"GROW-032", 2, 15, 1, -1, 6, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 3, -2, true},
  {"GROW-033", 4, 45, 4, 2, 1, false, false, 2, 10, 2, false, false, 7, 14, 3, false, false, false, 4, 26, false},
  {"GROW-034", 2, 0, -3, 2, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, true, 1, -3, false},
  {"GROW-035", 2, 0, 4, -1, 6, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 3, -1, true},
  {"GROW-036", 2, 30, 4, -3, 0, false, false, 2, 10, 2, false, false, 7, 14, 0, false, false, false, 3, -4, true},
}};
struct CostCase { const char* id; game::backend::base::SmacNutrientCostInputs input; int32_t expected; };
inline const std::array<CostCase, 20> costs{{
  {"NUTRIENT-COST-001", {false, 0, false, true, false, 2, 10, 0}, 12},
  {"NUTRIENT-COST-002", {false, 1, false, true, false, 2, 10, 0}, 11},
  {"NUTRIENT-COST-003", {false, 2, false, true, false, 2, 10, 0}, 10},
  {"NUTRIENT-COST-004", {false, 3, false, true, false, 2, 10, 0}, 9},
  {"NUTRIENT-COST-005", {false, 4, false, true, false, 2, 10, 0}, 7},
  {"NUTRIENT-COST-006", {false, 5, false, true, false, 2, 10, 0}, 6},
  {"NUTRIENT-COST-007", {true, 5, false, true, false, 2, 10, 0}, 10},
  {"NUTRIENT-COST-008", {false, 5, false, false, false, 2, 10, 0}, 7},
  {"NUTRIENT-COST-009", {false, 5, true, true, false, 2, 10, 0}, 7},
  {"NUTRIENT-COST-010", {false, 5, true, true, true, 2, 10, 0}, 6},
  {"NUTRIENT-COST-011", {false, 5, false, true, true, 2, 10, 0}, 5},
  {"NUTRIENT-COST-012", {false, 5, false, true, false, 0, 11, 1}, 4},
  {"NUTRIENT-COST-013", {false, 5, false, true, false, 1, 10, 1}, 5},
  {"NUTRIENT-COST-014", {false, 5, false, true, false, 2, 10, 6}, 3},
  {"NUTRIENT-COST-015", {false, 5, false, true, false, 2, 10, -10}, 8},
  {"NUTRIENT-COST-016", {false, 5, false, true, false, 2, 10, 10}, 3},
  {"NUTRIENT-COST-017", {false, 5, false, true, false, 2, 10, 0}, 6},
  {"NUTRIENT-COST-018", {false, 5, false, true, false, 2, 0, 0}, 0},
  {"NUTRIENT-COST-019", {false, 5, false, true, false, 0, 1, 0}, 0},
  {"NUTRIENT-COST-020", {false, 5, false, true, false, 3, 10, 0}, 6},
}};
}
