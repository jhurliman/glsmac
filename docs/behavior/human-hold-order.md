# Human Hold, Sentry and Skip commands

RE-SMAC-HUMAN-HOLD-001. Original SMAC `terran.exe`, SHA256 `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`. Thirty checked original-x86 contexts, including continuous Former work and ten owning-turn passes. These are controlled execution receipts, not a replay of a rendered game or a native save/load test. [Literal cases](human-hold-order-cases.json).

## Commands must remain distinct

| Input / caller | Result for an ordinary ground Scout or Former | Movement and retained work |
|---|---|---|
| H, character `h` | Hold, order 2; countdown becomes 0 | Preserve spent movement and work progress |
| Shift+H, character `H` | Hold, order 2; countdown becomes 10 | Preserve spent movement and work progress |
| Unit context menu **Hold** | Hold, order 2; **preserve the existing countdown/order parameter** | Preserve spent movement and work progress |
| L, character `l` | Sentry/Board Transport, order 1; reset its first payload and countdown | Preserve spent movement and work progress |
| Space / Skip Turn | No new Hold or Sentry order; spend the full allowance if actionable | Preserve the current order and work progress |

Installed menu text explicitly lists Hold This Position/H, Hold 10 Turns/Shift+H, Sentry/Board Transport/L and Skip Turn/Space. Upper/lower character arguments matter: Windows virtual-key values alone do not distinguish H from Shift+H. The menu text was read as an input binding; the character dispatch and resulting transitions execute original instructions.

## Admission and state

Normal keyboard processing requires unit control, a nonnegative selected unit and non-editor mode. H then reaches its own handler, not Skip's availability predicate. H succeeds with movement spent 0, 1 or the full 3 for the ordinary Fission Scout. A working Former can be held immediately after spending its full movement on work. Hold neither refunds movement nor rolls back work.

The keyboard handler trusts the selected unit supplied by its caller: a diagnostic supplied foreign selection also receives Hold. **Do not cite this handler as an ownership authentication boundary.** Ordinary automatic selection separately excludes foreign units. The context-menu dispatch explicitly checks that the selected unit is still the one used to open the menu, that its owner matches the focused faction, and that the transport restriction permits the action. Changed-selection and foreign menu controls refuse without mutation. Earlier rendered selection, modal and event admission are outside this receipt.

For keyboard Hold, the complete 52-byte controlled records change only the order and countdown. The context-menu Hold changes only the order. Neither performs a general route, automation or status reset. Supplied automation marker/selector and other queued route parameters survive; the countdown occupies a parameter also used by other orders. This is evidence of retention, not a claim that active automation can never affect a held unit.

The keyboard ground-unit handler also rejects a ground unit stranded on unbased water. Land is admitted; its additional water-Base and aircraft branches are source-inventoried, not generalized from the ordinary land controls. Transport and network semantics are excluded.

## Following selection, owning turns and movement

Hold does not immediately replace the selected unit inside the key command. Normal availability excludes ordinary Hold, Sentry and Convoy orders, even with unspent movement. The actual outer controller, starting with a held Former selected and a ready owned Scout, calls its selector and stores the Scout as the new selection. This receipt stops after that store and before drawing the newly selected unit. The held Former and its progress remain unchanged.

During the owning faction's unit upkeep, spent movement resets. For Hold and Sentry, a **nonzero** 16-bit countdown is decremented; if it reaches zero the order becomes No Orders. A zero countdown is left alone, so keyboard H remains Hold after upkeep. Shift+H produces 10 → 9 on the next owner pass and becomes No Orders on the tenth owner pass. Another faction's complete unit pass does not decrement it. Countdown expiry restores eligibility; it does not itself advance Former work.

The context-menu difference is observable: inherited 1 becomes 0 and clears Hold on the next owner pass; inherited −1 becomes −2 and remains Hold. A context-menu Hold must not silently normalize that retained value to keyboard H's zero. The source operates on a 16-bit word; a complete negative-counter wrap cycle was not executed.

Sentry's repair-related wake branch is separate: it tests order 1, not order 2. Reuse [the measured repair contract](unit-upkeep-repair.md) for its actual damage/floor conditions; do not make ordinary indefinite Hold wake merely because repair completed.

After a normal Hold and following owner upkeep, ordinary directional input is refused and preserves the unit. It does **not** automatically cancel Hold. The tested unit-menu **Activate unit** clears the order while retaining work and spent movement; a subsequent directional input then moves normally. The lower-level movement wrapper itself can move a directly supplied held unit and preserve Hold, so it must not be substituted for human input admission. Special status overrides in the availability predicate remain outside this ordinary-state contract.

## Continuous Former acceptance

1. Actual Farm work creates progress 2 and spent movement 3 at (18,16).
2. Keyboard H changes Farm to Hold, countdown 0; progress 2 and spent 3 survive.
3. Actual next owner upkeep leaves Hold/progress 2 and resets spent to 0. Selection prefers the ready Scout; directional input to the still-selected held Former refuses.
4. Menu Activate unit changes to No Orders without subtracting progress.
5. Actual east directional input moves to (20,16), spending 3; progress 2 survives.
6. Next owner upkeep, Solar assignment and actual work produce Solar progress 4 at the destination.

Space differs at step 2: it retains Farm and progress 2 while spending the allowance. Sentry differs by assigning order 1 and resetting its order payload/countdown. Explicit activation while a terraforming order is still active has additional caller-specific rollback behavior; see [Cancel versus Activate](former-cancel-and-activate.md). Do not apply that rollback to Hold assignment.

No new production implementation, overlay re-probe, AI, diplomacy, full event loop, rendered notification, native save/load, multiplayer or SMAX claim is included. The broad RE goal remains paused.
