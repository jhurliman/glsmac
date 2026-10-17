#pragma once

#include "SmacBaseSquareYields.h"

// RE-SMAC-BASE-INTEGER-001: 25 original-x86 checked synthetic named cases.
// Fixture SHA-256: c92671c1dbd7fe16d403109bf78d801d07e34363284ee684de212ecea032c059
// Complete center routines; no text parsing, whole-base overflow or live reachability claim.
namespace gse::tests::smac_fixture {
inline const std::array< Case, 25 > integer_cases = {{
	{
		"BASE-INT-0001", "nutrient doubling wraps negative",
		{ { 2147483647, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::Nutrients, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ -2, 1, 1 }
	},
	{
		"BASE-INT-0002", "nutrient doubling wraps zero",
		{ { -2147483648, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::Nutrients, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 0, 1, 1 }
	},
	{
		"BASE-INT-0003", "nutrient doubling wraps positive",
		{ { -2147483647, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::Nutrients, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 1 }
	},
	{
		"BASE-INT-0004", "recycling addition wraps each resource before energy clamp",
		{ { 2147483647, 2147483647, 2147483647 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, true, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ -2147483648, -2147483648, 0 }
	},
	{
		"BASE-INT-0005", "recycling negative overflow becomes positive energy",
		{ { -2147483648, -2147483648, -2147483648 }, { 2, 2, 2 }, { -1, -1, -1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, true },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2147483647, 2147483647, 99 }
	},
	{
		"BASE-INT-0006", "mineral resource addition wraps",
		{ { 2, 2147483647, 2147483647 }, { 2, 1, 1 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::Minerals, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, -2147483648, 99 }
	},
	{
		"BASE-INT-0007", "energy resource overflow clamps to zero",
		{ { 2, 1, 2147483647 }, { 2, 2, 1 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::Energy, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 0 }
	},
	{
		"BASE-INT-0008", "HQ increment wraps energy before clamp",
		{ { 2, 1, 2147483647 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ true, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 0 }
	},
	{
		"BASE-INT-0009", "local increases wrap nutrients and minerals",
		{ { 2147483647, 2147483647, 2147483647 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { true, true, true }, { false, false, false }, false, false },
		},
		{ -2147483648, -2147483648, 100 }
	},
	{
		"BASE-INT-0010", "local decreases wrap negative nutrients and minerals",
		{ { -2147483648, -2147483648, -2147483648 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { true, true, true }, false, false },
		},
		{ 2147483647, 2147483647, 0 }
	},
	{
		"BASE-INT-0011", "both local signs after positive overflow",
		{ { 2147483647, 2147483647, 99 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { true, true, true }, { true, true, true }, false, false },
		},
		{ 2147483647, 2147483647, 99 }
	},
	{
		"BASE-INT-0012", "both local signs retain zero guard",
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
		"BASE-INT-0013", "Golden Age wraps maximum economy",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 2147483647, true, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 0 }
	},
	{
		"BASE-INT-0014", "minimum economy reaches clamp through signed negative branch",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ -2147483648, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 0 }
	},
	{
		"BASE-INT-0015", "huge positive economy saturates base term",
		{ { 2, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 2147483647, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 6 }
	},
	{
		"BASE-INT-0016", "economy addition wraps negative base energy positive",
		{ { 2, 1, -2147483648 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ -2, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 99 }
	},
	{
		"BASE-INT-0017", "pre-event cap permits 300 energy",
		{ { 2, 1, 2147483647 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, true }, { false, false, false }, false, true },
		},
		{ 2, 1, 300 }
	},
	{
		"BASE-INT-0018", "overflow zero then heat and solar",
		{ { 2, 1, 2147483647 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ true, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, true }, { false, false, false }, false, true },
		},
		{ 2, 1, 3 }
	},
	{
		"BASE-INT-0019", "governor increment wraps energy",
		{ { 2, 1, 2147483647 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, true, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 0 }
	},
	{
		"BASE-INT-0020", "river increment wraps energy",
		{ { 2, 1, 2147483647 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, true },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 0 }
	},
	{
		"BASE-INT-0021", "Merchant increment wraps energy",
		{ { 2, 1, 2147483647 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, true },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 0 }
	},
	{
		"BASE-INT-0022", "per-square economy increment wraps energy",
		{ { 2, 1, 2147483647 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, false, false },
			{ 2, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, 1, 0 }
	},
	{
		"BASE-INT-0023", "both facilities do not double wrapping addition",
		{ { 2147483647, 2147483647, 2147483647 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, false, false }, false },
			{ false, true, true },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ -2147483648, -2147483648, 0 }
	},
	{
		"BASE-INT-0024", "landmark point wraps nutrients",
		{ { 2147483647, 1, 1 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { true, false, false }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ -2147483648, 1, 1 }
	},
	{
		"BASE-INT-0025", "landmark point wraps minerals and energy",
		{ { 2, 2147483647, 2147483647 }, { 2, 2, 2 }, { 1, 1, 1 } },
		{
			{ true, ResourceBonus::None, { false, true, true }, false },
			{ false, false, false },
			{ 0, false, 2, false, false },
			{ { false, false, false }, { false, false, false }, false, false },
		},
		{ 2, -2147483648, 0 }
	},
}};
}
