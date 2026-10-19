# Native Base card final-display palette

The console's admitted University Base portraits already preserve original
indexed sampling and geometry. Their final RGBA conversion used raw
`CvrActorMaterial::ReadSoftwareColors`, bypassing the measured Gamma100 display
cap. This is a display-stage issue, not a reason to alter model quantization.

The original Base copy leaves non-key game indices unchanged (with its separate
source0 destination lookup); see [the copy contract](behavior/base-sprite-scale.md).
The established native indexed-DIB palette uploads corrected common entries:
[active palette / final RGB](behavior/ordinary-land-texture-palette.md#active-palette-and-final-rgb).
For this Gamma100 profile common indices10..245 cap each channel at235; the20
measured system-reserved entries remain intact. Loading source art does not
replace that active table with the art's embedded palette.

`GetSmacNativeBaseCard` now uses the same `MakeSmacSoftwarePalette` final-display
helper as Turn Complete and ordinary terrain. Source rebasing, key9, opaque
source0 black lookup, independent selected/roster sampling, badges, geometry and
fallback admission are unchanged. The raw CVR palette/quantization API is not
changed. Nondefault gamma, historical monitor/color-management behavior and
unsupported portrait variants remain outside this measured profile.

The actual SDL loader regression uses valid synthetic indexed PCX assets with
deliberately different common and embedded palettes. It verifies capped common
colors, reserved white/red, key9, source0 black, both card sizes, cache reuse and
retention after loader destruction. The raw model palette remains uncapped in
the same case. It failed on the old loader's common-color output and passes with
the final-display correction. Original indexed-copy controls remain unchanged.

External UNIV.PCX audit using the measured sampler finds236affected selected
pixels and100roster pixels in the land/pop9 body; land/pop1 has51/21. These are
sampled sprite pixels before UI occlusion, not screenshot-diff counts or proof
of full Base-screen parity. Original art stays external to Git.
