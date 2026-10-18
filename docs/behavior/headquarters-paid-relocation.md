# RE-SMAC-HEADQUARTERS-PAID-RELOCATION-001: paid HQ escape during capture

One original capture relocates the defender's HQ to **Base 2** before taking
the old HQ at Base 1. The population-3 target becomes population 2, and the
caller reaches its captured-Base screen without an answer. This is the sole
paid variant of the frozen [ordinary HQ capture](headquarters-capture.md).
[Cases](headquarters-paid-relocation-cases.json) give its exact snapshots.

The sole changed gameplay input is defender pre-loot credits **1,200** instead
of 200. Everything else retains the ordinary local two-human Librarian setup:
turn 6, captor focus/controller/active faction 1, equal technologies, no projects,
undefended target, target population 3, cached defender population 4, remaining
defender Base 2 at (26,16), initial CRT/shared states 3, and original movement
caller. No selector result, treasury result, RNG value or gameplay choice is
supplied.

## Native payment and installation order

| Stage | Captor credits | Defender credits | Result |
| --- | --- | --- | --- |
| Before capture | 100 | 1200 | Target Base 1 has HQ |
| Calculate saved booty | 100 | 1200 | `1200 * 3 / (4+1) = 720` |
| Test relocation / choose Base | 100 | 1200 | Pre-loot gate passes; Base 2 scores 51 |
| Pay relocation | 100 | 200 | Charge exactly 1000 |
| Install relocated HQ | 100 | 200 | HQ added immediately to Base 2 |
| Clear old HQ | 100 | 200 | HQ removed from still-defender-owned Base 1 |
| Transfer saved booty | 820 | 0 | Defender's `200−720` is clamped to zero |
| Complete capture | 820 | 0 | Base 1 changes owner; population 3→2 |

The booty amount is **not recalculated after paying relocation**. Applying the
cost first and recomputing booty as 120 would produce the wrong result. The
native subtraction clamps the overdraw to zero; it does not reduce the captor's
already-saved 720-credit award.

Relocation is an immediate facility installation, not an HQ production order.
It occurs before owner/population change, donor selection, random destruction,
home repair and resource calculation. The old HQ is therefore already absent
when its facility-loss draw occurs. Its random and mandatory clears are both
idempotent in this paid case.

## Selected-candidate authority and static selector

Only Base 2 passes this fixture's owner/exclusion filters:

| Selector input | Native value |
| --- | --- |
| Candidate Base / owner | 2 / 2 |
| Candidate population / region | 1 / 1 |
| Retained regional Base counter source | **Owner 2, region 1** |
| Retained regional counter value | **3** |
| Live defender Base count at this point | 2 |
| Distance sum, including initial one | 1 |
| Numerator / denominator | 256 / `(3 + 2*1) = 5` |
| Integer score | **51** |

The regional authority is per owner, not global. Its retained value 3 is an
explicit inherited fixture input and is not normalized from the two live
defender Bases. Substituting live count 2 would give 64 and would fail this
receipt. The capture retains these regional counters while changing live
ownership; see [retained restart inputs](restart-input-lifetimes.md).

The exact static selector rule is:

1. Scan Base IDs ascending; require losing owner; exclude captured Base.
2. Start distance sum at 1. Add distance from candidate to every other remaining
   losing-owner Base, excluding candidate and captured Base. Double each
   cross-region contribution.
3. Divide `(sum << 8)` by `retained owner/region Base counter + 2*population`,
   using signed integer division with truncation toward zero. For these positive
   inputs this is floor division.
4. Start best score at 9999 and replace only for a **strictly lower** score.
   Equal scores keep the earlier Base ID; scores at or above 9999 do not win.
5. Require final selected ID **greater than 0**. If ID 0 won, skip relocation
   rather than retrying the next-best positive ID.

For wrapped absolute dx and absolute dy, the inline distance is:

```
max(dx,dy) - ((dx+dy)//2 - min(dx,dy) + 1)//2
```

On wrapped maps, replace horizontal dx with `width−dx` when raw dx exceeds
half-width. On flat maps do not wrap. Preserve the two inner integer divisions;
this is not Euclidean distance or a rounded floating-point approximation.
The native numerator uses a 32-bit left shift before signed division. Overflow,
malformed denominators, cross-region candidates and ties are not exercised by
this one runtime case; their instruction rules are frozen as static evidence.

## Continuation, resource refresh and limits

Captor focus 1 differs from loser 2, so native local confirmation is bypassed.
The reached `ESCAPED` notification is a five-second timed display; its caller
ignores the return value. The original stop at this notice is preserved. The
final execution supplies only that exact display service, then continues native
capture to the unanswered Base screen. It does not answer a relocation choice
or supply a gameplay result.

The human defender receives no production-choice call. Base 2's **only native
record write is the HQ facility bit**; its Scout production, queue, mineral
stores and cached resource fields remain unchanged. No immediate Base 2
resource rebuild is inserted. A later real recalculation will read its new HQ.

The remaining ordering matches the ordinary receipt: target population becomes
2; governor donor is captor Base 0; the remote Former retains owner/position/ID
and changes home 1→2; target production is selected as Scout; then captured-Base
resource calculation runs and returns before screen entry. Final target intake
is nutrients/minerals/energy **2/1/1**, nutrient surplus **−2**, mineral surplus
**1**, energy waste/net **0/1**, economy/psych/labs **2/0/0**. No upkeep or
per-turn resource posting is implied.

Exactly **37 CRT draws** occur, with the same ordered facility draws and exit
state **1951609278** as the ordinary case. Shared state stays **3** with zero
draws. Paid selection, payment, installation and continuation add **no RNG**.
All three Base and Unit identities are stable; active/human masks remain 6,
focus/controller/active faction remain 1, and ending state remains 0.

The selected candidate/score and paid arithmetic are observed. General tie and
distance rules are static. Focused-loser confirmation, networking, AI production
policy, later upkeep, actual notice rendering, Base-screen construction/answer,
capture return and movement return remain outside this receipt. The ordinary
and earlier receipts are preserved independently.
