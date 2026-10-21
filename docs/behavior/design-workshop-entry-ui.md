# Workshop entry UI, no-Base opening and picker action translation

Use a shared **owner-based Workshop entry**, with an optional retained-design
hint. Global U/HQ passes`(focusedOwner,-1)` and does not require a Base.
The production picker passes its actual Base owner and highlighted design.
Do not allocate a hidden Base or turn every negative hint into forced New.
[22 focused native controls](design-workshop-entry-ui-cases.json) supplement
the existing`449630` selection/open contracts.

**Correction to the earlier [open-callers handoff](design-workshop-open-callers.md):
the visible Workshop button's raw ID1 becomes final picker action2.** The earlier
document omitted`4a3860` between button creation and Base dispatch. Its description
of the final action1 branch remains valid, but that branch belongs to **Help**,
not the visible Workshop button. This additive correction supersedes that binding;
the earlier frozen evidence is unchanged.

Pinned original executable SHA256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

## Global player route and availability

Installed`menu.txt`, section`#HQ`, contains **Design &Workshop...|U**.
The displayed route is **HQ → Design Workshop...**, shortcut **U**. F4 is
**Base Operations Status...**, not Workshop. The ampersand marks the Workshop
menu item's W mnemonic; it is separate from the global U shortcut.

`46c370` creates this HQ item, command`0x103`, at`46c521..46c530`. It has no
Base-count or selected-Base predicate. Menu items are created enabled
(`6118b0@611b18` stores item flag1); ordinary refresh`46d690` has no Workshop
command103 enable/disable operation and does not test Base count/current Base
for it. Zero Bases does not disable this item.

The command routes are:

| Input | Source | Workshop request |
| --- | --- | --- |
|Ordinary U|character table`530740/53070c`, character−`0x2b`; U target`52c348`|map-view owner field at`[ebp-1c0]`,−1|
|HQ Design Workshop command103|`530e05..530e11 → 5310a2`|focused owner`9430d4`,−1|
|Shared wrapper|`59abb0 → 449630`, editor`73e238`|forwards the two arguments unchanged|

These are active-game commands, not a title-screen command. Common dispatch
requires game-ready`944608 != 0` and command guard`9ba554 == 0`. The U branch
also rejects the common restricted-network state computed at`52bc76..52bcac`:
`949308 != 0` with any of`944600/9492dc/949260` nonzero. The menu dispatcher
has its corresponding network/busy command-group gate at`530ccd..530d25`;
command103 is group1 and does not take the busy-state exceptions for groups0/7/8.
These are general input/command gates, not a requirement to own a Base or Unit.
Use the normal unmodified U route; Shift+U and Ctrl+U are separate Unit commands.

## Normal production-picker controls

The normal production picker is`4a3990` with mode`+b68 = 0`. Its ordinary row
contains **Cancel**, **OK**, **Workshop**, **Help**, plus a conditional Hurry
control. The new constructor fragment executes the normal human-Base branch
without Hurry, observing the four native label/button arguments. Source strings
are installed LABELS102/101/621/103 respectively.

`621620` stores the raw button ID at Button`+a78`; activation`621e90@621f43`
forwards that ID unchanged to the parent's virtual`+b0`. For picker vtable
`68c948`, that method is **`4a3860`**. It writes a different result at
picker`+a778`, the field read by the Base caller as`BaseWindow+b194`.

| Visible input | Raw callback ID | Final picker action | Meaning at Base caller |
| --- | ---: | ---: | --- |
|Workshop button|1|**2**|Redesign/open Workshop|
|Help button or context Help|0|**1**|Help for highlighted candidate|
|OK or Enter|−1|0|Accept highlighted production|
|Cancel or Escape|−2|0, selection cleared to−1|Cancel production selection|
|Programmatic New callback|3|3|New Design branch; no visible normal-picker control established|

Help requires a selection other than−1 before closing/dispatching; Workshop
raw1 and programmatic New raw3 do not have that guard. Native complete button
activation → picker callback controls cover both selected2 and selected−1.

Generic key delivery`607910` translates Enter13 (also encoded`0x1000d`) to
raw−1 and Escape27 to raw−2, then invokes the picker key method. The picker
key method`4a3510` handles arrows, Home/End, PageUp/PageDown and navigation
aliases. **R and N do not invoke Redesign/New** in this normal picker; two
complete native key chains preserve its action field. Single-click selection
is`4a3460`; activation/double-click path`4a34d0` first selects the pointed item,
then dispatches raw−1 in normal mode. A focused native activation control
confirms action0 with the existing highlight; its point-selection boundary
is supplied, not a full OS hit-test claim.

## Actual Workshop arguments and New semantics

After`4a3990` returns, highlighted result is retained in EDI. The Base caller
`41cc10@41d550..41d560` dispatches **final** action, not raw button ID:

| Final action | Branch | Arguments / result |
| --- | --- | --- |
|2, from visible Workshop|`41dbb4..41dbd0`|`449630(Base.owner, highlighted >= 0 ? highlighted : -1)`|
|3, New Design|`41dc52 → 41dbc0 → 41dbd0`|`449630(Base.owner,-1)`|
|1, Help|`41db3c..41dbaf`|Unit0..511: open Workshop for that raw ID; negative/encoded facility: Datalinks help;9999: rebuild chooser|

