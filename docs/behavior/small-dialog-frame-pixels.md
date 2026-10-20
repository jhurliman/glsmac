# Small dialog frame pixel semantics

This resolves the primitive behavior behind `5fbbd3` for the existing small-frame
bank, including styles0,3,6,9,27,28,29. It uses the pinned original executable,
**11 new bounded frame controls and3 primitive microcontrols**, plus independent
PCX reconstruction against six already-recorded parent DIBs. No modal layout,
glyph probe, desktop capture or foreground interaction is repeated.

## Keys and operation order

Loaded PCX indices are `(raw+10)&255`: raw253 becomes loaded7, raw255 becomes9.
These two values have different roles:

* **Sprite source key9**: skip that source pixel and retain the current destination.
  Loaded7 is copied as an ordinary byte by the sprite blitter.
* **Pattern overlay destination mask7**: copy a pattern pixel **only if the current
  destination byte is7**. The source pattern byte is copied literally, including
  source7 or9. This is not source-key transparency.
* **Final center bitmap copy**: copy every pattern byte in its clipped rectangle;
  neither7 nor9 is skipped.

Native microcontrols, all one-row indexed buffers:

| Primitive | Source | Destination before | Destination after |
|---|---|---|---|
| Sprite key9 (`5fc7d0 → 5fd990`) | 7,9,31,41 | 23,23,23,23 | **7,23,31,41** |
| Destination-mask7 (`5fb900`) | 9,7,31,41 | 7,7,9,31 | **9,7,9,31** |
| Center bitmap copy (`5f3660`) | 7,9,31,41 | 23,23,23,23 | **7,9,31,41** |

Thus **do not globally treat raw253/loaded7 as transparent**. In these frames it
usually disappears because the subsequent masked overlay replaces it with the
center pattern. If the current destination was already7 under a source-key9 hole,
that destination also gets replaced by the overlay. Its test uses destination
bytes, not the source sprite mask.

`5fbbd3` processes pieces in this exact order:

1. Top-left corner, then its masked pattern overlay.
2. Top-right corner, then its masked pattern overlay.
3. Bottom-left corner, then its masked pattern overlay.
4. Bottom-right corner, then its masked pattern overlay.
5. Entire tiled top edge, then its masked pattern overlay.
6. Entire tiled bottom edge, then its masked pattern overlay.
7. Entire tiled left edge, then its masked pattern overlay.
8. Entire tiled right edge, then its masked pattern overlay.
9. Final interior pattern copy.

An overlay runs only when style-table `+20` is a nonnull center bitmap; its mask
byte is style-table `+24`, measured7. The final center copy has the same pointer
gate. The asset builder suppresses that bitmap when the center's first loaded
pixel is key9; style3 has no center, and therefore no masked overlays or final
center fill. That builder gate should not be confused with a per-pixel center
transparency rule.

## Bounds and phase

Let frame rectangle be half-open `[L,T,R,B)`. For the measured small atlas bank,
corner size is6×6, horizontal tiles24×6, vertical tiles6×24, center24×24.
The existing atlas crop contract is unchanged.

| Piece | Destination rectangle |
|---|---|
| TL | `[L,T,L+6,T+6)` |
| TR | `[R-6,T,R,T+6)` |
| BL | `[L,B-6,L+6,B)` |
| BR | `[R-6,B-6,R,B)` |
| Top | `[L+6,T,R-6,T+6)` |
| Bottom | `[L+6,B-6,R-6,B)` |
| Left | `[L,T+6,L+6,B-6)` |
| Right | `[R-6,T+6,R,B-6)` |
| Final interior | **`[L+6,T+6,R-6,B-6)`** |

Each keyed border edge repeats from its **own piece rectangle's top-left**:
top/bottom tile x starts at L+6; left/right tile y starts at T+6. The corner sprite
is placed1:1. The masked pattern pass covers the **entire corresponding corner/
edge rectangle**, including its outermost row/column. It does not use an inset1
rectangle or restrict itself to the inset6 interior.

All pattern passes, including the final interior, sample the center by
**absolute coordinates in the destination bitmap**:

