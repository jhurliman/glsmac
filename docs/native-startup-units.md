# Canonical startup Unit records and stack placement

The original Unit initializer (`0x5d8420`) and stack insertion/removal
(`0x5d81d0` / `0x5d80f0`) now have production C++ implementations. The existing
live wild-Unit factory delegates to the common initializer. These helpers are
building blocks for native seven-faction startup; complete starting packages
and factory visibility are still separate work.

## Record initialization

`InitializeSmacUnitRecord` initializes every logical field of the original
52-byte record, including all eight waypoint coordinates, cleared control bytes,
unplaced coordinates (-4,-4), absent home/stack indices and facing 2. Stored
morale is the low byte of definition morale plus one. Owner zero is valid.
Identity is external to those 52 bytes. The existing wild entry point retains
its owner-zero/design8..10 admission.

`OriginalSmacUnitBytes` exposes the original little-endian memory image for
composition and comparison. It is not a replacement for the validated,
versioned port save codec. In particular, removal leaves transient (-1,-1)
coordinates, which remain inadmissible to that save codec.

## Stack and tile effects

The borrowed placement world contains a dense original Unit pool and complete
44-byte tile records. Placement finds the existing head using tile occupancy,
an ascending pool scan and predecessor links; the lowest identity need not be
the head. It inserts before that head and repairs its predecessor. Only an
empty non-Base tile receives the incoming owner in its low nibble, preserving
the high nibble. Valid placement sets occupancy and, for a positive owner with
a non-air triad, the original presence bit. This includes sea Units.

Removal splices both adjacent links. Only removing the sole Unit clears tile
occupancy; other tile history remains. Placement invalidates the last-Unit
cache without rewriting its coordinates, whereas removal records the removed
identity and old position. Raw off-map placement and signed16 coordinate stores
follow the original helper. Callers own coordinate normalization/parity.

The view borrows storage synchronously and does not resize/erase the pool,
publish a frontend object, reveal terrain or bind live registry identities.
Stale occupancy requiring the original global repair path is rejected. Local
invalid links are rejected before mutation; this is not a whole-pool validator.

## Original evidence and verification

Frozen bundle `independent-startup-unit-stack-evidence-20261020` has manifest
SHA256 `9e01a3692c1ae31ea247f2856bf15e9bd1d333eb1c9d4b3c733896e1de92e778`.
All 149 files were verified before importing the compact numeric fixtures.
Executable SHA256 is
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Purchased executable/art and full capture receipts stay outside Git.

- 64 initializer controls compare every output byte, across owners 0..7,
  definitions 2/64*owner and morale -1/0/2/127. Original storage began as A5;
  the controls establish complete initialization rather than relying on zeros.
- 65 placement/removal controls reconstruct and hash-check the declared input
  recipe, then compare complete tile/Unit buffers and invalidation globals.
  Cases cover zero/one/three existing Units, head identity 2, owners 0/1/7,
  land/sea/air, Base presence, head/middle/tail/sole removal, foreign stacks
  and off-map insertion. Original head query and tile setters execute normally.
- Port-only rejection checks cover stale occupancy, broken reciprocal links
  and self-links without partially changing storage.

Neither helper draws RNG. Full factory count/home/visibility/technology effects
are not established by these controls. The next composition work is tracked in
[generated-world integration](native-generated-world-integration.md).

The cached incremental build and 21 selected native groups pass, including
existing live factory, mixed-stack, dense-index, codec and lifecycle callers.
All four selected production translation units pass correctness analysis.
A focused four-TU ASan/UBSan build passes the same 21 groups, with other objects
uninstrumented and leak detection disabled. No platform matrix was repeated.
