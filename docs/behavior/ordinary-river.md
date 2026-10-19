# Ordinary River texture selection and indexed composition

**33 native producer contexts / 172 ordered commands; 37 composed indexed
frames / 5,103 spans / 163,184 pixel attempts / 101,069 writes** match the
portable [reference](river_reference.py) and [cases](ordinary-river-cases.json).
This extends the existing land, Forest/Fungus and water/coast contracts at
zoom0/default relief on the known-viewer3 profile. Original PCX art and pixel
captures remain external; public cases contain rules, coordinates and hashes.

## Admission and neighbors

River rendering requires renderer flag **`0x400`**, center **live item `0x80`**,
and a land center (altitude band>=3). Renderer flags1 alone draw no River.
The cases use flags`0x401`, preserving the ordinary base-material pass.
Center and neighbor River tests read the live item word, independently of
remembered items used by other material paths. A remembered-only River does
not draw; a live-only River does in this admitted visible profile. This is
not a general fog or exploration contract.

Build a four-bit adjacency mask in this order:

| Bit | Doubled-coordinate offset | Direction |
| --- | --- | --- |
| 0 | `(1,-1)` | NE |
| 1 | `(1,1)` | SE |
| 2 | `(-1,1)` | SW |
| 3 | `(-1,-1)` | NW |

A valid neighbor contributes its bit exactly when its live items contain
`0x80`. **No neighbor land/sea, elevation, contour, ownership, region or
reciprocal-flow test occurs here.** Thus an adjacent sea tile carrying River
participates in the land bank selection; the sea center itself draws no River.
There is no separate shore-mouth selection branch in this continuation.
Do not automatically connect every coast to the river.

Round maps wrap X once at the width seam. Flat-map invalid X and all invalid
Y neighbors contribute no bit. All16 masks have direct native controls,
including isolated art, ends, bends, straights, junctions and all-four neighbors.
Four shore-facing controls, west wrap/flat pairs and both polar controls cover
the bounded topology cases.

## Texture and draw order

The sixteen56×56 sources are in external `TEXTURE.PCX`. For mask `m`:

`crop = (280 + 57*(m&3), 259 + 57*(m>>2), 56, 56)`.

The River quad uses the existing deformed **W,N,E,S outline vertices**, the
corresponding four ordinary vertex shades, UVs
`[0,0,55,0,55,55,0,55]`, and land shade bounds`[-4,+4]`. It introduces no
height lift, independent terrain mesh, flowing animation or UV rotation.
An isolated raised-center control verifies deformed corners and signed shades.

Order is base land fan, existing Forest/Fungus overlay, land coast sectors,
then River. Therefore River is painted over both the coast mapping and the
tested Forest/Fungus pixels. Earlier optional political coloration is a
separate stage. If the admitted center also has live item **`0x200`**, draw
the mask0 River quad again after its selected River quad, with identical
geometry/UV/shades. This documents the raw conditional without assigning the
extra item an unverified semantic name.

## Pixel rule and palette domain

River uses the already closed **keyed shaded** polygon raster. Loaded source
indices include the PCX loader's+10 rebase. Source index9 preserves the existing
destination **before** lighting; all other pixels write
`(loadedSourceIndex + signedInterpolatedShade) & 255`.
It uses the shaded UV half-texel initialization and existing fixed-point span
interpolation/clipping rules. There is no River destination-color CLUT or RGB
blend. Indexed output is interpreted through the already closed common palette.

Four additional clipped replays cover a bend, west seam, north pole and shore
mouth. Every frame matches its full indexed SHA256 and per-draw span, attempt
and store-stream hashes. The source-extra control records both writes even
when repainting identical art leaves the final frame unchanged.

Run the portable producer checks without assets:

```text
python docs/behavior/river_reference.py
```

For indexed raster verification, additionally pass `--texture-pcx` and
`--rainfall-pcx` pointing to the user's installed originals. No original art
is embedded in the reference or public cases.

## Correction and next continuation

The formerly labeled “river/color-map” branch at the earlier handoff boundary
is **territory-boundary coloration**. It queries retained tile owner identity,
compares adjacent owners within the same region/terrain category, and remaps
existing destination palette indices through owner-selected CLUT rows. It is
enabled by renderer flag`0x10000`, independently of River flag`0x400`.
The River closure above does not claim that CLUT construction or its native
pixel stores are closed.

The precise next unresolved requested color-material continuation is that
political/territory compositor and the construction of its owner-color CLUT.
For ordinary River-bearing land with that flag disabled, the bounded dependency
closed here is ready for integration. Other zooms, roads, fog, landmark art,
whole-scene scheduling and arbitrary feature combinations remain separate.
