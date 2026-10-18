# Riot search uses the last produced direct faction rank

RE-SMAC-RIOT-RETAINED-RANK-001, 2026-10-18. Stock SMAC.
[Literal permutation/consumer supplement](riot-retained-rank-cases.json).

**The riot word is the direct rank assigned by the ordinary ranking producer:
7 is strongest,1 weakest. For a faction included in the same produced active
inverse table, its table index equals this word. Do not use8−index.** The inverse
table is filtered; it cannot reconstruct every direct word in all slots.

The original assignment/publication fragment takes the supplied score checkpoint
below. It selects the highest remaining score, writes the selected faction's
direct rank7 down to1, and consumes its score slot. It then clears all eight
inverse positions and publishes only factions in the active membership mask.

| Faction | Supplied score | Produced direct rank | Active in publication |
| --- | ---: | ---: | --- |
| 1 | 30 | 3 | Yes |
| 2 | 70 | 7 | Yes |
| 3 | 10 | 1 | Yes |
| 4 | 60 | 6 | Yes |
| 5 | 20 | 2 | No |
| 6 | 50 | 5 | Yes |
| 7 | 40 | 4 | Yes |

The produced `faction_at_rank[0..7]` is **[0,3,0,1,7,6,4,2]**. Thus searching it
for active faction1 returns index3, exactly its direct rank. Faction5 has direct
rank2 even though it is absent from the inverse table; index2 is zero. Do not
replace that absent result with a fabricated direct rank or reverse the scale.
Faction0 is not assigned by this seven-faction loop; its explicit99 direct-rank
canary remains unchanged. No gameplay meaning is assigned to that canary.

Without rerunning scoring or assignment, the **actual ongoing-riot handler** for
active human faction1 reads its retained rank3. Population4 and difficulty5 give
`L=4+20−5−3=16`, so it consumes17 unsuccessful candidate selections after its
POLICE gate, then returns normally at POLICE0. Its direct ranks and inverse table
remain unchanged, and no ranking-provider entry occurs. Separately, the original
limit-arithmetic fragment reads inactive slot5's direct2 and computes17. That
last fragment demonstrates the stored-word distinction; it is not an admitted
inactive-faction Base upkeep.

Lifetime is the existing [ranking publication contract](ending-input-producers.md):
a nonzero update assigns ranks and rebuilds the inverse snapshot; a score-only
argument0 update does not. The global update precedes turn increment/owner
upkeep in the admitted original startup path. The riot consumer uses retained
rank, not fresh population/technology/unit scores. Preserve the last produced
direct rank (and snapshot membership) until its actual producer runs again.
The separate [score-storage/order receipt](ranking-dominance-score-order.md)
still governs consumed score-table lifetime.

This supplement executes one assignment/inverse-publication fragment from an
explicit score checkpoint, one full active riot-handler return and one small
inactive arithmetic fragment. It does not execute a fresh full ranking scorer,
Base/faction upkeep, altered membership lifecycle or new scoring formula.
Nonnegative riot RNG returns are supplied0 and notification/map callbacks are
omitted without intervention. The preceding [full Base aftermath receipt](riot-facility-aftermath.md)
supplies the ordinary caller/maintenance boundary. No earlier frozen receipt,
production or installed asset is modified; broad research remains paused.
