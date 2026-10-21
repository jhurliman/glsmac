# Workshop New cards and inactive-slot draft initialization

2026-10-20. Original `terran.exe`, SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This closes the inactive-selection gap in
[custom editing](design-workshop-custom-edit.md). Active-record reopening,
allocation and Apply/Done retain their existing contracts.

Private additive evidence:
`tmp/reference/independent-workshop-fresh-draft-evidence-20261020/`, manifest
`8aa6446f3743ece75820d38b20a5ae853ef9fd548001f6c882922f5c79e1e8d6`.
Ten complete original selection controls and four bounded browse-arrow controls
pass. One selection includes the ordinary negative-open scan, one includes
rename cancellation, and one includes editing then reselection. No desktop,
saved-commit matrix, active-reopen matrix or production code was run/changed.

## The selector's second argument matters

`4490b0(requestedId, initializeFresh)` has **two inactive-record paths**.
The argument called refresh in older notes is not merely a repaint request.
Normal Workshop opening calls it with 1. Card selection, Apply reselection,
post-retirement reselection and post-obsolete reselection call it with 0.

| Selected record inactive | `initializeFresh == 0` | `initializeFresh != 0` |
| --- | --- | --- |
| Chassis / weapon / armor | Set all three to 0 | Technology-based provisional defaults, then recommendation controller |
| Reactor | Set to 1 | Highest available reactor from `5958d0(owner)` |
| Ability mask | Set to 0 | Begin at 0, then recommendation controller |
| Ability selector indices | Both -1 | First two set bits, ascending among indices 0..23 |
| Draft name | **Retain existing draft bytes** | Clear to empty string |
| Selected raw flags | **Preserve all bits** | Clear only `0x80` after controller |
| Draft field `+14210` | Set to 0 | Set to 0 |
| Mineral multiplier `+14214` | Preserve | Preserve |

For stock component rows, IDs `0/0/0/1` mean Infantry, Hand Weapons, No Armor
and Fission. Clicking a fresh card therefore does not copy its zero reactor,
stale components or stored name into the draft. It also does not allocate,
activate, rename, clear or build its backing record.

The zero-argument path deliberately preserves the *current draft name*, not
the inactive record's stored name. A prior explicit draft name can survive
selecting another inactive card, while its components reset to the basic tuple.
The selected record's named bit `0x02` also survives. Do not normalize either
away as part of this path. Subsequent Rename and commit use the established
named-bit rules.

The decisive branch is `449355..449369`: weapon and armor are zeroed, then
the **second argument** at `[esp+6c]` is tested. The zero branch at
`4495ba..4495ec` sets the remaining basic fields and returns without touching
the draft name or selected record. It does not pass through the active-record
name normalization code.

## Requested slot versus effective selection

Before either draft path, `4490b0` validates the requested ID. A nonnegative
active record is accepted directly, except an intrinsic record with negative
weapon power. A negative request or inactive requested record instead calls
`5bdb60(owner)` and scans its browse list until the first inactive entry.
That entry becomes the effective selection, even if a different empty card
was clicked. If no inactive list entry exists, use `64 * owner`.

Use the existing browse-list admission/order contract. Inactive own slots sort
by raw ID with key `ID+0x400`, after active included designs and before
excluded designs. Hence the first inactive entry is the lowest inactive own
ID. All inactive slots remain in the list; they are not collapsed into one
synthetic New record.

Controls explicitly verify:

- An all-zero owner-1 bank: request 64 or 127, effective selection 64.
- Active 64 followed by inactive records: request 127 or -1, effective 65.
- An all-zero owner-7 bank: request 511, effective 448.

The retained inactive fixture 65 contains a stored name, stale component tuple,
byte 47 `A5`, prerequisite `1234`, and flags `0282`. Selecting it with argument
0 preserves the entire pool, including `0282`, while setting basic draft
components and retaining `Old Draft`. Its raw record is not a draft template.

### Ordinary open with a negative design argument

`449630(owner, design)` has an earlier owner-bank scan when `design < 0`.
Starting at the owner's first slot, it stops at the first inactive record;
before reaching one it remembers each excluded active record. If the whole
bank is active, the last excluded active record wins; absent one, the owner's
first record is the fallback. It then calls `4490b0(chosenId,1)`.

The bounded open-prefix control enters `449694` with a negative requested ID
and declared owner registers, stops at `4496f7`, and selects 65 for the retained
fixture. The subsequent complete selector clears flag `0x80`, giving `0202`,
and clears the draft name while preserving stored components/name/metadata.
This prefix excludes window construction and the later modal loop. Argument
-2 uses the same selection scan; its separate later modal bypass is source
evidence, not an alternate fresh-default algorithm.

## Full initialization and recommendation handoff

For nonzero `initializeFresh`, `4490b0` first obtains the best reactor from
`5958d0`: test reactor prerequisites from ID 4 downward and use the first
owned/admitted one; fall back to 1.

It then computes provisional components using actual technology admission
`5d17b0(prerequisite,owner)`:

1. Chassis IDs 0..8: require land domain byte `+1 == 0`; maximize unsigned
   speed byte `+0`, starting best score/ID at zero. Equal speed replaces the
   prior choice, so the later ID wins ties.
2. Weapon IDs 0..20: signed power must be below 99 and at least the current
   best score, starting at zero. Later equal-power IDs win. Negative powers
   therefore do not replace the initial choice.
3. Armor IDs 0..9: maximize signed strength byte `+0`, starting at zero;
   later equal-strength IDs win.

