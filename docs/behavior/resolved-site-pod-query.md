# RE-SMAC-RESOLVED-SITE-001: resolved site/pod presence

Use this query as the site-signal input to site quality and regional site counts.
`FEATURE_UNITY_POD` alone is insufficient for an imported original map. The
query consumes saved raw tile flags, the resource-pattern offset, coordinates,
elevation and the automatic-land-resources option. It does not read a displayed
pod marker, Base, faction, exploration, terrain moisture or cached site rating.
[Cases](resolved-site-pod-query-cases.json) cover complete original queries.

## Return meaning and evaluation order

Return0 means no resolved signal;1 is the secondary/forced signal class;
2 is the primary/explicit signal class. These are not nutrient/mineral/energy
types, yield quantities or the cached1..15 site-quality score. The site-quality
and regional-count callers consume **nonzero presence**, adding one per tile
regardless of whether the return is1 or2. Preserve the original result if other
callers need it; this receipt does not establish distinct pod-event rewards.

Use matching-parity, valid doubled-x coordinates. All flag inputs below are
decoded from the original item word. Apply this order:

1. A Monolith or the special site-suppression marker returns0 immediately.
   These suppress every later branch, including both kinds of force/explicit
   input.
2. If automatic land resources are enabled, return2 when either explicit
   primary-site bit is set; otherwise0. Ignore pattern, force-secondary,
   coordinates and elevation in this branch. It also works on deep water.
3. Otherwise, a force-secondary-site bit returns1, even with pattern0 or a
   deep-water elevation. The explicit primary-site bits have no special effect
   in this branch.
4. Otherwise, pattern0 returns0. A nonzero multiple of16 or32 is not zero.
5. Compute the pattern rules below.

Fungus, Farm, Base/Unit markers and Base-radius exclusion are not independent
suppression gates in this query. Other callers can still reject such tiles
for scoring, allocation or construction. Do not import those caller gates here.

## Pattern formula

Define:

```
u = floor((x+y)/2)
v = x-u
phase = (u mod4) + 4*(v mod4)
```

Residues are nonnegative. For elevation band>=3, return2 when:

```
phase == (-5*floor(u/4) - 3*floor(v/4) + pattern) mod16
```

Only this primary-pattern test requires land. If it does not match, or the
tile is water, return1 when:

```
phase == (pattern + 11*trunc(u/4) + 61*trunc(v/4) + 8) mod32
```

Otherwise return0. `trunc` rounds toward zero: the secondary formula deliberately
differs from the primary formula's floor for negative v. The nonnegative map
coordinates still produce negative v when y>x. Do not reuse one division rule
for both formulas. Coefficients11/61 are the pinned original executable's
initialized defaults, preserved by actual Planet loading/preparation. They are
not serialized map-header fields or a new random draw.

The actual helper does not wrap/clamp coordinates or perform bounds validation;
its callers provide valid tiles. Use the wrapped coordinate delivered by the
caller, not a distance-normalized coordinate. Tiny/invalid geometry and overflow
outside the tested32-bit input range are not newly admitted gameplay contexts.

## Original v5 map wire bindings

This extends the [standard Planet import contract](standard-planet-map-import.md).
The header starts at file15, and its signed little-endian resource-pattern
DWORD is at relative8, absolute23. The pinned Planet value is27067. The tile
block begins at2739; each44-byte record's item word is the unsigned little-endian
DWORD at relative8. Relevant item-word bit indices, numbered from0:

| Bit | Query interpretation |
| --- | --- |
| 13 | Monolith; suppresses the query |
| 20 | Special site-suppression marker; also used by the known-counter Search strategy |
| 26 | Explicit primary-site bit A, used only with automatic land resources enabled |
| 27 | Explicit primary-site bit B, same gate; either suffices |
| 28 | Force secondary-site signal, used only with automatic land resources disabled |

Elevation is the record's first byte shifted right5. Keep these raw flags and
the original pattern as simulation inputs independently of any displayed pod
feature. Bit20's complete gameplay/editor production/removal lifecycle remains
unverified; do not rename it to an exploration bit or assume it means a visible
pod. It suppresses the site query even though the Search known-counter counts
tiles where it is set.

## Planet results and producer lifetime

All3200 tiles of the actual loaded Planet asset match the formula. Repeating
after the actual original menu load and complete terrain/climate/region
preparation produces the **same ordered results**:

| Query options | Return0 | Return1 | Return2 |
| --- | ---: | ---: | ---: |
| Standard, automatic land resources disabled | 3031 | 109 | 60 |
| Same low rules with automatic land resources enabled | 3200 | 0 | 0 |

Thus Standard has169 resolved signals on this admitted prepared asset. This
is not a rendered pod census or a test of every subsequent faction-start
modification. Rules are supplied at the query boundary; the run stops before
the rules/faction setup controller. It establishes the missing map preparation
and resolver inputs, not a new full menu-to-first-command receipt.

The query is read-only: no map writes, RNG calls or cache mutation in the new
controls. Repeated calls use current saved inputs. The earlier
cache lifetime (reference receipt `territory-cache-refill-and-radius-repair.md`) still applies:
correcting the signal does not justify clearing every cached site rating at
an arbitrary read-only query. Existing nonzero ratings may intentionally outlive
changed inputs until a measured invalidation/refill boundary.

Full native fresh-map reset clears poisoned tile item flags. Original Planet
loading preserves these particular site-input bits. Nine complete ordinary
save/load controls preserve raw item flags and resource-pattern authority;
the coefficient canaries are not restored from the file. Faction known-counter
state has its separate [constructor/load rules](known-count-authorities.md).

Validation:20 named complete queries,109824 complete synthetic grid queries,
16 coefficient-input controls and12800 actual-map queries (raw-loaded and
fully prepared, two option settings each). The independent model checks every
result. No gameplay helper result is substituted. General pod outcomes,
editor/map-generation bit writers, full scoring over the prepared map and
rendered play remain outside this receipt.
