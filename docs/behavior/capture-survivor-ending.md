# RE-SMAC-CAPTURE-SURVIVOR-ENDING-001: loss requests and conquest ownership

This bounded follow-up separates the local human-loss request from the
survivor/conquest branch after a genuinely failed restart. The
[cases](capture-survivor-ending-cases.json) record two native captures and a
separately admitted original ending-controller poll. It reuses the single-region
setup from [last-Base capture](last-base-capture.md); no placement failure,
ending result, notification answer or controller return is supplied.

## Existing two-human loss request

With both factions human, the original capture again fails restart after
1,025 coordinate pairs. Active membership changes 6 → 2; human membership
stays 6. The defender has no Base or Unit. Focus, unit controller and active
faction all remain captor 1. The local human-loss branch writes reason **7**,
sets ending-request mask **1**, and returns before survivor counting. It does
not convert the surviving captor into a conquest winner.

The capture reaches its unanswered Base screen with that state. On the same
native state, a separate admitted fragment executes the original zero-argument
global ending poll. This is not a claim that the Base screen was closed or the
outer capture/movement controller returned.

The global controller sees the pending request and dispatches reason 7. It
does not rerun normal victory discovery or change focus to the defeated human.
Its winner candidate scan finds no victory-marked faction in this fixture;
reason 7 makes no winner assignment. The reason-specific optional loss cinematic
is gated by program mask `0x10000000`; that mask is clear here. The reached path
therefore skips that cinematic, winner-history recording and conquest-specific
bookkeeping, then enters the global ending presentation/controller bridge.
The probe stops at that bridge before constructing or answering it. Reason 7,
pending request, focus 1 and all ownership masks remain unchanged at the stop;
the global controller has not returned and the session-exit request is still 0.
There are no new RNG draws in this controller prefix.

Static continuation after the presentation work sets processed mask **8**.
For local reason 7 it skips the "continue playing" prompt and reaches the
session-exit request, set to 1, before returning 1. This tail is statically
established, not an observed completed controller or displayed loss screen.
The optional cinematic uses the **current focus's** retained identity, not a
saved defeated-human argument. Preserve that distinction when handling a
two-human loss request while the captor remains focused.

## Only captor human, defender AI

The second capture changes only explicit human membership to **2** before
entering the same original movement/capture caller. Active membership starts
at 6. It uses the same clear rainy single-region map, target population 2,
undefended target, old remote homed Former, turn 6 and Librarian setup.

The native search genuinely exhausts its attempts: 2,050 placement CRT draws
after 37 capture draws, final CRT state **3640575528**, shared state **3** with
zero shared draws. No replacement Units are created. Home cleanup removes the
Former, capture transfers the Base and reduces its population to 1, and the
failed initializer still adds the 10-credit grant. Credits finish **233/77**.

The losing faction is outside human membership, so it bypasses the local
human reason-7 return. It clears the loser's active bit and enters survivor
counting. At the first conquest presentation:

| Field | Native value |
| --- | --- |
| Active / human membership | 2 / 2 |
| Focus / unit controller / active faction | 1 / 1 / 1 |
| Retained reference faction | 1 |
| Counted survivors / cooperative partners | 1 / 0 |
| Base counts, captor / defender | 2 / 0 |
| Defender Units | None |
| Ending reason / pending request | 0 / clear |
| Session-exit request | 0 |

The reference faction equals focus, so the reached branch presents **single
conquest** (`CONQSING`). The probe stops at this first victory notification
without answering. The initializer, disposition and capture have not returned,
and the global ending controller has not run.

The notification's return value is unused. Static continuation after it sets
reason **4 only if the existing reason is zero**, otherwise preserves the
existing reason, then ORs game flags with **`0x2001`**: conquest marker plus
pending ending request. These writes occur **after** the unanswered presentation
and must not be described as already observed. No separate new winner ID is
written by that tail; the reached single-conquest owner is the matching retained
reference/current focus, faction 1.

## Survivor branch rules established by the continuation

The branch after a nonhuman or network refusal clears the losing active bit
and a retained faction field, then uses a retained reference identity: first
ranking identity, falling back to second if first equals the losing faction.
In this fixture those inputs are 1 and 0. This is not a fresh ranking computation
or unconditional use of the captor argument.

Scan active factions **1 through 7**, excluding native faction 0. Human
membership does not filter survivors. Each active faction initially adds one.
In local mode, if its primary relation toward current focus has **both** masks
`0x1` and `0x02000000`, remove it from that count and skip partner counting.
The wider meaning/producer of the second mask is outside this receipt.

Otherwise count a cooperative partner when the faction differs from the retained
reference, its primary relation toward that reference has mask `0x1`, and game
option mask `0x1000` is enabled. Cap the partner count at 2. Enter the conquest
tail when `survivors - partners < 2`; at 2 or more, return without this ending
request. Nonzero partner count selects the cooperative notification and reason
**5 if the existing reason is zero**. Zero partners selects the single-conquest
notification/reason only when reference equals focus. The common reached
conquest-request tail ORs flags with `0x2001`.

This survivor tail has no additional requirement that the ordinary conquest
victory option be enabled: the observed option word is 0 and it still reaches
single conquest. Do not add such a gate to this disposition branch based on
the separate normal victory-discovery rules.

Only the one-survivor, zero-partner, local nonhuman-loss branch is executed here.
The cooperative alternative and additional relation/option predicates are
static rules, not new runtime cases. The focused-loser presentation branch,
network transitions, occupied restart sites, other survivor counts and general
AI behavior remain outside this investigation. Prior receipts and their frozen
evidence are preserved.
