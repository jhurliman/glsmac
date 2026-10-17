# SMAC production switching calculation, contract v1

`game::backend::base::SmacProductionSwitch` calculates owned production-state
transitions from explicit inputs. It does not mutate a Base, call a script,
choose a buildable item, update faction counters or run a UI. The persisted
[production-state schema](base-production-state.md) supplies independent current
and baseline items/balances; all results preserve both baseline fields exactly.
Unconfigured state has no implicit target and cannot be passed as configured
state to this API.

## Explicit inputs and three operations

Items retain the schema's semantic tags and opaque owned identities. The caller
supplies strictness, loss percentage, exemption, human ownership, stored
difficulty, Skunkworks presence, and a set of present local-facility identities.
No definition lookup or numeric original-game index is needed. Correct semantic
tags and facility membership remain caller responsibilities; this calculation
does not establish eligibility or resolve unknown identities. The Skunkworks
flag is explicit, independent of the supplied identity set.

`Automatic` takes an optional target: absence is the distinct keep-current
request. Keep-current and an exactly equal target return the original state and
no refresh request. An actual change classifies using the **new target** as
context, recalculates from the unchanged baseline, and returns a refresh request.
AI ownership, zero loss, equal categories or baseline at/below exemption retain
the full baseline. Otherwise the retained amount is
`exemption + trunc_toward_zero((baseline - exemption) * (100 - loss) / 100)`.
Easy difficulty and a Stockpile baseline do not add automatic exemptions.

`Manual` covers an already-admitted primary replacement and takes an explicit
Accept/Cancel penalty choice. Classification uses the **old target**. Zero
baseline leaves current minerals intact. Otherwise it restores the full baseline
without prompting when categories match, stored difficulty is at most one,
loss is zero, baseline is at/below exemption, or the baseline is Stockpile.
If the current category already differs from the baseline, it preserves the
current balance without another prompt. Only the remaining case calculates a
penalty and consumes the choice. Accept commits the retained balance and target;
Cancel returns the unchanged state with prompt/cancel metadata. The choice is
ignored on nonprompting branches. Unlike automatic selection, manual replacement
still runs these decisions for an equal target.

`BaselineLoss` is a separate query using the human/loss/category/exemption gates,
with current-target context. It does not include manual difficulty/Stockpile
exemptions and is not necessarily the difference between two current balances.
An easy-difficulty manual change can retain 30 while this query reports 10.

The classifier first groups all non-unit items into one exceptional category
when the context target is a present local facility. Units bypass this exception;
it precedes even strictness zero. Otherwise:

| Strictness | Categories |
| --- | --- |
| 0 | One category for all items |
| 1 | Units; local facilities; orbitals plus Stockpile; Secret Projects |
| 2 | One Secret Project category; every other item has exact tagged identity |
| 3 | Exact tagged identity for every item |

Skunkworks changes positive strictness to one and leaves zero unchanged.
The existing-facility exception is not permission to build an existing facility.

## Supported arithmetic and error domain

Both stored balances must fit signed32, despite the wider signed64 persistence
schema. Exemption and stored difficulty are signed32 inputs without an additional
lower bound; difficulty uses the original signed `<= 1` comparison. Negative
balances/exemptions are preserved when the evaluated arithmetic is representable.
Strictness must be 0..3 and loss percentage 0..100. All supplied item records and
facility identities are structurally validated before a no-op or exemption can
return; a facility identity cannot be empty. Unused but supplied metadata still
undergoes this validation. Choice must be a known enum even if no prompt occurs.

Arithmetic widens to signed64, then checks each original signed32 intermediate:
the percent complement, baseline minus exemption, their product, and the ordered
subtractions for loss or the manual retained-value addition. Integer division
truncates toward zero. Automatic storage subtracts the computed loss from the
baseline in the original order. No overflow is hidden by a later division, zero
multiplier or cancellation. Conversely, an exempt/no-op branch does not evaluate
unused penalty arithmetic. Manual cancellation happens after the original
pre-prompt calculation, so it does not bypass a calculation-domain rejection.

Invalid structure, unsupported rules or an overflowing evaluated intermediate
throws `std::invalid_argument`. Input remains unchanged and no partial result is
published. These rejection rules are a conservative GLSMAC arithmetic domain,
not a claim about the original executable's signed-overflow or malformed-input
behavior. Negative-boundary tests extend the checked mathematical domain; the
retained original experiment used the recorded nonnegative mineral/rule grids.

## Original evidence and provenance

This is the arithmetic/state projection of **RE-SMAC-RETOOL-001**, whose retained
experiment checked 4,594 scenarios and 4,598 steps. Thirty named cases cover
multi-step switches, same-item/keep-current, zero baseline, easy difficulty,
Stockpile, Skunkworks, category boundaries, existing-facility context and penalty
cancellation. Additional schedules contain 2,048 automatic, 2,048 manual, 216
existing-facility-context and 252 percentage/exemption cases.

