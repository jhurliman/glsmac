# Leading-human victory-pressure shortcut

RE-SMAC-VICTORY-PRESSURE-SHORTCUT-001, 2026-10-18. Pinned stock SMAC.
[Literal cases and lifetime controls](leading-human-victory-pressure-shortcut-cases.json).
This extends the existing [dominance admission/score predicate](leading-human-dominance-score-path.md)
and [potential-majority selector](potential-diplomatic-majority-selector.md).

After the existing human-leader, retained Base-count, difficulty and strict-time
admission, dominance returns true before comparing scores only when:

```
victory_pressure() && potential_majority(querying_leader, exclude=-1) == querying_leader
```

Pressure alone does not establish dominance. If the shortcut fails, the existing
score comparison still runs; a failed shortcut is not an unconditional false.
The new controls use equal scores so the ordinary ratio fails and the shortcut's
effect can be isolated.

## Exact pressure inputs

Pressure is true if **any faction 1..7 marked human** has a saved economic-victory
deadline strictly greater than the current turn. This is the Corner Market
deadline, not a general estimated victory date. The predicate does not require
a positive stored payment, a live roster bit, a Base, current technology or the
Economic Victory rule still being enabled. Date equality and past dates fail.
It is not restricted to the querying leader's deadline.

Otherwise it calls the majority selector with query 0 and exclude -1. Under the
actual technology helper's nonpositive-owner gate this returns no candidate;
all new installed-catalog controls confirm that result. Do not substitute the
human leader as the technology perspective for this intermediate call.

Otherwise, when the saved Voice of Planet host marker is anything other than
the absent sentinel -1, pressure is true if any human faction owns either:

- The technology required by Psi Gate: installed **Matter Transmission**.
- The technology required by Voice of Planet: installed **Threshold of Transcendence**.

These are current catalog requirement lookups and technology ownership tests.
Psi Gate need not be built, Voice need not belong to the human, and this check
does not validate the host Base ID. Controlled invalid host IDs demonstrate
that last distinction; they are not valid project-construction states. The
Transcendence Victory toggle is not an additional gate in this helper.

There is no saved pressure latch to refresh. Each call reads current deadlines,
human mask, project marker and technology/catalog inputs. The observed calls
consume no RNG and preserve faction state.

## The exact exclude=-1 history read

The final majority query uses installed Mind/Machine Interface or an immediate
prerequisite known by the **querying leader**, with the previously established
vote sums and tie preference. Diplomatic Victory must be enabled for a candidate.

With Cooperative Victory enabled, this caller's `-1` exclusion also tests bit 0
of each candidate's saved **Base-expansion/advice history turn word**. An odd
word excludes that candidate; an even word permits it. The denominator still
includes excluded candidates' votes. Cooperative Victory off bypasses this
filter. It is not a query of a real “faction -1,” nor a symmetric Pact test.

The original selector indexes one word before the candidate's primary relation
row for this particular argument. The resulting history-word test is measured
behavior, even though it is an unusual consequence of the generic selector.
Other negative selectors remain outside this contract.

This word must not be modeled as a fixed faction birth turn. Initialization
writes the current turn, but native Base founding and Base acquisition also
refresh it for the gaining owner. Source-inspected subversion/ownership-transfer
logic has the same store, and the native SECOND BASE / BUILD MORE tutorial
advice paths refresh it when those gated notices are presented. An opaque
`last_base_expansion_or_advice_turn` is a suitable port label; the original
symbol name is not established here. The live founding and capture controls
confirm their stores; ordinary manual movement and nonlast removal preserve
the supplied word in those controls.

## Producer and storage lifetime

Fresh faction initialization clears economic deadline and paid cost. Nonzero
faction initialization preserves them, while setting the Base-history word
to the current initialization turn. Four actual initializer prefixes distinguish
these lifetimes.

The existing [manual-start cases](corner-market-manual-start-cases.json) establish
the accepted economic action's payment and deadline `turn + delay`. Its cancel/
refusal paths leave the prior state. The existing [HQ-loss cases](corner-market-hq-loss-cases.json)
cover cancellation of an ordinarily paid future attempt on qualifying HQ loss.
These earlier action checks are reused, not repeated as new experiments.
The ordinary victory controller separately recognizes deadline equality; the
pressure predicate has already stopped treating that date as future.

An actual ordinary file round trip restores all eight history words, economic
deadlines and paid costs, plus the Voice marker. Loading does not generate
new deadlines or reinterpret the odd/even history word. The marker-storage
control uses a declared diagnostic host without constructing a project;
complete project production, election/winner legality and frontend restore are
separate boundaries. For a current no-project campaign, retain the actual absent
Voice marker and ordinary deadline history rather than inventing pressure.

Validation adds 48 complete pressure/majority/dominance contexts using scalar
requirements produced by the native installed common-rule loader, four initializer
prefixes, a 25-word ordinary storage round trip and the linked native Base-event
history controls. Existing score/ranking/majority measurements remain unchanged.
Three additional complete owner summaries connect this to the real regional
consumer: equal scores produce strategy 8 with no pressure, 7 with a future
economic deadline and eligible leader majority, then 8 when the odd history
word excludes that leader. Their retained inputs make the ordinary threshold
comparison exactly equal before the dominance increment, distinguishing 7/8
without supplying any helper or strategy result.
No production code or live original-game window changed; the broad goal stays paused.
