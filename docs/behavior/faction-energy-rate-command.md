# Faction energy-rate controls and immediate refresh

RE-SMAC-ENERGY-RATE-COMMAND-001, 2026-10-18. Pinned stock SMAC.
[Literal controls](faction-energy-rate-command-cases.json). Reuses established
[allocation/rounding](psych-allocation-and-specialist-output.md),
[inefficiency](energy-waste-and-headquarters-distance.md) and
[startup defaults](startup-economic-state.md); no new allocation calculator.

## Domain and coupled controls

The original rates are whole **tenths**, displayed in10% steps. Psych and Labs
are stored faction words; Economy is `10 - Psych - Labs`. All66 nonnegative
triples summing10 are admitted by the native button/normalization path. Every
triple is reachable from the ordinary5/0/5 start using the measured unlocked
buttons. The actual upper-limit helper returns literal10 without reading its
faction argument, difficulty, rules or faction definition. There is no special
50%/60% difficulty cap on this stock energy-rate limit. Broader access to the
containing Social Engineering window, scenario UI and multiplayer are separate
admission boundaries; the port must still authenticate the local human owner.

Six native buttons decrease/increase Economy, Labs or Psych by one tenth, clamp
the edited component to0..10, then normalize the three rates back to total10.
They do not allow the player to submit an invalid total. A port API accepting a
whole triple should validate its integer/range/sum domain explicitly; that is a
port transaction contract, not a discovered original triple-Apply error dialog.
Mouse-drag or arbitrary free-text input is not established by this button receipt.

Normalization uses cyclic axis order **Economy → Psych → Labs → Economy**.
Starting with the axis after the edited one, find an unlocked other component
that can increase (when total<10) or decrease (when total>10). Change it by one,
then restart the search after the edited axis until total10. Locked/full/empty
other axes are skipped. If all other axes are blocked, the search reaches the
edited axis and adjusts it back, even if it is locked. Thus an edit may visibly
remain unchanged. This is deterministic priority, not equal redistribution.
Locks belong to local control state; they are not faction rate authorities.

With unlocked E/P/L5/0/5:

| Button | Result E/P/L |
| --- | --- |
| Economy − | 4/1/5 |
| Economy + | 6/0/4 |
| Labs − | 6/0/4 |
| Labs + | 4/0/6 |
| Psych − | 5/0/5 |
| Psych + | 5/1/4 |

For3/4/3, Economy+ gives4/3/3, Labs+ gives2/4/4, Psych+ gives3/5/2.
With Labs locked at5/0/5, Psych+ instead gives4/1/5. With both other axes
locked, the edited axis returns to its previous value. All3,168 original button
prefixes cover66 triples ×6 buttons ×8 lock masks, including0/100% endpoints.
They execute actual normalization and actual faction stores, not supplied results.

## Original modal semantics

The containing Social Engineering window retains its opening rates and working
rates separately. **Button edits immediately update live faction rates and
recalculate owned Bases while the window is open.** This is not an untouched
simulation followed by a single deferred rate Apply.

- **Enter** keeps the current rates and closes through the ordinary close path.
- **Escape** runs Reset, restoring opening rates, then closes.
- The source command dispatcher has separate reset-and-close, close-only and
  reset-only actions. The keyboard compositions below execute the actual paths;
  graphical button labels/mouse admission are not newly observed.

Reset restores the saved opening rate words and the working control array, then
recalculates. It is a compensating calculation, not restoration of an entire
Base-state snapshot; do not promise all allocator/cache side effects undo
byte-for-byte. The containing close routine can ask for payment/confirmation for
**Social Engineering model changes**. With only rates changed and model choices
unchanged, measured Enter/Escape paths make no confirmation-dialog call and
charge no credits. Do not add a rate-change fee or reuse a paid policy-change
confirmation for this command.

A port may deliberately implement a staged preview/Apply UI, but it must identify
that modal timing difference. Reuse native saved-rate authority and worker,
resource and citizen producers; do not create UI-only category arithmetic.

## Immediate engine work versus upkeep

For an ordinary local human outside network mode, each rate button:

1. Stores Psych/Labs from normalized working rates.
2. Refreshes social state and performs one forced full resource recalculation
   over each owned Base in global Base-record order.
3. Rebuilds working/current Social Engineering projections.
4. Builds the economy summary with its recalculation flag set, performing a
   **second forced full resource recalculation over each owned Base**.
5. Updates forecast/summary values and redraws the window.

Each full pass includes Support → intake/worker allocation → nutrients →
minerals/ecology calculation → energy/citizen calculation. The force argument is1;
a remembered same-Base cache ID does not skip it. The loops select each Base and
leave the last owned calculation selected/cached. Do not confuse this internal
calculation selection with which Inspector the port should display.

Two actually founded owned Bases therefore receive **four** complete resource
calls for one button. Escape from changed rates adds another four while restoring
them. Five Psych+ presses receive20 calls; rate-only Enter adds none. Enter with
no edits receives none. The first social-state loop has a human/non-network
condition; the second summary force loop still checks ownership. These counts
are for the admitted ordinary human path, not a network/AI universal policy.

The composed controls execute actual callbacks, social producers, native forced
pipelines and native close/reset logic. Only busy/widget/redraw/status services
are headless. They observe **zero** manual citizen-upkeep/effects calls, research
submissions, ecological event-handler calls and shared/CRT RNG calls during the
button/close phases. Stores, treasury, research progress and ecology aggregate
are unchanged then. Resource calculation itself may change current allocation,
category and citizen outputs; it is not a post-another-turn action.

## Literal two-Base results and persistence

Initial E/P/L5/0/5 yields native E/Psych/Labs1/0/2 and1/0/1. One Psych+ press gives
5/1/4 but leaves those rounded category outputs unchanged. Five presses give
5/5/0 and outputs **1/1/1 and1/1/0**. Zero Labs percentage can still leave a
residual Labs point because the original rounds Psych and Economy separately
and gives Labs the remainder; never force that final output to0 from its label.

Before/during either UI sequence: treasury100, research progress40 and both
nutrient/mineral stores0/0. After accepted one-press normal upkeep: treasury102,
research43, both stores1/3. After accepted five-press upkeep: treasury102,
research41, both stores1/3. Each ordinary upkeep enters research and ecology once
per Base and consumes the two existing zero-damage CRT draws. These are admitted
local fixtures, not claims of universal posting deltas. Escape restores5/0/5,
without posting anything.

A separate actual ordinary file-body save/load pair restores all sixteen
Psych/Labs words across slots0..7 after poisoning; no RNG is consumed. Economy
remains a derived remainder. Fresh0/5 defaults come from the earlier measured
initializer, not a new hardcoded restore default. The control locks, opening
window snapshot and widget markers are transient UI state; their persistence is
not established or required by this stored-rate receipt.

## Port acceptance

Use an authenticated faction-wide command with expected owner/turn/rate state,
explicit tenths validation and the established producer order. Refresh all owned
Base views from engine outputs. Distinguish draft/preview timing from original
live modal changes, define Cancel accordingly, and yield to mandatory research/
Colony continuations. Cover coupled endpoints/locks, unchanged accepted values,
two owned Bases, category rounding, Cancel, stale/foreign requests, save/restart
and subsequent upkeep. Avoid rate-preview research posting or ecological RNG.
No production edits, fresh graphical session or full platform test matrix was
performed in this RE handoff. Full Social Engineering model changes/fees and
exceptional citizen effects remain outside this rate-only contract.
