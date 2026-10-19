# Scout effect indexed compositing

Stock SMAC, 2026-10-18. Supplement to [Scout combat audiovisual binding](scout-combat-audiovisual.md). [Numeric discriminators](scout-effect-indexed-compositing-cases.json), [private source/execution map](../re-smac-scout-effect-indexed-compositing.md).

**The effect PCX's own palette does not reproduce the original.** Its pixels select rows of a destination-dependent indexed lookup. Ordinary Gun hits use `SYSTEM.TXT` `EXPLOSIONS3`; Fission death uses `EXPLOSIONS0`. Both banks are16×256 bytes. Preserve source indices and a background palette index until compositing; resolve the resulting index through the common game palette afterward. Ordinary RGBA decoding or straight alpha blending loses this quantization and background dependence.

For source pixel `s` and destination index `d`:

1. Replace source253 with255 when loading either atlas. Existing255 remains255.
2. If source255, preserve destination unchanged. It is not passed through the CLUT. Source0 is a real effect color, not transparency.
3. For source0..15, output `CLUT[bank][256*s+d]`.
4. Display the common game palette RGB at the output index.

Each SYSTEM row supplies a relative palette color `c` and opacity percentage `a`. Its target is common palette index `c+10`. For destination indices10..245, form each channel with integer truncation:

`mixed = ((100-a)*P[d] + a*P[c+10]) // 100`

Then select the nearest common palette entry10..245 by squared RGB distance; ties retain the first/lower candidate index. Destination indices0..9 and246..255 are preserved by the generated table. Configured protected palette ranges are also excluded from nearest-color search and restored to identity. The measured startup software palette has no such ranges.

Both atlases load with offset0 and no common-palette installation. Their own embedded RGB palettes do not enter this effect conversion. The common palette comes from `PALETTE.PCX`: its entries0..235 install at game indices10..245, retaining reserved entries. Reuse the established Gamma Correction pipeline before generating tables. For measured Gamma100, its RGB channels are capped at235; another gamma setting needs its corresponding palette and regenerated tables.

Banks0 and3 have the same relative colors. Their opacity differs from source8 onward:

| Source index | Target game index | Death bank0 opacity | Hit bank3 opacity |
| --- | --- | --- | --- |
| 8 | 230 | 93 | 90 |
| 9 | 231 | 92 | 85 |
| 10 | 232 | 91 | 70 |
| 11 | 133 | 90 | 50 |
| 12 | 134 | 80 | 40 |
| 13 | 135 | 60 | 20 |
| 14 | 136 | 40 | 0 |
| 15 | 81 | 95 | 95 |

The fixture contains all16 input rows and twelve small seven-pixel numeric controls. For example, bank0 over background10 converts source `[0,1,7,14,15,253,255]` to indices `[59,93,229,17,81,10,10]`. Thus a decoded source0 pixel is opaque color59, while253 and255 preserve the existing pixel. Zero opacity still passes through nearest-color quantization for ordinary destination indices; do not assume index identity when the palette contains duplicate RGB entries.

Two additional **asset-free** controls use `P[i]=(i,i,i)`, target59 and three source rows with opacity100/40/0. Source `[0,1,2,253,255]` over `[44,100,200,249,255]` produces `[59,83,200,249,255]`. Changing only `P[200]` to `(199,199,199)` changes that result to `[59,83,199,249,255]`, demonstrating tie selection even at zero opacity. Original generator, substitution, extraction and compositor execute in both controls; no asset file is opened.

## Established execution and remaining edge

The new control executes the original PCX decoders,253→255 substitution, sprite extraction,32 complete CLUT-row generators and original software compositor. Both4,096-byte generated banks match an independent integer/nearest-color model and the installed explosion cache files byte-for-byte. Two256×16 discriminator blits test all8,192 ordinary source/background combinations. Twelve transparency controls verify arbitrary destination preservation. Gamma100 is supplied using the earlier executed [title palette receipt](smac-title-screen.md), with the resulting1,024 bytes checked against the installed palette cache; this supplement does not rerun gamma's CRT path. Bitmap backing, allocation, rectangle intersection and UI polling are supplied. CLUT generation, lookup, pixel writes and transparency comparisons are executed, not raster sinks.

The XI row0 crops contain only indices1..14 and255 after substitution. **XF row0 has an additional index252:**75 pixels in frame0 and9 in each remaining frame,210 total. Actual original crop plus compositor controls on frames0 and8 show these pixels performing the literal lookup at row252, outside the generated bank; changing only diagnostic bytes there changes only the corresponding output pixels. They are not automatically skipped, clamped, mapped to15 or converted to alpha. The cold harness's bytes there are not evidence of initialized stock-game output. This supplement closes the normal CLUT conversion and transparency; the later [initialized index252 follow-up](scout-effect-index252.md) establishes the measured SMAC software startup mapping and a different SMAX constructor mapping, with live-display/lifetime limits. Any port fallback for252 must be labeled as a fallback. No decoded asset pixels or full palette/CLUT payloads are published here.
