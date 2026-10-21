# Workshop browse, reselect and component-picker transitions

2026-10-20. Original `terran.exe`, SHA-256
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.
This consolidates the interactive contract for connecting the live editor.
It reuses the existing saved-design/commit matrices and adds only missing
distinguishing picker controls.

Private additive freeze:
`tmp/reference/independent-workshop-picker-transitions-evidence-20261020/`,
manifest `99b731728c802634308707bcd522e1705f4519e84c7c0e00452d99a23a53b16d`.
**20 focused native controls pass**: 15 accepted mapped-picker mutation bodies,
three generic post-modal returns and two ability-entry guards. The
[independent transition reference](workshop_picker_transition_reference.py)
matches every byte of all fifteen 76-byte draft regions and their recorded
panel-refresh sequences. Complete engine regions remain unchanged across these
mutation bodies. Existing active-reopen, Apply/Done/Cancel, allocation,
compatibility and fresh-open matrices were not rerun.

Six additional full-refresh controls are frozen separately in
`tmp/reference/independent-workshop-full-refresh-evidence-20261020/`, manifest
`ae27efbc20fe6e389411316e102ce5b7ca87c74a91397c1be29af624013703d0`.
These distinguish selector output from the subsequent full-window cleanup,
and verify the full-refresh entry guards. Total new focused controls: **26**.

New controls enter after the modal choice has been mapped to a component ID,
or at explicitly identified post-modal/guard instructions. Painters and panels
are supplied offscreen boundaries. This is not a full modal-loop or desktop
test. The reference's technology/suggestion fixture is owner 1 with all
technology, optionally without Neural Grafting, and explicit automatic-setting
state. General UI admission still uses the actual owner's technology.

## Saved rows, New rows and reselect

Keep selected raw design, editable draft, ability slots and browse offset as
separate state. The selected raw record is not a backing object to mutate on
each component change.

| Operation | Draft/components | Names and flags |
| --- | --- | --- |
| Select active saved row via `4490b0(id,0)` | Copy stored ch/we/ar/mask/reactor; rebuild slots ascending, first two set bits | Clear raw `80`; named custom record supplies stored draft name, unless equal to automatic name, in which case clear named bit and draft text; intrinsic/unnamed record gives empty draft |
| Click New/inactive row via argument 0 | Choose first inactive browse entry; reset ch/we/ar to 0, reactor 1, mask 0, slots -1/-1 | Preserve current draft name and every selected raw flag |
| Normal open inactive row via argument 1 | First inactive entry; technology defaults and recommendation controller | Clear draft name and selected raw `80`; preserve other raw flags |
| Reselect after component edits | Apply the selected row's active/inactive path above | Discards unsaved components; name behavior follows that path |
| Reselect after retirement | Dispatcher calls argument 0 | New/basic defaults if selection now resolves to inactive; do not run normal-open recommendations here |

Active-row reselect does **not** run ability compatibility cleanup or downgrade
stored components to the owner's current technology. It copies the saved
reactor over the provisional best-reactor query. A saved two-ability mask can
therefore leave the selector with both slots even without Neural Grafting;
the ensuing full-window refresh can immediately clean it, as detailed below.
Custom saved rows are not filtered by the
intrinsic prerequisite field as though they were built-in definitions.

Click callback `447a40` supplies the raw hit ID to `4490b0(id,0)`. Inactive
requested IDs are not guaranteed to remain selected: fallback chooses the
first inactive entry. Browse offset `+ce4` is separate; arrow actions 23/24
scroll/repaint without selecting or committing. See
[fresh selection](design-workshop-fresh-draft.md) and
[browse ordering](design-workshop-custom-edit.md) for exact wrap and sort keys.

Picker mutations preserve owner, selected ID, reactor unless directly changed,
mineral multiplier `+14214`, opaque draft field `+14210`, draft name and all raw
design flags. The new controls poison `+14210=77` and keep `Retained Draft`
through every mutation. They do not allocate or commit a saved record.
Actual painters can use temporary design 63 / scratch Unit 2048, so a complete
UI paint is not promised to leave scratch storage untouched.

## Apply, Done, Cancel and Rename

These rows reuse the 41 complete saved-edit controls; they are not new native
runs in this pass.

