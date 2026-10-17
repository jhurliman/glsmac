#pragma once

#include "game/backend/resource/SmacBaseSquareYield.h"

// Test-owned semantic cases from RE-SMAC-BASE-001 (2026-10-16).
// Original-x86 checked with synthetic inputs; NOT full-game observed.
// No original executable, layout, instruction stream, or assets are included.
// Source handoff: docs/behavior/base-square-yields-cases.json (research-owned).
// Handoff SHA-256: b91c0075c1dc9ddd03242d2c4a2a6a2b4289326ed40a7df24923554c1f5aa789
// terran.exe SHA-256: 0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01
// alpha.txt SHA-256: 38399e1d0b967455a9146b1c468b9ec9decda43dd168afd8c3c23e0f46e357d4
// The three installed defaults are locally verified data, not calculator defaults.
// Resource resolution, landmark eligibility and solar activation are INPUTS.
// No map generation, worker traversal, event scheduling, or SMAX is exercised.

namespace gse::tests::smac_fixture {

using namespace game::backend::resource::smac;

struct Case {
	const char* id;
	const char* name;
	BaseSquareRules rules;
	BaseSquareInputs inputs;
	Yield expected;
};

// Rules: Base Square, Bonus Square, Recycling Tanks (N/M/E).
// Inputs: terrain, facilities, faction, active events. Field order is declared
// by the required constructors in SmacBaseSquareYield.h; none have defaults.
inline const std::array< Case, 57 > cases = {{
	{
		"BASE-X86-001", "ordinary non-HQ",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-002", "ordinary HQ",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ true, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 2 }
	},
	{
		"BASE-X86-003", "sea base uses Base Square row",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ false, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-004", "Recycling Tanks",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, true, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 3, 2, 2 }
	},
	{
		"BASE-X86-005", "Pressure Dome",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, true },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 3, 2, 2 }
	},
	{
		"BASE-X86-006", "both facilities do not stack",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, true, true },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 3, 2, 2 }
	},
	{
		"BASE-X86-007", "nutrient resource",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::Nutrients, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 4, 1, 1 }
	},
	{
		"BASE-X86-008", "nutrient resource doubles before recycling",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::Nutrients, { false, false, false }, false },
			{ false, true, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 5, 2, 2 }
	},
	{
		"BASE-X86-009", "mineral resource",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::Minerals, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 3, 1 }
	},
	{
		"BASE-X86-010", "energy resource",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::Energy, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 3 }
	},
	{
		"BASE-X86-011", "land jungle",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { true, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 3, 1, 1 }
	},
	{
		"BASE-X86-012", "sea freshwater",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ false, ResourceBonus::None, { true, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 3, 1, 1 }
	},
	{
		"BASE-X86-013", "freshwater flag on land gives no nutrient point",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-014", "jungle flag at sea gives no nutrient point",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ false, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-015", "crater detail 8",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, true, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 2, 1 }
	},
	{
		"BASE-X86-016", "crater detail 9 boundary",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-017", "volcano detail 8",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, true, true }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 2, 2 }
	},
	{
		"BASE-X86-018", "volcano detail 9 boundary",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-019", "overlapping mineral and energy landmarks count once per resource",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, true, true }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 2, 2 }
	},
	{
		"BASE-X86-020", "land improvement bit adds separate energy point",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, true }, true },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 3 }
	},
	{
		"BASE-X86-021", "same improvement bit at sea adds no energy",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ false, ResourceBonus::None, { false, false, false }, true },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-022", "economy -1 non-HQ",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ -1, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-023", "economy -1 HQ multiple bases",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ true, false, false },
			{ -1, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-024", "economy -1 HQ only base",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ true, false, false },
			{ -1, false, 1, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 2 }
	},
	{
		"BASE-X86-025", "economy -2 non-HQ clamps zero",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ -2, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 0 }
	},
	{
		"BASE-X86-026", "economy -2 HQ",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ true, false, false },
			{ -2, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-027", "economy +1",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 1, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 2 }
	},
	{
		"BASE-X86-028", "economy +2",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 2, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 2 }
	},
	{
		"BASE-X86-029", "economy +3 machine-code versus tooltip discrepancy",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 3, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 4 }
	},
	{
		"BASE-X86-030", "economy +4 machine-code versus tooltip discrepancy",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 4, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 6 }
	},
	{
		"BASE-X86-031", "economy +5 base term saturation",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 5, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 6 }
	},
	{
		"BASE-X86-032", "Golden Age moves economy +2 to effective +3",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 2, true, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 4 }
	},
	{
		"BASE-X86-033", "governor faction gets base energy point",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, true, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 2 }
	},
	{
		"BASE-X86-034", "Merchant Exchange location includes center",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, true },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 2 }
	},
	{
		"BASE-X86-035", "both event signs cancel in increase then decrease order",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { true, true, true }, { true, true, true }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-036", "three positive local events",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { true, true, true }, { false, false, false }, false, false },
		},
		{ 3, 2, 2 }
	},
	{
		"BASE-X86-037", "three negative local events",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { true, true, true }, false, false },
		},
		{ 1, 0, 0 }
	},
	{
		"BASE-X86-038", "dust applied before solar tripling",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, true, true },
		},
		{ 2, 1, 0 }
	},
	{
		"BASE-X86-039", "solar tripling",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, true },
		},
		{ 2, 1, 3 }
	},
	{
		"BASE-X86-040", "opposite solar phase bit does not apply",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-041", "nonzero selector uses bit 2",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, true },
		},
		{ 2, 1, 3 }
	},
	{
		"BASE-X86-042", "custom rules expose nutrient multiplication not addition",
		{ { 3, 1, 1 }, { 7, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::Nutrients, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 6, 1, 1 }
	},
	{
		"BASE-X86-043", "cap precedes heatwave and solar multiplier",
		{ { 2, 1, 99 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, true }, { false, false, false }, false, true },
		},
		{ 2, 1, 300 }
	},
	{
		"BASE-X86-044", "global dust follows local increase",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, true }, { false, false, false }, true, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-045", "bonus recycling and jungle all stack in correct order",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::Nutrients, { true, false, false }, false },
			{ false, true, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 6, 2, 2 }
	},
	{
		"BASE-X86-046", "nutrients minerals have no 99 clamp",
		{ { 100, 100, 100 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 100, 100, 99 }
	},
	{
		"BASE-X86-047", "zero values resist event decrements",
		{ { 0, 0, 0 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { true, true, true }, true, false },
		},
		{ 0, 0, 0 }
	},
	{
		"BASE-X86-048", "negative crater detail also sets suppression bit so gives no bonus",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-049", "high landmark flag suppresses jungle",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-050", "sea freshwater ignores high landmark suppression flag",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ false, ResourceBonus::None, { true, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 3, 1, 1 }
	},
	{
		"BASE-X86-051", "high landmark flag suppresses crater despite negative signed detail",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-052", "negative custom nutrient and mineral yields decrement when nonzero",
		{ { -1, -2, -3 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { true, true, true }, false, false },
		},
		{ -2, -3, 0 }
	},
	{
		"BASE-X86-053", "both event signs raise minus-one custom yields to zero",
		{ { -1, -1, 0 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { true, true, true }, { true, true, true }, false, false },
		},
		{ 0, 0, 0 }
	},
	{
		"BASE-X86-054", "negative recycling parameters expose energy-only lower clamp",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { -5, -5, -5 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, true, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ -3, -4, 0 }
	},
	{
		"BASE-X86-055", "stacked bonuses clamp before increase dust and tripling",
		{ { 2, 1, 98 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ true, true, false },
			{ 0, false, 2, true, false },
			{ { false, false, true }, { false, false, false }, true, true },
		},
		{ 3, 2, 297 }
	},
	{
		"BASE-X86-056", "maximum positive volcano detail is outside eligible region",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-X86-057", "high flag suppresses overlapping mineral and energy landmarks",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
}};

}
