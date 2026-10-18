# RE-SMAC-CITIZEN-STATUS-001: Golden Age and riot admission

Status: original-x86 checked for the SMAC fingerprint in this directory's
README. All 24 named and 1,456 exhaustive small-population combinations pass.
These execute the upkeep caller's turn gate and original status routine,
including recovery or entry into the riot handler. Notification/map callbacks
are logged and omitted without state changes. Riot consequences and callback
interventions are outside the checked scope.

## Upkeep gate and Golden Age

The checked base-upkeep caller skips this status stage on turn indices 0 and
1. From index 2 onward it calls the status routine. A skipped stage leaves
previous Golden Age and riot flags unchanged. This is the observed caller's
gate; it does not redefine the status routine's behavior when called directly.

With population P, final talents T and final drones D, Golden Age qualifies
when all three conditions hold:

- P is at least 3.
- D is exactly zero.
- T is at least `ceil(P/2)`.

The threshold uses total population, without subtracting specialists. Population
3 needs two talents, population 4 needs two, and population 5 needs three.
Additional calming burden S is not read by this predicate.

If qualified and not already in Golden Age, request the Golden Age
notification, then set the Golden Age flag. If already qualified and active,
keep it without repeating that notification. If no longer qualified, clear
the active flag. Request Golden Age over only when D<=T. When D>T, the flag
still clears but that notification is omitted, even if a later protection
prevents riots.

Static caller order places this check before the population-update routine.
The tests do not execute a whole growth/upkeep turn, but the port should
preserve that order when reproducing the observed caller.

## Riot admission

After Golden Age processing, dispatch to the riot handler if:

`D > T`, the faction does not own Telepathic Matrix, and the nerve-stapling
counter is zero.

Equality is peaceful. S is not compared with talents or added to D here.
Telepathic Matrix protects every base of its owning faction, including bases
other than the project location. A rival's Matrix gives no protection.
The nerve condition is a nonzero-byte test; a counter with its high bit set
still protects against admission.

These checks do not change the supplied citizen counts. Matrix prevents
dispatch without making the drone majority disappear. The earlier
[citizen suppression stage](citizen-final-projects-and-suppression.md) separately
determines the usual counts under active nerve stapling. The status cases
can supply counts directly to isolate this later protection predicate.

On entry the riot handler sets the riot-status flag and records whether it
was previously clear. The checks stop there, before its notifications,
governor response, random outcomes or other consequences. An already-rioting
base still dispatches when the conditions hold, marked as an ongoing riot.

## Recovery when riot admission fails

If the base was rioting but the admission condition is no longer met, clear
its riot flag. In the checked human-base path:

- If Golden Age is active after the preceding stage, omit the separate
  riots-over notification.
- Otherwise request riots over.
- Request a map refresh after clearing the flag.

This can recover an existing riot despite D>T when Matrix or nerve protection
now prevents admission. Recovery into Golden Age can request Golden Age and
clear the riot flag while omitting riots over. An unchanged peaceful base
does not request a recovery refresh.

The ordinary notification path is tested. Alternate base automation/message
modes and the AI recovery governor call are not executed. Notification
callbacks could change state in the full game; these tests model callbacks
that make no interventions.

## Acceptance scope

The [24 named cases](golden-age-and-riot-admission-cases.json) cover population
thresholds, strict drone comparisons, prior flags, message suppression,
faction-wide Matrix ownership, high-bit nerve counter, S independence, first
two turn indices and population 127. The grid covers populations
1/2/3/4/5/7, every nonnegative T/D combination with T+D<=P, both prior flags
and four protection states.

Comparisons check gate admission, both resulting flags, riot dispatch/newness,
notification requests, recovery refresh and preservation of unrelated status.
The complete base record is checked for unintended changes outside the status
field. Original gameplay predicates/status instructions execute unmodified;
only notification and map callbacks are replaced with nonmutating returns.

Upstream citizen generation, live-base turn integration, riot effects,
automation intervention, mode-specific messages, AI recovery, UI rendering
and full-game observations remain outside these status checks. The subsequent
[riot facility-loss branch](riot-facility-loss.md) now has separate coverage
with supplied random draws and explicit callback/consequence limits.
