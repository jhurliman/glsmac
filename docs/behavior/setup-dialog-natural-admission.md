# Stock setup dialog mode and console admission

For the **normal cold-launch local single-player route**, accepted Standard Rules,
MAPMENU “THE MAP OF PLANET”, faction selection, leader name and first-base name:

| Caller | Effective mode | Console at dialog show | Previously measured layout case | Outer rect at1024×768 |
|---|---|---|---|---|
| FACTION (`5a16b4`) | 0 | Hidden | `faction-mode0-plain` | `[472,545,1019,763)` |
| NAME/NAME2 (`64110a`) | 0 | Hidden | `name-mode0-plain` | `[337,649,1019,763)` |
| FIRSTBASE (`5c622c`) | 0 | Visible | `firstbase-mode0-console` | `[238,255,818,555)` |

This resolves the geometry-column choice by **static caller/state propagation**
against the pinned executable, rather than a new naturally booted capture.
The earlier bounded layout observations remain unchanged. NAME2 shares this
mode/visibility decision, but its female label/edit inset was not separately
measured in that bundle. The NAME outer rectangle above is the measured NAME
control, not a new NAME2 execution claim.

## Mode and view provenance

`59f880` ends by calling `59f240` at `59fd95`. That routine zeros ESI
(`59f252`) and writes zero to frame-mode global `6c718c` at `59f259`.
The mode global and view-stack index `7f0850` begin in PE zero-fill memory.
Console construction clears its entire view-type stack at
`466125..46612f`:30 DWORDs starting at `7cb6d8` become zero, and its root
descriptor is installed at `7cb6a8` (`466147`). Thus the cold route starts at
view index0/type0, with mode global0.

The selected stock setup chain uses ordinary application/generic dialog windows
and the frontend backdrop. It does not enter any of the major-game-view show
wrappers that push a nonzero view through `469fe0`; its complementary pop helper
is `46a050`. FACTION's preview `5a06d0` is another application dialog, not a view
stack push. NAME's wrapper `641090` likewise uses an application dialog. Showing
a generic window with `606850` does not itself push a game-view type.

At `4085b4..408601`, the application dialog reads the active view type and mode
global. View type0 jumps directly to `4085ea`, retaining mode0. Consequently the
type-driven forced-mode6 branch does not apply to these fresh setup calls.
The existing mode6 profiles remain useful for other contexts; they should not be
chosen for the three calls in this route.

The stock generic-dialog factory is also resolved: application entry `46c1a0`
unconditionally selects factory `408980` at `46c1ec`. That factory allocates an
application dialog, runs `619b70` and `61e660`, then installs vtable `68b638`
(`4089b6..4089d1`), matching the local `408270` objects used in the completed
layout probe. `61da30` creates reusable primary/secondary dialog objects through
that factory at `61db2d/61db44`; `641090` selects one for NAME.

`61afa0`/`61a8c0` clear dialog parent field `+a14` unless flag `0x100000` is
present. FACTION flags `0x40`, NAME wrapper flags `0x44`, FIRSTBASE flags4 all
lack it. The cleanup path also resets `+30ac` (`61a573`) before the new template
and layout selection. Reusable NAME objects therefore do not inherit an earlier
parent or large frame mode in this route.

## Accepted stock map path

TOPMENU START GAME reaches `5a4f31..5a4f39` and calls `5a3ee0(0)`.
That function presents MAPMENU. Its dispatch table at `5a43bc` maps row2 to
`5a4067`, which pushes the pinned string `maps\planet.mp` (`6aee28`) and calls
`5c0b00` at `5a4073`. After accepted load, the common continuation calls
`5a0b80` at `5a4089`.

In local single-player setup, `5a1522..5a153b` loads FACTION with flag `0x40`.
Rows and the preview are prepared, then `5a16b4` runs its modal routine.
Accepted selection reaches the leader-name wrapper at `5a19ad → 5d7880 →
641090`. That wrapper ORs `0x44` into the flags at `6410b8` and calls
`61afa0`/`61b9d0` at `6410cb/64110a`. The preview does not show the game console.
Standard-rule selection changes game rules rather than this dialog-mode global.

## Console visibility and FIRSTBASE

`606850` is **show**, including when its argument is0. It checks creation and
visibility bits, then ORs bit0 into `window+9c` at `60689c..6068a4`. Its argument
controls redraw/placement behavior; it is not a boolean visible/hidden value.
`606b50` is **hide**, clearing bit0 at `606c2f..606c32`.
`610d10` tests bit0 and recursively tests the parent chain.

The console begins hidden: original construction calls `5edd10` at `466933`,
which creates via `604c00`. The latter resets state with `6044c0` (zero at
`604a9a`) and sets the creation bit4 at `605482..605484`, without setting the
visible bit. The executable body of full console construction has no call to
`606850`. Its later child/button creation does not show the root console.
Thus FACTION and NAME during the frontend setup encounter a hidden console.

After setup returns, `53f97d` calls `5a57e0`. Its tail explicitly shows the game
console (`ECX=7cb7c0`, `push0`, `5a58e6 → 606850`) and then the map window at
`5a58fb`. Only afterward does the local route enter `53be70` at `53f9c0`, whose
fresh-intro gate calls `5c5b30` at `53bedc`. Landing animation/polling does not
hide the console or push a major view before FIRSTBASE at `5c622c`.

FIRSTBASE therefore takes the console-visible mode0 show path in `4082f0`:
null `+a14`, visible console, mode0, then reposition and `46a110` attachments.
Its final outer position uses that show path, rather than keeping the template's
initial x10,y40. Console geometry field `7cc53c` is root+`d7c`, assigned213
at `466185`; that is the constructor value used in the earlier measured profile.

**FIRSTBASE has one OK control and no Cancel.** The source caller passes flag4
at `5c61d4`. Cancel construction at `61c77c` requires flag`0x40`, and
`61c783` skips that block when absent. The previously executed FIRSTBASE case
already observes just the action−1 OK control. Remove the extra Cancel from the
port's stock FIRSTBASE UI. This does not change the earlier documented result
transfer/duplicate-name rules.

For a later return to the frontend, `5a5970` tears down gameplay UI and tail-jumps
to console hide at `5a59ac`. That is additional explicit visibility provenance;
this receipt does not claim a complete proof of all view-stack states after an
arbitrary previous game/UI session.

## Evidence boundary

Public machine-readable decisions are in
`setup-dialog-natural-admission-cases.json`. The private pinned-source receipt
contains60 literal instruction checks, relevant complete indexed functions,
PE zero-fill section metadata and the MAPMENU jump-table entries. It links to
the immutable earlier layout manifest
`12c7de6a49f863abc99f5b49df3e29b98ed8dadb4e6cd99239a0a542e36935c6`.
**No new original execution, geometry/glyph measurement, desktop capture or
foreground interaction was performed.**

The mode/parent/console fields above are resolved for the stated accepted
cold-launch local route. Re-entry after an arbitrary prior session, scenario/
editor, multiplayer, load/resume and external view changes remain outside this
proof. The exact view stack during those other routes remains unresolved here.
Default owned assets/font configuration and the earlier geometry probe's service
limits still apply. Original modal input dispatch remains separate evidence work.
