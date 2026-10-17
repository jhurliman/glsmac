#pragma once

// Reduced semantic RE-SMAC-RETOOL-001 fixtures; counters are intentionally absent.
// Source fixture SHA256: 1e824953c44c89db6509025bda4cef2cd2591588fd1980fc83e942aa8ba9bd96
// Numeric IDs exist only in this evidence adapter; production uses tagged owned IDs.

#include <cstdint>
#include <optional>
#include <vector>

namespace gse::tests::fixtures::production_switch {
struct Step { std::optional<int32_t> target; int32_t item, minerals, baseline_item, baseline_minerals, loss; bool refresh, prompt, cancelled; };
struct Case { const char* id; bool manual, human; int32_t difficulty, strictness, percent, exemption, baseline_item, baseline_minerals, current_item, current_minerals; std::vector<int32_t> facilities; bool accept; std::vector<Step> steps; };
inline const std::vector<Case> cases = {
	{ "RETOOL-001", false, true, 2, 2, 50, 10, 0, 30, 0, 30, {}, true, {
		{ 1, 1, 20, 0, 30, 10, true, false, false },
	} },
	{ "RETOOL-002", false, true, 2, 2, 50, 10, 0, 30, 0, 30, {}, true, {
		{ 1, 1, 20, 0, 30, 10, true, false, false },
		{ 2, 2, 20, 0, 30, 10, true, false, false },
		{ 0, 0, 30, 0, 30, 0, true, false, false },
	} },
	{ "RETOOL-003", false, true, 2, 2, 50, 10, 0, 30, 0, 7, {}, true, {
		{ 0, 0, 7, 0, 30, 0, false, false, false },
	} },
	{ "RETOOL-004", false, true, 2, 2, 50, 10, 0, 30, 0, 30, {}, true, {
		{ std::nullopt, 0, 30, 0, 30, 0, false, false, false },
	} },
	{ "RETOOL-005", false, false, 2, 2, 50, 10, 0, 30, 0, 7, {}, true, {
		{ 1, 1, 30, 0, 30, 0, true, false, false },
	} },
	{ "RETOOL-006", false, true, 0, 2, 50, 10, 0, 30, 0, 30, {}, true, {
		{ 1, 1, 20, 0, 30, 10, true, false, false },
	} },
	{ "RETOOL-007", false, true, 2, 2, 50, 10, -38, 30, -38, 30, {}, true, {
		{ 1, 1, 20, -38, 30, 10, true, false, false },
	} },
	{ "RETOOL-008", false, true, 2, 2, 50, 10, -39, 30, -39, 30, {}, true, {
		{ -40, -40, 30, -39, 30, 0, true, false, false },
	} },
	{ "RETOOL-009", false, true, 2, 3, 50, 10, -39, 30, -39, 30, {}, true, {
		{ -40, -40, 20, -39, 30, 10, true, false, false },
	} },
	{ "RETOOL-010", false, true, 2, 1, 50, 10, -3, 30, -3, 30, {}, true, {
		{ -4, -4, 30, -3, 30, 0, true, false, false },
	} },
	{ "RETOOL-011", false, true, 2, 2, 50, 10, 0, 30, 0, 30, {10}, true, {
		{ 1, 1, 30, 0, 30, 0, true, false, false },
	} },
	{ "RETOOL-012", false, true, 2, 2, 50, 10, 0, 11, 0, 11, {}, true, {
		{ 1, 1, 10, 0, 11, 1, true, false, false },
	} },
	{ "RETOOL-013", false, true, 2, 2, 50, 10, 0, 0, 0, 7, {}, true, {
		{ 1, 1, 0, 0, 0, 0, true, false, false },
	} },
	{ "RETOOL-014", false, true, 2, 2, 0, 10, 0, 30, 0, 7, {}, true, {
		{ 1, 1, 30, 0, 30, 0, true, false, false },
	} },
	{ "RETOOL-015", false, true, 2, 2, 100, 10, 0, 30, 0, 30, {}, true, {
		{ 1, 1, 10, 0, 30, 20, true, false, false },
	} },
	{ "RETOOL-016", false, true, 2, 2, 50, 10, 0, 30, 0, 30, {}, true, {
		{ 1, 1, 20, 0, 30, 10, true, false, false },
	} },
	{ "RETOOL-017", false, true, 2, 3, 50, 10, -39, 30, 0, 30, {3}, true, {
		{ -3, -3, 30, -39, 30, 0, true, false, false },
	} },
	{ "RETOOL-018", false, true, 2, 0, 50, 10, 0, 30, 0, 30, {3}, true, {
		{ -3, -3, 20, 0, 30, 10, true, false, false },
	} },
	{ "RETOOL-019", false, true, 2, 3, 50, 10, -39, 30, -3, 30, {3}, true, {
		{ -40, -40, 20, -39, 30, 10, true, false, false },
	} },
	{ "RETOOL-020", true, true, 2, 2, 50, 10, 0, 30, 0, 30, {}, true, {
		{ 1, 1, 20, 0, 30, 10, false, true, false },
		{ 2, 2, 20, 0, 30, 10, false, false, false },
		{ 0, 0, 30, 0, 30, 0, false, false, false },
	} },
	{ "RETOOL-021", true, true, 2, 2, 50, 10, 0, 30, 0, 30, {}, false, {
		{ 1, 0, 30, 0, 30, 0, false, true, true },
	} },
	{ "RETOOL-022", true, true, 0, 2, 50, 10, 0, 30, 0, 30, {}, true, {
		{ 1, 1, 30, 0, 30, 10, false, false, false },
	} },
	{ "RETOOL-023", true, true, 2, 2, 50, 10, -38, 30, -38, 30, {}, true, {
		{ 1, 1, 30, -38, 30, 10, false, false, false },
	} },
	{ "RETOOL-024", true, true, 2, 2, 50, 10, 0, 30, 0, 7, {}, true, {
		{ 0, 0, 30, 0, 30, 0, false, false, false },
	} },
	{ "RETOOL-025", true, true, 2, 2, 50, 10, 0, 0, 0, 7, {}, true, {
		{ 1, 1, 7, 0, 0, 0, false, false, false },
	} },
	{ "RETOOL-026", true, true, 2, 2, 50, 10, 0, 30, 1, 9, {}, true, {
		{ 2, 2, 9, 0, 30, 10, false, false, false },
	} },
	{ "RETOOL-027", true, true, 2, 3, 50, 10, -39, 30, -3, 30, {3}, true, {
		{ -40, -40, 30, -39, 30, 10, false, false, false },
	} },
	{ "RETOOL-028", true, true, 2, 3, 50, 10, -39, 30, 0, 30, {3}, true, {
		{ -3, -3, 30, -39, 30, 0, false, false, false },
	} },
	{ "RETOOL-029", false, true, 2, 2, 50, 10, 0, 30, 0, 30, {}, true, {
		{ 1, 1, 20, 0, 30, 10, true, false, false },
	} },
	{ "RETOOL-030", false, true, 2, 1, 50, 10, -33, 30, -33, 30, {}, true, {
		{ -34, -34, 20, -33, 30, 10, true, false, false },
	} },
};
}
