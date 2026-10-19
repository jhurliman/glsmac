# Retained Base sprite downscale

`base_sprite_scale_reference.py` reproduces the original retained University land/pop1 sprite copy at12/16 and8/16. Four bounded native copy cases match every1576 sampled source coordinate/index and all complete indexed-frame hashes. Both previously captured selected/roster art hashes also match. No whole Base UI rerun was needed.

Run `python docs/behavior/base_sprite_scale_reference.py --faction-pcx PATH/TO/UNIV.PCX`. The PCX remains external. `base-sprite-scale-cases.json` contains the numeric inputs, retained-source hash, sample hashes and complete indexed-frame hashes. It does not contain art bytes.

## Exact sampling

The logical100×75 crop is trimmed to retained41×38 pixels at offset27,17. Scale retained width, retained height and both offsets independently. For positive anchors and the measured binary-exact ratios:

```
left   = anchorX + trunc(offsetX * numerator / denominator)
top    = anchorY + trunc(offsetY * numerator / denominator)
width  = trunc(retainedWidth  * numerator / denominator)
height = trunc(retainedHeight * numerator / denominator)
```

Selected anchor34,66 at12/16 gives[54,78,84,106]; roster anchor156,197 at8/16 gives[169,205,189,224]. These are half-open destination rectangles. Do not resample the full100×75 logical canvas, or derive scale from the rounded destination dimensions.

Both axes use the same precomputed integer step table. Let `high=max(n,d)`, `low=min(n,d)`, `q=high//low`, `r=high%low`. Initialize `error=r-2*low`; add `high` if q is odd. For each table entry: add `2*r` to error, start step=q, and if **error>0**, increment step and subtract `2*low` from error. The original creates800 entries; all800 entries per measured ratio match the portable recurrence.

- At12/16 the repeating steps are `2,1,1`; sample offsets start `0,2,3,4,6,7,8,10,11,12,...`.
- At8/16 every step is2; sample offsets are `0,2,4,6,...`.

Sample before advancing the source. Each row restarts at the same horizontal source start, and Y advances by the corresponding table entry. This is neither `floor(destinationIndex/scale)` nor a generic center-sampling formula. Selected land/pop1's changed-pixel bounding box starts atx55 even though its destination rectangle starts atx54; its source-coordinate sequence explains the discrepancy seen with simple floor sampling.

## Clipping and index handling

Intersect the half-open destination rectangle with the target clip. If left/top clip removes k destination pixels/rows, advance the source by the sum of the first k table entries, then continue from table entry k. Preserve the unclipped scale and phase. Right/bottom clipping only shortens output. The selected clipped control[58,83,77,97] exercises all four sides; the roster clipped control preserves a later source phase.

Read the loaded source index, compare it with key9, and skip the write if equal. Otherwise, source0 writes `lookup[existingDestinationIndex]`; other indices copy unchanged. The measured Base lookup is uniformly0, so source0 paints black. This special path is a destination-index lookup, not alpha blending. Key tests precede it. The native cases sample both key9 and source0; complete-frame and per-attempt checks include them.

The complete roster art hash includes a preceding selected Base draw and its black population border. The executable replay recreates that baseline before the roster copy. TextOutA was observational in these native receipts, so font glyph bytes are absent from this comparison. This is an indexed art-copy contract, not a whole-console screenshot claim.

## Supported boundary

Closed here: the original loaded University land/pop1 retained sprite,12/16 and8/16 positive-anchor copies, loaded key9, initialized indexed1024×768 target/pitch1024, the measured zero lookup, and the two bounded clipping contexts. Native sampling code executes in full, with only bitmap/GDI services supplied by the existing harness. All four copy results match a portable executable starting from external PCX bytes and numeric inputs.

Other faction/art variants, the remaining original13 UI contexts, sea art, other scale ratios, upscaling, negative placement, scale-cache invalidation across arbitrary sessions, empty/degenerate retained sprites, target formats/pitches and over800-entry dimensions are unsupported by this parity claim. The recurrence's readable general form is not proof for those profiles. Broad reverse engineering remains paused.

Frozen evidence: `tmp/reference/base-sprite-scale-evidence-20261019`. Earlier archives remain unchanged.
