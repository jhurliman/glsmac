# Territory-boundary owner colors and indexed compositor

**31 native tile contexts / 154 ordered commands; 43 composed indexed frames /
5,024 spans / 265,448 pixel attempts / 97,028 writes** match the portable
[reference](territory_boundary_reference.py) and [cases](territory-boundary-cases.json).
All2,048 initial CLUT entries and all2,048 active entries also match independent
integer color mixing and palette quantization. Original art/palettes and native
pixel captures remain external.

This closes the earlier misnamed “river/color-map” dependency. The actual
[River continuation](ordinary-river.md) is separate. The bounded profile is
known viewer3, zoom0/default relief, renderer flags`0x10001`, with one
`0x10401` River composition control. Roads, fog and landmark profiles are
outside this closure.

## Admission and mapping

Renderer flag **`0x10000`** enables this boundary pass. In the ordinary mode,
the center must be visible to the global current viewer, unless the viewer's
capability flag`0x200` or scenario flag`0x80` bypasses that visibility test.
The special renderer override mode skips this boundary compositor. These gates
are source-proved; the new native controls use the already admitted visible
profile rather than extending fog behavior.

The owner query reads the signed retained territory-owner byte, independently
of the occupying-unit affiliation. Nonpositive owner values return−1. Own
territory returns its owner directly; foreign territory requires both relation
bits **`0x08000008`** in the viewer→owner entry, unless scenario bypass applies.
The native center query must return a nonnegative owner. A foreign center
without those bits produces no boundary quad in the controlled case.

Inspect eight neighbors in order NE,E,SE,S,SW,W,NW,N, using doubled-coordinate
offsets`[(1,-1),(2,0),(1,1),(0,2),(-1,1),(-2,0),(-1,-1),(0,-2)]`.
Wrap X once on round maps; skip invalid flat-map X and all invalid Y.
Only a neighbor of the **same land/sea category and same retained region byte**
participates. Different land elevations still share the land category.
Query that neighbor's resolved owner using the same viewer authority.
If its result differs from the center's result, including neighbor−1, enable
the corresponding mapping entries below.

Start ten mapping bytes at zero. Let`v=centerOwner+1`, and let`d` be the
neighbor index0..7:

| Neighbor index | Mapping bytes set to v |
| --- | --- |
| 0 | 8,2,1 |
| odd d | d+1 |
| even d>0 | d,d+2,d+1 |

Repeated neighbors assign the same center value; they do not mix adjacent
owners' colors. Bytes0 and9 stay zero. Emit one quad if any qualifying neighbor
differs, even if its particular source classification has no pixels.

**Native asset edge case:** east-only and west-only owner differences emit a
quad but perform zero stores with the installed classification image. Preserve
this outcome; do not synthesize an extra boundary line.

## Classification source and order

The native classification source is generated from external `RAINFALL.PCX`
crop **`(2,254,60,60)`**, loaded with index rebase+10. A256-entry lookup starts
at zero; these original source indices map to classes0..8 respectively:

`[211,208,41,143,116,151,206,95,192]`.

Apply the lookup to loaded indices `(sourceIndex+10)&255`; source key9
preserves the temporary background9. The installed result contains classes
0,1,3,4,5,7,8. The reference reconstructs it from the owned external asset and
checks its full SHA256 without embedding its pixels.

Use the current deformed W,N,E,S outline and UV
`[0,0,59,0,59,59,0,59]`. No vertex shade is added to the remapped index.
However, the native edge helper **does apply the shaded raster's half-texel UV
bias**; use zero shade inputs with that helper's UV rules. Using the unlit
Fungus UV initialization would change sample addresses.

Order: base terrain, existing Forest/Fungus, **territory boundary**, land coast,
River. The composed controls verify Fungus, coast and River ordering, ordinary
sea, a raised center, all eight directions, all-neighbor junction, region/type
rejections, west wrap/flat, both polar edges and four clipped replays.

## Constructing owner CLUT rows

The initial eight rows sample loaded color indices from external `PALETTE.PCX`
at **`(580+8*owner,56)`**. Opacity comes from the eight `BOUNDARYPERCENTS`
values in `system.txt`; installed values are`[0,50,60,50,50,60,50,70]`.
These are row inputs, not percentage adjustments to RGB after drawing.

Faction graphics can replace a row. For the installed seven faction files,
sample target color at`(161,747)` and encoded opacity at`(161,757)`, using the
same loaded+10 indices. Convert opacity with
`((loadedOpacity-10)&255)*100//255`. In the checked stock setup, those overrides
produce the same rows as the initial color/percentage inputs. The reference
still samples the faction files rather than assuming this equality for custom
art or alternative faction assignment.

For each destination index10..245, calculate three integer channels:

`mixed[c] = ((100-opacity)*palette[dest][c] + opacity*palette[target][c]) // 100`.

Choose the nearest common palette entry10..245 by unweighted squared RGB
distance. Equal distances retain the first/lower index. Entries0..9 and246..255
remain identity. The checked software palette has no extra protected ranges;
alternate protected-range profiles need the existing generator's additional
exclusions. Zero opacity still quantizes through nearest-color selection and
need not preserve duplicate-color indices.

Use the already closed Gamma100 palette authority: external common palette
entries0..235 become game10..245 with each RGB channel capped at235; reserved
entries come from the software palette profile. No palette RGB data or source
pixels are copied into this public handoff.

## Actual pixel operation

Sample class`k` from the generated60×60 source. Read`selector=mapping[k]`.
If selector0, preserve the destination and perform no store. Otherwise:

`destination = ownerCLUT[(selector-1)*256 + existingDestinationIndex]`.

There is no source-color write, RGB alpha operation at this stage, transparency
test on the resulting color, or post-lookup lighting. Reserved destination
indices can still receive an identity-valued **store**; count stores according
to selector rather than whether the index changed.

Eight additional diagnostic frames exercise each row against a changing
destination-index background. Each has246 distinct destination inputs at
admitted stores, including all20 reserved indices. Complete row generation
separately checks every256-column row. Every native frame and sampling/store
stream matches the reference.

Run without owned assets for command/admission checks:

```text
python docs/behavior/territory_boundary_reference.py
```

Pass `--asset-directory` pointing to the original installed game for classification
source, CLUT, raster streams and indexed-frame checks. Faction assignment in the
fixtures is explicitly declared; it does not establish full startup faction-art
selection or persistent cache behavior.

Initial cache-wrapper selection/rebuild timing, arbitrary diplomatic states,
general fog, alternate render flags/zooms, landmark geometry and whole-scene
scheduling remain separate. The next distinct ordinary material branch to
investigate on request is the optional Road/Tube overlay; it is not required
for the flags`0x10001` profile closed here.