```
patternPixel = center[destY % 24][destX % 24]
```

This is parent-bitmap phase, not screen position and not frame-relative phase.
For example an inner caption starting at7,7 samples phase7,7; a faction row
starting at0,19 samples phase0,19. Moving the outer window on screen does not
rephase its existing backing bitmap. A separate child backing bitmap has its own
coordinate origin.

The compositor passes identical phase and destination x/y to `5f34c0` and
`5f3660`. Those tilers align their first tile by subtracting signed IDIV remainders
of phase x/y. `5f37f0` instead receives phase0,0 for edge sprites. Clip intersection
does not reset phase. The implementation must intersect each operation's rectangle
with the existing destination clip and bitmap bounds, preserve the old clip, then
restore it. Negative origins are allowed; the clipped control
`[-3,-2,51,39)` with additional clip `[5,7,47,36)` matches exactly.

There is no frame-wide minimum-size clamp or scaling of corners. A10×11 frame
still stamps its four6×6 corners in source order; edge/interior passes with
nonpositive extents contribute no pixels. The primitive controls include that
overlap case. General source code uses the pieces' actual stored dimensions;
the6/24 constants above describe this measured small-bank family.

Equivalent implementation sketch for this family:

```
for part in [TL, TR, BL, BR, top, bottom, left, right]:
    stamp_or_tile_sprite(part, source_key=9)
    if center_exists:
        for (x,y) in clip_and_bounds(part.rect):
            if destination[y][x] == 7:
                destination[y][x] = center[y % 24][x % 24]
if center_exists:
    for (x,y) in clip_and_bounds([L+6,T+6,R-6,B-6)):
        destination[y][x] = center[y % 24][x % 24]
```

## Validation and shortcut limits

The exact-order independent model decodes the installed INTERFACE.PCX directly,
rebases indices and implements the rules above; it does not read native sprite
bytes. It matches original execution for **43,431 pixels across11 frame controls**,
including shifted origins, clipping, undersized overlapping corners, ordinary/
hover/selected rows, and a destination initially filled with7. All three synthetic
microcontrols also match their literal expected results.

Using the earlier corrected layout receipts, four frame calls reconstruct each
FACTION/NAME/FIRSTBASE parent backing DIB in plain and console-visible mode0.
Across all six components there are **zero differences in733,418 frame pixels**.
Caption text rectangles are excluded; the console-visible comparison also excludes
the exterior of its frame rectangle, where separate console/background/rail logic
applies. The source reference PNGs and their hashes are retained privately. No new
layout execution is involved in this comparison.

The proposed shortcut—prefill inset1 with absolute-phase pattern, then globally
skip loaded7/9 when stamping borders—fits the ordinary outer/caption cases, but is
**not a general substitute for the source algorithm**:

| Control | Shortcut mismatches against original |
|---|---:|
| Row style27 at `[0,19,75,35)` | 174 |
| Row style29 at `[0,0,75,16)` | 174 |
| Style9 over existing destination7 | 4 corners |

The first two expose loaded7 on outermost border rows/columns, outside inset1.
The third exposes the destination-mask behavior under source-key9 holes. The
exact model has zero mismatches for every control. The same source-order compositor
should therefore serve the runtime outer frame, caption, body and row backgrounds.

Four ordinary source-key9 corners retain their previous destination when that
destination is9. Their `(9,0,0)` appearance in the diagnostic reference is a
representation of the retained indexed canvas value, not a new opaque production
corner color or permission to map loaded7 through raw common-palette index253.

Public metadata: `small-dialog-frame-pixels-cases.json`. Private source, primitive
outputs, raw indexed comparisons and component references are in the frozen
`tmp/reference/small-dialog-frame-pixels-evidence-20261019/` bundle. Raw `.bin`
frame buffers are one byte per pixel, top-down, tightly packed to the recorded
width; they remain private owned-asset evidence. Source guards cover32 instruction
literals, including interleaved calls, destination-byte comparisons and tiler
phase arithmetic. Large-bank outer frames, arbitrary sprite dimensions and
non-default stretch ratios are outside these new controls.
