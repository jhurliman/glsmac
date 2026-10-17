# SMAC movement caller history

This slice adds owned caller history to the existing conventional movement
route. It does not enable distant fungus encounters, native creation, or combat.
The existing destination-fungus fence still requires an admitted endpoint HQ.

## Authority and original evidence

`SmacMovementHistory` keeps two distinct values: an optional retained
nearest-base distance and packed per-tile non-air placement bits. Neither value
is reconstructed from current occupancy, visibility, exploration, or unit
movement points. Normal ordinary commands retain the distance; accepted HQ
fungus attempts run the query and retain its result even when the subsequent
shared-RNG movement lottery fails. Unsupported requests remain rejected before
history, RNG, placement, or unit state changes. Human actionability still rejects
exhausted commands before entry to the lower movement caller.

The query uses every canonical Base in ascending stable manager-ID order. Its
original record-order tie policy replaces on equal distance, so the later ID
wins. Horizontal wrapping and the distance formula are:

```
dx = abs(query_x - base_x); dy = abs(query_y - base_y)
if wrap && dx > width/2: dx = abs(width - dx)
distance = max(dx,dy) - ((dx+dy)/2 - min(dx,dy) + 1)/2
```

All divisions are nonnegative integer divisions. Distance 9999 is eligible;
farther candidates are not. With no eligible Base, the query preserves the
previous optional distance, including unknown. It does not substitute the
port's general tile-distance helper. Diagnostic query coordinates are separate
from the bounded world bitmap dimensions.

The owned native fixture projects all 14 literal outputs from
`RE-SMAC-NEAREST-ANY-BASE-001`, source fixture SHA-256
`0ce8640e79c9f1e57cf3ac47a5387d6541ed8c90e26a7bfa53fb69f99b243123`.
The owner-only distinction in two source cases intentionally has identical
query inputs here: this query has no owner filter. The original fixture's 9999
boundary uses diagnostic coordinates and does not enlarge supported maps.

`RE-SMAC-DISTANT-FUNGUS-001` establishes actual non-air placement/departure
history and the nearest query's position before fungus movement. Its source
fixture SHA-256 is
`ce639e132350bc8d98e71a5b437c9aa379568bc4ee589a7d8fd6135c54e163fb`:
22 normal returns and nine creation prefixes. This implementation does not claim
to replay that whole encounter corpus. No original executable was rerun here.
The earlier 36-sequence native combat diagnostics had incomplete armor inputs;
none of their strengths, rewards, or promotion outcomes is implemented here.

## Generated-world policy and restore

`GeneratedCanonicalPlacementV1` is an explicit GLSMAC policy. Only successful
actual generated-map publication sets the admission's provenance flag. A new
SMAC simulation with that flag starts with every placement bit false and the
nearest distance unknown. File loads, debug imports, downloads, and older
synthetic/legacy contexts do not infer that policy. This is a port reset policy,
not a claim about the original game's full map reset or save lifecycle. It is
independent of the map-resource policy.

Actual non-air Unit placement sets its destination bit. Air placement does not.
Departure, despawn, and upkeep never clear bits. Conventional move preparation
validates the destination index and any nearest query before its existing
authority commit; only successful movement records a new placement. Unit
deserialization bypasses placement recording for both legacy and conventional
records. The saved bitmap can therefore differ from current occupancy and
survives restore exactly.

Simulation V7 appends optional history; absent means unavailable. The independent
`SMCH` V1 record has 28 fixed bytes and a dimension-derived bitmap. Accepted
dimensions are even width 4..512 and height 4..256: at most 65,536 canonical
tiles, 8,192 packed bytes, and 8,220 total record bytes. Length, version, policy,
dimensions, presence, distance, and unused padding bits are checked before bitmap
allocation. The outer string reader applies the same cap before copying it.
The existing 64 KiB aggregate Simulation frame remains enforced. Binding checks
history dimensions against accepted settings, including staged restores.

World V19 requires Simulation V7 and rejects V1..V18 headers before reading the
body. It retains the World18 optional map-resource policy after Simulation and
before Map. GlobalSettings V11 is unchanged.

## Checks and limits

Native tests cover the 14 literal queries, unknown empty results, byte padding,
maximum extent, optional Simulation copies, actual generation/cancellation,
non-air/air placement, departure, despawn, real move events, failed partial
fungus attempts, and rejection atomicity. Actual immediate and queued Client
download tests reject late malformed/oversized/different-dimension history
before Map construction while retaining the prior history. A complete saved
world restores exact independent history and continues a real command; a late
body failure retains current history. That last assertion does not claim
transactional rollback of an already decoded map or other manager bodies.

The feature starts from root `9850933c`, with the exact World18 map-policy source
`40d993fb0a01ee3e49dd7194dce70209bfb5d293` inherited as
`85e0a868506c59c58a05c910f98f51852500d018` (identical prerequisite tree).
The history feature remains a separate commit.

Final Linux validation on that combined source:

- Full normal CTest: 8/8, 94.35 seconds.
- Full ASan/UBSan/LSan CTest with leak detection and halt-on-error: 8/8,
  208.59 seconds.
- All 57 selected native groups pass with allocation tracking in both binaries,
  including the eight new history groups and existing movement/restore groups.
- The actual immediate and queued Client matrices each retain all 63 prior
  cases and add eight history cases, for 71 cases per path. Padding rejection is
  isolated by the pure codec test; the malformed Client row also differs in map
  dimensions and is not an isolated padding control.
- An independent source-to-owned-fixture projection check matches all 14 nearest
  queries against the pinned human fixture. No original executable was rerun.
- Seven copied production controls fail their intended assertions: wrong metric,
  strict tie handling, omitted placement, placement replay during restore,
  nearest updates only on successful movement, inferred generated provenance,
  and an unbounded outer history frame. The checked-out source remains unchanged.
- LLVM 15 correctness analysis of all 341 production translation units: zero
  failures, 243.33 seconds. The Windows-only network TU is outside this Linux scan.

Logs, copied control TUs, binary/source fingerprints and the final manifest are
retained under `/home/jhurliman/src/glsmac-movement-caller-history/`.
Initial retained diagnostic failures were fixture mistakes: expecting a rejected
MoveEvent to throw rather than return without effects, and configuring the old
file-acquisition control as SMAC even though that profile rejects map-file setup.
The final fixtures distinguish explicit validation failure from actual event
handling, and retain file provenance separately from generated SMAC admission.
These are local Linux results, not hosted CI or native Windows/macOS receipts.
No native-life density conversion, sensor reduction, CRT encounter draw, native
owner/design mapping, or default startup activation is supplied by this slice.
