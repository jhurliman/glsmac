#pragma once

#include "game/backend/base/SmacWorkerAllocationRules.h"

// Literal inputs and original-x86 checked outputs; no expected arithmetic is generated.
// Executable SHA256: 0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01.
// RE-SMAC-WORKER-SCORE-001 09aa4afffd0ae435427b1ce1733205494cf7c23f4686b2b64450bea05ce235b1
// RE-SMAC-WORKER-PRIORITY-001 d6cd85551652050f926e801c3f16402ac4bba691c7e1bcc7abbdf723020320e0
// RE-SMAC-WORKER-STEP-001 0d366ecb3e044af488dd56520d6dfdf064863ab26555c29abd0d9d743830dbe4
// RE-SMAC-WORKER-PRIORITY-FULL-001 718aad5636252179fc8d7e465d5c776294c2148d5893b75242c1105c6fc28f25
// Source contracts: docs/behavior/{worker-candidate-scoring,worker-allocation-priorities,
// worker-assignment-step}.md. Resolved inputs; no full-game or live allocator claim.

namespace gse::tests::fixtures::worker_allocation {
// Matching original report SHA256 pins, in the same order as the four fixtures:
// 56e43f0da032e62911931024a93ac65173b07c2f741452570f5c5a6175c34ed5
// 4622b47db9409ac01b374b688d9dda8a52e5f97f9d7898eaacd2b7d4d7a81586
// fea9eb7e2aed8dc1369dbb3514d62a6fc6bfaba4d3bfe1a2bad0c59d3b15bf21
// f9982c588864618b4784ca2bfb07d12970f3cbd4c829b23187cfee8c48d38076
using namespace game::backend::base;
struct ScoreCase { const char* id; SmacWorkerScoreContext context; std::array<int32_t,3> yield; bool collected; int32_t best, worst, score; bool sufficient, accepted_best, accepted_worst; };
inline const ScoreCase scores[] = {
	{ "WORKER-SCORE-001", { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 0, 9999, 5376, true, true, false },
	{ "WORKER-SCORE-002", { 3, 1, 2, { 0, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 0, 9999, 9472, false, true, false },
	{ "WORKER-SCORE-003", { 3, 2, 2, { 4, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 0, 9999, 9472, false, true, false },
	{ "WORKER-SCORE-004", { 3, 2, 2, { 5, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 0, 9999, 5376, true, true, false },
	{ "WORKER-SCORE-005", { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 0, 2, 2 }, false, 0, 9999, 1280, true, true, false },
	{ "WORKER-SCORE-006", { 3, 1, 2, { 20, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 0, 2 }, false, 0, 9999, 334, true, true, false },
	{ "WORKER-SCORE-007", { 3, 1, 2, { 200, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 0, 9999, 1321, true, true, false },
	{ "WORKER-SCORE-008", { 3, 1, 2, { 6, 3, 3 }, 10, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 0, 9999, 6528, true, true, false },
	{ "WORKER-SCORE-009", { 3, 1, 2, { 6, 300, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 0, 9999, 4247, true, true, false },
	{ "WORKER-SCORE-010", { 3, 1, 2, { 6, 3, 0 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 0, 9999, 5632, true, true, false },
	{ "WORKER-SCORE-011", { 3, 1, 2, { 6, 3, 200 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 0, 9999, 5251, true, true, false },
	{ "WORKER-SCORE-012", { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::HumanGenomeProject, {} }, { 2, 1, 1 }, false, 0, 9999, 5376, true, true, false },
	{ "WORKER-SCORE-013", { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::LaterProject, {} }, { 2, 1, 1 }, false, 0, 9999, 5760, true, true, false },
	{ "WORKER-SCORE-014", { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { -1, -2, -3 }, false, 0, 9999, -4736, true, false, false },
	{ "WORKER-SCORE-015", { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 0, 0, 0 }, false, 0, 9999, 0, true, false, false },
	{ "WORKER-SCORE-016", { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, true, 0, 9999, 5376, true, false, true },
	{ "WORKER-SCORE-017", { 20, 1, 2, { 0, 0, 0 }, 0, SmacWorkerProduction::Unit, {} }, { 10, 10, 10 }, true, 0, 9999, 216064, false, false, false },
	{ "WORKER-SCORE-018", { 3, 1, 1, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 0, 9999, 2304, true, true, false },
	{ "WORKER-SCORE-019", { 3, 1, 3, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 0, 9999, 9472, false, true, false },
	{ "WORKER-SCORE-020", { 0, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 0, 9999, 383, true, true, false },
	{ "WORKER-SCORE-021", { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, false, 5376, 5376, 5376, true, false, false },
	{ "WORKER-SCORE-022", { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, true, 5376, 5376, 5376, true, false, false },
};
struct PriorityCase { const char* id; SmacWorkerPriorityContext context; bool automatic_workers; SmacWorkerPriorities expected; SmacWorkerScoreContext scoring; std::array<int32_t,3> yield; int32_t score; bool sufficient; };
inline const PriorityCase priorities[] = {
	{ "WORKER-PRIORITY-GRID-0004", { false, false, false, false, false, false, { -2, -1, 2, 3, -1 } }, false, { -2, -1, 2, 3, -1 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 5205, true },
	{ "WORKER-PRIORITY-001", { true, false, false, false, false, false, { -1, 1, 0, 1, 0 } }, false, { 0, 0, 0, 0, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 5376, true },
	{ "WORKER-PRIORITY-002", { true, false, true, true, true, true, { -1, 1, 0, 1, 0 } }, false, { 0, 0, 0, 0, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 5376, true },
	{ "WORKER-PRIORITY-003", { true, true, false, false, false, false, { -1, 1, 0, 1, 0 } }, false, { -1, 0, 0, 0, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 5184, true },
	{ "WORKER-PRIORITY-004", { true, true, true, false, false, false, { -1, 1, 0, 1, 0 } }, false, { 0, 1, 0, 0, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 6400, true },
	{ "WORKER-PRIORITY-005", { true, true, false, true, false, false, { -1, 1, 0, 1, 0 } }, false, { -2, 0, 2, 0, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 5120, true },
	{ "WORKER-PRIORITY-006", { true, true, false, false, true, false, { -1, 1, 0, 1, 0 } }, false, { 0, 0, 0, 2, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 6016, true },
	{ "WORKER-PRIORITY-007", { true, true, false, false, false, true, { -1, 1, 0, 1, 0 } }, false, { 2, 0, 0, 0, 2 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 6058, true },
	{ "WORKER-PRIORITY-008", { true, true, true, true, false, false, { -1, 1, 0, 1, 0 } }, false, { 0, 1, 2, 0, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 6528, true },
	{ "WORKER-PRIORITY-009", { true, true, false, true, true, false, { -1, 1, 0, 1, 0 } }, false, { 0, 0, 2, 2, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 6144, true },
	{ "WORKER-PRIORITY-010", { true, true, true, true, true, true, { -1, 1, 0, 1, 0 } }, false, { 2, 1, 2, 2, 2 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 7338, true },
	{ "WORKER-PRIORITY-011", { true, false, false, false, false, true, { -1, 1, 0, 1, 0 } }, true, { 0, 0, 0, 0, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 5376, true },
	{ "WORKER-PRIORITY-012", { false, false, false, false, false, false, { -1, 1, 0, 1, 0 } }, false, { -1, 1, 0, 1, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 6528, true },
	{ "WORKER-PRIORITY-013", { false, true, true, true, true, true, { -1, 1, 0, 1, 0 } }, false, { -1, 1, 0, 1, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::Unit, {} }, { 2, 1, 1 }, 6528, true },
	{ "WORKER-PRIORITY-014", { true, true, false, true, false, false, { -1, 1, 0, 1, 0 } }, false, { -2, 0, 2, 0, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::LaterProject, {} }, { 2, 1, 1 }, 5888, true },
	{ "WORKER-PRIORITY-015", { true, true, false, true, false, false, { -1, 1, 0, 1, 0 } }, false, { -2, 0, 2, 0, 0 }, { 3, 1, 2, { 6, 3, 3 }, 0, SmacWorkerProduction::HumanGenomeProject, {} }, { 2, 1, 1 }, 5120, true },
};
struct StepCase { const char* id; SmacWorkerStepState state; SmacWorkerStepContext context; SmacWorkerTileYield old_tile, new_tile; SmacWorkerStepState expected; SmacWorkerStepAction action; bool rescan; };
inline const StepCase steps[] = {
	{ "WORKER-STEP-001", { 3u, { 6, 5, 7 }, 1, false }, { 3, 2, 1, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 5u, { 8, 3, 5 }, 1, false }, SmacWorkerStepAction::Swapped, false },
	{ "WORKER-STEP-002", { 3u, { 5, 5, 7 }, 1, false }, { 3, 2, 1, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 3u, { 5, 5, 7 }, 1, false }, SmacWorkerStepAction::RolledBack, false },
	{ "WORKER-STEP-003", { 3u, { 4, 5, 7 }, 1, false }, { 3, 2, 1, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 3u, { 4, 5, 7 }, 1, false }, SmacWorkerStepAction::RolledBack, false },
	{ "WORKER-STEP-004", { 3u, { 6, 5, 7 }, 1, false }, { 3, 2, 1, 0, { 2, 100, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 3u, { 6, 5, 7 }, 1, false }, SmacWorkerStepAction::None, false },
	{ "WORKER-STEP-005", { 3u, { 6, 5, 7 }, 1, false }, { 3, 2, 1, 0, { 2, 99, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 3u, { 6, 5, 7 }, 1, false }, SmacWorkerStepAction::None, false },
	{ "WORKER-STEP-006", { 3u, { 6, 5, 7 }, 1, false }, { 3, 2, 1, 0, { 0, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 3u, { 6, 5, 7 }, 1, false }, SmacWorkerStepAction::None, false },
	{ "WORKER-STEP-007", { 3u, { 6, 5, 7 }, 1, false }, { 3, 2, 1, 0, { 2, 200, -1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 3u, { 6, 5, 7 }, 1, false }, SmacWorkerStepAction::None, false },
	{ "WORKER-STEP-008", { 3u, { 6, 5, 7 }, 1, false }, { 3, 2, 1, 0, { 2, 200, 0, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 3u, { 6, 5, 7 }, 1, false }, SmacWorkerStepAction::None, false },
	{ "WORKER-STEP-009", { 3u, { 6, 5, 7 }, 0, false }, { 3, 2, 0, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 5u, { 8, 3, 5 }, 0, false }, SmacWorkerStepAction::Swapped, false },
	{ "WORKER-STEP-010", { 3u, { 6, 5, 7 }, 1, false }, { 3, 2, 1, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, true }, { 5u, { 8, 3, 5 }, 1, true }, SmacWorkerStepAction::Swapped, false },
	{ "WORKER-STEP-011", { 3u, { 4, 5, 7 }, 1, false }, { 3, 2, 1, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, true }, { 3u, { 4, 5, 7 }, 1, true }, SmacWorkerStepAction::RolledBack, false },
	{ "WORKER-STEP-012", { 3u, { 6, 5, 7 }, 1, false }, { 3, 2, 1, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, true }, { { 4, 1, 2 }, false }, { 5u, { 8, 3, 5 }, 1, true }, SmacWorkerStepAction::Swapped, false },
	{ "WORKER-STEP-013", { 3u, { 6, 5, 7 }, 1, true }, { 3, 2, 1, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 5u, { 8, 3, 5 }, 1, true }, SmacWorkerStepAction::Swapped, false },
	{ "WORKER-STEP-014", { 3u, { 6, 5, 7 }, 2, false }, { 3, 2, 0, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 7u, { 10, 6, 9 }, 1, false }, SmacWorkerStepAction::Added, true },
	{ "WORKER-STEP-015", { 3u, { 6, 5, 7 }, 1, false }, { 3, 2, 0, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 7u, { 10, 6, 9 }, 0, false }, SmacWorkerStepAction::Added, false },
	{ "WORKER-STEP-016", { 3u, { 0, 5, 7 }, 1, false }, { 3, 2, 0, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 7u, { 4, 6, 9 }, 0, false }, SmacWorkerStepAction::Added, true },
	{ "WORKER-STEP-017", { 3u, { 20, 5, 7 }, 3, false }, { 3, 2, 0, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 7u, { 24, 6, 9 }, 2, false }, SmacWorkerStepAction::Added, true },
	{ "WORKER-STEP-018", { 3u, { 6, 5, 7 }, 2, false }, { 3, 2, 0, 0, { 0, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 3u, { 6, 5, 7 }, 2, false }, SmacWorkerStepAction::None, false },
	{ "WORKER-STEP-019", { 3u, { 6, 5, 7 }, 1, false }, { 3, 3, 1, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 3u, { 6, 5, 7 }, 1, false }, SmacWorkerStepAction::RolledBack, false },
	{ "WORKER-STEP-020", { 3u, { 200, 200, 200 }, 1, false }, { 3, 2, 1, 0, { 2, 200, 1, 100 } }, { { 2, 3, 4 }, false }, { { 4, 1, 2 }, false }, { 5u, { 202, 198, 198 }, 1, false }, SmacWorkerStepAction::Swapped, false },
	{ "WORKER-STEP-GRID-0019", { 3u, { 20, 300, 0 }, 1, false }, { 1, 3, 0, 1, { 2, 200, 1, 100 } }, { { 8, 1, 4 }, true }, { { 4, 8, 4 }, true }, { 5u, { 16, 307, 0 }, 1, true }, SmacWorkerStepAction::Swapped, false },
};
struct CompositionCase { const char* id; SmacWorkerPriorityContext priorities; uint32_t selected; std::array<int32_t,3> intake; };
inline const CompositionCase compositions[] = {
	{ "WORKER-PRIORITY-FULL-001", { true, false, false, false, false, false, { -1, 1, 0, 1, 0 } }, 3u, { 3, 5, 1 } },
	{ "WORKER-PRIORITY-FULL-002", { true, true, false, true, false, false, { -1, 1, 0, 1, 0 } }, 5u, { 3, 1, 5 } },
	{ "WORKER-PRIORITY-FULL-003", { true, true, true, false, false, false, { -1, 1, 0, 1, 0 } }, 3u, { 3, 5, 1 } },
	{ "WORKER-PRIORITY-FULL-004", { true, true, false, false, false, true, { -1, 1, 0, 1, 0 } }, 3u, { 3, 5, 1 } },
	{ "WORKER-PRIORITY-FULL-005", { false, false, false, false, false, false, { -2, 0, 2, 0, 0 } }, 5u, { 3, 1, 5 } },
	{ "WORKER-PRIORITY-FULL-006", { false, true, false, true, false, false, { -1, 1, 0, 1, 0 } }, 3u, { 3, 5, 1 } },
	{ "WORKER-PRIORITY-FULL-007", { true, false, false, true, false, false, { -1, 1, 0, 1, 0 } }, 3u, { 3, 5, 1 } },
};
}
