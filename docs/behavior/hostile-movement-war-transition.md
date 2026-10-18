# RE-SMAC-HOSTILE-WAR-CALLER-001: no-peace hostile movement

**An initial neutral hostile move does not require preinstalled Vendetta or a
confirmation in the covered local two-human state.** Three focused contexts
complete four original movement-wrapper returns: no existing relationship,
already mutual Vendetta, and two consecutive neutral battles in one initialized
process. [Literal cases](hostile-movement-war-transition-cases.json) include
the directed metadata and both RNG streams.

Inputs retain the prior [human Scout caller](human-scout-hostile-movement.md):
two human factions, Librarian, neutral healthy Gun/Scout Infantry/Fission pairs,
full movement, no abilities/Bases/instigator/third-party alliances, ordinary
local directional input and initialized diplomacy-display records. Initial
relations have no Pact/Treaty/Truce. There is no supplied confirmation answer,
battle outcome, diplomacy result or random result.

## Caller transition

The actual selected-unit keyboard gate admits the initial direction. The local
movement wrapper reaches the original battle caller, which requests the mutual
relation setter with **Vendetta plus its auxiliary encounter marker**. It does
not call the separate treaty/war-break handler on either checked initial
relationship. The setter executes before battle rounds, and normal attack,
casualty/remapping, cleanup and wrapper return follow.

The human-specific path reads the defender's directed relationship, agreement/
contact-related flags and a reverse exception flag; the no-peace/zero-exception
inputs take the ordinary path without a warning. Do not generalize this to
Treaty/Truce/Pact, unknown secondary flags, incomplete movement, networking or
uninitialized diplomacy display. No cancelled/proceeded pair is needed here
because no decision prompt is reached.

For implementations retaining compatible logical relation values, this caller
requests mask32784, rather than Vendetta16 alone. The original mutual setter's
normalization rules remain those in [relationship primitives](diplomatic-relation-primitives.md).
The following are **logical state values**, not memory-layout instructions:

| Directed state, attacker→defender / reverse | Before | After one attacker victory |
| --- | --- | --- |
| Primary relation words, no-peace context | 0 /0 | 2147516696 /2147516696 |
| Primary relation words, existing-Vendetta control | 16 /16 | Same as above |
| Secondary relation words | 0 /0 | 0 /0 |
| Directed transition-turn fields | 2 /3 | 2 /3 |
| Tracked paired counter A | 0 /0 | 1 /0 |
| Tracked paired counter B | 5 /7 | 6 /7 |
| Faction integrity blemishes | 0 /0 | 0 /0 |
| Faction bookkeeping counters | 4 /7 | 4 /7 |

The distinct dates/counters are supplied historical controls, not claimed
startup defaults. Counters A/B increment during successful combat, **after**
the mutual setter. Their complete meanings/lifetimes are not renamed by this
receipt. In particular, do not call a general war-break wrapper merely because
a hostile move occurs: that would introduce timestamp/bookkeeping work absent
from this caller, including when already at Vendetta. The private trace retains
all observed faction-record writes, including native loss/roster counters.

Both first battles have19 rounds/38 CRT draws, remove the defender and leave
the promoted attacker at the source with damage9, morale2 and spent3. Wrapper
return0 is the normal count-change exit, not an attacker-loss indicator. This
agrees with the earlier caller contract.

## Second actual battle and shared initialization

The third context preallocates a second healthy neutral pair before either
battle. There is **no world restore or RNG reseed between its two attacks**.
After first casualty compaction, the original keyboard gate admits the second
attacker using its actual new unit index. The second wrapper returns normally,
again with an attacker victory, damage9, promotion1→2, source position retained,
spent3 and dead-defender selection cleared.

| Stream | Before first battle | After first / before second | After second |
| --- | --- | --- | --- |
| CRT | 3 | 1369276809 | 928401183 |
| Shared | 3 | 1018897798 | 1018897798 |
| Draw counts | — | CRT38, shared1 | CRT38, shared0 |

The first shared draw is inside a lazily constructed hit-presentation palette
object. The original presentation-initialized guard is set before construction;
the constructor obtains a nonzero identifier using the shared RNG. Identifier9605
and the initialized guard persist into the second battle, which does not
construct that object or make that shared draw. The original also registers
its destructor for process exit. Do not implement a shared draw on every battle.

**Remaining lifecycle boundary:** new-world/reset/save-load transitions for this
presentation guard/object were not executed or exhaustively traced. Constructor,
guard, exit registration and persistence across two battles are established;
this is not proof that every new-world/load path preserves or clears it. A port
must keep that lifecycle dependency explicit rather than attach the draw to a
gameplay reset or save operation by assumption.

The later [state-lifetime receipt](hostile-state-initialization-lifetime.md)
now executes ordinary save/load and the registered palette destructor. File-body
load preserves the process palette and excludes it from campaign serialization;
complete reused-newworld frontend lifetime remains outside the checked boundary.

Headless rendering/time, no-input presentation polling, notifications and
stack-object teardown use the prior declared services. Native diplomacy,
strength/round/promotion/RNG/death and wrapper control flow execute. The keyboard
decoder-to-emission boundary is joined using its actual arguments; a complete
window/event-handler return is not claimed. No visible UI/platform run, gameplay
C++ edit or general diplomatic declaration policy is part of this handoff.
