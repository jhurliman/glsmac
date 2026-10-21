# Workshop saved-design strip: geometry, navigation and drawing context

2026-10-20, pinned original SMAC executable as in the
[human producer](design-workshop-human-producer.md). The independent scalar
reference is [workshop_strip_reference.py](workshop_strip_reference.py).

## Resolve the two rectangles first

The saved-design strip is painted by **447d40** using editor rectangle **+b0c**,
not43d730/+a4c. In the supplied1024x768 geometry:

| Element | Console-local rectangle | Screen rectangle (console y511) |
| --- | --- | --- |
| Status/action explanation panel,43d730/+a4c | (266,70)-(759,168) | (266,581)-(759,679) |
| Saved-design strip,447d40/+b0c | (8,192)-(764,248) | (8,703)-(764,759) |
| Left arrow23,+cbc | (12,199)-(24,241) | (12,710)-(24,752) |
| Right arrow24,+ccc | (748,199)-(760,241) | (748,710)-(760,752) |

**The old parent fixture stopped too early to establish visible saved cards.**
Its console constructor stopped at466f98. Native console navigation rectangles
are produced later at4671da..467268, from the loaded12x42 arrow assets and
console panel geometry. Zero arrow bounds in that earlier receipt were not the
finished runtime geometry. In particular the old model call at(30,207) came
from incomplete arrow/card geometry; it is not a verified visible thumbnail.

The new bounded control continues the *original* console constructor through
46726b, before Workshop construction. Console left-arrow source rectangle is
(143,199)-(155,241), right is(748,199)-(760,241). Workshop446f8f..447000 relocates
the left rectangle to x12..24 using console frame-left+8, retains its12x42 size
and y, and copies the right rectangle. Its constructor then creates the real
console-parented buttons23/24. These are geometry initialization, not a later
per-paint guessed placement or a resize of the Workshop client.

## Card geometry and browse consumption

At447dd7..447ef2 the painter rebuilds5bdb60, reads count94f7a0 and saved offset
editor+ce4, then scans exactly one cyclic pass through that many raw IDs. It
stops after **ten admitted cards**. Lists shorter than ten do not repeat to
fill the remaining positions. Offset is a browse-list index, not raw design ID
or selected-card index. The declared-list controls supply5bdb60's output at
that boundary; they do not replace or retest its existing construction proof.

Let `L=leftArrow.right+3`, `R=rightArrow.left-3`:

```text
cardWidth = truncTowardZero((R-L+2)/10) - 2
stride = cardWidth + 2
outer[j] = (L+j*stride, strip.top+3,
            L+j*stride+cardWidth, strip.bottom-3)
inner[j] = inset(outer[j],2)
```

The controlled result is width70, height50, stride72. First outer card is
**(27,195)-(97,245)**, inner(29,197)-(95,243). Tenth outer card is
(675,195)-(745,245). The native width always reserves ten columns, even for a
four-card list. It does not stretch four cards across the available width.

Additional painter admission, source447ef9..447f64: foreign nonzero-owner IDs
are skipped; intrinsic records require current owner technology and reject
negative weapon strength. Own records can be active or inactive. Skipped IDs
consume scan positions but do not advance the visible card column. The four
new paints exercise mixed active/inactive cards, cyclic wrap, the ten-card cap,
and an empty list; foreign/negative-intrinsic admission remains source-traced.

Selection uses actor+141f8 and hover uses+ce0. Border style index is2 for the
selected raw ID, otherwise1 for hover, otherwise0, selecting the native style
at7b0058+40*index. Selection takes precedence over hover.

## Hit routing and offsets

The hit list lives at editor+12758. The painter clears it with613a90, registers
outer rectangles for every card, and additionally registers inner rectangles
for active cards. Payload is `(kind=0, value=rawDesignID)`, not visible column
or browse ordinal. Thus the four-card mixed control has seven hit records.

Native613b00 scans registrations **backward**, with half-open rectangles:
`left<=x<right`, `top<=y<bottom`. Inner records win overlap but return the same
raw ID. Fifteen native hit queries agree with independently generated boxes,
including corners, outer border, inner overlap and right/bottom exclusion.

Click447a40 is called on the Workshop WindowBase subobject at editor+a14.
Its+11d44 resolves to editor+12758. A nonnegative hit value is stored as the
selected raw ID; it calls4490b0(rawID,0), then virtual+f8 full refresh. Use the
existing reselect/cleanup contract; drawing a selected border alone is not a
selection implementation. No new full mouse-loop or desktop test is claimed.

