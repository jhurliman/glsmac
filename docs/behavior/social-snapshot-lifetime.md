# Social snapshots exist before owner refresh

Current, retained and third produced social vectors are separate stored authorities. A faction that has not yet had its first upkeep still has numeric retained/third storage. **Do not equate “not refreshed for this world/turn” with “snapshot unavailable.”** This distinction permits derived Unit queries for future computer factions at the first human command without prematurely running their upkeep.

## Fresh-image baseline

All eleven axes of all four social vectors—current, retained, third and intrinsic—for all eight slots occupy the PE loader's zero-filled virtual `.data` tail in the pinned executable. They are zero in the admitted fresh-image environment. No new whole-world instruction producer is needed to explain the retained/third zeros in the accepted first-world setup. This establishes image storage provenance, not full process-entry execution or a universal New Game reset.

The per-owner initializer `0x5c8650` explicitly clears **current only**: `0x5c8bad` selects `0x95d3f4+owner*0x20bc`; `0x5c8bb3` sets count11; `0x5c8bbf` executes the zero stores. The range ends exactly before retained starts at`0x95d420`. It clears the model fields separately. It does not clear retained`0x95d420` or third`0x95d44c`. Intrinsic`0x95d478` is produced later, after the owner's initial Base/Scout, as mapped in [first-upkeep authority](seven-first-upkeep-authority.md).

## Two full setup calls in one native guest

The new control executes complete Standard Rules/Planet seven-faction setup through its native return boundary, calls actual `0x5cbc40` for owners1..7 to establish a previously refreshed world, then executes another actual Planet reload/preparation and complete setup controller `0x5a0b80` in the same guest. Both setup flows call `0x5cadb0`, initialize slots in order1,0,2,3,4,5,6,7, and execute global balancing. The social fields are not injected between these worlds.

The second setup writes **no retained or third axis at all**. Every axis for every slot equals its previous-world value at setup entry, parent setup entry, each child initializer entry, balancing entry and completed setup. Current is explicitly cleared for all eight slots:88 DWORD stores per setup at`0x5c8bbf`. All positive slots rebuild the same intrinsic values. In particular:

| Owner | Current after second setup | Retained/third preserved from previous world |
| --- | --- | --- |
| Gaia | all0 | EFFIC+2, MORALE−1, POLICE−1, PLANET+1 |
| University | all0 | PROBE−2, RESEARCH+2 |
| Sparta | all0 | MORALE+2, POLICE+1, INDUSTRY−1 |

The other four factions likewise preserve their full vectors; [numeric cases](social-snapshot-lifetime-cases.json) contain all eight slots and all eleven axes. This extends the earlier retained-MORALE prefix controls through complete all-faction setup and balancing, and extends the result to the full retained and third vectors.

The replay invokes the real setup controller twice with its existing headless presentation boundaries. It does not navigate the main menu, execute a played previous game, reconstruct all old UI destruction, or prove the absence of a reset in every earlier entry path. Thus the proven statement is preservation across these complete mapped setup invocations, not an unrestricted claim about every ordinary second New Game path.

## Later producer and port consequence

At owner upkeep, `0x53bb60` calls `0x5cbc40`. That function computes current via the call at`0x5cbc82`, retained via`0x5cbc94`, and third via`0x5cbca6`, each using actual evaluator`0x5cb9a0`. The third call supplies its distinct mode flag1; do not collapse it into a permanent alias of retained. Neutral models make their values equal in these controls. Refresh is owner-scoped; it must not refresh future factions merely to make them inspectable.

For the declared fresh-world profile, carry known numeric zero retained/third baselines independently of the Early/Refreshed phase marker. Consumers can read that baseline while current remains Early0. For reused storage, preserve the actual snapshot authority until its real producer or an established reset/load boundary replaces it. A schema that treats the baseline as absent until first upkeep loses an original state that already supports derived queries; a schema that substitutes current/intrinsic for retained changes their timing.

Validation: checker`tmp/reference/check_social_snapshot_lifetime.py --verify-only` verifies32 PE storage classifications, two full setup flows, all-slot/all-axis preservation at every observed boundary, and byte-for-byte reconstruction from every social-vector write. It does not rerun rendering or claim new visual output. Native raw receipt and producer are private under`tmp/reference/social-snapshot-lifetime-original-x86-20261019.json` and`probe_social_snapshot_lifetime.py`.


## Port integration checkpoint

The actual backend new-world producer supplies known zero retained/third arrays
from fresh engine storage. Real Reset preserves each admitted owner's exact
snapshot authority, including legacy absence, before retiring the simulation.
New-world setup consumes that storage while current ratings remain Early0.
Late intrinsic publication and per-owner refresh retain their native ordering.
Derived Unit contexts can read a known baseline independently of Early/Refreshed
phase. Actual pending Units still reject queries, serialization and commands
until their startup seal; ordinary ready creation at turn0 is explicitly rejected.

Social V2 owns third separately and reads V1 with unknown third authority.
No decoder or new-world migration guesses missing legacy values. Neutral model
refresh computes equal retained/third values as separate copies; later nonneutral
model behavior remains unfinished. The script getter copies each known vector.

Validation:86 affected startup/social/Reset/simulation-codec groups pass normally
and with nine affected translation units instrumented by ASan/UBSan (other linked
objects are normal; leak detection disabled). Eight production translation units
pass targeted correctness analysis. The original reference checker passes
verify-only. Full native per-faction scheduling is not activated by this change;
its saved world cursor and computer decisions remain required.