| Action | Required transition |
| --- | --- |
| Apply 1 | Commit; store result in `+cdc`; **always** reselect with argument 0 and rebuild browse list, even on failure; remain open |
| Done 4 / -1 | Commit; close only on success; failed overwrite leaves edited draft available |
| Cancel 8 / -2 | Set result zero and close without commit; no rollback of earlier successful Apply or bookkeeping already changed by selection/Rename |
| Rename 5 | Edit draft text and selected named bit before any commit; empty-draft default generation can clear named bit even when dialog is cancelled |
| Obsolete/reactivate 6 | Update raw exclusion/flag overlay; advance through the **old** browse list, reselect with argument 0, then rebuild list |

Use `+cdc` as modal result; dispatcher EAX can be cleanup output. A failed Apply
restores an active selected record's old components, while failed Done retains
the edited draft. If reselection resolves to inactive, its basic/name-retention
path applies instead. Names are not tuple identity: Apply can alias another
record, including an intrinsic design, under the existing naming rules.

Retirement and queue/cache lifetimes remain the
[saved-edit](design-workshop-custom-edit.md),
[retained-registry](design-workshop-retained-registry-and-history.md) and
[production-cache](production-reference-base-lifecycle.md) contracts. Do not
recount unit/production cache rows merely to refresh the UI.

## Picker admission and Cancel are separate from mutation

Generic picker `43cfc0` works with filtered ordinals. Chassis, weapon, armor
and reactor callers enumerate definitions whose prerequisite is >=-1; entries
below -1 are omitted. Actual owner technology controls availability of admitted
cards. Unknown cards can be drawn as unavailable silhouettes. Hover callback
`43c880` stores the card painter's validity result in picker `+4648`.

After the modal loop, negative/no selection or validity zero returns the
**original choice**, not the last hovered value. Valid reactor results convert
zero-based card ordinal to one-based rating; initial reactor highlighting has
the matching decrement at `43d0c4`. Three new bounded return controls verify
Cancel→old 2, unavailable hover 4→old 2, and valid reactor hover 2→rating 3.
Do not use a reactor rating as an unadjusted zero-based widget index.

The caller then runs its normal mutation/cleanup/refresh path even when the
returned choice equals the original. Cancel means no hovered component is
applied; it does **not** guarantee a byte-identical draft. For example, returning
the same Synthmetal armor while Hand Weapons and no abilities are selected
offers Comm Jammer and adds it when technology/setting permit. This same-value
caller path is a new native control; its full modal cancellation is source-
connected through the separately verified old-choice return.

Some legal-looking/known cards are still rejected by caller combination rules.
The weapon caller restores the old weapon when Probe is returned for an air
chassis. Do not replace caller validation with only a technology check.

Armor cards have a separate special-chassis rendering branch:
`43c100@43c125..43c131` forces the rendered armor index to No Armor and bypasses
ordinary ordinal/technology resolution. This is not an extra coercion in the
accepted armor setter: `444932` assigns the mapped armor ID unconditionally.
Keep card presentation, modal result and setter behavior distinct; the new
post-choice controls do not claim a complete missile-armor modal interaction.

## Cleanup and automatic suggestions

`443d20` is the existing checked compatibility cleanup. It does not recheck
each ability's technology prerequisite. It removes incompatible bits, the
specified conflicting pair, excess abilities and a second ability without
Neural Grafting. Removal order is ascending bit index, not UI-slot order.
It rebuilds slots only when it removes a bit; a legal mask preserves existing
slot order and gaps. Missile/special chassis clears mask and both slots.

`443e50(flag)` first calls cleanup, then offers the ability only when:

- Automatic setting `959f98 & 4000` is enabled.
- The owner's actual ability prerequisite is owned and `59bd40` accepts the
  current chassis/weapon/armor.
- At most one ability remains; adding with one present also requires Neural
  Grafting.

An accepted suggestion ORs the bit and rebuilds slots ascending. Suggestions
are conditional additions, not obligatory properties of a selected component.
Reuse [the compatibility reference](design-workshop-general-construction.md)
and its 216 native predicates / eleven cleanup controls.

## Component mutation order

### Chassis: 443fe0, mutation 4440b6..444201

1. Save old domain and set new chassis.
2. On missile/special chassis, set armor 0; retain Planet Buster only on its
   admitted branch, otherwise use Conventional Payload ID 20. Leaving special
   chassis resets Planet Buster/payload to Hand Weapons as the caller requires.
