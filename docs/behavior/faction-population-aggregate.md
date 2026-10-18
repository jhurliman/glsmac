# RE-SMAC-POPULATION-AGGREGATE-001: cached owner population lifetime

**Capture uses a persistent, potentially stale population total owned by the
faction-wide summary/policy refresh. It does not sum live Bases on demand.**
One original full faction upkeep proves the summary is rebuilt **after** that
owner's Base processing, including growth. Same-world founding/removal checks,
fresh initialization and an ordinary save/load roundtrip establish the other
covered lifetime boundaries. [Literal cases](faction-population-aggregate-cases.json).

## Owner and update phase

The stored value is a per-faction signed integer population aggregate. Its
native producer first clears the requested faction's aggregate to0, then adds
the current population of each Base whose current owner equals that faction.
The inspected producer uses the signed Base population value; malformed
negative population and overflow are not tested here.

The ordinary faction caller performs unit recovery, owner Base/research
processing, additional unit processing and another preparatory pass, then
invokes this faction summary/policy pass. Consequently this aggregate includes
growth and surviving/deleted Base state from the **just-completed** owner Base
pass. The same native pass also rebuilds Base counts and other faction/region
statistics. Base-count mutation during an action does not imply population
aggregate mutation.

In the executed Gaia/Librarian case, a population1 Base starts with20 stored
nutrients and aggregate canary777. The native population step grows it to2;
the aggregate stays777 through that step, the later Base recalculation and
the full Base return. Only the later owner summary pass writes777→0→2.
Full faction upkeep then returns normally. An independent other-faction
aggregate888 remains888. The canaries are sensitivity inputs, not startup
defaults or valid population totals.

## Immediate versus deferred changes

| Operation and evidence | Live owner population | Stored aggregate |
| --- | --- | --- |
| Actual growth step, then Base return | 1→2 | Canary777 preserved |
| Later native owner summary in that full upkeep | 2 | 777→0→2 |
| Actual additional founding, same world | 2→3 | 2 preserved |
| Explicit native owner summary after founding | 3 | 2→0→2→3 |
| Actual ordinary removal of the new Base | 3→2 | 3 preserved |
| Prior capture case, attacker /defender | 1 /3→2 /1 | 1 /3 preserved |

The same-world founding and removal bodies return normally. The explicit
summary call between them is a diagnostic invocation of the actual producer;
it is not claimed to be part of ordinary founding. Founding changes the owner
Base count1→2 immediately; removal changes it2→1 immediately. Neither writes
the population aggregate in these covered bodies.

The [capture receipt](undefended-base-entry.md) and
[screen recalculation follow-up](captured-base-screen-close.md) retain the
pre-capture faction totals even after captured-Base recalculation. Therefore
resource recalculation is not the owner of this aggregate. Capture's final
screen/outer return remains outside that evidence.

For starvation, the inspected population routine's surviving decrement does
not contain an aggregate write. The already executed
[population1 starvation-removal caller](starvation-base-removal.md) invokes
the same removal helper tested here; its normal owner-upkeep path subsequently
reaches the owner summary phase. That is a **static composition with prior
caller evidence**, not a new starvation run watching this field. No last-Base
restart, editor, advanced-start or arbitrary UI-lifecycle update is dynamically
claimed by the immediate-change table.

## Initialization and save ownership

Two executed fresh turn0 faction-initialization prefixes replace an aggregate
canary777 with0 for the initialized faction, preserving the other faction
until its own initialization. This reset is in the fresh-turn branch. Static
inspection shows that the nonzero-turn restart path skips that reset block;
do not unconditionally zero this cache merely because a faction restarts.
The exact aggregate through a complete restart is not newly executed here.

The complete ordinary save writer and reader both return0. They serialize and
restore the aggregate as part of faction state. With no Bases present, supplied
cached values111 /222 survive save, live poisoning to999 /999, and load back
to111 /222. The ordinary file body does not reconstruct these values from the
empty Base roster. This is a stale-state preservation discriminator, not a
valid empty-world default. Full frontend/load-to-command behavior, alternative
save modes and save compatibility policy remain outside the roundtrip.

## Port integration consequence

Keep the cached total in faction state and preserve it across the ordinary
save/load boundary. Rebuild it at the owner summary phase after owner Base
processing, rather than in Base resource recalculation or each population/
ownership action. Capture should read the defender's stored total **before**
capture losses and ownership changes. In the covered booty branch the
denominator is that cached value plus1, even when live population has changed
since its last refresh.

Static direct-reference scanning finds the fresh reset and the summary clear/
accumulation writes. It also finds an explicit producer call in advanced-start
setup, separate from the ordinary upkeep caller. It does not prove the absence
of every possible aliased bulk write or indirect caller. Broader world-reset,
editor, restart and frontend scheduling remain explicit gaps.
