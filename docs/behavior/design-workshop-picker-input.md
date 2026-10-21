# Generic Workshop picker input: exact source state transitions

2026-10-21. Pinned terran.exe SHA256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
Source-only completion of keyboard43ca60, hover43c880 and acceptance43c990.
The [scalar reference](workshop_picker_input_reference.py) preserves native
state assignments and caller boundaries; it is not a new desktop/input matrix.
The [scalar cases](design-workshop-picker-input-cases.json) check12 source-derived
keyboard expectations and3 hover/release boundaries; they are not native runs.
It applies to all four generic component pickers. The ability list modal has
different input behavior.

## State and final admission

All offsets below are hex, relative to the picker:

| Symbol | Field | Meaning |
| --- | --- | --- |
| N | +2b74 | Admitted definition count |
| S | +2b80 | Selected/hovered admitted ordinal, or-1 |
| p | +2b60 | First displayed admitted ordinal |
| C | +2b6c | Columns5 |
| K | +2b64 | Visible cells20 |
| T | +1440 | Scrollbar position (scrollbar+ a2c) |
| Q | +1454 | Page delta3 (scrollbar+ a40) |
| A | +4648 | Stored availability result from a card painter |

Constructor43b7d0 initializes A0. Initial grid paint stores S but does not
assign painter return to A. Arrow navigation does **not** skip unavailable
cards. The final modal return at43d483..43d4de requires S>=0 and A!=0;
otherwise it returns the incoming initial ordinal. Reactor success then adds1.
Weapon callers separately map admitted ordinal back to definition ID.

Confirmed initial-input consequence: with initial S valid and A0, Enter13
closes and returns that same initial ordinal. Moving within its unchanged
hover ordinal does not refresh A. Releasing on that same ordinal leaves A0,
sets S=-1 and closes; final admission again falls back to the initial ordinal.
This remains true even if the initial card is available. No mutation is implied
by this isolated modal result; the component setter's same-value cleanup is a
separate caller contract.

In pseudocode below `setT(v)` calls61f1e0 and clamps to the supplied inclusive
range0..`trunc(N/5)-4+2` before returning T. It repaints the scrollbar; it
does not itself invoke the parent's43cf80 callback. Integer division truncates
toward zero, including S=-1. Save entry values `oldS,oldP` for the epilogue.

## Exact keyboard branches

`43ca60` receives its key code as the **second** argument. The first argument
is unused here. Ordinary keys and numpad aliases are decoded by43cd40/43cd14.

| Key | Native branch |
| --- | --- |
| Up38 / numpad8=104 | 43cab9 |
| Down40 / numpad2=98 | 43cb0d |
| Left37 / numpad4=100 | 43cb9d |
| Right39 / numpad6=102 | 43cbde |
| Home36 / End35 | 43cc4f /43cc21 |
| PageUp33 / PageDown34 | 43cc5f /43cc83 |
| Enter13 / Escape27 | 43caaa /43ca91 |

**Up:** if S<C, return immediately, without invalidate. Otherwise subtract C
at least once, repeating while S>=p+K. Store that S. If S<p, store p=p-C and
call setT(T-1). There is no extra N clamp or scan for a known card.

**Down:** first test S>N-C-1. If true and N<=K, return immediately. If true
and N>K, call setT(T+1), then store p=C*T, **leaving S unchanged**. Otherwise
add C at least once, repeating while S<p, and store S. If S>=p+K, store
p=p+C and call setT(T+1). The direct p assignment and clamped T call are
separate operations; do not silently recompute p from T in this latter branch.

**Left/Right:** calculate `lo=C*trunc(S/C)` and `hi=min(lo+C-1,N-1)`.
Candidate is S-1 or S+1. If candidate<lo or hi<lo, store S=lo; otherwise store
S=min(candidate,hi). p and T do not change. This clamps within the **global
ordinal row**, including a short final row. It does not wrap at row edges.
Do not first normalize S=-1 to0: for example Down from-1 enters ordinal4
at p0, whereas Left/Right from-1 yield0 under the original signed division.

**Home:** store p0, S0. **End:** store S=N-1; only if N>K, store
`p=C*(trunc((N-1)/C)+1)-K`. Neither branch calls setT. Thus their scrollbar
position can remain stale relative to p; subsequent branches read that stored T.

**PageUp/Down:** if N<=K, return immediately. Otherwise call setT(T-Q) or
setT(T+Q), then store p=C*T. **S is unchanged**, even if it is now outside
the displayed window. Do not choose the first visible card automatically.

**Enter13:** call virtual+e8 (607100, modal close), retaining S and A.
**Escape27:** first store S=-1, then call the same close. Both continue through
the epilogue below; neither runs a new availability test before closing.

