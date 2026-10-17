#pragma once
#include "game/backend/base/SmacBaseAllocationState.h"

namespace gse::tests::smac_base_allocation_fixture {
// Literal original logical values; private packed bytes are not exported.
// Fixture SHA256: 301c4ed8fca338ee01d3570e491537ca390e1a52c85efd4858d722819fbe6381
using game::backend::base::SmacBaseAllocationState;
struct Capture { const char* id; SmacBaseAllocationState state; };
inline const Capture captures[] = {
	{ "gaians/before_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "gaians/after_scout_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "gaians/after_colony_factory/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "gaians/after_second_founding_action/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "gaians/after_second_founding_action/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "gaians/after_native_visibility/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "gaians/after_native_visibility/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "hive/before_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "hive/after_scout_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "hive/after_colony_factory/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "hive/after_second_founding_action/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "hive/after_second_founding_action/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "hive/after_native_visibility/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "hive/after_native_visibility/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "univ/before_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "univ/after_scout_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "univ/after_colony_factory/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "univ/after_second_founding_action/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "univ/after_second_founding_action/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "univ/after_native_visibility/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "univ/after_native_visibility/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "morgan/before_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "morgan/after_scout_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "morgan/after_colony_factory/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "morgan/after_second_founding_action/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "morgan/after_second_founding_action/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "morgan/after_native_visibility/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "morgan/after_native_visibility/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "spartans/before_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "spartans/after_scout_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "spartans/after_colony_factory/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "spartans/after_second_founding_action/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "spartans/after_second_founding_action/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "spartans/after_native_visibility/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "spartans/after_native_visibility/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "believe/before_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "believe/after_scout_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "believe/after_colony_factory/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "believe/after_second_founding_action/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "believe/after_second_founding_action/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "believe/after_native_visibility/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "believe/after_native_visibility/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "peace/before_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "peace/after_scout_move/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "peace/after_colony_factory/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "peace/after_second_founding_action/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "peace/after_second_founding_action/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "peace/after_native_visibility/0", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
	{ "peace/after_native_visibility/1", { 1u, { { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, 1 } } },
};
struct Control { const char* id; bool upkeep; SmacBaseAllocationState input, expected; std::array<int32_t,3> intake; };
inline const Control controls[] = {
	{ "selected_word_baseline", false, { 3u, { { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0 } }, { 3u, { { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0 } }, { 4, 4, 5 } },
	{ "selected_word_bit21", false, { 2097155u, { { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0 } }, { 5u, { { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0 } }, { 6, 2, 3 } },
	{ "selected_word_bit31", false, { 2147483651u, { { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0 } }, { 5u, { { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }, 0 } }, { 6, 2, 3 } },
	{ "padding_technician", true, { 1u, { { 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 }, 1 } }, { 1u, { { 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 }, 2 } }, { 50, 1, 1 } },
	{ "padding_doctor", true, { 1u, { { 0, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 }, 1 } }, { 1u, { { 1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 }, 2 } }, { 50, 1, 1 } },
	{ "padding_engineer", true, { 1u, { { 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 }, 1 } }, { 1u, { { 3, 1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 }, 2 } }, { 50, 1, 1 } },
};
}
