# Workshop open arguments: production picker and global command

> Correction (2026-10-21): the visible Workshop button's raw ID1 is translated
> by4a3860 into final picker action2. The action1 branch below is Help.
> [The complete callback/input handoff](design-workshop-entry-ui.md) supersedes
> the earlier literal-button binding; the frozen earlier source packet is unchanged.

Use the highlighted production-picker unit's raw ID for its literal
**Workshop** button; use **-1** for the ordinary global Workshop command.
The captured parent request `(1,2)` was a declared active-Scout fixture,
not evidence of either caller's universal default.

| Caller | Actual `449630(owner, design)` argument | Source |
| --- | --- | --- |
| Production picker **Workshop**, action1, highlighted unit0..511 | Base owner; highlighted raw unit-design ID unchanged | `41db7d..41db8f` |
| Picker Redesign action2 | Base owner; highlighted picker result if nonnegative, otherwise-1 | `41dbb4..41dbd0` |
| Picker New Design action3 | Base owner;-1 | `41dc52 → 41dbc0 → 41dbd0` |
| Global Design Workshop menu command`0x103` | Focused owner`9430d4`;-1 | `5310a2..5310aa → 59abb0@59abbf` |
| Global U shortcut | Map-view owner field at caller`[ebp-1c0]`;-1 | `52c352..52c35b → 59abb0@59abbf` |
| Tutorial/help Workshop entry | Focused owner;-2 | `4d1127..4d1134`; separate caller |

## The literal production-picker button is action1

Picker constructor`4a3990@4a41c7` loads LABELS offset`0x9b4`
(zero-based index621), **Workshop**, then constructs its button with
action1 at`4a4221..4a422e`. Do not bind it to the similarly nearby
Redesign/New branches merely because those also open Workshop.

The chooser result from`41d45e → 4a3990` is retained in EDI at`41d463`.
The caller dispatches picker action through`41ee2c` at`41d560`:
action1→`41db3c`, action2→`41dbb4`, action3→`41dc52`.
This makes the argument the **highlighted chooser candidate**, not an
unconditional reread of the Base's current production target`Base+50`.
For example, highlighting Scout2 while another unit is currently in
production opens Scout2.

Action1 has additional guards: a negative facility target opens Datalinks
via`458be0`, rather than creating a New draft. Sentinel9999 returns to the
picker. Nonnegative candidates below512 enter Workshop unchanged. Encoded
facility candidates512+ use the corresponding facility-help branch, not
an out-of-range design open (`41db99..41dbaf`). Thus action1 is not
"negative→-1"; that normalization belongs to action2.

After Workshop returns, the production caller refreshes Base state and
rebuilds the picker. Opening/saving a design does not by itself replace the
Base queue; retain the [existing production caller contract](design-workshop-first-path.md).

## Global shortcut and negative-selection semantics

Installed MENU `#HQ` labels **Design Workshop...|U** and **Base Operations
Status...|F4**. The original global Workshop shortcut is **U, not F4**.
The decoded character table`530740/53070c` routes lowercase`u` to`52c348`,
whose admitted path passes-1. The global menu dispatcher separately
routes command`0x103` to`5310a2`, also passing-1. Wrapper`59abb0` forwards
both arguments unchanged to the main Workshop actor`73e238`.
If the port deliberately uses F4 for Workshop, its corresponding global
Workshop request can still use-1; that key assignment is a port UI choice.

A negative design request is the existing **owner-bank scan**, not an
unconditional forced-New result: first inactive slot wins; with a full bank,
use the last excluded active slot, or the owner's first record if none.
Then`4490b0(chosenId,1)` takes its active/inactive branch. See
[negative-open selection](design-workshop-fresh-draft.md). Consequently
global-1 usually reaches fresh recommendations when a free slot exists,
but a full-bank active fallback must retain active-reopen behavior.

The tutorial/help-2 request uses the same negative-selection scan but has
its separate later modal bypass. Do not use that tutorial argument for the
ordinary global command.

Source-only caller handoff: no new native screen, key-delivery, or model
matrix. Additive private source packet:
`tmp/reference/independent-workshop-open-callers-evidence-20261021/`.

Manifest SHA256:
`bd9c4f7b3d0bb10f1ba2e7ca36dc6227dece0ffb8143c3a56266b354fb0c8913`.
