# Retained score table at whole-new-world entry

RE-SMAC-NEW-WORLD-SCORE-TABLE-001, 2026-10-18. Stock SMAC.
[Literal eight-word control](new-world-retained-score-table-cases.json).

**All eight score-table words preserve through the checked whole-new-world
prefix and resumed fresh-faction prefix, including positive faction slots.**
There is no observed score clear at either boundary.

The control uses prior words, slots0–7:

`[17,31,47,61,79,97,113,127]`

Actual whole-new-world execution resets turn17→0 and reaches its first faction1
initializer with that exact table. Resuming the same child with its native
caller stack/registers through the checked fresh initialization region again
leaves the exact table. Write watches cover all32 bytes and observe zero writes
through both boundaries. Fresh faction cache/count resets do not implicitly
reset these separate score words.

The [score provider and slot0 receipt](dominance-runner-slot0-score.md) owns the
subsequent writes: score refresh computes positive slots, assigning zero to
inactive positive factions, while preserving slot0. The full nonzero ranking
update also destructively uses the positive score buffer during sorting, as
documented by the [ranking producer](ending-input-producers.md). The table above
is the **pre-provider retained state**, not a claim about the final buffer after
that full ranking update.

This extends the [strategy-row prefix control](new-world-regional-strategy-storage.md)
using the same actual parent/child ordering. It does not replay two completed
games or the frontend, execute the remaining faction initializers or establish
the last later writer before first command. Label these boundaries as preserving
prior score storage; do not describe a cold value or newly cleared faction
statistics as a measured whole-world score-table reset. No other ranking fields
or broad standing goal are changed by this work.
