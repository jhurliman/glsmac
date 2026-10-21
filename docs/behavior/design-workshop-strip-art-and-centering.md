# Workshop strip text, installed arrow art, styles and centering

This supplements the [saved-strip geometry](design-workshop-saved-strip.md)
and the [native whole-parent capture](design-workshop-native-parent.md).
All art remains in the installed files or private evidence bundle.

## Exact installed labels

| Caption | LABELS zero-based slot | Native table byte offset | Installed labels.txt line |
| --- | --- | --- | --- |
| Empty Slot | 110 | 0x1b8 | 121 |
| Obsolete | 912 | 0xe40 | 923 |

The native reader is62e860 using the token at `9c15b8 + offset`.
Empty Slot is read at448033..44803a; Obsolete at448292..448299 and
4482f9..448305. The installed file also has another Empty Slot at line471;
that is not the strip's label. Append punctuation/slot numbers according to
the existing caption rules; these labels do not contain the final ` #N` or
` (N)` suffixes.

## Arrow atlas and state mapping

Source file: **interface.pcx**. Rectangles below are `(x,y,width,height)`,
not inclusive right/bottom bounds. Each is12x42 with a one-pixel gutter.

| State | Left crop | Right crop |
| --- | --- | --- |
| 0 normal | (397,153,12,42) | (397,196,12,42) |
| 1 hover | (410,153,12,42) | (410,196,12,42) |
| 2 pressed | (423,153,12,42) | (423,196,12,42) |

Original466f81..466f93 loads interface.pcx;46702a..467097 creates the six
sprites through5fc7d0. Left sprites are7ee5e8/7ee614/7ee640; right sprites are
7ee66c/7ee698/7ee6c4. Workshop binds them to button fields+ad4/+ad8/+adc.

Generic painter6222a0 selects a forced+ab8 value when it is0..2. Otherwise
`+a9c != 0 || +a18 != 0` selects2; else `+a74 != 0` selects1; else0.
622990 sets/clears+a74 on pointer entry/exit, and622af0 clears it on leave;
621a30/621b90 manage the held+a9c state. Keep the forced/other-active-state
precedence rather than deriving every state from pointer hover alone.

All six original RLE-sprite draws compare byte-for-byte with independent
installed PCX crops after palette relocation. The crops use only source
indices0..235, which become active palette indices+10, and source transparent
255, which becomes **key9**. Drawing raw PCX indices directly against the active
game palette produces incorrect colors. The private receipt retains each
decoded sprite and source-file hash.

## Background and frame palette identities

The strip background uses the24x24 bitmap referenced by7b01e0, through
5f3660 at447dd2. In this captured theme it alternates whole rows **198,65**,
starting with198 at row0. Active palette65 is RGB(0,0,0);198 is RGB(12,12,16).
This is a theme tile, not a flat invented fill or one of the arrow crops.

Card styles are selected at447f92..447fa7 from `7b0058 + 40*state`:

| Card state | Style address | Border index | Alternating interior indices |
| --- | --- | --- | --- |
| 0 ordinary | 7b0058 | 34 | 65 / 196 |
| 1 hover | 7b0080 | 29 | 65 / 194 |
| 2 selected | 7b00a8 | 26 | 83 / 191 |

The shared outer strip frame is style3 at **7b00d0**, with outline index31
and no center tile. These are eight-sprite frame styles, not just stroke color
arguments: their corners carry transparency. The native70x50 card controls
have transparent corner pixels and alternating interior scanlines; the outer
frame has a two-pixel chamfer. Preserve the existing generic style renderer or
the recorded source masks rather than filling rectangular corners.

The private frame-style receipt records four complete original70x50 rasters,
their pitches, eight sprite identities and dimensions, center texture pointers
and active palette. Card corners are6x6, horizontal edges24x6 and vertical edges
6x24. Native5fbbd3 stretches/tiles these components. The source-style field+0x24
contains7; it is not the visible border's palette index.

Normal caption/stat color is159. Excluded own captions use248 with no cached
units,255 otherwise, as in the prior strip handoff. The narrow addition does
not infer another faction theme or alternate palette profile.

## Opening offset and reselection

Constructor4469a0 writes **+ce4=0** at446b4a. A cached open can retain an old
offset. After5bdb60,449740..449772 scans one complete cyclic pass starting at
that current offset, looking for the open routine's local **EDI target ID**.
On a match at browse position `i`, it stores:

```text
offset = signed_remainder(i + browseCount - 5, browseCount)
```

It leaves the old offset unchanged if the target is absent or count is0.
The target is the request/initial candidate held by449630, not a reread of
`actor+141f8` after4490b0. Those IDs can differ on inactive-slot fallback; do
not silently substitute the selected readback into this source contract.

For the captured70-entry browse array, Scout2 is position2, so opening stores
**67** and displays it at zero-based column5. Entries125,126,127 precede Colony0,
Formers1 and Scout2; those custom raw IDs display slot numbers62,63,64.

A card callback447a40 calls `4490b0(rawID,0)` and refreshes; **4490b0 does not
write+ce4**. The original full-window control clicks Transport3 at console-local
(500,220), changes selection2→3, and retains offset67. Plain reselect therefore
does not recenter. Separate dispatcher paths do: Apply1 at442707..442a63 and
retirement2 share the centering store442a5d; Obsolete6 and Upgrade7 have their
own guarded recenter stores442bc0 and442f63. Treat these as caller behavior,
not a universal side effect of selecting a design.

Seven original fragments match the independent `workshop_strip_reference.recenter`:
fresh/reopened Scout, Transport, absent target, empty list, left-arrow
count sentinel and a synthetic two-entry signed-remainder control. The latter
returns-1; it is not painted or claimed as an admitted real own-bank state.
Real owner banks supply64 slots, so this short-list arithmetic edge does not
justify replacing native signed remainder with a different modulo operation.

The additive evidence is frozen with the native-parent handoff. No component
or model raster matrix was added for these lookups.
