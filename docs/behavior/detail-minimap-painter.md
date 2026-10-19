# Actual Detail minimap painter

Detail is the separate native view selected by command1007, not a World overview
bitmap with a different label or scale. Its native initializer produces
flags `0x80000082`, zoom-7, and the239x119 console child rectangle. The existing
[camera/button contracts](minimap-world-detail-camera.md) establish selection
and navigation; this handoff adds actual full fresh terrain repaint and pixels.

The [terrain cases](detail-minimap-painter-cases.json) execute the complete native
fresh painter to return, including its tile iteration and flat polygon raster.
Eleven indexed frames match410 fills,8,569 nonempty spans and217,303 pixel stores.
The [reference](detail_minimap_reference.py) independently reconstructs every
tile call/anchor, ordered polygon/color, pixel-store stream and final frame hash.
Run `tmp/reference/check_detail_minimap.py --verify-only` with the emulation venv.
These terrain cases have explicitly empty Base/unit rosters. The separate
[Base cases](detail-minimap-base-painter-cases.json) add 13 complete fresh frames,
seven admitted Base composites and 2,663 checked sprite samples. The
[Base reference](detail_minimap_bases_reference.py) independently checks admission,
stock sprite crops, scaling, clipping, population-box geometry and frame hashes.
Run `tmp/reference/check_detail_minimap_bases.py --verify-only` for this supplement.

## Caller and geometry

Use the Detail view's own local camera center. Reuse native first-coordinate/span
geometry with zoom-7 and239x119 viewport. Here the quantized diamond is56x28,
half-width28, half-height14. With camera40,40, first tile is35,35 and all four
span fields are5. Iterate11 rows, five tile positions per row, with alternating
row parity, then apply normal world bounds/wrap. This is55 tile calls in an
interior round-world case, not a downsampled80x80 world pass.

The supplied center is subject to native integer viewport snapping; do not force
that tile to the exact pixel center of239x119. For each visible tile, let dx be
its native wrapped displacement from firstX and dy its displacement from firstY.
The initial projector anchor is
`(28*(dx-1),14*(dy-1)-(sum(outerHeights)>>2))`. The final inclusive iteration row
uses the native nearest-copy projection branch; the reference models it.

Detail retains **elevation deformation**. Its default effective elevation step
is `trunc(25*9/16)=14`; ordinary shared center/corner heights and coast rules
still apply. Flat projected center is `(28*dx,14*dy)`. The colored polygon is
W/N/E/S with offsets `(-28,0),(0,-14),(28,0),(0,14)` and the respective height
subtracted from Y. Do not substitute flat, uniformly spaced map diamonds.
Painter row/column order matters where raised tiles overlap. The cases include
all eight altitude bands, a round seam, flat west edge, and both poles.

## Palette and knowledge

The actual initialized Detail flags select physical flat colors:

| Altitude band | Palette index |
| --- | --- |
| 0..2 sea | 190 |
| 3 / 4 / 5 / 6 / 7 | 22 / 20 / 18 / 16 / 14 |

These are literal constant-index polygon fills, without per-vertex light or
terrain texture sampling. Shape still deforms. The active game palette at
gamma100 has SHA256
`c7129c7433e5d3028e4e3d7990e696fd9a5b321ebf24694cee6316e5d9a32ba6`;
RGB bindings are the same established physical colors in the
[palette contract](minimap-console-controls-and-colors.md). Detail selection
does not enable the separately supplied rainfall or political renderer flags.

The normal fresh painter passes the global display faction as renderer viewer;
editor mode uses its separate omniscient viewer path. Ordinary geography admits
a tile if its exploration mask includes the viewer, initial-survey rule is on,
or the faction has all-map revelation. The accepted contexts use viewer3 and
check all these gates through the actual caller. Standard initial survey is off.
An imported tile's retained terrain alone is not evidence that its geography is
known. Checkerboard exploration verifies per-tile admission. Do not use territory
recognition, current unit visibility, or an unrelated viewer for this gate.

Unknown geography produces **no polygon**, preserving the existing indexed
backing. The Detail high flag also bypasses the ordinary non-coarse clear call
in this fresh repaint path. A diagnostic unknown frame initialized to73 remains
entirely73. Thus black on first display is a backing-initialization fact, not a
forced unknown-tile color. The accepted fresh black-backed frames explicitly
initialize background0; arbitrary redraw/cache history is not modeled here.

## Exact flat fill

The flat fill uses integer polygon edge walkers, not the textured triangle
sampler or generic GPU triangle coverage. Its right/bottom clipping has a native
quirk: right is limited to `clipRight-1` and then treated as an exclusive span
end; rows stop before `clipBottom-1`. Therefore the last visible column and row
can remain backing pixels even for a polygon crossing them. Other sprite/line
kernels have their own clipping rules; do not apply this quirk globally.

Walk both polygon edges from the first minimum-Y vertex towards the first
maximum-Y vertex, skipping horizontal edges, retaining integer stepping phase
while clipping. The executable reference specifies the signed edge quotient,
remainder, initial error and strict `error>0` carry. Full per-store equality
checks the rule, including deformed mixed-band polygons and clipped edges.

The native8-bit DIB uses pitch240 for239 visible pixels. Raw frame hashes include
the one padding byte per row, initialized like the rest of the backing. Strip
padding when uploading/displaying239 pixels. The last visible column's retained
backing and the separate padding byte must not be confused.

The current accepted terrain proof uses default relief, ordinary unmarked tiles,
known viewer3, an80x80 world and a full239x119 clip. It does not claim alternate
zoom sizes, arbitrary tiny maps, dirty-copy repaint history, styled window frame
pixels, or a World overview resampling implementation.

## Bases and units

The actual Detail flags admit the Base pass and return before unit-model drawing.
A valid Scout fixture confirms zero unit draw calls. Base names and the optional
Base overlays require other flags and are absent here.

Base admission requires an active game and a live Base on that tile. Its geography
gate accepts exploration or all-map revelation, but **not initial survey alone**.
Then the Base must belong to the display faction or carry that faction's visited
bit. Revelation does not bypass the foreign-Base visited gate. These controls use
normal noneditor renderer/display faction3; they do not generalize to deliberately
disagreeing viewers or editor mode.

The Base anchor is the tile projector anchor with Y reduced by14. Native city
art scales by9/16. Destination origin is `trunc(anchor + retainedOffset*9/16)`
with truncation towards zero on the complete sum. Retained width and height are
scaled and truncated separately. Preserve source-step phase when clipping. The
reference checks 800 native step values and clipped top/left/bottom composites.
Index9 is transparent; source0 applies the initialized native shadow lookup to
the destination. It is not a uniform black pixel.

The bounded fixture uses a population1 University Base with no facilities. Its
ordinary UNIV.PCX crop is `(1,1,100,75)`, trimmed to41x38 at offset27,17. An explicit
Citizens' Defense Force control selects crop `(1,77,100,75)`, trimmed to68x46 at
offset17,16. This does not establish every faction, population tier or facility
art selector. The normal project-location input is explicitly -1; PE zero-filled
globals must not accidentally make every Base own that project.

Population1 has a black10x11 border anchored at the Base anchor. A visited foreign
Base adds an index252 inner fill; an owned Base retains the underlying interior.
The numeral's baseline is anchor+(3,9), using Arial height-9, weight700,
transparent background and baseline alignment24. Owned text RGB is216,224,235;
the hostile foreign fixture uses black. Layout and native requests are captured;
glyph pixels are supplied by current Windows GDI through a declared generic API
boundary. These hashes do not establish historical Windows glyph parity.
