# RE-SMAC-LAST-BASE-CAPTURE-001: undefended last-Base capture

Two original SMAC movement/capture callers reach defeated-faction notification,
before the known captured-Base screen boundary. Both use a human Librarian
Scout entering an undefended population-two Vendetta Base, with the defender's
only remote unit homed there: a Former, or a Colony Pod. The controlled map is
clear rainy land with actual region rebuilding; both factions have the same
retained technology and no projects. [Cases](last-base-capture-cases.json).

## Observed ordering

1. Capture changes the Base to the attacker, reduces population two to one,
   initializes assimilation to 50 and performs the ordinary capture changes.
2. The lost-home scan finds no surviving defender Base. It removes the remote
   homed unit. The Colony Pod receives no exception in this scan.
3. Capture itself calls faction disposition before recalculating the captured
   Base and before opening its final Base screen. With no defender Base left,
   disposition enters remaining-unit cleanup and then the original restart
   placement routine. The remote unit has already disappeared.
4. In both checked worlds, restart placement fails after 1,025 coordinate pairs.
   No replacement units are created. The human defender's active bit clears;
   the human-membership mask stays unchanged. Ending state becomes 7 and the
   ending-request flag is set before defeated-faction notification.
5. WIPEOUTYOU0 is the captor's eradication/interrogation notification. Its
   caller ignores its return, so the established display-only acknowledgment
   applies. The following notification-history work executes natively.
6. Disposition returns 1 to capture, despite restart returning 0. Capture then
   performs its normal Base recalculation and reaches the captured-Base screen
   entry. Both runs stop there without an answer, before screen construction.
   Capture and outer movement have not returned.

At that boundary the attacker owns both retained Bases; their IDs remain the
same. The captured Base is population one, former owner defender, timer 50,
nutrients five and minerals zero. The entering Scout is inside it, spends all
three movement points, retains its old home and remains selected. The other
attacker Scout survives unchanged. The defender has no Base or unit.

Local focus, unit controller and active faction remain the attacker. Clearing
the defender's active bit does not switch any of them. This is a two-human
local context: the reached human-loss branch records ending state 7 even though
the current focus is the captor. It returns before the separate survivor-count
and cooperative/conquest-ending branches; the global ending controller is not
executed in the checked prefix. This receipt establishes an ending
request, not a victory result or a displayed loss screen.

The checked capture transfers 133 credits, leaving 233/67; restart initialization
then adds the starting-energy grant of 10 to the defender, leaving 233/77 even
though placement fails. Capture uses 37 CRT draws; restart search adds 2,050.
Shared RNG is unchanged. SEIZEBASE and WIPEOUTYOU0 text use display-only
acknowledgments; notification history executes natively. No new gameplay choice,
restart outcome, coordinate or draw is supplied.

## Limits and port implications

The Colony Pod discriminator changes only the remote unit's actual factory
design. It proves that this homed Pod does not preserve the faction through
the lost-home scan. It does not establish every unhomed Pod or transport case.

Zero Bases is not an unconditional elimination rule. The separate
[last-Base restart receipt](last-base-restart.md) executes successful recovery
after starvation under admitted placement conditions, as well as turn/option
refusals. These capture cases execute the actual search and fail to place;
their result must not replace general search eligibility with automatic defeat.
The restart search also rejects a candidate whose nearest Base belongs to the
captor and lies in the candidate's region. A same-caller diagnostic executes
that rejection at candidate (47,23), in region one, with the newly captured
Base as the nearest Base. Both retained Bases share that single land region.
Earlier rejected candidates are not individually classified by this receipt.
Other maps, multiple land regions, AI/network contexts, Do or Die, later turns,
complete capture/ending returns and subsequent controller behavior remain open.
