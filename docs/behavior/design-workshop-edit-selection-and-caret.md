# DESIGNNAME custom edit: selection palette and caret

2026-10-21. Pinned terran.exe SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
One native DESIGNNAME layout reused through the existing template/GDI harness,
then four bounded original edit draw/callback states. No model generation,
real desktop/modal input loop or elapsed-time measurements. Font/palette/state
and native rectangle/line calls are retained in the
[compact receipts](design-workshop-edit-selection-and-caret-cases.json).
[Scalar reference](workshop_edit_draw_reference.py).
Verified original610x20 PNG targets, including selected/collapsed caret off/on,
are now exported in the [native DIB packet](design-workshop-edit-native-dibs.md).
The [edit-key supplement](design-workshop-edit-keys.md) adds39 bounded native
character/navigation/deletion controls and the exact31-byte boundary behavior.
All four draw states, two native callback transitions, eight source literals
and five scalar palette checks pass. Additive private freeze has19 payloads:
`tmp/reference/independent-workshop-edit-draw-evidence-20261021/`, manifest
`9c8e3cebb39ce6d13b672d3864cbed0a4705db8389a9b644dce1082a13b1eb63`.
This provenance paragraph was added after the immutable freeze.

## Resolved native palette indices, not OS highlight colors

After the field manager finishes configuring the DESIGNNAME Edit:

| Purpose | Palette index | Authority |
| --- | ---: | --- |
| Normal text / caret | 224 | Edit `+980` |
| Normal background | 0 | Edit `+b1c` |
| Selected text | 0 | Edit `+b20` |
| Selected background | 36 | Edit `+b24` |

These values were read from the initialized native control. They are palette
indices, not COLORREF/RGB values or guessed Windows selection colors.
The edit draws the ordinary text, then recolors the selected rectangle with
three calls to original `5fae70`, a clipped exact-index replacement:

1. Replace background0 with temporary37.
2. Replace normal text224 with selected text0.
3. Replace temporary37 with selected background36.

For `Scout Patrol`, whole selection `[0,12)` issues all three operations on
child-local rectangle `[0,0,84,20)`. This keeps selected glyphs black on
palette36. It is not a translucent overlay or a fill that erases the text.
Other palette indices normally survive; any existing37 in the rectangle also
becomes36 because of the temporary-index technique. Source
`62eda0@62f0d5..62f167` resolves optional selected-color overrides: if selected
background is -1, use normal foreground; if selected foreground is -1, use
normal background. DESIGNNAME supplies explicit36 and0 here.

Selection start/end can be reversed. Draw the span between their minimum and
maximum, using measured prefix/subspan widths rather than character-count
times a fixed width. Equal endpoints skip all three replacements. Horizontal
scroll `+b18` is a text byte index, not a pixel offset. Preserve generic edit
measurement/clipping for a long name; do not draw the full name outside its
field merely because Rename has a31-byte limit.

## Whole-default selection and final inset

The field manager `62bde0@62c139..62c1ea` sets max31 and calls the text setter,
then explicitly writes selection anchor `+b40=0` and active endpoint
`+b44=strlen(text)`. Text setter `62ece0` resets scroll `+b18=0`.
Empty text has0/0; ordinary first typing replaces the whole default and
collapses both endpoints after the inserted byte (`62f180`). Preserve the
default's case. The selection is logical state, independent of caret visibility.

The **final configured DESIGNNAME edit inset is0**, at `+b34`, and border
count `+b30` is0. Its earlier constructor paint used default inset4; the old
layout journal therefore contains both x4 and x0 glyph passes. Use the final
x0 pass and this resolved configuration for the final field. The control is
610x20 inside the retained Name row; see
[Rename layout](design-workshop-rename-and-obsolete.md) for parent coordinates.
For the actual loaded Arial Narrow -16 bold in this draw control, measured
`Scout Patrol` prefix width is84 pixels and effective font height20.

## Caret is a vertical line

When Edit `+b48` is nonzero, `62eda0@62eedc..62ef48` measures bytes from scroll
index to active endpoint, adds final inset, and calls original `5fa9f0`.
That helper draws one vertical pixel column with inclusive y endpoints and
clips it to the bitmap/clip rectangle. Color argument is normal text index224.

```text
x = measuredWidth(text[scroll:activeEndpoint]) + inset
y0 = floor((bitmapHeight - effectiveFontHeight) / 2)
y1 = y0 + effectiveFontHeight        # inclusive line endpoint
```

In the retained610x20 field, collapsed end selection12/12 gives the native
line request `(x=84,y0=0,y1=20,color=224)`. Bitmap clipping limits it to y0..19,
so it is visibly **one pixel wide and20 high**. It is not an underscore.
The caret is drawn before ordinary text and selection recoloring; preserve
that order when matching intersections with selected text.

## Focus and500 ms callback toggles

Constructor `62eae0@62ec2c..62ec3b` configures the timer at Edit `+b4c` with
callback62f920, payload=this, interval500 ms and stored resolution5. Since500
is at least50, `630160` uses ordinary Win32 SetTimer, not timeSetEvent.
That timer API does not receive the stored resolution5.

Focus handler `62f8d0(nonzero)` starts the timer and requests repaint; it does
not force `+b48=1`. Constructor initializes visibility0. Focus loss stops the
timer, clears visibility0 and repaints. Callback62f920 toggles `+b48` between
0 and1 and requests repaint. Two direct native callback controls confirm
off→on→off; the visible state produces the vertical line and the off state
does not. The probe's explicit paint after callback can duplicate the recorded
line call because the callback's repaint also draws it; it does not mean a
two-pixel caret.

The OS timer wrapper `6302f0` posts message401 hex with a pending latch, so
delivery/repaint is queued and coalesced. Do not make the port catch up missed
ticks by drawing many blink transitions. Nominal cadence is500 ms per toggle,
1000 ms for a full on/off cycle; this is source/configuration, not a measured
wall-clock guarantee. Neither text insertion62f180 nor edit key handler62f360
directly resets visibility or restarts this timer; do not assume modern
restart-on-every-keystroke behavior.

## Main title reminder

An unchanged intrinsic component tuple keeps its saved main title even after
Rename sets a nonempty draft and named bit2. Changed intrinsic tuples and
custom selections use a nonempty draft only when selected named bit2 is set;
otherwise the main painter generates a name and clears that bit. The title
temporary buffer is uppercased. Full source clarification is in the
[Rename contract](design-workshop-rename-and-obsolete.md).
