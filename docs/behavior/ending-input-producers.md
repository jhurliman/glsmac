# Human membership and retained ending references

Research ID: RE-SMAC-ENDING-INPUT-PRODUCERS-001. Original SMAC only.
This supplements [conquest dispatch](conquest-controller.md), without changing
its completed receipts. [Cases](ending-input-producers-cases.json) separate
reused native setup evidence from a newly observed ranking update.

## Ordinary local human membership

The [setup choice-transfer receipt](setup-wizard-choice-transfer.md) already
executes the producer. In local single-human setup, accepting loaded faction
slot **f** and keeping the leader name binds focus to f and writes the sole
human membership **1 << f**. The slot is the selected loaded faction identity,
not a network role or a newly allocated human-player ordinal. For example,
the accepted slot-6 case produces focus 6 and human membership 64.

The remembered-choice setup path reaches the same binding. Cancellation before
that binding preserves the earlier membership; accepted earlier setup choices
can already have changed. This is not one atomic setup transaction.

Human and active membership are distinct authorities. New-game initialization
sets active membership separately and preserves the previously bound human
membership. The confirmed failed-restart human-loss path clears the loser's
active bit while retaining its human bit. Changing focus therefore does not
by itself redefine who is human, and elimination does not imply human-to-AI
conversion. Other explicit editor/network/multiple-human membership changes
are outside this ordinary contract.

For the port, bind a persistent per-faction human authority from the accepted
campaign selection, then retain it independently of activity, local focus and
Single/Host/Player transport roles. Restore that authority from saved gameplay
state; deriving it afresh from the currently focused faction loses information.
Original save-format/loading behavior is not newly traced here.

## Producer of the two ending reference identities

The disposition survivor scan reads **rank-position 7** as its first reference
and **rank-position 6** as its fallback when the first equals the eliminated
faction. These are the retained inverse ranking table's leader and runner-up
positions. They are not the best/second human identities, local focus, captor,
or a fresh survivor scan. The ranking routine separately derives human extrema.

The ranking routine first computes scores. With update argument zero, it stops
before assigning ranks and rebuilding the inverse table: the ending identities
remain unchanged. With a nonzero update argument, it:

1. Assigns ranks 7 down to 1 by repeatedly selecting the highest remaining
   score among faction slots 1 through 7. Strictly-greater replacement means
   equal scores prefer the lower faction slot for the higher rank.
2. Includes inactive slots in that sort, with their score set to zero. Native
   faction 0 does not participate in this seven-slot ordering.
3. Clears all eight inverse positions to zero, then writes each **active**
   faction into its assigned position. It does not compact gaps left by
   inactive factions. An empty leader or runner-up position remains zero.
4. Computes separate best/worst human identities and performs later retained
   policy work. Those human extrema do not replace the ending references.

The sort uses its score buffer destructively, marking selected entries -1.
Do not derive the retained order by inspecting that buffer after assignment.
The signed sort and its -1 sentinel here are established for ordinary
nonnegative scores; malformed/overflowing scores are not covered.

## Update timing and validity

Ordinary global turn advance calls the full nonzero ranking update **before
incrementing the turn**, then runs the owning factions' upkeep/actions.
The local owner controller calls that global advance on its normal new-round
path. Its resume path instead directly calls the nonzero ranking update.
The first-command receipt reaches this real controller/global-advance path.

These references are consequently retained snapshots, not values recomputed
after every movement, capture, Base deletion or ending poll. Disposition can
still read a reference identifying the just-eliminated faction and deliberately
uses the runner-up in that case. Preserve that timing rather than rebuilding
the ranking from surviving factions at the point of elimination.

Treat references as produced only after a real nonzero ranking update, or as
retained input when restoring an already initialized game. This investigation
does not establish a universal value before the first update. No setup rule
authorizes initializing the references to focus/zero.

## Small producer receipt and remaining boundary

The newly observed case reuses the admitted Gaians first-command path: one
active human faction 1, one native starting Scout and one Base, supplied local
controller/calendar admission, and ordinary new-game scenario score parameters
9999/9999/0. Human/active membership is supplied in this particular harness;
its human producer evidence comes from the separate setup receipt above.

At ranking entry, the old first reference is the explicit fixture value **2**.
The complete original ranking update computes scores **[1,0,0,0,0,0,0]** for
slots 1..7, assigns ranks **[7,6,5,4,3,2,1]**, and rebuilds the inverse table
as **[0,0,0,0,0,0,0,1]**. Thus first/second become **1/0** by actual native
writes. They are already produced at turn 0 before its increment and remain
1/0 at the turn-1 first-command boundary. This is producer evidence for that
one-active case, not an ordinary seven-active generated-world receipt.

An observation of the unchanged older admitted path is also retained privately:
its zero-initialized scenario parameters select objective scoring and produce
all-zero scores. It is not advertised as normal campaign scoring. The ordinary
case explicitly supplies the defaults written by the native new-game initializer;
the test does not claim continuous execution from the setup wizard through all
world/faction initialization to this controller.

Complete score construction across ordinary seven-faction worlds, score changes,
other scenario modes and all report/UI refresh callers remain outside this
bounded closure. The ranking owner, update flag, timing, tie handling, active
filter and reference positions are closed; a general port must obtain valid
scores from its ranking authority, not use this receipt's 1/0 as constants.