The generic Window607910 wrapper first maps Enter13/1000d to virtual+b0(-1)
and Escape27 to+b0(-2), but the picker resolves+b0 to **407bc0, a no-op**.
It then dispatches virtual+64 to43ca60. Consequently **1000d alone does not
take this picker's Enter13 close branch**; do not import the ability-modal
keypad-Enter behavior into this picker. Unrecognized keys reach the ordinary
epilogue/invalidate path without changing S or p.

## Repaint and availability epilogue

`43ccc0..43cd06` is shared after non-early-return branches:

1. If p changed: virtual+f8 calls5ee890, which reaches the full grid painter
  43ca10. It paints all20 cells with selected=(ordinal==S), **without storing
  any painter result in A**. A retains its old value.
2. Otherwise, if S changed: paint oldS unselected, paint newS selected, then
  store the new painter's return value in A.
3. Call virtual+fc (407cb0, invalidate).

`43cda0` returns1 immediately if its ordinal is outside `[p,p+20)` and also
returns1 on its >=N blank-cell branch. Thus the epilogue may set A1 for an
off-window/blank selection; it is not a fresh technology check in those cases.
Escape can leave A1 after its repaint of S=-1, but S=-1 still fails final
admission. This is literal caller behavior, not a recommendation to weaken the
port backend's range validation.

Examples for N21/C5/K20, with native stored T shown explicitly:

| Input (S,p,T) | Key | Result (S,p,T) | A update |
| --- | --- | --- | --- |
| 15,0,0 | Down | 20,5,1 | Retained because p changed |
| 20,5,1 | Down | 20,10,2 | Retained |
| 20,10,2 | Down | 20,10,2 | Retained; no selection/window change |
| 5,5,1 | Up | 0,0,0 | Retained because p changed |
| 20,5,1 | Left or Right | 20,5,1 | Retained; final row has one item |
| 0,0,0 | PageDown | 0,10,2 | Retained; selected ordinal is off-window |
| 20,10,2 | Home | 0,0,2 | Retained; T is not synchronized |
| 0,0,0 | End | 20,5,0 | Retained; T is not synchronized |

## Hover, hit boxes and mouse release

The picker's vtable68d16c maps virtual+44→43c880 (move/hover), +48→407a30
(leave no-op), +50→407a50 (left-down no-op), +54→43c990 (left release),
and +64→43ca60. Hover and keyboard share S, not separate highlight states.

At **43c880 entry**, input coordinates must satisfy
`4<=x<4+108*C` and `4<=y<4+94*rows`, with rows4. For the5x4 picker that is
**`[4,4,544,380)`**. Candidate ordinal is
`p+C*trunc((y-4)/94)+trunc((x-4)/108)`.
This includes the gutters of each108x94 cell; the handler does not separately
test the inset model/text clip or painted105x91 frame. These are the literal
receiver operands: there is no subtraction of23 inside43c880. Native Window
routing converts coordinates through its stored+13c/+14c origins in606150/
60fd90 before dispatch; do not confuse source callback coordinates with
screen coordinates or replace the hit test with the painted RECT formula.

For a different valid candidate<N: **paint new candidate selected first and
store its return in A**, paint oldS unselected if oldS>=0, store S=candidate,
then invalidate. An unavailable candidate remains highlighted but sets A0.
If candidate==S, return immediately: **no repaint, A refresh or invalidate**.

Outside the hit area, or on candidate>=N: paint oldS unselected if nonnegative,
then set A0 and S=-1. That branch contains no explicit invalidate call.
Moving outside the picker Window itself invokes its leave no-op, so a Window
leave is not, by itself, proof that43c880 cleared selection. The clearing rule
applies when43c880 actually receives the outside-grid coordinates.

**43c990 first invokes hover+44 with the release coordinates.** If resulting
A is zero, it sets S=-1. It clears owned modal/capture globals9bffa4/9bffa8
when they refer to this picker, invokes virtual+10 for owned9bffa4, and then
**always calls close+e8**, including release on an unknown/blank/outside-grid
location. The modal's final S/A gate decides whether the selected ordinal or
original choice is returned. A click on an unknown card closes and falls back;
it is not an indefinitely open rejection flow.

Native WM202 routing at60991a..60996f dispatches Window+148→60f3d0; its normal
visible-window release branch at60f513..60f538 dispatches picker+54→43c990.
WM201 uses609894→60b1b0→Window+144/60f240→6075d0→picker+50, which is no-op.
WM203 double-click uses6098eb with the double-click flag through that same
down route; its extra down callback has no picker-specific confirmation.
**A normal single left release already accepts/closes**. Do not require a
double-click or add a second acceptance on the double-down event. These are
source routes, not newly timed OS double-click controls.

