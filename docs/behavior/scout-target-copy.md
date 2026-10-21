# RE-SMAC-SCOUT-TARGET-COPY-001: Scout target copy, transparency and cleanup

Status: **original-x86 checked**, [12/12 complete caller cases](scout-target-copy-cases.json) for the pinned SMAC executable. This continues the [internal Scout capture](scout-idle-render-capture.md) through the original software bitmap copy, dirty-area cleanup and normal idle-render return. Pixel-copy and cleanup results are not supplied by the harness.

The unit/design/view inputs remain the same admitted reactor1 Scout case. Software bitmap state, initial target pixels and OS rectangle services are supplied. This is a complete draw into a controlled software target, not an original game-window screenshot, a DirectDraw test or proof of the full game palette/faction-overlay lifecycle.

## The actual transparent key is index9

Normal renderer initialization fills the internal color surface with **palette index9**, not0. The final color copy also uses **index9 as its transparent key**. The original bitmap fill and copy bodies execute in these checks.

For each source pixel admitted by target clipping:

- Source index9 preserves the target pixel.
- Any other source index copies to the target, including index0.
- The target copy does not consult the depth buffer to decide transparency.

Explicit data controls applied after the actual3D draw establish the last point: source index0 or77 copies even where depth is untouched65535; index9 is skipped even with depth42. Changing one real Scout pixel to9 creates a hole while its existing depth remains written. These controls isolate copy semantics; they are not naturally produced Scout frames.

This is an **index comparison**, not a comparison of RGB values or palette entry flags. Captured palette methods give index9 RGB(252,0,252), but that color triple must not replace the index test. Palette entry flag5 is not alpha.

The earlier internal-only capture deliberately supplied a zero-filled color buffer and exported a depth-based alpha mask. Those remain valid controlled results for that fixture. They do not describe the normal source-clear/transparency convention now established here.

## Placement and clipping

For this fixed case, map-caller input anchor(128,128) becomes surface-wrapper anchor(178,178), then the256×256 internal color surface is copied with target offset **(50,50)**. Thus internal source(x,y) maps to target(x+50,y+50), subject to the target's half-open clip rectangle.

The baseline patterned target is512×512. All341 non-key Scout source pixels are eligible to copy. Only339 target bytes differ afterward because two copied values already equal the supplied target pattern. Do not infer visible/covered pixel counts solely by comparing before/after target bytes.

Cases also use pitch520 with width512. The complete target buffers, including row padding and background, match an independent keyed/clipped copy model. Narrow, left, right, top, bottom and completely outside clips execute the actual caller; the outside case changes no target bytes. All target guards remain intact.

The copy covers the supplied source extent rather than only the3D dirty rectangle. The controls at internal positions(0,0) and(1,0), outside the tracked3D dirty area, still copy their non-key indices to target positions(50,50) and(51,50).

## Dirty-area cleanup

The caller clears tracked dirty rectangles, not necessarily the entire internal allocation. In the unclipped Scout case that rectangle is **[110,88,137,139)** in internal coordinates.

The observed sequence is:

1. Clear tracked color pixels to9 while retaining depth.
2. Later in the same complete idle-render caller, clear the tracked color area again and reset tracked depth to65535.

With ordinary renderer-produced pixels, the result is a clean internal surface. With deliberate untracked data overrides, values outside that rectangle survive: index0 at(0,0), index77 at(1,0), or depth42 at(2,0). These are checks of cleanup scope, not a claim of normal rendering artifacts. The fixture records the actual tracked rectangle and both cleanup modes for every clip case.

## Reproducible color output

The normal-fill baseline's indexed/depth foreground matches the earlier internal capture; only its untouched background changes from the explicitly supplied0 to the normal key9. Original palette methods are executed again, producing the same RGB/entry-flag data.

The retained keyed RGBA conversion uses original RGB entries and alpha0 iff source index9, otherwise255. This reproduces the proven software-copy mask, including the distinction from depth-based masking. It remains a **host RGBA conversion**, since this original path produces indexed surfaces. Transparent pixels retain the palette's magenta RGB; consumers may see that RGB in tools that ignore alpha.

Private retained outputs include the complete final indexed target, the internal indexed/depth capture before cleanup, original-method palette entries, and the keyed RGBA/PNG. The address-level audit lists paths and hashes. No installed or derived asset payload is committed.

Other views/frames, scaling, active shadows and their remap tables, hardware surfaces, full target/window palette state, normal faction overlays, and live-game screenshot comparison remain open.
