# Conventional destination-fungus admission

The live move event rejects destination Xenofungus unless the existing friendly
HQ route has admitted an endpoint HQ. This check runs before edge projection,
pricing or the copied-RNG movement kernel, and therefore before any authority
commit. Source-only fungus to ordinary ground remains supported. The owned
NoProjectsOrdinaryTerrain capability does not establish native-life encounter
state or a prior nearest-base distance.

This corrects the earlier [base-free movement](conventional-empty-ground-movement.md)
caller scope. Its complete original mode-3 request fixture used no bases and
zero-initialized global storage. The nearest-any-base query preserves the prior
distance when it finds no base; it does not supply zero. Consequently that
fixture establishes its supplied zero-distance context, not arbitrary initialized
base-free fungus movement.

## Original caller boundary

The human-owned NEAREST-ANY-BASE-001 contract proves all-owner lookup, inclusive
ties and preservation of prior distance when no candidate exists. Read-only
inspection of the pinned executable confirms the destination-feature branch:

- `0x5acba0` tests the destination tile's fungus bit `0x20`.
- `0x5acc47` calls nearest-any-base `0x4f9450`; `0x5acc4c` reads the retained
  distance at `0x91871c`.
- `0x5acc54` compares with one, and `0x5acc57` bypasses the later encounter path
  when the distance is at most one.

The farther branch can enter native-life/CRT checks before movement, which this
live consumer does not implement. An adjacent admitted HQ proves distance at
most one even when another base wins a tie. No-base state provides no such proof.
The branch tests destination fungus; it does not require fencing an ordinary
exit solely because the source contains fungus.

The executable SHA256 remains
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
The static disassembly is retained privately in `fungus-caller-static.log`.
No original executable run, new encounter implementation, inferred zero-distance
state or RNG reseeding is claimed. Existing pure edge/KEY/lottery kernels, HQ
literal comparisons, wire formats and synchronous authority commit are unchanged.

## Regression and validation

Parent: `6152f5da2e5be851cee726f0332da9e5a4417052`.

A new actual-event group checks four rejected no-HQ contexts: neutral PLANET,
positive PLANET, prior failed-move history with spent points, and fungus on both
endpoints. Both direct validation and the real event reject. The complete saved
world, Unit bytes, tile membership, next ID and frontend Unit effects remain
unchanged. Each case then exercises source fungus to ordinary ground as a
positive control with unchanged paired RNG.

The prior live fungus lottery/upkeep group now uses an admitted HQ and healthy
Scout. It preserves strict draw, retry-marker and real upkeep coverage. The
base-free restore group now uses an ordinary partial-point failure, restores the
complete state, and compares uninterrupted versus restored next-command state
and RNG after real upkeep. This avoids retaining a positive test of the unsupported
caller context. Existing pure-kernel fungus tests remain intact.

- The new regression fails on the prior production code at no-HQ scenario zero,
  before the guard is added.
- All 33 focused conventional groups pass with allocation tracking in normal
  and leak-enabled ASan/UBSan/LSan builds. This includes all movement, HQ and
  Early raw-unit lifecycle groups.
- Explicit LLVM15 analysis of the one changed production TU passes with zero
  failures in 2.46 seconds.

These are focused results on this correction, not a new full-suite or rendered
startup claim. Root runs the combined platform checkpoint after integration.
Logs and the final manifest remain under
`/home/jhurliman/src/glsmac-unit-weapon-parser/` with `fungus-fence` names.
