# Air-summary checkpoint order: upkeep, startup and Workshop

The sole direct `5773c0(owner)` call in faction upkeep `53bb60` is at
**`53bbbf`, after Base production**. The earlier summary-inputs packet's
pre-Base wording was wrong and is corrected in the live document. There
is no separate pre-Base call and no second post-Base call in that function.

| Order | Call site | Operation |
| --- | --- | --- |
| 1 | `53bb72 → 5cbc40` | Social refresh |
| 2 | `53bb7d → 53a950` | Unit maintenance |
| 3 | `53bb88 → 53b7a0` | Base processing; owned-Base loop calls `50c760` at `53b951` |
| 4 | `53bba9 → 53b0f0` | Allocation/advice stage |
| 5 | `53bbb4 → 575db0` | Diplomacy stage |
| 6 | `53bbbf → 5773c0` | Owner summary, including capability refresh and air counters |
| 7 | `53bbd3 → 5cbee0` | Later faction stage; no second owner-summary call |
| 8 | `53bdab → 5ca7f0` | Conditional empty-owner disposition near tail |

The mode guard at `53bb95..53bba2` can skip stages4–7 when
`(959fc8 & 1) != 0 && (959fc8 & 8) == 0`. Base processing precedes that
guard. This is a conditional checkpoint, not an unconditional recount on
every possible mode's return.

Inside the summary, `577983 → 577140` refreshes capability fields, then
the three air counters clear and the live Unit scan rebuilds them. A
qualifying newborn produced by `50c760`, still live when this scan reaches
it, is therefore included in that upkeep's refreshed count. Do not defer
its eligibility until the next turn or add an extra early refresh. This
is checkpoint state: later mutations, including conditional disposition,
do not trigger a second recount merely because upkeep is about to return.
The full summary has its own later policy/Unit effects; this timing receipt
does not replace its established predicate or promise perpetual live counts.

## Standard startup and Time Warp differ

The only two direct call sites to `5773c0` found in the pinned executable's
decoded call graph are `53bbbf` and `5c76e0` in `5c6760`.
New-world controller `5cadb0` tests primary rules bit `0x80` at `5cb970`:
set selects Time Warp `5cb979 → 5c6760`; clear selects standard compensation
`5cb986 → 5c7c90`. The latter has no owner-summary or capability-refresh call.

Consequently, **do not add a final all-faction air-summary checkpoint to
Standard Rules setup return**. Its own-air counters have the fresh
initializer zeros; the separately documented foreign-threat field is not
cleared by that initializer slice, so do not invent its value. The first
ordinary scheduled owner upkeep runs Base processing and then `53bbbf`.
At first human command entry that owner's checkpoint has run, when admitted
by the normal mode guard. It does not refresh every other owner's counters.
The introduction/FIRSTBASE phase is earlier than that upkeep; see
[startup schedule](firstbase-startup-schedule.md).

Time Warp's `5c6760` has an earlier recommendation-generator call at
`5c697a`, then later Unit placement, then summary at `5c76e0`. Do not
generalize that conditional setup call to Standard Rules.

## Workshop refreshes capabilities before emission

There are two distinct entry levels:

- Full Workshop entry `449630` calls `577140` at `4496fe` before browsing.
- Recommendation producer `597de0` unconditionally calls `577140` at
  `597ede`, after its initial diplomacy/threat-local calculation and before
  component selection, existing-design upgrade submissions, or category
  proposals. Its earliest direct `597390` submission is `598419`.

Normal New/open `4490b0` sets mode1 and filter-1 at `449507/449511` and
calls `597de0` at `449522`. Category presets in dispatcher `442600` set
their filter and mode1 at `443020/443025`, copy the draft, then call the
same producer at `44306d`. Both reach `597ede` before emission, including
an unmatched preset whose proposals are all rejected. Neither generator
call nor full Workshop entry calls `5773c0`: capability fields can become
newer than the retained three air counts. Do not opportunistically refresh
those counts on open or a preset click.

For the port's post-Base `RefreshCanonicalPopulationAggregate` integration,
place the minimal human air-counter subpass after its capability refresh
at the existing post-Base owner checkpoint. Preserve saved counts between
checkpoints, including on Workshop calls that refresh capabilities only.

## Evidence

This is a source-only call-order correction and caller distinction, with
no new native matrix. Existing Base/upkeep and summary-helper controls are
reused. Private additive source packet:
`tmp/reference/independent-air-summary-order-evidence-20261021/`.
The earlier immutable summary-inputs freeze is unchanged; its narrative
timing claim is superseded here. Same pinned executable SHA256:
`0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01`.

Frozen manifest SHA256:
`1403f02867c76d5f4d66f03fb1a61d3ad426fbb07dfce668b40b855c3b59bcff`.
