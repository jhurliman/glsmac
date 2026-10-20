# Native low-zoom Base and Unit markers

Ordinary actor callers use geometric markers below zoom-8 instead of continuing
to draw model/Base artwork and population/flag/health/order overlays. Zoom-8
still uses artwork. Base names have a separate cutoff: hidden at-11 and below,
with names allowed alongside markers at-10/-9 subject to existing disclosure.

The port now has that marker path in its Base and canonical CVR Unit callers.
Markers retain the current owner's primary palette color and the native black
border (the exact SPARTANS faction-section identifier selects index44). Their rectangle arithmetic
uses the independent art ratio `(zoom+16)/16`, not the quantized terrain q.
Horizontal squaring truncates half the width/height difference, so some markers
are one pixel wider than tall. The frame is one physical pixel at every zoom.

The original dispatcher anchor is P-(0,2q). Frontend object coordinates project
to P+(4q,2q), so the pixel-locked marker canvas is offset by the captured marker
rectangle minus(4q,4q). The generated actor keeps its Base/Unit depth layer and
bypasses world RGB lighting. RGBA stores preserve both palette bytes and opaque
alpha; they do not use the legacy packed Color decoder's divide-by256 path.

Zoom refresh preflights marker resources before publication, retires prior body
instances and restores admitted visibility. It does not invoke general gameplay
refresh or mutate selection/work visits. Returning to artwork allocates IDs
from the correct actor. Base console meshes and native Unit card descriptors
retain artwork independently of the marker body. Population/flag/health/order
actors are removed while a marker is active; no per-stack flag leaks through.

## Source and validation

The frozen actor handoff manifest is
`d7e35fc2ad5b53961c73e7b85a7066fc55b9042fffaab08f078308392df0d2f2`.
All26 files were hash-verified and its standalone75 caller controls,441
arithmetic comparisons and20 source guards pass. The consumed subset is
[12 marker cases](behavior/native-low-zoom-marker-cases.json): Unit plus two
Base population inputs at zooms-14,-11,-10,-9. They preserve caller rectangles;
bitmap/font leaves were sinks, not full original framebuffer captures.

Native tests compare the shared arithmetic and actual production sprite
factory's canvas, dispatcher offset, frame and owner fill to these controls.
Frontend fixtures switch admitted Base/Unit bodies through marker zooms and
back to0, checking stale-instance removal, overlay/name cutoffs and card
independence. The Base test supplies explicit detached palette metadata; it
is not a claim about additional faction-identity discovery. Five focused native
groups and correctness analysis of eight affected production units pass, with
the final pixel-store producer rechecked separately. Optional diagnostic
captures use `GLSMAC_MARKER_CAPTURE` during the native marker test and export
production textures rather than reconstructing pictures from the formulas.

## Remaining integration

The ordinary interactive map has not yet switched to the native discrete
camera. These APIs prepare its actor consumers; they do not claim that current
mouse-wheel zoom now matches the original. Name fonts/positions at-10/-9,
nonzero full-art Base layout, population/order font metrics and full-art Unit
overlay scaling remain open. Raw forced/expanded marker flags, positive scale
overrides, the Unit marker-suppression flag and the alternate whole-body blink
policy are not newly implemented by the ordinary-zoom branch. Unsupported
custom palette/atlas definitions keep their existing presentation path.

The original Base source0 copy uses the active-palette indexed shadow lookup,
not uniform black or an RGB opacity substitute. The native Base lighting bypass
is separate from that still-open compositing integration. Main-map exploration,
coastlines, terrain palette/lighting and relief remain required before gameplay
screenshot parity; this marker work does not close those findings.

The initial frozen actor prose mislabeled the border exception as Gaia. The
verified marker-literal supplement supersedes that label: native59de20 stores
the second #FACTIONS column at record+0x1c, separately from the art file base at
+4, and the marker compares that section identifier byte-for-byte to SPARTANS.
Both marker consumers use Faction.m_id, not PCX filenames or former art owner.

The corrected supplement manifest
`882e83e9c0d26dc7cebceb47e3c8ce6afc23083baf8355f8debfe75f62712dd0`
was hash-verified (19 files). Its standalone checks pass160 font-metric rows,42
prior native constructor matches and eight marker-literal controls. This PR
consumes the literal/identifier correction only; font profile integration is
separate. Five native groups pass with ten affected production/test translation
units instrumented by ASan/UBSan; other linked objects are normal and leak
detection is disabled. The two asset-independent groups also pass without
original game assets. No interactive platform matrix was run for this adapter.

Native-life and placeholder worker bodies still need their own art consumers.
