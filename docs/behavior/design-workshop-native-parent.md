# Original Workshop whole-parent composite

One controlled **1024x768 original window-tree frame** is now captured, with
Scout Patrol2 selected, component models and labels, action buttons, console,
saved strip, inactive slots and both native arrow sprites. A separate standalone
execution reproduces its whole indexed buffer byte-for-byte:

`ecf6318a1d4a01e7f33389144c468f757bf9f600c5903e8d2c40452a423d9439`.

Private original image:
`tmp/reference/workshop-parent-capture-private-20261021/native-parent.png`.
The indexed buffer and palette are retained separately. The PNG is only palette
expansion of that native buffer; no host layout or assembled mockup is used.
The background is an explicitly blank primary map context, not a booted
seven-faction campaign or a native desktop screenshot.

## Native prerequisites and capture boundary

The old4d9a7e blocker is resolved. The original startup loop5a5b29..5a5b69
allocates seven `0x22480`-byte auxiliary map-window objects through46f110. Its
pointer table at7f0b58 had previously been empty. The primary map object is
91f500; its Window base is943294, also the Workshop parent.

Creating auxiliary objects alone permits449630 to reach modal show but is not
sufficient for the whole frame. Workshop's parent943294 must also be created
by original5327e0 and shown by its original606850 method. An uncreated/hidden
parent permits isolated client painting but excludes Workshop from the visible
window-tree composite. This explains the previous missing-parent result.

The replay completes original console constructor4660d0 through its return,
including arrow crops, navigation rectangles, widgets and fonts. It then opens
`449630(owner1,design2)` with7528f0=0, executing the fresh Workshop constructor
and ordinary selection path. It stops at60708b after native modal show and
before the input pump. Original6101a0 then composes the visible window tree
into the native1024x768 front bitmap9bf978, initialized by the original
609281..6092a5 screen-bitmap fragment.

All model, client, frame, child-control and console painting/copy paths execute
in the original x86 code. The inherited harness supplies memory Win32/GDI/file
services and host GDI font metrics. Two additional boundaries are explicit:

- ReleaseDC returns success for GetDC's guest offscreen DC; no desktop DC is
  owned. The native caller then clears its current-DC field normally.
- timeSetEvent records the original multimedia timer request and returns inert
  timer1. No asynchronous callback is delivered; animation counter8f3c60 is0.
  The receipt retains the actual preview-angle bytes after native paint calls.

There is no unresolved prerequisite for this controlled composite. Live OS
message delivery, focus/hover from a real pointer, timer-driven animation and
a normally loaded map behind the window remain outside it.

## Geometry and fonts in this frame

Workshop Window rectangle is **(216,168)-(839,555)** with client623x387.
Console rectangle is **(0,511)-(1024,768)** with client1024x257. Coordinates
inside their canvases are local; the native compositor performs placement.
The parent tree supplies the child action buttons that were absent from the
isolated Workshop client bitmap.

| Representative native text | Canvas-local x / baseline | Arial LOGFONT / measured height |
| --- | --- | --- |
| SCOUT PATROL | 232 / 53 | -16, weight700 / 19 |
| CHASSIS | 54 / 18 | -12, weight700 / 15 |
| HAND WEAPONS | 38 / 174 | -10, weight0 / 13 |
| FISSION PLANT | 472 / 77 | -10, weight0 / 13 |
| APPLY | 30 / 15, button-local | -14, weight700 / 16 |
| DONE | 32 / 15, button-local | -14, weight700 / 16 |

The full glyph journal identifies every destination bitmap, font record,
baseline, color and clip. For example SCOUT PATROL clips to(140,9)-(451,320),
and CHASSIS to(30,7)-(129,92). Do not apply a main-title font or parent-canvas
coordinates to button-local captions. Strip text remains Arial-10/height13.

The owner1 material row is the previously independently checked supplied
Spartan tint; owner ID alone does not establish faction colors. Inputs include
the actual before-open/final Design and Faction pools, ownership, Base data,
palette, declared renderer/material initialization and native source sequence.
The fixture is a bounded UI state, not a replacement for campaign startup.

## Card lifecycle and integration details

The frame's70-entry browse list opens at offset67, centering Scout at column5.
An original447a40 callback at console-local(500,220) selects Transport3 from
Scout2, loads Foil/Transport/No Armor/Fission into the draft, and triggers real
441ba0/447d40 repaint paths. Offset stays67.

Only inactive **scratch Design63** changes during that callback's drawing;
its chassis/weapon bytes change to3/16. Saved custom records do not change.
The complete pool diff is retained, so this renderer scratch side effect is
not confused with saving or mutating the selected stored design.

[Strip art and centering](design-workshop-strip-art-and-centering.md) pins
the installed label slots, six verified arrow crops/state selection, theme tile,
card/frame palette indices and exact open-versus-reselect offset behavior.

## Frozen handoff

Additive bundle:
`tmp/reference/independent-workshop-native-parent-evidence-20261021/`.
Manifest SHA-256:
`826a933ae6c5f1fa979af9c5bd1575aa00759fc6ae2ba56355dd6d5a68aebeff`.
It contains the standalone original capture and declared inputs, native PNG and
indexed bytes, reproduction comparison, font/glyph and window data, actual card
callback/diff, seven centering fragments, six original sprite/crop comparisons,
four native2D frame-style observations, source disassembly and probe scripts.
Earlier immutable bundles are unchanged. No broad model matrix was repeated.

This supplies a concrete presentation target. It does not claim independent
whole-asset renderer regeneration; use the prior staged renderer/view/shadow
proofs for those implementation boundaries.