3. If domain changed to a non-land domain, offer Deep Radar bit 1 **before**
   applying air-package restrictions. Air Probe resets to Hand Weapons. Former
   resets on air chassis whose flight/range byte `9b7142+ch*144` is nonzero.
4. On a domain change back to land, explicitly remove Deep Radar and rebuild
   slots when that bit was present.
5. Run final cleanup, then refresh dependent views.

Same-domain Infantry→Speeder retains weapon and armor, and keeps a compatible
reverse slot order `[18,9]`. Entering missile with Laser/Synthmetal clears armor
and abilities and selects payload 20; leaving missile for Speeder resets that
payload to Hand Weapons. New air-from-Hand and air-from-Probe controls both
finish with Hand Weapons plus Deep Radar under the all-tech setting; do not
invent an incompatible-Probe result from the order alone—use native predicates.

### Weapon: 444400, mutation 4444d0..44465d

1. Save old weapon/power, assign requested weapon.
2. Apply air Probe/finite-flight Former restrictions and missile/payload rules;
   disallowed ordinary-chassis payload choices restore the old weapon.
3. Power >=99 clears abilities/slots.
4. Only if mask is **already empty** and effective weapon differs from the old
   one, offer Heavy Transport for transport mode 7; Super Former then Fungicide
   for former mode 9; Blink if offense >= twice armor strength.
5. Run final cleanup.

With empty mask, selecting Former under all technology offers Super Former
then Fungicide, resulting in mask `2001`, slots `[0,13]`. With an existing
compatible Comm Jammer mask, suggestions are skipped and Comm Jammer remains.
With existing incompatible Blink, suggestions are skipped, then cleanup removes
Blink and leaves an empty mask. **Do not clean the mask before the caller's
empty-mask test**, which would incorrectly add Former abilities in that case.
Automatic setting off yields no additions, while cleanup still runs.

### Armor and reactor

Armor `444860@444932..444969` assigns armor; if strength >1 and weapon offense
<=1, it offers Comm Jammer; then cleanup runs. The offer is not guarded by
“armor changed,” so same-value returns can add the ability.

Reactor `444b50@444c23..444c30` assigns the selected rating, then cleanup.
It does not proactively suggest abilities. A new control starts with mask
bits 9/18, reverse slots `[18,9]`, and no Neural Grafting. Cleanup removes bit
9 first, leaving bit 18 and slots `[18,-1]`, despite slot 0 originally containing
18. Changing reactor did not itself invalidate that ability; capacity cleanup
caused the change.

### Manual ability: 444e20

Before showing choices, editing slot 1 requires technology global `[9ba470]`.
Missile/special chassis is rejected. Two bounded native guards verify warning
and no draft/engine mutation. The chooser begins with None, then filters each
ability by technology, `59bd40` compatibility and duplication in the other slot.

Accepted result `4461f2..446234` maps values >=24 to -1, changes only the chosen
slot, and rebuilds mask by OR-ing both nonnegative slots. It does not call
`443d20` or sort/compact slots afterward. Clearing slot 0 from `[9,18]` yields
`[-1,18]`; choosing 18 in slot 0 beside 9 yields `[18,9]`. These are verified.
Active saved-record reselection later sorts bits into slots; manual editing
and reselect therefore have different normalization behavior.

## Refresh ordering after mutation

All following card/panel calls occur **after** mutation and its required final
cleanup; manual ability editing has no final cleanup. New controls record the
draft at each supplied callback, so each consumer observes the final tuple.

| Change | Ordered card/panel requests |
| --- | --- |
| Chassis | Chassis, Armor, Reactor, Weapon, Main, Ability 0, Ability 1, `4487c0`, `43d730` |
| Weapon | Chassis, Armor, Reactor, Main, Weapon, Ability 0, Ability 1, `4487c0`, `43d730` |
| Armor | Armor, Main, Ability 0, Ability 1, `4487c0`, `43d730` |
| Reactor | Reactor, Main, Ability 0, Ability 1, `4487c0`, `43d730` |
| Ability | Ability **1**, Ability **0**, Main, `4487c0`, `43d730` |

