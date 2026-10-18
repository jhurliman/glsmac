# Defeating a Base garrison before capture

The native `move_unit` caller now resolves a foreign occupying Unit before
checking undefended Base capture. Ordinary Gun/Scout Infantry can fight a
singleton garrison in an unfortified enemy Base on otherwise clear flat land.
Perimeter Defense, Tachyon Field, projects, other weapon/chassis profiles and
the existing unsupported combat contexts still require their own rules.

The [original garrison receipt](behavior/scout-defending-base.md) fixes the
important distinction: defeating the garrison does not enter or capture the
Base in that command. The attacker remains at its source, spends its combat
movement, and promotes through the existing native battle result. The Base
keeps its owner, population, facilities and conquest history. Capture requires
another admitted command after movement becomes available.

## Strength and persistent inputs

Host preparation freezes signed `alpha.txt` RULES22/28/30 percentages for
intrinsic Base defense, Infantry-versus-Base attack and FANATIC. Settings and
world headers compare this profile on load/download. Runtime combat consumes
these values instead of hardcoding the previously implemented FANATIC bonus.
FANATIC and Infantry bonuses are separate integer percentage stages; with
vanilla morale1 values, `224 -> 280 -> 350`, rather than additive336. The
unfortified defender becomes `224 -> 280`.

The [status lifetime evidence](behavior/base-combat-status-lifetime.md) identifies
native status mask8 as a temporary defender-loss aftermath marker. The port
stores `recent_defender_loss` in the Base context. A victorious attack sets it
before casualty notifications, together with acquired Base/owner knowledge.
An attacker loss does not publish victory-only discovery or set the marker.
Serialization and Start preserve it; successful capture clears it, as does the
ordinary surviving-Base upkeep tail. Queries and a global turn boundary do not
independently clear it. The native governor/AI consumers remain future work;
storing this field does not implement the full production planner or target AI.

All discovery allocations, casualty queues and refresh records are prepared
before survivor, Base status, history and RNG authority change. The existing
casualty transaction publishes the saved knowledge and Unit/Base refresh queues;
callbacks see coherent ownership and aftermath. Movement sight is not applied
to an attacker that never left its source.

## Validation and formats

The pure combat regression compares every raw draw, modulo result, health pair
and CRT state in the original17-round execution. Separate checks cover FANATIC
composition and independent truncation with non-default percentages. The live
test covers victory and attacker loss, read-only validation, observer discovery,
casualty callbacks, exhausted-command refusal, actual Start, subsequent capture,
and the real Base upkeep tail. The world-download fixture supplies the same
admitted lobby roster; full campaign files separately retain that roster.

The format changes are global settings V17, world rules header V32 and inner
Base context V4. Inner V2/V3 contexts migrate without inventing defender-loss
history. Earlier whole-campaign/settings formats are rejected, rather than
guessing combat percentages absent from their frozen profiles.

This is a joined ordinary Scout/Base interaction. It does not establish the
remaining global conquest controller, network match, strategic AI, SMAX combat,
or a complete M2 match.
