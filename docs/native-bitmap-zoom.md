# Dimensioned unexplored-map bitmap rendering

The original-backed unknown renderer now carries actual world and bitmap
sizes through owned tiles, record lookup, coverage, replacement flags and pan
history. It no longer assumes 80x80 cells, a 1024x511 bitmap, or a96-pixel pan.
A previous bitmap must match both dimensions and world identity; equal byte
length alone does not establish a matching stride. Tile previews also retain
their own bitmap dimensions. The legacy retained Planet session still requires
its original authority and dimensions.

## Original controls and images

`docs/behavior/native-bitmap-zoom-cases.json` imports nine query/cache schedules
and six complete unknown-frame controls from the frozen
`native-zoom-stack-handoff-evidence-20261020` handoff (manifest SHA256
`47b8e09e8f6bffcef687c176b9fe17a00aac7015fae073d1bc271ef9673afea1`).
The preceding camera PR verified the manifest and all 238 entries. This subset
contains semantic measurements and hashes, not purchased map/art bytes.

The production probe receives only the owned original Planet bands/landmarks,
a supplied 21 exploration footprint or all-unknown mask, and viewport/camera
inputs. It generates its own selection, geometry, cache updates, indexed-line
commands, scroll, and pixels. Captured queries, anchors, draw lists and pixels
are comparison targets, not production inputs.

All nine schedules match 16,304 original returns and their semantic cache hashes:
Planet partial exploration at zoom-1; Planet unknown at zoom3; flat64x32 unknown
at zoom8 in an 800x403 viewport; each with first/east/back events. The six unknown
frames match 2,536,992 indexed pixels exactly. The partial exploration controls
compare scheduling/cache only at zoom-1, not textured output.

Production unknown renderer, Planet zoom3, 1024x511:

![Production black/grid frame](https://raw.githubusercontent.com/jhurliman/glsmac/dae80cee86137af3a4b3e09732092c6154d793fa/native-bitmap-zoom/planet-zoom3-glsmac.png)

[Original controlled-executable frame](https://raw.githubusercontent.com/jhurliman/glsmac/dae80cee86137af3a4b3e09732092c6154d793fa/native-bitmap-zoom/planet-zoom3-original.png)
has identical indexed bytes. Both PNGs apply the original common palette at
gamma 100, including index 194=(13,28,48) and 65=black, with no resizing. These are
controlled renderer frames, not normal interactive-game screenshots. Flat64x32
zoom8 comparison images are stored beside them with `flat64-zoom8-` filenames.

## Original line-entry behavior

Original 5fa3f0 clips the diagonal entry before checking the original segment
length and stores the first clipped point unconditionally. A descending line
(120,511)->(60,541), clip[0,0,1024,511], stores(122,510) even though that is past
the original endpoint. This affects the complete zoom3 grid comparison.

The shared indexed-line kernel now preserves that behavior using a bounded
binary search into the same strict-positive midpoint phase. It does not walk
an arbitrarily long invisible prefix. All 709 prior native line controls and
3,362 ordered stores remain unchanged; the new endpoint case is tested too.

## Validation and limits

- Cached incremental production build and affected native groups, including
  dimensioned bitmap/pan history, original indexed lines, previews, and frontend
  visibility callers.
- `verify_native_bitmap_zoom.py --planet OWNED/planet.MP --out PRIVATE` compares
  all nine schedules and six unknown frames; the same probe passes ASan/UBSan.
- Existing unknown-view replay still matches 25,112 queries, 5,344 outlines,
  230,400 cache bytes, and2,093,056 complete indexed pixels.
- Existing mixed explored/unexplored replay still matches all four complete
  original textured frames (2,093,056 indexed pixels).
- Nine production TUs pass the normal targeted correctness policy. LLVM 15
  crashes while analyzing existing backend Game.cpp EventBatchHandoff
  `unique_ptr` handoff code. That TU passes with the same checks and
  `c++-stdlib-inlining=false`; this narrower analyzer run is a recorded tool
  limitation, not a full-policy pass. No repository analysis rule was disabled.

Known material/sprite composition still requires zoom-zero geometry. Nonzero
zoom plans with known terrain reject explicitly; sprite resampling and texture
geometry must be completed before enabling those combinations. The normal-game
saved-exploration bridge, generic backend terrain authority, and general camera
integration remain unfinished. These renderer results do not claim that the
ordinary playtest's revealed terrain is fixed.