| Evidence | SHA-256 |
| --- | --- |
| SMAC `terran.exe` | `0a61eb3642df750c0d5e516c3e3efeb4ad5ae7e1ce3163617759449327952a01` |
| Named semantic fixture | `1e824953c44c89db6509025bda4cef2cd2591588fd1980fc83e942aa8ba9bd96` |
| Original execution report | `2c9f31239dcd528c9fe56ff3a6e0d340e8c671d1e81464891516909e93a62a73` |
| Private execution harness | `fc3c9d36865ab24cbced1b4027ee78bc8f9287e79d9a62090536dbddd7047eb6` |

The experiment executed classifier `0x4fa010`, facility-bit helper `0x51fc50`,
baseline-loss inquiry `0x4fa120`, and automatic setter `0x4fb2c0`. The complete
automatic setter used owner equal to viewer and supplied only presentation
refresh `0x477740`. Its off-view propagation tail was not exercised.

Manual evidence composes admitted decision `0x41d6a3`, retention
`0x41d7d0..0x41d809`, supplied dialog-result dispatch `0x41d960`, and actual
mineral/item commit fragments. The saved UI baseline identity equaled the base
baseline. These are real original fragments with supplied admission/dialog
context, not one continuous full chooser execution. Queue choices, early status
flags, later orders, final production-count reconstruction and full cancellation
cleanup remain outside that evidence.

The new comparator reads the pinned fixture/report and verifies the private
harness hash without importing or executing it. It reconstructs only the input
schedules, calls the real C++ probe, and compares **observed** original state,
decision/refresh metadata and inquiry outputs. Later sequence requests use the
previous C++ result, not expected original state. Original unit-count fields are
explicitly excluded: their modulo-256 updates and lifecycle are not implemented
by this calculator. No proprietary executable bytes or report are committed.

```sh
clang++-15 -std=c++17 -O2 -DNDEBUG \
  -ffunction-sections -fdata-sections -Wl,--gc-sections -Isrc \
  src/game/backend/base/ProductionState.cpp \
  src/game/backend/base/SmacProductionSwitch.cpp \
  tools/tests/smac_production_switch_probe.cpp -o /tmp/smac-switch-probe
python3 tools/reference/compare_smac_production_switch.py \
  --fixtures /private/production-switching-cases.json \
  --report /private/production-switching-original-x86-results-20261016.json \
  --harness /private/check_production_switching.py --probe /tmp/smac-switch-probe
```

The opt-in CLI is a bounded numeric evidence adapter, not a save/network format.
Each line has decimal mode (0 automatic/1 manual), human flag, difficulty,
strictness, loss, exemption, baseline ID/balance, current ID/balance, keep flag,
target ID, accept flag, Skunkworks flag, facility count and facility IDs. IDs use
the experiment's encoding only in this tool; keep-current remains a distinct
flag. Input is limited to a 4,095-byte line and 64 facility identities. Booleans
are 0/1, manual keep is invalid, IDs exclude `INT32_MIN`, and local facility IDs
are 1..33. Syntax rejection exits 2; arithmetic rejection prints `invalid` with
exit 0. Success prints `ok`, current ID/balance, baseline ID/balance, inquiry,
refresh, prompt and cancellation. Empty streams are allowed.

## Validation scope

Six native groups cover the reduced original named fixtures, complete category
tables, pre/post-write facility context, automatic/manual/query differences,
signed endpoints and intermediate overflows, branch-sensitive rejection,
structural validation and owned opaque identities. The production code has no
graphics, assets, GSE or multiplayer dependency. Run the complete native/script
suite with `GLSMAC --gse-tests` or CTest on this focused publication ancestry.

Local Clang 15 Debug validation on parent
`6389d520bbd621eab34a5cddec0b0a117e0a2f5c` passed full normal CTest 4/4 in
18.16 seconds. All six groups passed allocation tracking and ASan/UBSan/LSan
with `detect_leaks=1`. Because this ancestry predates the native-filter CLI,
ignored copied registration TUs select these exact six groups while linking the
unchanged application and production objects. This does not claim that the older
parent's full-suite leak baseline is clean. The existing local non-PIE sanitizer
linker workaround remains local.

The production Release probe matched all 4,594 original scenarios / 4,598 steps.
Debug and Release explicit-TU correctness analysis reported zero findings.
Fourteen malformed CLI requests exit 2 without output; four arithmetic-domain
rejections print `invalid` with exit 0. Python syntax and evidence-hash rejection
checks also pass. Independent read-only production/test/comparator review found
no actionable issue; that review did not rerun the original experiment.

Three copied-production controls fail the actual native named-case assertion,
each with exit 1: compounding from the current balance (`RETOOL-002`), using old
context for automatic classification (`RETOOL-017`), and replacing manual logic
with automatic selection (`RETOOL-020`). Intended checkout source is never
replaced. Logs, copied registration/source and binaries are retained in
`/home/jhurliman/src/glsmac-production-switching-audit/`.

This slice activates no gameplay caller and makes no SMAX parity, baseline
lifecycle, counter bookkeeping, eligibility, production queue, mineral posting,
completion or rendered chooser claim. Those require their own explicit inputs
and integration tests.
