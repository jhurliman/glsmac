# Spawn-event expansion history rollback

The `spawn_base` event captures the prior expansion/advice timestamp together
with its spawned Base. Rejecting that event now despawns the Base and restores
only its owner's previous timestamp. Ordinary despawn still retains history.
An unavailable history stays unavailable; a missing history value for a
canonical Base with available history rejects before removing the Base.

The native factory's existing three-argument call still returns a Base wrapper.
The event uses an explicit fourth boolean to request its rollback record, then
passes that trusted apply result to `rollback_spawn_base`. This adds no saved
Base fields, reconstruction from remaining Bases, or general history reset.

The actual GSE apply/rollback functions are exercised inside claimed immediate
and queued startup, including all six difficulty settings. Each context tests
both available history with a distinct prior73 and unavailable legacy history;
available history is written to0 by turn-zero founding, then restored to73 by
rollback. A malformed missing-value rollback leaves the Base and published
history intact. These are controlled startup interventions, not naturally
occurring stale timestamps or a full remote server-rejection replay.

Validation: three affected native groups (eight claimed-start contexts), seven
legacy initial-world event groups, the Base population GSE script, targeted
BaseManager correctness analysis, and focused BaseManager ASan/UBSan with the
three startup groups. Other sanitizer objects reuse the normal incremental
build; leak detection is disabled. The original expansion-history write itself
is unchanged from [the retained history port](victory-input-history-port.md).
