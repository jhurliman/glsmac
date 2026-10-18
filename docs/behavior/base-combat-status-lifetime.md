# Base combat status lifetime

Research ID: RE-SMAC-BASE-COMBAT-STATUS-001. Original SMAC only.
This bounded static follow-up reuses the frozen
[defended-Base combat](scout-defending-base.md) receipts. No new battle or
turn matrix was executed.

The stored Base status mask **8** is a temporary aftermath marker with
gameplay consumers. A useful provisional name is `recent_defender_loss`;
the original symbolic name is unknown. Do not treat it as permanent, as
merely cosmetic, or as a flag set by every attack against a Base.

## Producer and outcome distinction

- The accepted Scout victory sets the defending Base's bit after lethal
  combat and casualty processing. It preserves all other status bits.
  The Base remains enemy-owned and population 2; capture is a separate action.
- The preserved Scout attacker-loss diagnostic performs no Base writes and
  leaves the bit clear. That diagnostic has an older active-faction setup
  discrepancy, so it is supporting evidence rather than another accepted
  fully coherent gameplay case. Static control flow independently separates
  the ordinary attacker-loss aftermath from the writer's victory path.
- This does not establish bombardment, withdrawal, stack, Psi, or special
  attack behavior. Do not extrapolate a universal casualty-event producer.

## Consumers

These are static branch rules, not new end-to-end AI receipts:

| Context | Effect of the bit being set |
| --- | --- |
| Production posting/selection | After the existing human/governor gates admit this block, calls the production-selection wrapper directly instead of following the clear-bit territorial admission checks. |
| Further production management | Bypasses one branch that depends on the Base tile having an ordinary faction claim and other unit/production conditions; uses the alternate production-management path. |
| Production planner, early decision | Bypasses a human-owner mineral-surplus shortcut that can soften the local production category. Other planner predicates remain authoritative. |
| Production planner, later decision | Prevents one conditional reset of local planning categories/priorities. It does not independently force a unit design or production choice. |
| Unit AI Base-target evaluation | Multiplies the current local candidate score by 3. Other modifiers, admission checks, distance handling, and final selection still apply. |

The bit is not a strength, yield, repair, population-loss, or capture rule
merely because these routines operate on the same Base. No such direct
consumer was established here.

## Clearing and ordinary upkeep order

For a surviving Base that reaches the ordinary upkeep tail:

1. The entry mask preserves bit 8.
2. Resource/citizen calculation, production posting, further production
   management, population processing, and later upkeep work run first.
3. Near the end, upkeep clears mask **0x200008**, including bit 8. It preserves
   unrelated bits. This follows the assimilation and another timer decrement.
4. A separate periodic clear of another status bit follows; it is not the
   reason bit 8 clears.

This is **Base-upkeep lifetime**, not an unconditional global turn-start
clear. The faction-turn preliminary Base mask also preserves bit 8.
Removal, abandonment, ownership change, and other early exits can bypass
the surviving-Base tail; no blanket guarantee is made for those paths.

Successful capture clears bit 8 in its broader status cleanup. Another
ownership-transfer path uses that same cleanup mask. New Base initialization
zeros the entire status word.

## Port contract and remaining limits

Preserve the stored bit through serialization and restoration/Start. Do not
derive it from unit occupancy, current combat damage, or an empty garrison.
Allow admitted consumers to observe it during the owning Base's next upkeep,
then clear it at the ordinary tail. Capture's explicit cleanup takes precedence
if capture happens first. A Base whose owner has already processed its upkeep
can therefore retain the bit across the global turn boundary.

The save/load and Start implementation itself was not newly traced here;
preserving state is the port integration consequence of the confirmed stored
marker and its explicit cleanup points, not a new original save-format claim.
The complete AI chooser/score function, special combat outcomes, interrupted
upkeep, and all nonstandard ownership transfers remain outside this closure.
