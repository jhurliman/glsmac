# Original SMAC Design Workshop: bounded first path

Evidence date: 2026-10-20. Original purchased `terran.exe` SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This is the stock SMAC executable, not SMAX or PRACX.

The first implementable path is: open Workshop for faction 1, select built-in
Scout Patrol (design 2), choose Synthmetal shielding after Industrial Base,
keep Infantry / Hand Weapons / Fission / no abilities, Apply or Done, then
offer the saved faction design through the normal Base production list.
The existing component-cost and design-builder implementations should be
connected to this editor; a separate UI-only design store would diverge.

## Evidence and limits

Frozen private evidence: `tmp/reference/independent-workshop-first-path-evidence-20261020/`.
Manifest SHA-256:
`5547a09a357a54b4afce000e46f28f5f5af375472d5c017732522ee217ab5034`.
It contains indexed source disassembly/hashes, owned asset hashes, actual x86
constructor/drawing observations, four complete commit calls, and 43 native
picker-card admission results. Purchased pixels, loaded text, model provenance,
and native design-name bytes stay in this private directory, outside Git.

These are **offscreen emulator rasters**, not screenshots of a running desktop
game. The original x86 loads the installed PCX/CVR assets, initializes the CVR
renderer through `6326e0`, loads models through `45cb80`, constructs the Workshop,
selects Scout, and executes its painter. Windows GDI supplies actual font
rasterization into indexed guest canvases. File/memory/RECT/INI services are
platform adapters. Timers, cursor, HWND, profile writes, window enumeration and
message delivery are bounded/inert services documented in the receipt. The
emulated renderer uses the installed `ForceOldVoxelAlgorithm` preference.

The latest capture has a visible Scout model and COST 10. Earlier development
images without models or with COST 0 are invalid references. The zero was a
harness omission of the Workshop-open mineral multiplier, not a game rule.
Only the files named by the frozen manifest are current evidence.

Four picker constructors stop at `43d433`, immediately before their modal
message loop. The harness's expected early-stop assertion is caught only at
that exact PC. Actual card drawing/admission routines execute afterward.
Apply/Done/Cancel dispatcher behavior and the Base chooser linkage below are
source-traced; complete desktop input/close/reopen and Base chooser rendering
have not been exercised.

## Opening, assets, geometry, text

Main instance: `73e238`; entry `449630(owner, design)`. Constructor `4469a0`.
The entry initializes/reuses the window, refreshes faction capabilities through
`577140`, computes `4f9d60(owner, 1, -1)`, and selects through `4490b0`.
That computed value is **the mineral cost multiplier** at editor `+14214`,
not a Base index. A normal human fixture gives 10.

`deswin.pcx` is the original 623×368 client artwork, loaded at `446db4` through
`5f0be0`. The constructor's client width comes from this image; height is
`600 - [7cc53c]`. The admitted console profile has `[7cc53c]=213`, giving
623×387. The main screen resolution was supplied as 800×600 for the final
capture. This is a 4:3 host profile, not evidence that all Workshop widgets
belong to a single 623×387 logical screen. Shared console geometry positions
other controls, and the outer parent/frame is not naturally booted here.
Do not scale the entire screenshot to fit the client image or infer absolute
screen/button positions from this partial parent graph.

Original component rectangles in client coordinates (right/bottom exclusive):

| Element | Rectangle |
| --- | --- |
| Main preview | `(140,9)-(451,320)` |
| Chassis | `(30,7)-(129,92)` |
| Weapon | `(30,101)-(129,186)` |
| Shielding | `(30,195)-(129,280)` |
| Reactor | `(462,7)-(561,92)` |
| First ability | `(462,101)-(561,186)` |
| Second ability | `(462,195)-(561,280)` |

The original painter/controls add frames, captions, values and text. Other
bootstrap inputs include `palette.pcx`, `interface.pcx`, `text.pcx`, installed
labels, and SCRIPT sections `DESIGNAUTO` (14 categories), `DESIGNLABELS` (16),
and `DESWIN`. Load these from the user's owned installation rather than
shipping extracted pixels/text. `source-provenance.json` records hashes.

Workshop `630450` calls request the face at global `6affac`: Arial in this
fixture, with sizes 10 normal, 12 bold, and 16 bold; further native font setup
also appears in the receipt. GDI produced heights 13/15/19 and ascents 10/12/15
for those three requests on this machine. These are observed GDI metrics,
not universal substitutes for font-size semantics. The harness privately
registers the installed `arialn.ttf`; that does not mean every Workshop string
uses Arial Narrow. Preserve the requested face and native baseline/clip rules.

## Preview composition, scale, light, animation

Main preview painter `43f8c0` chooses two distinct paths:

* An unchanged design with nonnegative icon, or the qualifying intrinsic
  Infantry weapon path, populates temporary design 63 and calls
  `5d9ca0(63, owner)` to obtain scratch Unit 2048. It then calls the ordinary
  unit wrapper `56fd50` with scale argument 14 and flags `0x44`. Nominal
  centering uses 187×140, with the executable's optional `6b0114/6b0118`
  adjustments. **Correction from the follow-up call-site audit:** the frozen
  stock Scout main preview does not take this path. Its `5d9ca0` calls return
  to `4481b0` in the console design strip, not `43fa2e` in the main preview.
  Its main-preview scale call returns to `43faff`, proving the scene path.