## Pointer coordinate clarification: no23-pixel origin correction

Source construction and the retained native parent-window fields settle the
coordinate relationship. **Do not subtract23 from a canvas-local GUI pointer
to compensate for the painted frame margin.**

`43cfc0@43d2bf..43d2e0` constructs the picker with x0/y0, W/H copied from
Workshop canvas, parent73e238 and flags **01000020 hex**. Constructor604250
zeros its+14c RECT (`6043fb..60440d`). In604c00, these flags take
`604f37..604f4b` (no nonclient-frame bit2), then `604fcc..604fe6`, giving
picker+13c=`[0,0,623,387)` and leaving picker+14c=`[0,0,0,0)`.
The narrowed console frame is subsequently **painted** at x23; it does not
change either Window rectangle or install a23-pixel client inset.

The retained initialized parent receipt
`tmp/reference/workshop-parent-capture-private-20261021/controls.json`
records Workshop73e238 fields:

| Field | Value |
| --- | --- |
| +98 flags | 01000000 hex; parent-recursion bit20 clear |
| +13c | `[216,168,839,555)` |
| +14c | `[0,0,0,0)` |
| Canvas nominal RECT, +474 | `[0,0,623,387)` |

`606150@606159..606183` subtracts the current Window's+13c.left/top plus
+14c.left/top. If flag20 is set it recurses to the parent
(`606185..60619b`); flag8000 would add the parent's+13c origin back afterward,
but the picker does not set it. Thus this picker contributes(0,0), then its
Workshop contributes(216,168), and recursion stops at that parent's clear20.
The modal/capture cases in60fd90 apply the same origin arithmetic.

For this native profile:

```text
callbackX = screenX - 216
callbackY = screenY - 168
```

More generally, subtract the actual picker canvas screen origin. If the GSE
GUI has already supplied coordinates local to that canvas, pass them directly
to the receiver's hit arithmetic. A scaled GUI must first undo its own display
scale; there is no additional native23-pixel correction.

The native mismatch is consequently explicit: first painted frame is
canvas`[27,4,132,95)`, screen`[243,172,348,263)`, while first hover bin is
canvas`[4,4,112,98)`, screen`[220,172,328,266)`. The full hit grid is
canvas`[4,4,544,380)`, screen`[220,172,760,548)`. Gutter and edge behavior
therefore follows those bins rather than the painted rectangles. This is a
source-derived original asymmetry, supported by the retained parent geometry;
it is not a newly observed desktop cursor trace.

## Port integration and validation

The shared GSE input controller retains S, p, T and A separately. All four
component dialogs use it. Arrow/numpad aliases, Home/End, paging, Enter/Escape,
hover and single left release follow the source transitions above. The backend
continues to validate component identity, ownership and technology admission;
no out-of-table result is admitted.

Pointer callbacks are registered on the picker canvas. The port Window blocker
consumes pointer events before Window-level callbacks, so registering only on
the Window does not establish working delivery. Canvas-local coordinates need
no additional transform in the1024x768 profile. Highlight-only changes repaint
cached indexed models while retaining text nodes. Armor shares the bounded
model cache with before/after centering; Probability Sheath still blends against
the freshly painted frame. A changed destination cell does not change those
source color/depth planes, as checked against the existing native controls.

The new GSE test freezes the12 independent source-derived keyboard vectors,
plus hover/release, initial-A0 and signed-row boundaries. The live-widget test
covers dispatch, fallback, paging, retained text nodes and initial pointer hover.
Both scripts and the native34-frame component group pass under ASan/UBSan.
Targeted correctness analysis is clean for the changed Surface translation unit.
The three complete-base-screen regressions also pass after the shared DOM double
learned scrollbar construction in the parent weapon-picker change.

Actual SDL delivery exposed two harness assumptions before final verification:
Window-level mouse callbacks were blocked, and opening a dialog delivered hover
at the pointer, changing the selected ordinal before the first key. The corrected
canvas handler and a Home-normalized final sequence verify the actual highlighted
ordinals1/14 and1 for weapon/armor, Colony model appearance before/after Apply,
unknown-release fallback and Done. The instrumented sessions exit cleanly; the
final screenshot sequence uses the cached ordinary binary and exits0 without
tracked OpenGL leaks. Excluded captures are not evidence of Colony acceptance.
Software GL/no-audio capture does not measure native vsync/audio performance.

Full original parent decorations, scrollbar chrome/repeat timing and whole-screen
pixel parity remain outstanding. The source-derived state vectors are not new
native OS input executions. A full original parent framebuffer is the next visual
reference target.