For example, highlighting an ordinary facility encoded as a negative target
and clicking **Workshop** goes through action2 and passes−1. Clicking **Help**
on the same facility goes through action1 and opens its Datalinks entry.
For a highlighted Unit, Workshop passes that highlighted raw design; it does
not reread the Base's current production target. Valid ordinary candidate
encodings and the earlier action1 help branches remain as documented.

The normal picker does **not** construct a separate Redesign or New Design
button. The names exist in the label catalog, and final branches2/3 exist in
the Base dispatcher, but the new normal-row/context/key audit establishes no
player-visible New Design route in that picker. Treat an added port New button
as a deliberate UI extension using the source-backed action3 request, not as
an original R/N shortcut or an observed original context-menu item.

Every−1 request still uses the established
[owner-bank selection](design-workshop-fresh-draft.md): first inactive slot;
otherwise last excluded active slot; otherwise first owner-bank record.
An active full-bank fallback follows active reopen through`4490b0`, not forced
New. Three new no-Base prefix controls select64 for owner1,192 for owner3,
and64 for an owner1 full active bank. They preserve the existing resolver.

## Production context menu

Context builder`4a3150` selects the pointed cell, resolves the actual highlighted
candidate through its list node, then constructs these command IDs:

| Menu command | Text | Availability / native action |
| --- | --- | --- |
|5|Set production|normal mode only; dispatch raw−1 → accept action0|
|4|Add to queue|included; dispatch raw5 → final action5|
|1|Obsolete|candidate >=0; for intrinsic IDs<64, weapon power must be nonnegative; owner-exclusion update, repaint picker|
|3|Cancel all obsolete|only`949308 == 0`; clears this Base owner's exclusion bit on active designs, repaints|
|2|Help|included; dispatch raw0 → final action1, subject to selection guard|

There is no Redesign/New item in this menu. The three native context-builder
controls retain the actual command/text order for selected Scout2, a negative
facility, and the network-flag case. They stop at menu presentation, with menu
primitives and point selection supplied; no desktop context-menu screenshot
or click delivery is claimed.

The Obsolete list node must advertise a Unit candidate via its stored flag20h;
otherwise the helper uses−1 and omits that action. This is not a generic action
on every facility row. The context-menu command numbers above are distinct
from raw button IDs and final production-picker actions.

## Base authority and close/return behavior

The owner-based entry has **no Base argument**. It stores owner at editor
`+141f4`, selected design at`+141f8`, refreshes owner capabilities via`577140`,
and computes mineral multiplier through **`4f9d60(owner,1,-1)`** at`44970e`.
The third argument is explicitly no Base; the value stored at`+14214` is a cost
multiplier, not a disguised Base ID. The mineral path uses owner industry,
rules/difficulty/map settings rather than substituting a focused Base.

New bounded controls execute wrapper`59abb0` and the original entry prefix
through`44972f`, with Base count`959fd4 = 0`, current Base pointer`918720 = 0`,
and all512 potential Base records filled with canaries. Owners1/3 and a full-bank
case reach their expected selection and multiplier10 with **zero Base-pool
reads/writes**. The capability-refresh and bitmap-construction boundaries are
supplied in these controls; the earlier capability/selection contracts remain
their authority.

A separate admitted normal close tail`44990d..449979` executes window/timer
cleanup with those Base canaries preserved, current Base pointer still0,
and return0. This is a prefix plus close-tail control, **not** a complete
interactive449630 modal lifecycle. Source entry/close paths contain no Base
allocation or fallback selection. Opening/closing alone is not a production
queue transaction or owner production-reference recount. Actual edits, design
retirement and economic Upgrade retain their separate existing contracts.

The **production caller** has additional, real Base authority because it is
already inside a Base screen. After Workshop action2/3 returns it explicitly
reselects the same Base ID with`4f92a0`, which writes`6aae18` and`918720`, then
calls cached Base recomputation`501570(0)` and rebuilds the chooser. The ordinary
refresh path at`41d324` does likewise. That refresh is not an automatic queue
replacement with the saved design. The chooser's existing queue detach/attach
and acceptance rules remain in the
[production-reference contract](production-design-reference-cache.md).
Do not move that Base-specific refresh into the global owner-based editor API.

## Evidence

`tmp/reference/probe_workshop_entry_ui_20261021.py` supplies22 focused native
logic controls: ten complete button callback chains, Enter/Escape, two unbound
letter-key chains, one activation callback, one button-row fragment, three
context-builder fragments, and three no-Base prefix/close-tail controls. UI
presentation/window/timer boundaries are explicitly supplied. No broad redraw,
model matrix, game boot or desktop input run is added.

Private additive packet:
`tmp/reference/independent-workshop-entry-ui-evidence-20261021/`.
It preserves the new receipt, probe, source windows and installed input-text
snapshots. Public fixtures contain semantic controls, IDs, labels, source hashes
and addresses; production files and prior freezes are unchanged.

Manifest SHA256: `4b62a341c6bf2186fbfb64eb512804bfc8279e06dc28188e66fe8b8146c8909f`; all 32 payloads rehashed successfully.