* Edited combinations on the other path use the Workshop scene at `+1279c`.
  `4466c0` assembles chassis, weapon, armor, reactor and abilities through the
  original `4d88e0/4d8e40/4d9130/4d95f0/4d8700` helpers. It sets rotation with
  `6324b0`, increments `+12770` by float `0.05` **per painter invocation**,
  sets scale to float bits `0x3e0f5c29` (~0.14), and draws `631370` flags
  `0x17`. The constructor's scale ~0.18 is superseded. Do not reuse the map
  preview profile for this scene. Armor 5 has an additional drawing branch.

`6324b0` is Euler rotation-to-matrix, not lighting. `6324f0` stores scene scale
at `+d5`. Constructor light setup forms a matrix with input angles
`(0,0,0x3f48f5c3)` (~0.785 radians), negates the resulting final vector, scales
it by constant `68cab0 = -10000.0`, and passes vector/matrix to `6323e0`, which
copies them to scene `+a5..+d1`. Source instructions are frozen for exact
operation order. Small panel/picker models have independent scale choices:
the first main chassis/weapon/reactor calls use ~0.05/~0.15/~0.20 respectively;
chassis picker uses ~0.06, reactor picker ~0.20, and weapon picker varies by
weapon mode. A universal preview scale is incorrect.

The Workshop entry installs callback `4468a0` with interval `0x28` through
`62fe60`. The offscreen evidence covers a bounded paint, not real-time
animation cadence, timing jitter or whole-screen compositing.

## Component picker legality and cancellation

Click handlers: chassis `443fe0`, weapon `444400`, armor `444860`, reactor
`444b50`; common picker `43cfc0`. Kinds 0/1/2/3 mean those four categories.
Rows with prerequisite `< -1` are omitted entirely. Other unknown-tech rows
remain visible but return not selectable; this is not a list filtered down
to owned technology. Admission uses original `5d17b0(prerequisite, owner)`.
Do not confuse displayed ordinal with component ID after disabled-row removal.
Reactor ordinals map to IDs starting at 1.

For owner 1 with only Industrial Base added, the 43 native visible-card results
are: chassis 9 rows, only ordinal 0 selectable; weapon 20 rows, ordinals 0 and
14 selectable (Hand Weapons and Colony Module); armor 10 rows, ordinals 0 and
1 selectable; reactor 4 rows, only ordinal 0 selectable (Fission). Thus the
simple Synthmetal edit is legal. Speeder is not legal in this fixture; the
separate raw commit control for a Rover does **not** demonstrate its picker
eligibility without Doctrine: Mobility.

`43cda0` lays out 108-pixel column and 94-pixel row steps. Before the 3-pixel
inset, a cell is `(27+108*c,4+94*r)-(132+108*c,95+94*r)`. This profile admits
5 columns × 4 rows per page. Per-kind routines are `43b8d0/43bc40/43c100/43c510`;
their return value is the selectable result. Unknown components render a
disabled silhouette in the armor capture. Input updates `+4648` from that
result. Mouse accept with false validity resets selection to -1;
keyboard Escape also sets -1. Final return uses the original component when
selection is negative or validity false; valid reactor selection adds 1.

There are further chassis/weapon compatibility rules, beyond prerequisites.
For example `444400` rejects inappropriate missile/payload combinations for
the current chassis and runs `443d20` after mutation; armor selection has
a special missile-chassis branch. Implementing only the technology predicate
for all advanced designs is not sufficient. This first slice stays on legal
Infantry/Hand Weapons/Synthmetal/Fission/no abilities.

Ability picker `444e20` always starts with the no-ability choice. It gates the
second slot on tech `[9ba470]`, rejects missile chassis, then checks each of
24 abilities with both `5d17b0` and `59bd40(ability,chassis,weapon,armor)`, and
excludes a duplicate from the other slot. Advanced combinations remain a
separate slice; do not advertise two unrestricted ability slots.

## Draft, commit, allocation and names

Editor offsets: owner `141f4`, selected design `141f8`, chassis `141fc`, weapon
`14200`, armor `14204`, ability mask `14208`, reactor `1420c`, mineral multiplier
`14214`, two selected ability IDs `14218/1421c`, draft name `14220..1423f`.
`4490b0` copies a design to the draft and initializes missing ability slots
to -1, taking set bits in ascending order. Existing selection/refresh helpers
can mutate bookkeeping flags; Cancel is not a snapshot rollback of all engine
memory, and preview drawing also uses temporary design 63 / scratch Unit 2048.

Commit `441e20` first scans active built-ins 0..63, then the owner's 64 design
slots. Matching requires equal chassis/weapon/armor/reactor/ability mask;
built-in matches additionally require their prerequisite technology. A match
is reused, with owner obsolescence bookkeeping updated. Built-in names remain
intrinsic; attempting a different name takes the intrinsic-name warning path.
Do not allocate a duplicate merely because the draft name differs.