Card methods are `43de60/43e850/43ed10/43e2b0/43f8c0/43f120/43f4f0` in the
corresponding roles. Each caller finally requests its window virtual `+fc`
refresh; that final request is source-traced, while the table's callbacks are
recorded in the new controls. `4487c0` also reaches the saved-design strip;
its internal rendering is not another commit or normal-open recommendation.
These refreshes do not run `577140` or recount queues simply because a picker
changed. Owner technology is queried for admission/suggestions; engine technology
acquisition retains its separate producer/capability-update contract.

### Full-window paint is another cleanup boundary

Do not stop the browse contract at `4490b0`'s return. The Workshop vtable at
`68d328` maps `+f8` to `5ee890`, `+30` to full painter `441ba0`, and `+fc` to
`407cb0` (invalidation/presentation request). Browse click `447a40` calls the
selector, then `+f8`.

`5ee890` skips work with no main HWND (`9c0010==0`) or when reentry bit
`object+1a0 & 1` is set. Otherwise it sets that bit and synchronously calls
virtual `+30`, then performs presentation work and clears the bit. Its
hierarchy-visibility check occurs **after** the paint. Three new complete
refresh-helper controls prove no-HWND/busy suppression and an admitted full
paint despite hidden Workshop state; paint/flush are supplied boundaries.

Full painter `441ba0@441cf3..441d49` calls `443d20` **before** drawing Chassis,
Weapon, Armor, Reactor, Ability 0, Ability 1, Main, `43d730`, saved strip and
`4487c0`. Thus an admitted browse refresh can sanitize the copied saved draft.
A new control executes complete saved selection with bits 9/18 and no Neural
Grafting: selector output is mask `40200`, slots `[9,18]`; the actual full-paint
cleanup stage leaves mask `40000`, slots `[18,-1]`. The saved raw record remains
unchanged. Separate controls show legal reverse slots `[18,9]` remain reversed,
and legal single-ability gap `[-1,18]` remains a gap after this cleanup.

This full-paint cleanup is distinct from the timer callback's main-preview-only
`43f8c0` draw. Do not make every preview timer tick perform full compatibility
refresh, or assume a copied saved mask is the final displayed draft. The new
cleanup controls stop before card painting; they establish state transitions,
not a new full-window raster claim.

Actual painter calls can advance Workshop yaw or visible Copter phase under
the [animation controller](design-workshop-animation-controller.md). Do not
add a second time-derived advancement in component setters. No nonzero raster
matrix was added here.

## Normal-open contexts that need broader admission

The [normal-open reference](design-workshop-normal-open-recommendation.md)
is safe for its declared early/advanced profiles, not arbitrary `597de0` calls.
Before extending it, admit and check the following distinctions explicitly:

- Arbitrary active own designs can produce upgrade proposals before the
  defensive family. A first new proposal can lock a different category.
- Excluded matching records, live-unit/production cache protection, full banks
  and fuzzy ability matching can change candidate reuse and raw overlays.
- Mixed acquired technologies and advanced occupied banks change proposal
  ordering, cost comparisons and fallback ability policies. Extra University
  grants are not the named-starting-tech profile.
- AI versus human mask, faction personality/definition fields, diplomacy/peer
  capability state, PSI modifiers/projects and strategic Base/Unit state are
  controller inputs. Preserve their authority; do not substitute a faction-name
  lookup for the original rules.
- Dispatcher automatic-design actions 9..22 supply a **category filter** and
  seed recommendation reactor; their readback copies reactor too. Normal-open
  sets filter -1 and retains entry reactor instead. They are distinct callers.
- Custom component definitions, disabled rows, alternate faction assignments
  and SMAX catalogs require their own admission; filtered ordinals must not be
  treated as fixed stock IDs.

Ordinary saved-row selection and post-retirement argument-0 reselection do not
need the broader recommendation controller. Wire them through their own paths
so unsupported recommendation contexts are not reached incidentally.

The freeze contains complete opcode/address source for all five picker callers,
suggestion/cleanup, generic modal/card/hover acceptance, browse selection,
dispatcher and panel callbacks. `reused-manifests.json` pins the existing saved
edit, compatibility, fresh selection, normal-open and retained-overlay proofs.

## PSI and special strength text

For exact picker numbers, the armor Level line, central stats and the nearby
weapon/noncombat differences, use the
[PSI and special strength text addition](design-workshop-strength-text.md).
It adds 21 focused text controls in a separate freeze; the 26 interactive
transition/refresh controls above are unchanged.
