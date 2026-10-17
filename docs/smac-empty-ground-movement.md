# Supplied empty-ground movement admission and spending

`SmacEmptyGroundMovement` is a pure, owned SMAC movement consumer for an ordinary
conventional ground unit moving to an empty destination, without fungus
exemptions. It consumes explicit Q, full unit allowance, raw edge cost,
destination-fungus predicate, spent movement, prior-failure marker, stable unit
identity and both [gameplay RNG states](smac-gameplay-random-state.md).

It does not calculate edge cost or full allowance, inspect terrain/designs,
change a live Unit/Map/Game, serialize movement state, or implement command
legality. Existing `move_unit.gls.js` uses a provisional native-only float cost
model and the port's other random generator; it is not used by this component.
No inferred Q, clock seed, basic allowance, faction capability or default exists.

## Result and arithmetic contract

`ResolveEmptyGroundStep` returns a typed `Moved`, `FailedAttempt` or `Exhausted`
outcome, an owned movement-state copy, relative Source/Destination position,
singleton tile-presence observations, both resulting random words and an
optional draw transcript. Stable identity is never changed. No original mutable
unit-array index, coordinate, live reference or callback is retained.

The original wrapper-return field is separate evidence metadata: it is 1 for
both actual movement and failed-draw exhaustion, and 0 for an already exhausted
request. It is not a movement-success predicate or a rollback instruction.

After validating the declared port domain:

1. If full allowance minus spent is nonpositive, preserve state, marker and
   both RNG words. A zero-cost edge cannot bypass this guard.
2. Let R be that difference clamped to 0..999. Admit without a draw when C<=R,
   or when R>=Q and the destination is nonfungal ordinary land.
3. Otherwise advance only the supplied shared stream once and calculate
   `D=floor(low16*C/65536)`. Admit exactly when `D<R+(prior_failure?Q:0)`.
4. Successful movement adds the full C to spent, clears the marker and moves
   singleton presence to Destination. A failed draw sets spent to full allowance,
   sets the marker and keeps Source position/presence. The CRT word never changes.

Thresholds and the scaled random product use widened integer arithmetic.
There is no floating arithmetic, random retry or result-dependent reseeding.

The admitted port range is Q1..100, allowance/cost/spent0..255, and spent+cost<=255.
Negative values and unproved byte wrap are rejected before producing a result,
including an otherwise exhausted diagnostic whose supplied cost would wrap.
This is a deliberately narrower port admission policy, not an assertion that
the original rejects those inputs. The complete original movement-wrapper
captures below use Q3; the wider Q range has arithmetic boundary tests, not
original-wrapper equivalence coverage. The upstream raw rule/basic-allowance
producer and full per-unit allowance remain separate responsibilities.

`ResetSpentForCheckedUnitUpkeep` copies the stable identity and marker while
setting spent to zero. It models only the captured healthy-unit reset effect.
It takes no RNG object and does not perform turn advancement, healing, policy
refresh, base upkeep, allowance computation or a live lifecycle hook.

## Retained original evidence and reductions

Human-owned research documents and JSONs are read-only inputs:

| Record | Case JSON SHA256 |
| --- | --- |
| RE-SMAC-MOVE-ADMISSION-001 | `c6db902cff4485a0107bb806817e350902a4593cb526a684636dfd84697c0220` |
| RE-SMAC-MOVE-FUNGUS-EXEMPTIONS-001 | `5c5bd5e8ce6e72868e68d63fc527a2be188ba9271ee8d774b995dd60d481bedb` |

The admission record contains 66 single requests plus one two-request sequence,
68 original wrapper invocations. This fixture retains 65 ground single rows,
excluding the one Foil control. The three exhausted rows did not query edge cost
and record null; they supply zero as an unused pure-API input here, not as an
asserted original cost. Separate direct controls also supply nonzero costs to
exhausted requests. The immediate-retry test composes the captured
`flat_left1_draw1` input with the recorded sequence's exhausted second-request
expectations; it does not invent an original sequence seed absent from that JSON.

The fungus record contains 81 single requests and nine two-request/reset
sequences. This fixture retains 15 single rows with ordinary empty destinations:
nine ordinary-fungus controls and six positive-PLANET cost-relief controls.
The latter's cost is supplied explicitly; positive PLANET does not become an
admission exemption. Friendly occupied, Former/Artifact and diagnostic Psi
profiles are excluded rather than generalized from this API's boolean predicate.

All nine reset sequences retain the captured initial, post-failure, post-reset
and final shared states, strict draw outcomes, spent/marker state and membership.
They demonstrate failure, the narrow reset effect, and another attempt on the
same stream. The supplied-world original execution includes actual unit-reset
code, but the port test does not execute complete faction/turn upkeep.

The 80 retained single rows, nine two-request upkeep histories and one immediate
retry reduction compare 100 modeled wrapper outcomes. The test fixture owns
semantic literals only. It contains no original executable or asset bytes, and
neither the original executable nor a rendered live game was rerun for this port
comparison. Raw low16 transcript expectations derive from the captured final
shared word; the captures directly provide scaled draws and state words.

## Validation

Source base is main `ccb80244e1acfeb26338c0eb3745cb6b3384fd61`, containing the
independent RNG prerequisite #139. There is no combat-PR or live-runtime dependency.
Six native groups are registered in the existing GSE suite; no additional full
suite or slow default test was added.

```sh
build/bin/GLSMAC --gse-tests --memorydebug \
  --gse-tests-native-filter '^SMAC empty ground movement ' \
  --prefix build/empty-ground-movement-test-data
```

Ubuntu/Clang15 Debug: all eight existing CTests pass in 57.79 seconds, six focused
tracked groups in 1.03 seconds, and one-TU LLVM15 correctness analysis reports
zero failures. The same six groups pass ASan/UBSan/LSan plus tracking in 1.09
seconds, with leak detection and halt-on-error enabled. The local sanitizer
link uses the established non-PIE workaround; repository policy is unchanged.

An optimized `-O3 -DNDEBUG -Wall -Wextra -Werror` build of the actual production
TU and RNG dependency was compared directly with those research JSONs through
a temporary native bridge: all 100 modeled outcomes and nine reset transitions
match. Thirteen malformed/unsupported Release inputs reject.

Seven copied-production controls linked into the actual native test executable
fail at the intended assertions: nonstrict lottery comparison, capped spending,
omitted full-point bypass, clearing the marker during upkeep, consuming CRT,
allowing zero remaining movement to proceed, and omitted byte-wrap validation.
The intended source stays unchanged while those copies run.

Commands, fixture conversion, Release bridge, copied controls and logs are
retained locally under `/home/jhurliman/src/glsmac-empty-ground-movement/`.
These are source/Linux receipts; hosted and other-platform checks are separate.
Keyboard admission, cost production, ZOC, domain/transport legality, occupied
destinations, combat, fungus exemptions and complete turn behavior still require
their own admitted caller paths. Partial remaining movement is modeled here,
without claiming that this helper implements any keyboard or network handler.
