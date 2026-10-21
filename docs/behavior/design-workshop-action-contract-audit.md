# Remaining Workshop action contracts for live integration

2026-10-21. Source-only audit; no production edits or new action mutation matrix.
Original terran.exe fingerprint matches the Workshop evidence. Current
manual ability presentation is in [the chooser contract](design-workshop-manual-ability-chooser.md).

| Control | Existing contract / remaining work |
| --- | --- |
| Apply1, Done4/-1, Cancel8/-2 | Complete state/caller behavior in [custom editing](design-workshop-custom-edit.md) and [first path](design-workshop-first-path.md). Preserve Apply reselection even after declined overwrite, and modal result field rather than incidental dispatcher EAX. |
| Rename5 | [Rename/Obsolete supplement](design-workshop-rename-and-obsolete.md) adds native DESIGNNAME layout, button admission and empty-draft cancel/default controls. Preserve immediate selected named-bit effects and deferred stored-name commit. |
| Obsolete/reactivate6 | Same supplement adds unsaved-draft authority and toggle-then-Cancel controls. Immediate selected-record edit, old-list next selection then rebuild; no confirmation or rollback on Workshop Cancel. |
| New / inactive card | Complete caller contract in [fresh draft](design-workshop-fresh-draft.md), not dispatcher action3. |
| Previous23 / Next24 | Complete offset-only wrap/paint contract in fresh-draft and strip documents; not selected-design navigation. |
| Retire0 | Now covered by dedicated cancel/accept, empty/live, intrinsic, duplicate-queue and network sender controls in [explicit Upgrade/Retire](design-workshop-explicit-upgrade-and-retire.md). Source outline below is supplemented by that executed contract. |
| Upgrade7 | Now covered by33 combined dispatcher controls, receiver call fragments and independent whole-buffer economic/conversion reference in [explicit Upgrade/Retire](design-workshop-explicit-upgrade-and-retire.md). The recommendation upgrade-prefix evidence is not this action. |
| Automatic family buttons9..22 | Source caller mapping/seeding/readback is now appended below. Existing bounded human producer/recommendation contracts cover their declared profiles; this addition does not claim new end-to-end executions for14 category buttons. |
| Debug2 / Debug3 | Debug prototype toggle/reset, gated by959fc8 bit40 hex. They are not ordinary Retire or New. Do not expose them as gameplay substitutes. |

## Exact action identity correction

The jump tables at `4436a0/4436d0` map action0→442900 Retire;
action2→442777 debug prototype toggle; action3→442810 debug reset;
action5→442a73 Rename; action6→442a7f Obsolete; action7→442bdf Upgrade.
The older strip-centering document's wording "retirement2" is an incorrect
action label: its traced shared centering store442a5d belongs to **Retire0**.
The frozen disassembly is unchanged. Use the dispatch table and fresh-draft
correction when assigning live buttons.

## Direct Retire source outline

`442900..442993` substitutes selected design name, reads cached own unit count
at `95d9fc+owner*20bc+selectedID`, and chooses distinct confirmation tags
for a nonzero count versus zero. Decline jumps to the common exit. It does not
commit the current component draft before asking.

After acceptance, single-player `4429e5..4429f8` calls
`595ac0(selectedID,owner)`. Multiplayer takes the separate949308 branch:
54e820(25 decimal/19 hex) admission, command2415 submission through5aa510, followed by
network/UI coordination. Do not replace that branch with immediate local
retirement. Then select the same raw ID with `4490b0(id,0)`, rebuild5bdb60,
recenter the strip through442a5d if found, and request a full Window+f8 repaint.
The full painter's cleanup boundary remains applicable. Dedicated direct
retirement controls and supplied network boundaries are now in the supplement.

## Explicit Upgrade source outline

At `442bdf`, the action first calls545560 with mode2; rejection exits. It builds
an UPGRADE-style target list from the owner's64 custom slots, excluding the
source, inactive entries and owner-excluded entries. It requires identical
chassis and matching artillery bit8000. Stored role/weapon/armor comparisons
provide additional admission; no requirement here is safely replaced by a
generic "better stats" comparison.

At `442e77..442ebf`: zero targets warns, one target bypasses the chooser,
multiple targets open61b9d0, negative result exits. At `442ec7..442ef3` it calls
`504fe0(owner,targetID,sourceID,0)`, retaining whether cached source unit count
was zero before the call. Only a successful upgrade selects the target,
rebuilds the browse list, recenters and refreshes. The later442f7d branch uses
that retained no-unit condition for the informational YESUPGRADE completion
message, **not optional retirement**. Native504cc0 deactivates a custom source
inside the conversion. Its currency, unit, prototype and confirmation effects
are now checked by dedicated controls in the executed supplement. The old
frozen audit remains as historical evidence; use the supplement for these rules.