Native dispatcher442600 indexes its jump table with **actionID+2**. Two focused
dispatch controls verify23->442663 and24->442689. Their mutation fragments give:

- Left23 decrements offset; if negative, stores **count**, not count-1. The
  following painter uses modulo, so left at0 repeats the first view once;
  another left reaches count-1. Preserve this source edge if matching exactly.
- Right24 increments offset; it resets to0 only on equality with count.
- Both repaint447d40 and do not select, commit or alter the draft.

Six native offset controls include count1 and wrap boundaries. Empty-list paint
is safe because its loop never divides; arbitrary negative offsets are not an
admitted input contract. The keyboard handler443b40 additionally has source-only
page shifts±9 with cyclic remainder/clamp, Home0 and End=max(count-10,0).

Navigation captions are empty strings. Native arrow images are assigned by
4469a0@447847..447890: left states7ee5e8/7ee614/7ee640, right
7ee66c/7ee698/7ee6c4. Do not create text labels from the empty captions. The
controls' normal visibility/enable lifecycle remains separate from these
constructor bounds; no count<=10 arrow-hiding rule is introduced by447d40.

## Captions and model context

The painter uses a configured font size10 via630450, separate from the larger
main-preview font. The installed font fixture yields13-pixel line boxes.
Active title uses the inner rectangle inset a further3 horizontally, with a
single line at its top. It first removes trailing bytes until measured width
fits, then cuts at the **second ASCII space**, if any. Do not approximate this
with a generic ellipsis or always print the whole stored name.

| Card | Constructed title / controlled displayed example |
| --- | --- |
| Active intrinsic | `* ` + stored name; Scout becomes `* Scout` |
| Active own, not excluded | one-based bank slot + `. ` + stored name; `1. Custom` |
| Active own, excluded | localized Obsolete + ` (` + bank slot + `)`; `Obsolete (2)` |
| Inactive own | localized Empty Slot + ` #` + bank slot; `Empty Slot #3` |

The inactive title uses the whole inner box rather than the active single-line
rect. It does not display the stale stored name or a model. Active cards draw
the 5932d0 stat string in a bottom line (`1-1-1` or `1-2-1` in these controls).
Debug flags959fc8 can append raw-ID text; this is a source-only branch here.
Excluded own caption color is palette248 when cached unit count0, otherwise255;
normal active text uses159. The exclusion bit is owner-specific, not raw flag8.

For each active card, original5d9ca0(rawID,owner) constructs the **scratch Unit
2048** via5d8420. The actual observed call boundary is:

```text
56fd50(consoleCanvas=7cbc04, unit=2048,
       x=outer.left+15, y=strip.top+15,
       scale=-10, flags=0x1048, final=0)
```

First model anchor is therefore **(42,207)** after completed arrow geometry,
then114,186,... in the controlled strip. The unit buffer and complete call
arguments are retained in the new receipt. These coordinates are console-local;
the first screen anchor is(42,718). Model clipping is the inset card box at the
call, not status rectangle+a4c or main-preview rectangle+a9c.

This is the ordinary-unit draw wrapper, **not** Workshop scene4466c0 with camera
scale0.14. The source renderer dispatch at570174..5701cc takes the ordinary
idle4d7860 path for qualifying non-icon units under the supplied0x1048 flags
(no action bits10/20). Existing ordinary-unit model/animation evidence can be
reused for implementation. Keep the native -10 scale/flags and scratch Unit
context; resizing the main Workshop model cannot reproduce this contract.

The focused paints intentionally supply model rasterization at56fd50 and
record its arguments. They execute real scratch-unit construction, original
font measurement/text and card layout, but **do not add model-pixel evidence**
or prove the renderer branch dynamically. No fabricated whole-parent screenshot
or broad model matrix was produced.

## Evidence and independent work boundaries

Final additive bundle:
`tmp/reference/independent-workshop-strip-v2-evidence-20261020/`.
Manifest SHA-256:
`114ed324172b85145a60c06a99f8aa8d677e7ca244974b2fb4d06198251972a0`. It contains
four native paints, independent geometry/context comparisons,15 hit controls,
six offset fragments and two dispatcher controls. The earlier preliminary strip
freeze is superseded; this v2 includes the completed dispatcher checks.

The port can now implement browse cards, exact geometry, raw-ID hit routing,
cyclic navigation and captions independently of the main Workshop scene and
full-parent boot work. General conventional normal-open is separately ready in
the [human producer](design-workshop-human-producer.md). Use the
[input-authority correction](design-workshop-input-authority.md) when connecting
its typed engine inputs. Whole-parent console/client compositing and native
strip model pixels remain outside this new bounded evidence.
