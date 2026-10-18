# Ordinary conquest request and ending controller

Research ID: RE-SMAC-CONQUEST-CONTROLLER-001. Original SMAC only.
This closes one local, single-human conquest path after a genuinely failed
last-Base restart. It builds on [last-Base capture](last-base-capture.md),
[survivor counting and human loss](capture-survivor-ending.md), and the
[starvation caller](starvation-base-removal.md).

## Admissible ordinary case

The concrete setup has factions 1 and 2 active, only faction 1 human, local
focus/unit controller/active faction all 1, and network mode off. The retained
first and second reference identities are 1 and 0. The initial ending reason
and request are clear. No cooperative option or extra relation exclusions are
present. The captured Base is faction 2's last, but that fact alone is
insufficient to declare elimination or a winner.

Native home cleanup and disposition remove the loser's remaining Units. The
restart search genuinely fails. Disposition clears faction 2's active bit,
leaves human membership unchanged, and uses the retained reference identity.
The survivor scan finds one survivor and zero cooperative partners; the
reference equals local focus. This reaches the single-conquest notification.
Its return value is unused: after it returns, disposition stores reason **4**
if the old reason was zero and ORs **0x2001** into game flags, marking conquest
and a pending ending request. A nonzero existing reason is preserved.

This branch does not recompute rankings, use the captor unconditionally as
winner, or require the normal conquest-victory option. The ordinary case's
reference and focus both identify faction 1.

## Timing relative to the owning caller

The ending request originates **inside the restart/disposition work**. It is
already stored before restart returns to disposition and before disposition
returns to capture. Capture then refreshes the captured Base and enters its
Base screen. The global ending controller is not called synchronously by
this disposition/capture tail.

The unit-action controllers contain zero-argument ending polls between admitted
actions. The owner-turn loop also polls with argument zero immediately after
the faction upkeep call returns, before proceeding to that owner's human/AI
action handling, and polls again after that handling. A global turn wrapper
has another poll after its turn body. These are static caller connections;
the focused receipt separately admits the original unit-controller poll on
the unchanged capture-screen state. It does not answer the Base screen or
prove uninterrupted UI-loop continuity.

Within faction upkeep, a pending, unprocessed request after the Base pass
skips the ordinary later unit/AI work and takes the owner cleanup tail.
That tail still performs its normal cleanup and its conditional no-Base /
no-Colony-Pod disposition check before returning to the owner's ending poll.
It is not an immediate stack abort at the point of Base deletion.

Starvation calls the same disposition helper after native Base deletion,
inside population/Base/faction upkeep. A successful restart remains a live
faction; the earlier starvation receipt stops before restart and does not
itself establish failed restart or conquest. The owner-loop poll becomes
relevant after an actual completed upkeep, not merely when a Base count hits
zero.

## Controller inputs and winner identity

The controller first distinguishes stored pending/processed state:

- Pending clear: argument zero does not perform the broader normal victory
  discovery. It returns without an ending unless another request is present.
- Pending set and processed clear: dispatch the saved reason, including an
  already produced conquest request. No fresh survivor/ranking computation is
  required by this path.
- Pending and processed both set: return without repeating ending processing.

Before reason dispatch, the controller scans retained per-faction role masks
**0x20000** and **0x40000** for factions 1 through 7. This scan does not filter
by active or human membership. It can retain a candidate using those masks,
the candidate's primary relation toward local focus, and focus equality.
Its candidate is not a general replacement for the disposition reference or
the saved reason. Broader role producers and other victory types are unclosed.
Precisely, the scan starts with candidate 0. A faction with mask 0x20000
replaces it. A faction with mask 0x40000 replaces it if the current candidate
is 0, is local focus, or has primary relation mask 1 toward focus. Later
admitted factions can replace earlier ones.

For **reason 4**, history explicitly records **current local focus**, with
opponent/partner sentinel **-1**. It does not use that scanned role candidate.
Thus the bounded ordinary result is **conquest, winner faction 1**, even when
all scanned role masks are clear. Keep the input focus stable through this
dispatch; no defeated-player focus switch is implied.

For **reason 7**, the controller skips this winner-history call and conquest
bookkeeping. That reason comes from the earlier **local human-loser** refusal
branch, which tests the loser's human-membership bit and local network mode,
clears the losing active bit, stores reason 7/request, and returns before
survivor counting. Two human factions can therefore leave captor 1 focused,
active membership 2 and human membership 6, while requesting human loss rather
than publishing a captor conquest winner.

## Saved outcome versus presentation completion

For this ordinary conquest, publish/persist the already resolved gameplay
outcome with reason **4**, conquest marker **0x2000**, pending request **1**,
winner/local focus **1**, active membership **2**, and human membership **2**.
The retained reference inputs explain the producer; preserve them in restored
gameplay state. No new universal winner-ID field is established by this branch.

The controller records focus history and enters conquest-specific presentation
bookkeeping when its program-mode gate permits. Presentation work precedes
setting processed mask **8**. The later local conquest tail offers the existing
continue-playing prompt. A nonzero answer returns without requesting session
exit; the processed marker prevents repeat ending presentation. A zero answer
reaches session-exit request **1** and controller return **1**. Local reason 7
skips that continue prompt and takes the exit path.

Those final presentation/prompt paths are static continuations. The focused
native run stops at an unanswered conquest interlude entry; it does not supply
a continue-playing choice, completed ending screen, session exit, or full
controller return. Do not conflate the saved conquest outcome with an observed
completed presentation.

## Limits

No cooperative, network, alternative-reference/focus, preexisting-reason,
other-survivor, score/time/technology victory, or general AI matrix is added.
No winner may be inferred solely from zero Bases. Restore/Start must preserve
the resolved request/reason and identity inputs; original save-format and
load-time controller dispatch were not newly executed here.
