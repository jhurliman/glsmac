# Native final-generation helpers

Three owned raw-plane operations now support the generation tail:

- `NormalizeContours` (`0x5dda90`) classifies against working thresholds and sea
  offset, interpolates the retained contour against the static old/new tables,
  uses new endpoint 10 for band 6, then replaces all eleven working thresholds.
  It does not rewrite altitude classes or unrelated record bits.
- `ClimatePostpass` (`0x5dd550`) clears and rebuilds region wet counts, updates
  the ordered inner/outer land/sea-run flags and cross-region area gates, then
  clears each tile's site-rating nibble. Flat-map clipping preserves holes in
  the footprint; offset 9 starts the outer run only when that cell exists.
- `FinalFeatures` (`0x5db1d0`) recomputes deterministic rockiness and resource
  feature bits, preserving other storage. Positive Crater detail below 21 skips
  the entire tile. Freshwater sea skips resource changes after rockiness.
  Positive Jungle land consumes a CRT parity draw; negative membership words do
  not. Sea takes its resource branch without a draw.

These are helpers, not a completed live generator. Landmark placement and the
surrounding final climate/routing controller remain to be connected before
publishing generated gameplay. Current rendering is unchanged.

## Original controls and corrected coverage

Seventeen original-executable controls compare complete tile and region
buffers, outgoing RNG/dirty flags and working thresholds, **16,590 ordered tile
stores**, **384 ordered CRT draws**, and **408 row checkpoints**. Poisoned raw
inputs make preservation observable. Region wet-count output is compared as a
complete buffer, not as an ordered region-store trace.

The five corrected area controls explicitly assert center flags at 80/40,
79/40, 80/39, 40/40 and 39/40. Positive Jungle on an all-land plane consumes
384 draws and produces both parity outcomes; negative Jungle and freshwater
sea controls consume none. Additional controls cover wrapped/flat footprints,
Crater detail/sign boundaries and ordinary sea resource assignment.

During import, the input hashes exposed a v1 native-harness bug: its requested
region-area overrides were never installed. Both reference and native capture
had used the same unintended zero-area constructor. That agreement did not
prove the intended area gates or freshwater ties. The original freeze remains
unchanged. The corrected v2 supplement installs the requested values, uses a
separate independent region-input recipe and checks the intended result bits.
The port fixture replaces the affected postpass case with v2 and adds its new
discriminating controls. Future freshwater integration must also use the v2
tie/all-matched replacements, not the similarly named v1 cases.

The v2 manifest SHA256 is
`14bf175ce2548817e73cf887cc5ecdcbfd8d4cf0de8d5f7d07b023cc9796bdc4`
(`native-landmark-controls-v2-evidence-20261020`, all 106 files verified).
The inherited v1 manifest is
`e617d515b91a3bec9f83c2ea0e7b469b94c386d099f84eb3273d0435f53265c3`.
Both use original executable SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Only numeric recipes, hashes and observations are committed; purchased art and
executables remain outside Git. No captured intermediate map is a production
input.

Validation: cached incremental build; ten affected native groups; new
production-TU correctness analysis; focused three-TU ASan/UBSan run of the final
pass group. Final branch assertions were checked with that group again; prior
successful checks for unchanged source were reused. No platform matrix repeated.