For a changed built-in with no matching tuple, the owner-slot scan prefers
the first eligible obsolete/reusable slot whose faction unit and production
counts are both zero; otherwise each inactive slot updates the candidate, so
an entirely empty bank selects the **last** slot. Owner 1 therefore selects
127, not 64. Exhaustion that leaves a built-in selected fails with an intrinsic
type warning. Editing an existing custom design has a more involved reuse/
overwrite scan and may prompt when live units exist; it is not equivalent to
the simpler first-empty-bank case and remains outside the executed controls.

The write path invokes `5bd4f0(design,chassis,weapon,armor,mask,reactor)`;
clears flag byte `+30` bits `0xc0`; derives an automatic name through `594320`
when draft name is empty; terminates that automatic result at byte 31; copies
the name into the 52-byte design record; invokes `5aaa00`, then
`595db0(owner,design,1)`. Custom-name bit 2 distinguishes a supplied name
different from the automatic name. Use the shared installed-language naming
code/tables, including faction-specific grammar already researched for startup.
Do not assume arbitrary injected names are length-safe: the executed named
control uses a short legal input and bypasses no naming-input validation claim.

Executed controls, restoring design/faction buffers between each:

| Draft from Scout | Result | Design | Raw rows | Production rows | Own/foreign eligibility |
| --- | --- | --- | --- | --- | --- |
| Unchanged | success | 2 reused | 1 | 1 | 1 / 1 (built-in) |
| Synthmetal, automatic name | success | 127 | 2 | 3 | 1 / 0 |
| Synthmetal, short supplied name | success | 127 | 2 | 3 | 1 / 0 |
| Speeder, no picker tech check | success | 127 | 2 | 3 | 1 / 0 |

The last control shows that the commit boundary itself does not replace the
picker's prerequisite enforcement. The UI must validate component choices.

## Apply, Done, Cancel and production integration

Native `621620` creation receipts identify action IDs, resolving captions:

| Action | ID | Original dispatcher `442600` |
| --- | --- | --- |
| Apply | 1 | Calls `441e20`, stores result in `+cdc`, reselects with refresh false, rebuilds Workshop list, stays open. |
| Done | 4 (also -1) | Calls commit, stores result, closes only on nonzero success. |
| Cancel | 8 (also -2) | Stores zero result and closes, without calling commit. |

Cancel after a successful Apply does not undo the previously committed design.
Cancelling an individual component picker returns its original choice and
does not apply a newly hovered invalid component.

`5bd1e0` is the shared raw component cost. Workshop normal display multiplies
raw rows (with its reactor minimum handling) by editor mineral multiplier;
Scout therefore displays 10. It has a separate prototype surcharge display
path using `5d9790`. Base production uses `5d9810`, not the Workshop label as
an authoritative price: the native Synthmetal control is 2 raw rows and 3
production rows while unprototyped. Difficulty/faction prototype exemptions,
land/sea/air percentages, and the Base facility exemption are read by that
routine. A Base-aware call can waive surcharge; do not hard-code 50% for every
faction, difficulty and Base. The 3-row result above uses no Base (`-1`).

The Base chooser builds its sorted candidate array with `5bda00(owner)`, then
`4a1f50@4a20aa` calls `5d1eb0(design,owner,currentBase)` before presenting each
candidate and reads its name from the actual design record. This predicate
rejects inactive records, the owner's obsolete-mask bit at record `+2c`,
unavailable built-in prerequisite tech, the restricted plan/rules combination,
and a sea chassis at a Base without the required ocean access. Custom designs
must belong to that faction's bank (`design / 64 == owner`). A nonnegative
Base argument matters; passing -1 bypasses the sea/Base check.

Base production UI `41cc10@41dbd0` opens `449630` with the Base owner and
selected design, then refreshes Base state and rebuilds `4a1f50` after return.
Saving is not itself a command to replace the Base queue: expose the new
design as a production choice, then let the production selection path assign
it. Persist normal design records, invalidate/rebuild dependent lists, and
keep Apply/Done/Cancel behavior separate from queue selection.

## Exact remaining live captures

Native desktop launch could not run through the available Windows automation
session (`GetCursorPos` access denied). This did not block executable tracing
or the genuine offscreen model/card rasters. Still needed for a whole-screen
pixel-fidelity check are:

1. Stock 800×600 Workshop with Scout Patrol selected, including outer frame,
   bottom controls, design strip and console/background.
2. Shielding picker over that screen, with the legal Synthmetal choice selected
   and unknown-tech cards visible; a second view immediately after cancelling.
3. Workshop after applying the legal Synthmetal edit, then the Base production
   chooser showing that saved design and its prototype cost.

Advanced reactors, two abilities, presets, rename, obsolete/reactivate, retire,
redesign, upgrade-all and SMAX need their own source/control slices. This
handoff supplies the first playable path without claiming those are complete.
