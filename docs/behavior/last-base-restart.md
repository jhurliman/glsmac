# RE-SMAC-LAST-BASE-RESTART-001: last-base loss and local restart

Four original last-base starvation outcomes now return through complete faction
upkeep. A separate continuous replay restarts, founds a base with a replacement
Pod, and completes the next faction upkeep: six full faction returns in total.
Two original option-mapping fragments also verify the Do or Die checkbox.
See the [outcomes and continuation](last-base-restart-cases.json).

This extends the [starvation deletion receipt](starvation-base-removal.md),
which stopped before placement. These tests use ordinary human Gaia contexts,
no scenario objectives or projects, a supplied rainy wrapped land map, actual
region rebuilding and the original map-scale producer. General procedural
generation and start-site quality distributions are not established here.

## Restart versus elimination

After the last base disappears, actual deletion removes its own homed units.
The faction-disposition routine then removes the remaining own units, including
unhomed units. The supplied foreign unit survives with its lost home cleared.
Original placement search, eligibility queries and unit factories then execute.

| Controlled last-base loss | Returned result |
| --- | --- |
| Turn3, Do or Die off | Faction remains active; replacement stack, no base; no ending request |
| Turn100, Do or Die off | Faction remains active; larger replacement stack, no base; no ending request |
| Turn101, Do or Die off | No replacement own units; faction inactive; local ending requested |
| Turn3, Do or Die on | No replacement own units; faction inactive; local ending requested |

The reached ordinary restart branch permits turn100 and refuses later turns.
Do or Die prevents the restart. A valid restart placement must still be found;
other faction-status/history and placement-failure branches remain separate.
Do not equate having zero bases with immediate elimination: a restarted faction
returns with units and must be able to continue founding and taking turns.

At turn3 the observed package, in creation order, is Colony Pod, Former,
Colony Pod, Scout Patrol. All four share the selected site, are unhomed,
undamaged, have no order, and have zero movement spent. At turn100 this fixture
gets those units plus three more Scouts. Later-turn packages are not universally
four units. The [package selector](restart-unit-package-selector.md) now specifies
retained-catalog eligibility, exact advanced-design scores/ties, creation order
and strict turn20/40/60/80 reinforcement thresholds. Its admitted-site execution
boundary is separate from this receipt's full placement search.

The pinned controlled sites are (0,18) at turn3 and (62,22) at turn100.
They are acceptance outputs for this map and random history, not hardcoded
restart coordinates. No search result, decision draw or factory result is supplied.

## Treasury, research and the second disposition call

The nonzero-turn initialization preserves the existing treasury and adds the
resolved starting-energy rule plus faction ENERGY bonus. With supplied credits100,
starting-energy10 and Gaia bonus0, the first call returns credits110. This
differs from fresh-game initialization, which first clears the treasury.
See [startup economic state](startup-economic-state.md).

It clears the supplied pending faction debit7. The successful restart writes
psych0/labs5 tenths and invalidates the cached research threshold. The checked
owned technology, research target, accumulated research40 and fractional
remainder70 survive. Do not reset those values merely because units were replaced.
This is not a claim that every faction history or social-model field preserves.

There is another caller at the faction tail when no bases and no counted faction
units remain. In both refusal controls it invokes disposition a second time.
The initialization still adds another10 before its inactive-faction guard
returns: complete faction upkeep therefore ends at credits120, with debit0.
The faction stays inactive and the ending request remains set. The successful
restart has replacement units and does not take that extra call; it ends at110.
Do not silently deduplicate these calls when reproducing exact returned state.

The local refusal path records the ending state and returns. These tests do not
execute the subsequent global end-game presentation/controller or assert what
the final native loss screen looks like.

## Continuous restart, founding and next upkeep

The replay starts at turn3 with research0 rather than40. It follows the same
actual last-base removal and restart, then reaches the original manual founding
preflight's admitted boundary for the replacement Pod. The full admitted
founding action consumes that Pod and creates a population-one base at (0,18).
The original map-size scale32 is produced from the supplied64×32 dimensions;
the ordinary research cost query runs without a supplied threshold after restart.

The new base has food0 and mineral store10. At turn4, complete faction upkeep:

- Leaves population1 and posts food2.
- Completes a Scout and leaves mineral store/baseline1.
- Credits treasury110→111 and research0→1, retaining fractional remainder70.
- Leaves the faction active with one base, a remaining unhomed Pod, Former and
  Scout, and the newly produced Scout homed at the new base.

The foreign control also survives. Discovery UI is not reached in this replay;
technology completion/acquisition remains covered by its separate receipts.

## Evidence boundaries

Five actual starvation sequences execute, with four distinct outcome controls
and the continuous replay. The verifier independently checks exact captured
states and source/dependency/raw hashes; it does not rerun the expensive search.
The two option-mapping prefixes execute in the verifier. Neutral text/dialog,
destructor and absent-view backing services are declared; gameplay removal,
placement, factories, founding, research, resource posting and reported normal
returns execute original instructions.

The supplied map's area counters are rebuilt, while inherited secondary region
summaries remain explicit empty inputs. No generated-world fidelity, universal
site-selection rule, repeat-restart policy, other faction/difficulty, AI/network,
scenario objective, or full ending presentation is inferred from these cases.

[Accepted Colony abandonment](colony-abandonment.md) now has full returned
faction callers. It clears only the newborn Pod home before deletion and
continues through the successor tail without a fresh mineral posting, unlike
the starvation removed-base retry. Refreshed and stale-cache controls distinguish
that timing; last-base disposition removes the manufactured Pod before restart.
