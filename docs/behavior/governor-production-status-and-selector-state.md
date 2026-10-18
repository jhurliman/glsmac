# Governor production status, minerals and RNG

2026-10-18, stock SMAC. This extends the
[accepted Governor command handoff](base-governor-command-handoff.md), with
five native Apply compositions and two ordinary Base save/load roundtrips.
[Literal state and mutation cases](governor-production-status-and-selector-state-cases.json).

## Separate status bit and lifetime

The bit cleared before active Governor production selection is **`0x80000000`
in the Base status word**. Keep these three identities separate:

| Field | Mask | Established role |
| --- | --- | --- |
| Base status | `0x80000000` | Manual production-control marker set by the primary manual-choice change prelude; cleared by active Governor production Apply |
| Base Governor configuration | `0x80000000` | Governor activation option |
| Base Governor configuration | `0x40000000` | Beginner/startup Governor marker |

The [actual founder receipt](founding-production-initialization.md) establishes
ordinary status initialization to0, independently of copying the faction's
Governor configuration. Thus an active/startup Governor configuration does
not initialize this status marker to1. The measured manual chooser prelude
sets the status bit when a different item is proposed; it does so before the
loss decision/commit, so it is not merely a record of successful completion.
See [startup/manual override](startup-governor-difficulty.md).

Active production Apply clears only this status high bit at this point,
preserving other status bits, and clears the queue **count**, before calling
the selector. It does not equate this operation with clearing activation or
the startup marker in the other configuration word. Queue entry storage is
not zeroed by the count write.

Two native ordinary save/load roundtrips preserve status4 versus`0x80000004`
while independently preserving Governor configuration`0xC0DFBE41`, minerals23,
baseline item2 and baseline minerals31. The live status bit is deliberately
poisoned to its opposite before load. These are complete native file-body
calls with one declared coherent Base and zero Units, using FILE/OS/heap
services. Other status writers and whole-game action lifetimes are not
generalized from these controls.

## Actual all-permissions-disabled selector

The previous four Apply contexts are repeated with mineral and both RNG
snapshots added. Their initial current minerals0, baseline minerals0 and
baseline item2 remain0/0/2 after native Stockpile selection. Target2 changes
to Stockpile, but baseline item remains2: Apply is not the later upkeep
baseline-commit writer.

A fifth controlled discriminator supplies current minerals23, baseline
minerals31, baseline item2, strictness2, loss50% and exemption10. Active
production management with all production permissions disabled executes the
actual selector and automatic setter:

| State | Before Apply | Selector entry, after command prelude | Selector return / after Apply |
| --- | ---: | ---: | ---: |
| Status high bit | Set | Clear | Clear |
| Queue count | 1 | 0 | 0 |
| Current target | 2 | 2 | Stockpile |
| Current minerals | 23 | 23 | **20** |
| Switching-baseline item | 2 | 2 | **2, unchanged** |
| Switching-baseline minerals | 31 | 31 | **31, unchanged** |

The automatic switching rule uses the retained baseline31: exemption10 plus
half the remaining21, truncated, gives20. It does not merely retain the live
23 or change the target alone. The native write sequence commits the target
first and then the adjusted current mineral store. Earlier command writes
have already cleared status/queue count. Neither baseline is rewritten.
Reuse [automatic production switching](production-switching.md).

The native selector also temporarily substitutes Explore/Discover/Build/Conquer
directives while evaluating candidates and runs full resource recalculations.
In each measured production-enabled context it enters forced recalculation19
times internally; the combined citizen-management Apply then makes its separate
explicit reset/recalculation call. The final Base configuration restores the
submitted Discover directive and matches the remembered committed word. These
counts describe this fixture, not a universal Governor algorithm. Retain the
selection pipeline and its state order rather than assigning Stockpile as an
approximation.

## RNG observations

All five compositions start with gameplay state`0x13579BDF` and CRT
state`0x2468ACE0`. Both states are unchanged before/after the actual selector
and after Apply. Hooks observe **zero gameplay RNG/seed-setter or CRT random
entries**. Only CRT thread-storage access is supplied; no random result is
substituted. This establishes no RNG consumption in these declared
all-permissions-disabled/no-riot contexts, not a claim that every Governor
decision is deterministic or uses no RNG.

As before, these accepted-setter paths supply text/notification services and
use headless map views. General Governor production permissions, complete
dialog/admission/cancel/network behavior and SMAX parity remain outside this
slice. Standing research remains paused.
