# Regional flags before the Unit phase

RE-SMAC-REGIONAL-BASE-FLAGS-001, 2026-10-18. Pinned stock SMAC.
[Literal cases](regional-base-stage-flags-cases.json).

**The checked Base stage adds only bit 128, indicating an owned Base with an
immediate sea neighbor.** It does not set the Colony, Probe or off-Base combat
flags from the [separate Unit phase](regional-conventional-unit-scan.md).
All nested Base-stage calls were included in the flag-write watch.

At owner-summary entry, for every region `r` from 0 through 127:

```
flags[r] &= ((turn + owner + r) % 16 == 0 ? 0 : 8)
```

The test is on the low four bits of the 32-bit sum. Consequently bit 8 can
survive up to the staggered reset; bit 128 and every other old bit must be
rebuilt. The strategy array is preserved at this boundary. Region Base bytes
and Unit/attack words are independently cleared before the Base scan; this
receipt does not replace their later producer contracts.

The Base scan visits actual Base records in forward order. Every Base owned
by the queried faction increments its stored region's Base byte, and calls
the immediate-neighbor sea test. If that test returns nonzero, it ORs 128
into that **Base tile's stored region**, without masking the region to six
bits or requiring it to be a land-region number. Tests cover regions
0, 1, 62, 63, 64, 65 and 127.

The eight tested offsets, in original order, are:

```
(+1,-1), (+2,0), (+1,+1), (0,+2),
(-1,+1), (-2,0), (-1,-1), (0,-2)
```

Any valid neighbor with elevation class below 3 qualifies. Class 2 qualifies;
3 does not. Ownership, affiliation and the neighbor's region do not enter
this predicate. The Base's own center tile is excluded: a water center
surrounded by land does not set 128, but a water neighbor does. Distant sea
outside these eight offsets does not qualify. Cylindrical maps wrap X;
flat maps reject off-map X; off-map Y is ignored in both. An off-map square
does not count as sea.

Foreign Bases do not add this bit, including a controlled visible hostile
Base. Their separate strategic-goal bookkeeping is not a regional flag
producer in the observed complete summaries. Duplicate owned Bases in the
same region merely OR the same bit again. Unit-phase flags and later strategy/
peer-policy flags must be applied at their own stages rather than folded into
this Base-stage result.

The 51 complete original summaries include all 16 reset phases, each of eight
sea directions, elevation/region/map boundaries, foreign controls and a
two-summary sequence. Removing the sea neighbor before the second summary
clears the old 128; it is not an indefinitely retained coastal cache. All
eight owners' regional arrays were watched. Only the queried owner's reset
and coastal OR wrote flags before the Unit phase; peer arrays were preserved.
All summary controls returned normally without RNG calls.

Fresh turn-zero initialization clears these regional flag words. Two actual
fresh initializer prefixes clear poisoned arrays; two nonzero-turn prefixes
preserve them. The earlier [actual nonzero restart callers](regional-lifecycle-and-territory.md)
remain the authority for restart ordering, and were not rerun. A new ordinary
save/load control restores all 1,024 regional flag words across eight owners,
including nonstandard bit patterns. Save/load does not replace them with a
newly computed summary.

No port code, original assets or live game window changed. Terrain/Base/turn
changes in these probes are explicit controlled inputs, not player commands.
