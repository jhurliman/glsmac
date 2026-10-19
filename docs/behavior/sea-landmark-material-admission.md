# Sea landmark memberships in the ordinary water/Fungus profile

**27 complete native tile-renderer controls, 153 ordered draw commands:** raw
landmark-word `0x10` and Freshwater `0x80` can be ignored by the existing
ordinary sea water/Fungus continuation. Tests cover deep water, shelf water,
and shelf Fungus, with standalone memberships and selected combinations of
dirty bit `0x400000` and upper-byte detail, including negative signed detail.
Every captured command, source crop, UV, vertex, scalar, contour sample,
Fungus gate and projection equals its unmarked baseline.

This admission is for the already closed known-viewer3, renderer-flags1,
zoom0, ordinary-relief profile. It does not clear either membership from the
retained word, alter Freshwater yields, or admit neighboring land with unsupported
height/shade profiles. It adds command equivalence controls; no new indexed
raster-store or RGB measurement is claimed.

Use raw retained-word masks consistently: **`0x10` does not select the native
height profile selected by `0x100`**. Port feature enums may differ from these
raw field masks. Do not use a semantic feature label as evidence that the raw
bits are interchangeable.

The separate material paths relevant to admission are:

| Raw membership | Ordinary renderer consequence |
| --- | --- |
| `0x1`, `0x2`, `0x100` | Detail-selected special art as well as height profiles; sea height zero alone does not prove the art is absent |
| `0x4` | Unsuppressed Jungle land material and neighbor connectivity |
| `0x40` | Alternate land base material; sea skips this triangle-selection branch |
| `0x8`, `0x400` | Uranium/Geothermal sprite path requires renderer flag `0x200`, which is absent in flags1; keep the separate exclusion for profiles that enable that flag |
| `0x200`, `0x800` | Land height/shade profile issue already handled by the geometry filter; this supplement does not implement those profiles |
| `0x10`, `0x80` | No change in the bounded ordinary sea water/Fungus profile above |

Thus, within exactly flags1, the Uranium/Geothermal sprite branch is disabled.
Preserving an existing conservative `0x8`/`0x400` material exclusion is safe,
but it should be identified as a broader-profile policy rather than deformation.
Do not broaden this table into a whole-renderer or every-low-bit allowlist.
The later optional object overlays, other renderer flags, nonstandard artificial
membership combinations, landmark art and alternate zooms remain separate.

[Cases](sea-landmark-material-admission-cases.json) retain named inputs and
expected equivalence. See also the [geometry admission contract](terrain-landmark-field-admission.md).
