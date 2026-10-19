# Canonical SMAC Unit health bars

Canonical Unit spawn and refresh requests now retain the maximum hit points
from their resolved native/conventional stats. The frontend uses that denominator
to recover the integer remaining HP from its existing presentation ratio, including
the Scout playback path. It no longer uses a 25-step gradient for those Units.
Legacy script-only Units without canonical stats retain their existing renderer.

The original [overlay contract](behavior/map-unit-and-population-overlays.md)
supplies a two-by-21 opaque black backing, top-filled integer height, and discrete
green/yellow/red damage thresholds. The bar's size and pivot are expressed relative
to the existing 23x30 flag, placing the backing at flag offset(6,6). Textures cache
by both fill height and color; equal heights can have different colors at different
maximum HP. World retirement removes scene actors before releasing their textures.
The existing overlay blink group still controls visibility.

Three native groups cover original Fission/Fusion/clamped-reactor cases, actual
texture pixels/cache/retirement and the live backend spawn/refresh handoff. Seven
affected production translation units pass targeted correctness analysis. Focused
ASan/UBSan instruments 15 renderer, texture, request, Unit and test translation
units and passes these groups plus Scout playback/live battle consumers; remaining
objects reuse the normal build and leak detection is disabled.

This checkpoint does not certify framebuffer placement, map projection, original
flag artwork, zoom policy or full screenshot parity. A real map capture and review
are required before promotion. Hold commands/order glyphs and population badge
font/geometry remain separate follow-up work.
