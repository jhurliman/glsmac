# Scout departure from a real mixed-owner native stack

**Original movement permits departure from a source tile containing hostile native Units.** Actual `0x5abe40(Scout13, direction0, mode3)` moves owner-2 Scout13 from (47,67) to (48,66), spends 3, and returns 1 immediately after the real Isle/Worm births. Both native Units remain at the source. A blanket port source-tile rejection would reject this observed original move. Keep normal destination, terrain, budget, ZOC and encounter checks.

This is separate movement evidence from constructor admission. [Nine numeric requests](mixed-owner-scout-departure-cases.json) compare the same three actual movement requests in three independently restored phases. The complete original movement routine executes; no admission, path, encounter, RNG or stack-helper result is supplied.

## Exact phase distinction

1. **Positive Scout before global birth:** a real design2/owner2 Scout13 stands on supplied fungus-covered land (47,67), alone in its stack. The original factory created it. Global turn input is8, CRT seed1.
2. **After actual global birth, before wildlife phase:** the original `537790` accepts this fungal land candidate and creates Isle14 plus Worm15. Its original return is `53a820`. Linked stack is `[14,15,13]`, tile affiliation2; Scout is healthy. This is the source-hostility discriminator.
3. **After the complete actual wildlife phase, before owner1 upkeep:** resume that exact controller frame at `53a820`, run maintenance, native upkeep/planning and native commands, then stop at `53bb60(1)`. Planning deletes the land Isle; the surviving native Worm then fights the Scout and dies. Scout13 remains at (47,67), **damage5**, with no native Unit left in its source stack. This third phase is not evidence of departure from an enemy-occupied source.

The three directions were selected because their destinations are empty land in all three saved worlds. No terrain/occupancy result is injected. Each request starts from its restored phase snapshot; requests are not chained.

| Request direction | Solo before birth | Mixed source immediately after birth | After native phase |
| --- | --- | --- | --- |
| 0, destination(48,66) | Moves; return1; spent0→3 | **Moves; return1; spent0→3** | Moves; return1; spent0→3 |
| 1, destination(49,67) | Stays; return1; spent0→3 | Stays; return1; spent0→3 | Stays; return1; spent0→3 |
| 3, destination(47,69) | Stays; return1; spent0→3 | Stays; return0; spent0→0; **new native Worm16 at destination** | Moves; return1; spent0→9 |

Return1 alone does not prove relocation. Direction3's post-birth return0 accompanies a real new destination encounter birth, count16→17. Its CRT context differs from the solo and post-native phases. Do not classify that result as a source-occupancy prohibition or force all three phase results equal. The fixture records all RNG states/call sites and resulting state separately.

For the decisive direction0 mixed-source request, Scout becomes a singleton at (48,66); source stack becomes `[14,15]`, still native Isle/Worm at (47,67). Owner/design/damage are preserved. No CRT or shared draw is consumed by direction0 in any phase. Source tile affiliation is recomputed by original helpers and included in the fixture.

## Validation boundary

`tmp/reference/check_mixed_owner_scout_departure.py --verify-only` verifies nine complete actual movement requests, separate phase inputs, source/remaining stacks, Scout owner/design/damage, movement spent, encounter birth, both RNG state recurrences, and replay of captured non-stack writes against full live map/Unit/faction/Base snapshots. The original constructor and full native-phase traces remain private in `mixed-owner-scout-departure-original-x86-20261019.json`.

These are direct primitive requests through the native harness call service, not nine completed human turns or a UI interaction. The native phase itself follows the actual controller to its positive-owner boundary. Its combat is observed here, without claiming a new general combat-policy translation. Existing headless boundaries remain declared; no combat result is substituted. This establishes the concrete source-guard decision and leaves all other native movement checks in force.

## GLSMAC integration boundary

The live ordinary movement caller now admits authenticated mixed native source
stacks, retaining destination combat, terrain, budget and ZOC checks. Native
sight and ZOC consumers admit that same source while retaining their restrictions
on unrelated adjacent stacks. The restored factory integration control moves a
Scout away from a real Isle/Worm brood, verifies reciprocal remaining links,
integer spent3, unchanged RNG, native observation masks and retained source
affiliation. Validation is side-effect free; hostile return entry remains rejected.
The fixture deliberately uses ordinary terrain to isolate occupancy: the original
remote fungus-with-Bases encounter continuation is still separate work.
