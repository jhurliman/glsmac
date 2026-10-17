# Persistent Base allocation authority

`SmacBaseAllocationState` owns the full unsigned selected word, the persistent
specialist count and all sixteen ordered specialist catalog IDs. It preserves
inactive slots exactly. The temporary visited mask and provisional specialist
count belong to a calculation, not this saved value.

The original `RE-SMAC-BASE-ALLOCATION-AUTHORITY-001` receipt captures 49 Base
records across 35 expansion checkpoints. It also runs six complete allocator
controls. Selected bits21 and31 are outside the normal radius but affect the
original admission popcount; silently discarding them changes the outcome.
Changing only the first unused specialist slot changes a later active type
because compaction reads past the active prefix. These facts require lossless
storage rather than reconstruction from the port's unordered population map.

The captured initial padding is Technician-valued, but the source world began
with cleared storage. This implementation supplies no universal padding default
for newly founded, reused or restored bases. A caller must supply the complete
authority. Counts above sixteen retain sixteen stored positions and still need
the separate initial-selector rule when an allocation actually reads beyond
those positions.

## Codec

Version1 uses the existing Buffer framing: three integer frames for version,
selected word and count, followed by one string frame containing sixteen logical
slot bytes. Its fixed wire extent is64 bytes. Decode requires an isolated frame
at cursor zero, checks the extent before copying, bounds the slot string and
validates selected as unsigned32, count as0..127 and every slot as0..6. Truncated,
oversized, trailing, malformed or partly consumed frames reject without changing
the input. Encoding applies the same logical-domain checks.

The port format does not copy the original's private eight-byte slot packing or
claim compatibility with original save files. Optional absence, Base identity,
population bindings and world framing belong to the later manager integration.
No Base/World version is changed and no allocation is published by this value.

## Evidence and focused validation

The projector pins the public fixture SHA-256
`301c4ed8fca338ee01d3570e491537ca390e1a52c85efd4858d722819fbe6381`
and both original raw reports. It verifies captured fields and all six complete
outcomes against those reports before producing literal logical values; private
packed bytes are not exported. The four native groups check:

- Exact roundtrips for49 captured records, retaining all sixteen slots.
- Both original input and restored input against six complete original allocator
  outcomes, including full stored output and intake.
- Count/slot boundaries, full selected width and every slot's validation.
- Every truncated extent, trailing bytes, invalid fields and nonzero cursors.

All four groups pass with allocation tracking in normal Linux Debug and
ASan/UBSan/LSan with leak detection. Focused LLVM15 analysis reports zero failures
for the new production translation unit (0.85 seconds). Combined world/live
validation remains a separate integration gate.
