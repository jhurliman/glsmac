# RE-SMAC-START-CAP-RNG-001: two live Blind integration dependencies

This answers the startup capability and ecology-stream questions using the
existing receipts plus one instrumented admitted Gaian startup. It does not
implement the port dispatcher. [Numeric cases](startup-capability-and-ecology-rng-cases.json).

## Who raises the opening weapon maximum from zero to one

On the admitted turn-zero Gaian startup path, after starting technology grants,
**automatic prototype/design generation calls the capability refresh**. That
refresh sets the ordinary available weapon maximum's floor to1, then scans
eligible weapons. Gaia's ordinary starting technology set leaves the maximum1.

This happens before first owner Base/research processing. Later owner summary
calls the capability refresh again, after that Base pass. The observed order is:

1. Startup branch: own available attack0, foreign attack0, region strategy0.
2. Automatic design generator enters with attack0 and invokes capability refresh.
3. Refresh writes the own attack floor1 and evaluates available components.
4. First Base/research pass enters with own attack1, foreign attack0, strategy0.
5. Later owner summary refreshes capabilities again and eventually changes the
   occupied region's strategy to7; first command sees attack1/foreign0/strategy7.

The first transition is therefore a real startup generator side effect, not a
hardcoded research-default1 or an early execution of the later owner summary.
Do not add an extra refresh to technology valuation. PSI uses prior foreign
caches, so refresh placement can affect later games even when a simple opening
fixture appears idempotent. See [regional producer](regional-research-summary-producer.md)
and [PSI estimates](psi-capability-estimates.md).

One complete existing Gaian startup composition is newly instrumented; it
retains the earlier controlled map/catalog/site and headless service limits.
It is not an all-faction or nonzero-turn restart caller-order proof. In
particular, the restart path (reference receipt `restart-unit-package-selector.md`) bypasses normal
turn-zero automatic generation. No new capability result is supplied.

## Ecology draws from the already established CRT stream

The ecology handler directly calls **the existing CRT runtime random routine**.
It shares that word with checked combat and other CRT consumers. It does not
own another ecology seed, use map-generator randomness, or choose its stream
from the movement selector. The [live ecology receipt](base-ecology-research-aggregate.md)
establishes addition followed by one CRT draw even at damage0, before event gates.

The state-seam receipt (reference receipt `gameplay-rng-state-seams.md`) establishes:

`next = (214013 * current + 2531011) mod 2^32`

`raw = (next >> 16) & 32767`.

The ecological test uses raw modulo100. Thread-local storage is the original
runtime's storage arrangement; a port can represent that same stream as an
explicit uint32. Creating a third stream would change caller interleaving.

The existing **63 startup/controller controls already contain the ecology
draw**. In their controlled single-active-human composition, balancing draws
once, global advancement draws four times, then zero-damage first-Base ecology
draws once. There are six CRT advances total, not six plus a newly added seventh.
For explicit starting shared0/CRT0, ecology receives CRT2727824503, advances it
to773150046, returns11797 and tests remainder97. Shared remains0. First map
command retains CRT773150046 and switches movement selection to shared mode.
This count belongs to that controlled world, not every possible startup.

## Initialization and save/load: closed seams versus open origin

| Boundary | Established behavior |
| --- | --- |
| Application data-setup prefix | Two separate clock samples: first seeds CRT, second seeds shared; no draw in the prefix. |
| Complete initialized data-setup control | Explicit samples100/101 leave CRT100; faction graphics consume shared draws, not CRT draws in that run. |
| Controlled setup through first command | Supplied starting paired words; actual later calls advance them, including the ecology draw above. |
| Ordinary save writer and file reader | Preserve the current live words; neither serializes/restores both gameplay RNG words. |
| Local Iron Man save | Seeds CRT from the save-time sample, then stores that sample. |
| Iron Man file reader | Seeds CRT from the saved time sample; shared remains current. |

These are established in data setup (reference receipt `data-setup-rng.md`),
controller mode (reference receipt `startup-rng-controller-mode.md`) and
16 actual file-body round trips (reference receipt `gameplay-rng-save-load.md`). The ordinary read
preserves the *current pre-read* words, including intervening advances; it does
not restore the words that existed when saving. Full frontend directory reload
can reach later clock seeding; see directory reload prefix (reference receipt `load-data-directory-rng-prefix.md`).

**Still open:** a continuous full frontend/new-world-to-command seed origin,
second new game in the same process, and full load-controller-to-command final
words. Do not infer those from the supplied seed controls, define a universal
CRT0 opening, or present a port's deterministic RNG serialization choice as
original ordinary save behavior. These gaps do not change the identified ecology
consumer/stream, recurrence, draw ordering or the already counted startup draw.

Stock SMAC evidence only. No SMAX/PRACX or presentation pacing is inferred.