## Cost/action/animation integration priorities

The remaining integration work should consume the existing complete contracts
before requesting another broad research matrix:

1. **Cost and prototype presentation:**
   [main presentation](design-workshop-main-presentation.md) supplies the cached
   human mineral factor, actual recomputed draft cost, reactor minimum, marker
   and separate prototype warning. Its23 mineral-factor calls, five actual
   cost fragments and declared minimum control distinguish arithmetic from
   the cached display state. Use the existing general
   [prototype cost rules](unit-design-and-prototype-costs.md) for the arithmetic.
2. **Economic actions:**
   [explicit Upgrade/Retire](design-workshop-explicit-upgrade-and-retire.md)
   already has33 complete dispatcher controls and the whole-buffer conversion
   reference. Its unit-count quote, Nano Factory rounding, cancellation,
   source deactivation and queue behavior are ready for live callers.
3. **Animation:** [animation controller](design-workshop-animation-controller.md)
   has24 focused native controls; main presentation additionally connects the
   initialized parent timer. Preserve40ms coalesced redraw, per-main-paint
   binary32 yaw increment and the independent visibility-gated Copter phase.
   There is no need to invent elapsed-time catch-up or rerun static CVR matrices.
4. **AutoDesign caller:** use the source mapping below with the checked human
   producer/preview policy. New native coverage, if needed, should target a
   failing category or caller boundary rather than repeat all role proposals.

The exact DESIGNNAME layout is now checked by the Rename/Obsolete supplement.
Retire zero/nonzero-count prompts and the multiple-target Upgrade chooser are
now measured in the [modal layout supplement](design-workshop-retire-upgrade-modal-layout.md).
Auxiliary scrollbar chrome/held-button repeat and Upgrade's later currency/
prototype prompt geometry remain presentation research gaps. Existing action
mutation, generic edit/input and timer contracts are separate from those gaps.

## AutoDesign9..22: caller filter and seeded tuple

2026-10-21 source-only completion of `442600@442fa9..443139` and jump table
`4436ec`. Filter IDs are **decimal**:

| Action | Filter | Role family |
| --- | --- | --- |
| 9 | 2 | Defensive land |
| 10 | 3 | Offensive minimum-speed land |
| 11 | 1 | Artillery |
| 12 | 4 | Fast land attack |
| 13 | 10 | Scout |
| 14 | 6 | Naval combat |
| 15 | 7 | Naval transport |
| 16 | 8 | Air attack |
| 17 | 9 | Interceptor |
| 18 | 17 | Copter / finite-range air attack |
| 19 | 13 | Conventional missile |
| 20 | 14 | Planet Buster |
| 21 | 11 | Drop |
| 22 | 12 | Amphibious |

These buttons do not invoke normal Workshop open or commit the current draft
first. Before `597de0(owner)`, caller `44301b..44306d` writes:

| Controller field | Seed |
| --- | --- |
| 9498c8 | Category filter above |
| 9498e0 | Mode1 |
| 9498d8 | Current draft weapon, actor+14200 |
| 9498d4 | Current draft armor, actor+14204 |
| 9498dc | Current draft chassis, actor+141fc |
| 9498d0 | Current draft ability mask, actor+14208 |
| 9498cc | Current draft reactor, actor+1420c |

The reactor seed is explicit and must not be replaced with a default Fission
or a cached selected record's reactor. Generation uses the existing ordered,
interleaved human producer and preview policy, including their side effects
and no-match preservation; do not commit or batch requests as a new caller rule.

After generation, `443072..4430b2` copies those same five controller tuple
fields back into the draft. It clears both ability slots to-1, scans mask bits
0..23 ascending, and fills at most the first two slots; the mask itself is the
returned controller mask. This is the caller's slot reconstruction, not the
manual chooser's preserve-requested-slot rule.

`4430f0..443126` tests resulting controller mode: mode>1 takes feedback code43;
otherwise it opens SCRIPT **NOPRESET** and takes feedback code39. The caller
then clears controller mode to0 and requests Workshop virtual+30 followed by
virtual+fc (`44312c..443139`). It does not invoke the manual-picker acceptance
routine or introduce an unconditional tuple fallback on NOPRESET.

This closes the source caller contract; the existing human producer evidence
is reused, not relabeled as14 newly executed button controls.