These are **provisional**, not necessarily the final default tuple. The caller
clears the draft name, writes weapon/armor/chassis/mask into globals
`9498d8/9498d4/9498dc/9498d0`, sets mode `9498e0=1` and category filter
`9498c8=-1`, then calls the real `597de0(owner)` recommendation controller.

That controller executes its normal design/technology/faction logic and calls
`597390` in recommendation mode. It is not a normal saved-design allocation:
the constructor's mode branches at `5979a8..597a86` publish proposed components
to the recommendation globals instead of building a new record. Exact-match
handling can still clear the matched record's owner exclusion bit; do not
assume this controller is universally read-only. The controls here begin with
no such excluded match and prove unchanged full engine arrays in those states.

After the controller returns, `449527..44955a` copies **weapon, armor, chassis
and mask** back to the draft. It does not copy recommendation reactor global
`9498cc`; the reactor obtained at entry remains. It rebuilds the two ability
selectors from the final mask, clears only selected raw flag `0x80`, resets
recommendation mode to zero, and clears draft `+14210`.

The full controller is frozen with opcode/address provenance; the private
receipts retain all ordered constructor requests and before/after controller
globals. This is an exact caller contract and native oracle for the profiles
below, not an independent replacement for all of `597de0`'s strategic policies.
Keep that controller boundary explicit when extending the port beyond existing
recommendation coverage.

| Full initialization fixture | Provisional ch/we/ar/mask | Final ch/we/ar/mask/reactor | Selectors |
| --- | --- | --- | --- |
| No technology, owner 1 | 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 / 1 | -1, -1 |
| Every technology, owner 1 | 2 / 11 / 8 / 0 | 0 / 1 / 8 / `0200` / 4 | 9, -1 |
| No technology, owner 7 | 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 / 1 | -1, -1 |

Every-technology initialization submits 17 constructor proposals; its first
is `(owner=1,ch=0,we=1,ar=8,mask=0200,re=4,category=2,name=null)`. The final
default is not the provisional strongest land-chassis/weapon tuple. Do not use the
three-loop result as a substitute for the recommendation handoff.

## Reselection, Rename and browse actions

The edited-reselection control opens an inactive slot with argument 1, changes
armor to 5, mask to `8020` and draft name to `Unsaved Draft`, then reselects
with argument 0. It restores basic components/no abilities/Fission and retains
`Unsaved Draft`; the complete engine arrays stay unchanged.

Fresh initialization of the retained named slot clears its draft name and flag
`0x80`, but leaves named bit 2. Calling Rename and cancelling then clears named
bit 2 through the already documented pre-dialog default-name handling. The
only changed raw byte across selection plus cancellation is selected flags
low byte: `82 -> 00`, high byte `02` retained. No backing name/components are
committed. Cancel alone is not a guarantee that flags were unchanged.

`447d40` paints at most ten browse cards, wrapping indices modulo list count.
For inactive records it displays the New label and the one-based local slot
number `(rawId % 64)+1`, rather than stale stored design text, and registers
the hit rectangle with the raw design ID. Its click callback `447a40` obtains
that ID from `613b00`, ignores negative/no-hit results, stores the ID, then
calls `4490b0(id,0)` and requests refresh. This is how an inactive New card
enters the basic path. Hit-test/painting registration is source-traced; these
controls do not synthesize desktop pointer events.

Dispatcher actions 23 and 24 move browse offset `+ce4` and repaint the strip;
they do not select a design or commit the draft. Previous from zero stores
**list count**, not count-1; the painter's modulo makes this equivalent to
offset zero for that frame. Next wraps only when incremented offset equals
list count. Four bounded action-body controls verify both edges and ordinary
steps while comparing the complete 82,496-byte editor region: only `+ce4`
changes. Painter calls are supplied offscreen services.

There is no separate New allocator among ordinary dispatcher actions 0..8:
0 retires, 1 applies, 2 is debug prototype toggle, 3 debug reset, 4 Done,
5 Rename, 6 obsolete/reactivate, 7 upgrade and 8 Cancel. The private dispatch
table records the exact action-to-address mapping. Saved allocation remains
the existing commit operation after editing a New draft.

## Decisive instruction provenance

Private `0x*.asm.txt` files include opcode bytes and instruction addresses.

| Behavior | Instructions |
| --- | --- |
| Request validation and inactive fallback | `4490b0`: `4490bf..449151` |
| Initial reactor | `44915d..44916a`; helper `5958d0` |
| Fresh argument branch/basic reset | `449355..449369`, `4495ba..4495ec` |
| Provisional chassis/weapon/armor loops | `44936f..4494aa` |
| Empty name and recommendation setup | `4494b0..449522` |
| Recommendation readback / selectors / flags | `449527..4495aa` |
| Ordinary open owner-bank scan | `449630`: `449692..4496e9` |
| Open invokes full initialization | `449727..44972f` |
| Card New label / local number | `447d40`: `448014..448148` |
| Raw-ID hit rectangle | `447fc4..447fcd`, `4486fc..448705` |
| Click invokes basic selection | `447a40`: `447a6f..447a96` |
| Arrow bodies | `442600`: `442663..4426af` |
| Dispatch mapping | `442641..44265c`, tables `4436a0/4436d0` |
| Recommendation mode outputs | `597390`: `5973c3..59741d`, `5979a8..597a86` |
| Full recommendation policy and component helpers | `597de0`, `595910`, `5959c0` |

The freeze also includes `446550`, `5bdb60` and complete selector/open/card/
dispatcher functions. Ordinary active selection and saved commit were inspected
as caller context; their previously frozen native matrices were not repeated.
