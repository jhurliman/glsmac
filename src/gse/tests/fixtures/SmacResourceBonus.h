#pragma once

#include "game/backend/resource/SmacResourceBonus.h"
#include <array>

// Test-owned semantic data from RE-SMAC-RESOURCE-001, original-x86 checked.
// Synthetic routine inputs, not full-game observations. No original code/assets.
// Source: docs/behavior/resource-bonus-resolution-cases.json (research-owned).
// Fixture SHA-256: 6dce77a0cad7947f7df5884f29b72277883aa5d65270d42a0e3c2a381a4ed7dc
// terran.exe SHA-256: 0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01
namespace gse::tests::smac_resource_fixture {
using namespace game::backend::resource::smac;
struct Case { const char* id; const char* name; ResourceBonusInputs inputs; ResourceBonus expected; };
inline const std::array< Case, 15 > cases = {{
	{ "RESOURCE-001", "disabled pattern",
		{ 0, 0, AltitudeBand::Land3, 0, true, false, false, false, false }, ResourceBonus::None },
	{ "RESOURCE-002", "forced natural zero falls back to nutrients",
		{ 0, 0, AltitudeBand::Land3, 0, true, true, false, false, false }, ResourceBonus::Nutrients },
	{ "RESOURCE-003", "forced shelf natural phase zero gives nutrients",
		{ 0, 0, AltitudeBand::Shelf, 0, true, true, false, false, false }, ResourceBonus::Nutrients },
	{ "RESOURCE-004", "forced deep ocean gives none",
		{ 0, 0, AltitudeBand::DeepOcean, 0, true, true, true, true, false }, ResourceBonus::None },
	{ "RESOURCE-005", "forced trench gives none",
		{ 0, 0, AltitudeBand::Trench, 0, true, true, false, false, false }, ResourceBonus::None },
	{ "RESOURCE-006", "forced energy override",
		{ 0, 0, AltitudeBand::Land3, 0, true, true, true, true, false }, ResourceBonus::Energy },
	{ "RESOURCE-007", "both type markers choose energy",
		{ 0, 0, AltitudeBand::Land3, 0, true, true, true, true, true }, ResourceBonus::Energy },
	{ "RESOURCE-008", "mineral marker fallback",
		{ 0, 0, AltitudeBand::Land3, 0, true, true, false, false, true }, ResourceBonus::Minerals },
	{ "RESOURCE-009", "markers alone cannot bypass disabled pattern",
		{ 0, 0, AltitudeBand::Land3, 0, true, false, true, true, false }, ResourceBonus::None },
	{ "RESOURCE-010", "natural type wins without override",
		{ 1, 1, AltitudeBand::Land3, 0, true, true, false, true, false }, ResourceBonus::Nutrients },
	{ "RESOURCE-011", "forced presence bypasses automatic land suppression",
		{ 0, 0, AltitudeBand::Land3, 0, false, true, false, false, false }, ResourceBonus::Nutrients },
	{ "RESOURCE-012", "automatic land disabled",
		{ 2, 0, AltitudeBand::Land3, 5, false, false, false, false, false }, ResourceBonus::None },
	{ "RESOURCE-013", "matching automatic land coordinate",
		{ 2, 0, AltitudeBand::Land3, 5, true, false, false, false, false }, ResourceBonus::Nutrients },
	{ "RESOURCE-014", "matching shelf coordinate ignores land suppression",
		{ 2, 0, AltitudeBand::Shelf, 5, false, false, false, false, false }, ResourceBonus::Energy },
	{ "RESOURCE-015", "nonmatching automatic coordinate",
		{ 0, 0, AltitudeBand::Land3, 1, true, false, false, false, false }, ResourceBonus::None },
}};
}
